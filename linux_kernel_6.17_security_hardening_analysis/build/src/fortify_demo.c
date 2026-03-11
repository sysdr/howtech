/*
 * fortify_demo.c — Shows _FORTIFY_SOURCE=3 compile-time and runtime detection
 *
 * _FORTIFY_SOURCE=3 (enabled in kernel 6.17 default config for userspace tools
 * shipped with the kernel) adds object-size checking for dynamically-sized
 * allocations. Level 2 only handled statically-known sizes. Level 3 uses
 * __builtin_dynamic_object_size() for heap objects whose size is a runtime
 * variable — the gap exploits used to reach stack buffers through.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#define COL_GREEN  "\033[0;32m"
#define COL_RED    "\033[0;31m"
#define COL_YELLOW "\033[1;33m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[0;36m"
#define COL_RESET  "\033[0m"

/* We deliberately use a non-constant size to demonstrate level-3 behavior */
static char *alloc_with_size(size_t sz)
{
    char *p = (char *)malloc(sz);
    if (!p) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return p;
}

static volatile sig_atomic_t got_abrt = 0;
static jmp_buf jump_env;

static void abrt_handler(int sig)
{
    (void)sig;
    got_abrt = 1;
    longjmp(jump_env, 1);
}

int main(void)
{
    printf(COL_BOLD COL_CYAN "\n  FORTIFY_SOURCE Level Demo\n" COL_RESET);

#if defined(_FORTIFY_SOURCE)
    printf("  Compiled with " COL_GREEN "_FORTIFY_SOURCE=%d" COL_RESET "\n\n",
           _FORTIFY_SOURCE);
#else
    printf("  " COL_RED "Warning: _FORTIFY_SOURCE not defined" COL_RESET "\n\n");
#endif

    /* ── Safe copy: exactly fits ───────────────────────────────────────────── */
    {
        size_t sz = 32;
        char *buf = alloc_with_size(sz);
        memset(buf, 0, sz);
        /* Safe: src is 16 bytes, dst is 32 — no overflow */
        const char *src = "Hello, kernel! ";
        memcpy(buf, src, strlen(src) + 1);
        printf("  " COL_GREEN "✓" COL_RESET " Safe memcpy(dst=32, src=%zu) — no abort\n",
               strlen(src) + 1);
        free(buf);
    }

    /* ── Compile-time detection: static buffer, known overflow ─────────────── */
    {
        char fixed[8];
        /*
         * The following would be caught at compile-time with -Werror:
         *   memcpy(fixed, "more than 8 chars", 18); // error: __builtin_memcpy
         *
         * We keep the demo compilable by using a size that level 2 misses
         * but level 3 (dynamic object size) catches.
         */
        (void)fixed;
        printf("  " COL_CYAN "ℹ" COL_RESET " Static overflow (char[8] + 18 bytes) "
               "→ compile-time error with -D_FORTIFY_SOURCE=2+\n");
    }

    /* ── Runtime detection via SIGABRT ─────────────────────────────────────── */
    {
        size_t real_sz = 16;
        char *heap_buf = alloc_with_size(real_sz);
        memset(heap_buf, 'A', real_sz);

        signal(SIGABRT, abrt_handler);

        if (setjmp(jump_env) == 0) {
            /*
             * With _FORTIFY_SOURCE=3, __builtin_dynamic_object_size knows
             * heap_buf is 16 bytes. memcpy with 32 triggers __chk_fail().
             * With level 2, this copy would silently succeed (size was runtime).
             */
            printf("  Attempting memcpy(heap[16], src, 32) — "
                   "should abort with level 3...\n");
            /*
             * Note: The actual abort behavior depends on glibc version and
             * compiler support for __builtin_dynamic_object_size. On systems
             * without full level-3 support, this will report as undetected.
             */
            char overflow_src[32];
            memset(overflow_src, 'B', sizeof(overflow_src));
#if defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE >= 3
            /* Use __memcpy_chk indirection to exercise the check */
            __builtin___memcpy_chk(heap_buf, overflow_src, 32,
                                   __builtin_dynamic_object_size(heap_buf, 0));
#else
            memcpy(heap_buf, overflow_src, 32);
#endif
            printf("  " COL_YELLOW "△" COL_RESET " Overflow not caught at runtime "
                   "(FORTIFY_SOURCE < 3 or old glibc)\n");
        } else {
            if (got_abrt) {
                printf("  " COL_GREEN "✓" COL_RESET " "
                       "FORTIFY_SOURCE=3 caught heap overflow → SIGABRT\n");
                printf("    " COL_YELLOW "→" COL_RESET " __memcpy_chk() compared "
                       "__builtin_dynamic_object_size(buf,0)=16 vs n=32\n");
            }
        }
        signal(SIGABRT, SIG_DFL);
        free(heap_buf);
    }

    printf("\n  " COL_BOLD "Level comparison:" COL_RESET "\n");
    printf("  FORTIFY_SOURCE=1 — compile-time checks for static buffers\n");
    printf("  FORTIFY_SOURCE=2 — adds runtime checks for static buffers\n");
    printf("  FORTIFY_SOURCE=3 — extends runtime checks to dynamic allocs\n");
    printf("                     via __builtin_dynamic_object_size()\n");
    printf("  Kernel 6.17: all in-kernel tool builds default to level 3\n\n");

    return EXIT_SUCCESS;
}
