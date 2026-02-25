/*
 * bpf_verify_sim.c — Simplified BPF verifier: abstract interpretation
 *
 * The real BPF verifier (kernel/bpf/verifier.c, ~16000 lines) performs:
 *  1. Control flow graph construction
 *  2. Abstract interpretation (track register types & bounds)
 *  3. Termination check (must be DAG — no unbounded loops)
 *  4. Memory access validation (all map reads must be bounds-checked first)
 *
 * This simulation demonstrates why BPF programs are verified at load time
 * and what kinds of programs are rejected.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* BPF instruction types (simplified) */
typedef enum {
    BPF_ALU_ADD = 0,
    BPF_ALU_SUB,
    BPF_ALU_MOV,
    BPF_MOV_IMM,        /* r = imm */
    BPF_LOAD_MAP,       /* r = map_ptr */
    BPF_MAP_LOOKUP,     /* r = map_lookup(key) */
    BPF_CHECK_NULL,     /* if r == NULL goto label */
    BPF_LOAD_MEM,       /* r = mem[r + off] */
    BPF_STORE_MEM,      /* mem[r + off] = val */
    BPF_JMP_BACK,       /* unconditional backward jump = potential loop */
    BPF_CALL,           /* helper call */
    BPF_EXIT,
} bpf_op_t;

typedef struct {
    bpf_op_t    op;
    int         dst;    /* destination register */
    int         src;    /* source register / map fd */
    int64_t     imm;    /* immediate value */
    int         off;    /* jump target or memory offset */
    const char *comment;
} bpf_insn_t;

/* Abstract register state */
typedef enum {
    REG_UNDEF = 0,      /* unknown / uninitialized */
    REG_SCALAR,         /* known scalar, possibly bounded */
    REG_MAP_PTR,        /* pointer to BPF map */
    REG_MAP_VALUE,      /* pointer into map value (must bounds-check) */
    REG_MAP_VALUE_OR_NULL, /* return from map_lookup — MUST check null */
    REG_CTX,            /* pointer to program context (skb, etc.) */
    REG_INVALID,        /* poison — accessed invalid memory */
} reg_type_t;

typedef struct {
    reg_type_t  type;
    int64_t     smin, smax;    /* signed bounds for scalar regs */
    int         null_checked;  /* for MAP_VALUE_OR_NULL: has null been handled? */
} reg_state_t;

#define NREGS 11

typedef struct {
    reg_state_t regs[NREGS];
    int         insn_count;       /* verified instruction counter */
    int         warnings;
    int         errors;
    int         backward_jumps_seen;
} verifier_state_t;

static void reg_mark_scalar(verifier_state_t *st, int r,
                             int64_t smin, int64_t smax)
{
    st->regs[r].type   = REG_SCALAR;
    st->regs[r].smin   = smin;
    st->regs[r].smax   = smax;
    st->regs[r].null_checked = 0;
}

static void print_reg(const reg_state_t *r, int idx)
{
    const char *types[] = {
        "UNDEF", "SCALAR", "MAP_PTR", "MAP_VALUE",
        "MAP_VALUE_OR_NULL", "CTX", "INVALID"
    };
    printf("    r%d: %-22s", idx, types[r->type < 7 ? r->type : 0]);
    if (r->type == REG_SCALAR)
        printf(" [%ld, %ld]", r->smin, r->smax);
    if (r->type == REG_MAP_VALUE_OR_NULL && !r->null_checked)
        printf(" \033[1;33m← null not checked!\033[0m");
    printf("\n");
}

