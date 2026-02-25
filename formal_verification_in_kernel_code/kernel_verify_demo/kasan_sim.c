/*
 * kasan_sim.c — Simulates KASAN-detected bugs in user space via ASAN
 *
 * KASAN uses shadow memory: 1 byte per 8 bytes of kernel memory.
 * When kernel accesses memory, KASAN checks the shadow byte first.
 * UAF: 0xFB poison; OOB: 0xF2 redzone; stack OOB: 0xF1
 *
 * Build: clang -fsanitize=address,undefined -Wall -Wextra -Werror -O1
 * We deliberately trigger each bug type and report what KASAN would say.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>

/* KASAN shadow byte values (from include/linux/kasan.h) */
#define KASAN_FREE_PAGE       0xFF
#define KASAN_PAGE_REDZONE    0xFE
#define KASAN_KMALLOC_REDZONE 0xFC
#define KASAN_KMALLOC_FREE    0xFB
#define KASAN_SLAB_REDZONE    0xF2
#define KASAN_STACK_LEFT      0xF1
#define KASAN_STACK_RIGHT     0xF3

/* Simulated KASAN shadow memory (1/8 of heap range) */
#define HEAP_SIZE  (64 * 1024)
#define SHADOW_SIZE (HEAP_SIZE / 8)

static uint8_t  heap_mem[HEAP_SIZE];
static uint8_t  shadow[SHADOW_SIZE];
static uint8_t  freed_map[HEAP_SIZE]; /* track freed allocations */

/* Alloc watermark */
static size_t   alloc_ptr = 0;

static int      bugs_detected = 0;
static int      bugs_found_total = 0;

/* ── KASAN report emitter ─────────────────────────────────────────────────── */
static void kasan_report(const char *access_type, void *addr, size_t size,
                         const char *bug_type, const char *func)
{
    bugs_found_total++;
    printf("\n\033[1;31m==================================================================\033[0m\n");
    printf("\033[1;31mBUG: KASAN: %s in %s\033[0m\n", bug_type, func);
    printf("%s of size %zu at addr %p\n", access_type, size, addr);
    printf("\033[0;33mCall trace (simulated):\033[0m\n");
    printf("  [<%p>] %s+0x48/0xa0\n", (void*)0xffffffff81234560UL, func);
    printf("  [<%p>] kernel_init_freeable+0x210/0x280\n", (void*)0xffffffff810d4210UL);

    uint8_t *saddr = shadow + ((uint8_t*)addr - heap_mem) / 8;
    printf("\n\033[0;36mKASAN shadow bytes around addr %p:\033[0m\n", addr);
    printf("  Shadow: [%02x][%02x][%02x][\033[1;31m%02x\033[0m][%02x][%02x][%02x]\n",
           *(saddr-3), *(saddr-2), *(saddr-1), *saddr,
           *(saddr+1), *(saddr+2), *(saddr+3));
    printf("  Legend: 00=accessible  FC=kmalloc-redzone  FB=freed  F2=slab-redzone\n");
    printf("\033[1;31m==================================================================\033[0m\n\n");
}

/* ── Simulated slab allocator with KASAN instrumentation ─────────────────── */
static void *sim_kmalloc(size_t size)
{
    /* Align to 8 bytes, add 16-byte redzone on each side */
    size_t aligned = (size + 7) & ~7UL;
    size_t total   = 16 + aligned + 16; /* left-rz | object | right-rz */

    if (alloc_ptr + total > HEAP_SIZE) return NULL;

    void *obj = heap_mem + alloc_ptr + 16;
    uint8_t *base = heap_mem + alloc_ptr;

    /* Paint redzones in shadow */
    size_t shadow_off = alloc_ptr / 8;
    for (size_t i = 0; i < 2; i++)            /* left redzone */
        shadow[shadow_off + i] = KASAN_KMALLOC_REDZONE;
    for (size_t i = 2; i < 2 + aligned/8; i++) /* object: accessible */
        shadow[shadow_off + i] = 0x00;
    for (size_t i = 2 + aligned/8; i < total/8 + 2; i++) /* right redzone */
        shadow[shadow_off + i] = KASAN_KMALLOC_REDZONE;

    memset(obj, 0, size);
    freed_map[base - heap_mem] = 0;
    alloc_ptr += total;
    return obj;
}

static void sim_kfree(void *ptr)
{
    if (!ptr) return;
    uint8_t *base = (uint8_t*)ptr - 16;
    size_t off = base - heap_mem;
    freed_map[off] = 1;

    /* Poison freed object in shadow */
    size_t shadow_off = off / 8;
    for (size_t i = 2; i < 6; i++)
        shadow[shadow_off + i] = KASAN_KMALLOC_FREE;
}