static int verify_program(const char *prog_name,
                           const bpf_insn_t *insns, int n_insns,
                           int expect_accept)
{
    verifier_state_t st = {0};
    int rejected = 0;

    /* r1 = ctx pointer on entry */
    st.regs[1].type = REG_CTX;
    st.regs[1].null_checked = 1;

    printf("\n\033[1;34m── Verifying: %s ──\033[0m\n", prog_name);

    for (int pc = 0; pc < n_insns; pc++) {
        const bpf_insn_t *in = &insns[pc];
        st.insn_count++;

        if (in->comment)
            printf("  [%2d] %s\n", pc, in->comment);

        switch (in->op) {
        case BPF_MOV_IMM:
            reg_mark_scalar(&st, in->dst, in->imm, in->imm);
            break;

        case BPF_ALU_ADD:
            if (st.regs[in->dst].type == REG_SCALAR &&
                st.regs[in->src].type == REG_SCALAR) {
                st.regs[in->dst].smin += st.regs[in->src].smin;
                st.regs[in->dst].smax += st.regs[in->src].smax;
            }
            break;

        case BPF_LOAD_MAP:
            st.regs[in->dst].type = REG_MAP_PTR;
            break;

        case BPF_MAP_LOOKUP:
            /* map_lookup_elem returns pointer or NULL */
            st.regs[in->dst].type = REG_MAP_VALUE_OR_NULL;
            st.regs[in->dst].null_checked = 0;
            printf("       \033[0;33m→ r%d is MAP_VALUE_OR_NULL — must check null before access\033[0m\n",
                   in->dst);
            break;

        case BPF_CHECK_NULL:
            if (st.regs[in->src].type == REG_MAP_VALUE_OR_NULL) {
                st.regs[in->src].type = REG_MAP_VALUE;
                st.regs[in->src].null_checked = 1;
                printf("       \033[0;32m→ r%d null-check satisfied — type promoted to MAP_VALUE\033[0m\n",
                       in->src);
            }
            break;

        case BPF_LOAD_MEM:
            if (st.regs[in->src].type == REG_MAP_VALUE_OR_NULL &&
                !st.regs[in->src].null_checked) {
                printf("\n\033[1;31m  VERIFIER ERROR [insn %d]: %s\033[0m\n", pc, prog_name);
                printf("  R%d is NULL or pointer to map value, not checked for NULL\n", in->src);
                printf("  \033[1;31m→ REJECT: program accesses potentially-NULL pointer\033[0m\n\n");
                st.errors++;
                rejected = 1;
                goto done;
            }
            if (st.regs[in->src].type == REG_MAP_VALUE) {
                /* Check bounds: offset must be < map value size (assume 64B) */
                if (in->off < 0 || in->off >= 64) {
                    printf("\n\033[1;31m  VERIFIER ERROR [insn %d]: out-of-bounds map access\033[0m\n", pc);
                    printf("  Offset %d is outside map value bounds [0, 63]\n", in->off);
                    st.errors++;
                    rejected = 1;
                    goto done;
                }
            }
            break;

        case BPF_STORE_MEM:
            if (st.regs[in->dst].type == REG_MAP_VALUE_OR_NULL &&
                !st.regs[in->dst].null_checked) {
                printf("\n\033[1;31m  VERIFIER ERROR [insn %d]: write through unchecked NULL\033[0m\n", pc);
                st.errors++;
                rejected = 1;
                goto done;
            }
            break;

        case BPF_JMP_BACK:
            st.backward_jumps_seen++;
            printf("\n\033[1;31m  VERIFIER ERROR [insn %d]: backward jump detected\033[0m\n", pc);
            printf("  BPF programs must be DAGs — no unbounded loops allowed\n");
            printf("  \033[1;31m→ REJECT: potential infinite loop\033[0m\n\n");
            st.errors++;
            rejected = 1;
            goto done;

        case BPF_EXIT:
            printf("       \033[0;32m→ EXIT reached — control flow valid\033[0m\n");
            goto done;

        default:
            break;
        }

        /* Dump register state at key points */
        if (in->comment && strstr(in->comment, ">>>")) {
            printf("       Register state:\n");
            for (int r = 0; r < 5; r++)
                if (st.regs[r].type != REG_UNDEF)
                    print_reg(&st.regs[r], r);
        }
    }

done:
    printf("\n  Instructions verified: %d | Errors: %d | Backward jumps: %d\n",
           st.insn_count, st.errors, st.backward_jumps_seen);

    if (!rejected) {
        printf("  \033[1;32m→ ACCEPT: program passes all safety checks\033[0m\n");
    }

    int result_ok = (rejected == expect_accept);
    return result_ok ? 0 : 1;
}