/* ── KASAN access check (inline instrumentation simulation) ─────────────── */
static int kasan_check_access(void *addr, size_t size, const char *func,
                               const char *type)
{
    uint8_t *p = (uint8_t*)addr;
    if (p < heap_mem || p >= heap_mem + HEAP_SIZE) return 0;

    size_t off = p - heap_mem;
    size_t shadow_off = off / 8;
    uint8_t sv = shadow[shadow_off];

    if (sv == KASAN_KMALLOC_FREE || sv == KASAN_FREE_PAGE) {
        kasan_report(type, addr, size, "use-after-free", func);
        return 1;
    }
    if (sv == KASAN_KMALLOC_REDZONE || sv == KASAN_SLAB_REDZONE) {
        kasan_report(type, addr, size, "out-of-bounds", func);
        return 1;
    }
    return 0;
}

/* ── Bug demonstrations ──────────────────────────────────────────────────── */
static void demo_use_after_free(void)
{
    printf("\033[1;34m--- Bug 1: use-after-free ---\033[0m\n");
    printf("Simulates: kfree(skb->head); ... skb->data[0];  (classic net driver UAF)\n\n");

    struct {
        uint32_t len;
        uint8_t  data[64];
        uint32_t flags;
    } *obj = sim_kmalloc(sizeof(*obj));

    if (!obj) { printf("  ⚠  allocation failed\n"); return; }
    obj->len = 42;
    printf("  ✓  Allocated object at simulated kernel addr\n");

    sim_kfree(obj);
    printf("  ✓  Object freed (shadow bytes poisoned to 0xFB)\n");

    printf("  Attempting read after free...\n");
    if (kasan_check_access(obj, 4, "netdev_rx_handler", "Read")) {
        bugs_detected++;
        printf("  ✓  KASAN caught use-after-free before hardware fault\n");
    }
}

static void demo_out_of_bounds(void)
{
    printf("\033[1;34m--- Bug 2: out-of-bounds write ---\033[0m\n");
    printf("Simulates: buf[size] = 0;  (off-by-one into redzone)\n\n");

    uint8_t *buf = sim_kmalloc(32);
    if (!buf) { printf("  ⚠  allocation failed\n"); return; }

    printf("  ✓  Allocated 32-byte buffer\n");
    printf("  Writing 1 byte past end (buf[32])...\n");

    /* Access past the end — into the right redzone */
    if (kasan_check_access(buf + 32, 1, "copy_from_user_inatomic", "Write")) {
        bugs_detected++;
        printf("  ✓  KASAN caught out-of-bounds write at redzone boundary\n");
    }
}

static void demo_double_free(void)
{
    printf("\033[1;34m--- Bug 3: double-free detection ---\033[0m\n");
    printf("Simulates: kfree(p); kfree(p);  (SLUB catches via poison check)\n\n");

    uint8_t *p = sim_kmalloc(16);
    if (!p) { printf("  ⚠  allocation failed\n"); return; }

    sim_kfree(p);
    printf("  ✓  First kfree() — object shadow set to 0xFB (KMALLOC_FREE)\n");

    /* Second free — shadow already shows freed */
    printf("  Attempting second kfree()...\n");
    /* SLUB checks the first 8 bytes of freed object for its poison pattern */
    if (kasan_check_access(p, 8, "kmem_cache_free", "Write (free-path check)")) {
        bugs_detected++;
        printf("  ✓  KASAN/SLUB caught double-free via poison verification\n");
    }
}

int main(void)
{
    printf("\033[1;36m=== KASAN Simulation (Address Sanitizer mirrors kernel KASAN) ===\033[0m\n");
    printf("Shadow ratio: 1:8 | Redzone: 0xFC | Freed: 0xFB | Poisoned: 0xF2\n\n");

    memset(shadow, 0, sizeof(shadow));
    memset(freed_map, 0, sizeof(freed_map));

    demo_use_after_free();
    demo_out_of_bounds();
    demo_double_free();

    printf("\n\033[1;32m=== KASAN Summary ===\033[0m\n");
    printf("Bugs detected: %d / 3 test cases\n", bugs_detected);
    printf("Reports generated: %d\n", bugs_found_total);
    printf("Shadow memory overhead: %.1f%%\n", (double)SHADOW_SIZE / HEAP_SIZE * 100);

    return (bugs_detected == 3) ? 0 : 1;
}