int main(void)
{
    int pass = 0, total = 0;

    printf("\033[1;36m=== BPF Verifier Simulation — Abstract Interpretation ===\033[0m\n");
    printf("Models kernel/bpf/verifier.c: type tracking, null checks, loop detection\n");

    /* ── Program 1: correct — null check before access ─────────────────── */
    bpf_insn_t prog_good[] = {
        { BPF_LOAD_MAP,    2, 0, 0, 0, "r2 = bpf_map_fd(0)                    // load map" },
        { BPF_MOV_IMM,     3, 0, 0, 0, "r3 = 0                                 // key = 0" },
        { BPF_MAP_LOOKUP,  0, 2, 0, 0, "r0 = map_lookup_elem(r2, &r3)          // lookup >>>" },
        { BPF_CHECK_NULL,  0, 0, 0, 4, "if r0 == NULL goto exit                // null check" },
        { BPF_LOAD_MEM,    4, 0, 0, 8, "r4 = *(u64*)(r0 + 8)                   // safe read >>>" },
        { BPF_ALU_ADD,     4, 4, 1, 0, "r4 += 1                                // increment" },
        { BPF_STORE_MEM,   0, 4, 0, 8, "*(u64*)(r0 + 8) = r4                   // write back" },
        { BPF_EXIT,        0, 0, 0, 0, "return 0" },
    };
    total++;
    if (verify_program("correct_map_update", prog_good,
                        sizeof(prog_good)/sizeof(prog_good[0]), 0) == 0)
        pass++;

    /* ── Program 2: missing null check ─────────────────────────────────── */
    bpf_insn_t prog_no_null[] = {
        { BPF_LOAD_MAP,    2, 0, 0, 0, "r2 = bpf_map_fd(0)" },
        { BPF_MOV_IMM,     3, 0, 0, 0, "r3 = 0" },
        { BPF_MAP_LOOKUP,  0, 2, 0, 0, "r0 = map_lookup_elem(r2, &r3)          // lookup >>>" },
        /* BUG: no null check! */
        { BPF_LOAD_MEM,    4, 0, 0, 0, "r4 = *(u64*)(r0 + 0)                   // MISSING null check!" },
        { BPF_EXIT,        0, 0, 0, 0, "return r4" },
    };
    total++;
    if (verify_program("missing_null_check (SHOULD REJECT)", prog_no_null,
                        sizeof(prog_no_null)/sizeof(prog_no_null[0]), 1) == 0)
        pass++;

    /* ── Program 3: backward jump (loop) ───────────────────────────────── */
    bpf_insn_t prog_loop[] = {
        { BPF_MOV_IMM,     1, 0, 100, 0, "r1 = 100" },
        { BPF_ALU_SUB,     1, 1, 1,   0, "r1 -= 1" },
        { BPF_JMP_BACK,    0, 0, -2,  0, "if r1 > 0 goto -2                    // LOOP!" },
        { BPF_EXIT,        0, 0, 0,   0, "return" },
    };
    total++;
    if (verify_program("unbounded_loop (SHOULD REJECT)", prog_loop,
                        sizeof(prog_loop)/sizeof(prog_loop[0]), 1) == 0)
        pass++;

    printf("\n\033[1;32m=== BPF Verifier Summary: %d/%d tests passed ===\033[0m\n", pass, total);
    printf("The verifier runs at bpf(BPF_PROG_LOAD) — before any execution\n");
    printf("Real verifier limit: 1,000,000 insns (since kernel 5.2)\n");
    printf("Complexity: O(prog_size * path_count) — exponential in worst case\n");

    return (pass == total) ? 0 : 1;
}
