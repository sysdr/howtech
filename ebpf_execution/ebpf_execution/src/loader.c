#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <time.h>
#include <sys/resource.h>

#define ITERATIONS 1000000

static __u64 get_nsecs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int check_jit_enabled(void)
{
    FILE *fp = fopen("/proc/sys/net/core/bpf_jit_enable", "r");
    if (!fp) {
        return -1;
    }
    
    int enabled;
    if (fscanf(fp, "%d", &enabled) != 1) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return enabled;
}

static void set_jit_mode(int enable)
{
    FILE *fp = fopen("/proc/sys/net/core/bpf_jit_enable", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open bpf_jit_enable: %s\n", strerror(errno));
        return;
    }
    
    fprintf(fp, "%d\n", enable);
    fclose(fp);
}

static void benchmark_program(int prog_fd, const char *mode)
{
    __u64 start, end, total = 0;
    int ret;
    char data_in[64] = {};
    char data_out[64];
    __u32 size_out = sizeof(data_out);
    __u32 retval;
    
    printf("\n[%s Mode] Running %d iterations...\n", mode, ITERATIONS);
    
    start = get_nsecs();
    
    for (int i = 0; i < ITERATIONS; i++) {
        struct bpf_test_run_opts opts = {
            .sz = sizeof(opts),
            .data_in = data_in,
            .data_size_in = sizeof(data_in),
            .data_out = data_out,
            .data_size_out = size_out,
            .retval = &retval,
        };
        
        ret = bpf_prog_test_run_opts(prog_fd, &opts);
        if (ret < 0) {
            fprintf(stderr, "Test run failed: %s\n", strerror(errno));
            break;
        }
    }
    
    end = get_nsecs();
    total = end - start;
    
    double avg_ns = (double)total / ITERATIONS;
    double ops_per_sec = (1000000000.0 / avg_ns);
    
    printf("  Total time:    %lu ns (%.2f ms)\n", total, total / 1000000.0);
    printf("  Average time:  %.2f ns per iteration\n", avg_ns);
    printf("  Throughput:    %.2f M ops/sec\n\n", ops_per_sec / 1000000.0);
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_program *prog;
    int prog_fd;
    int err;
    
    printf("=================================================\n");
    printf(" eBPF JIT Compilation Benchmark\n");
    printf("=================================================\n\n");
    
    // Bump RLIMIT_MEMLOCK to allow BPF map creation
    struct rlimit rlim_new = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };
    
    if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
        fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK\n");
    }
    
    // Load BPF object
    obj = bpf_object__open_file("build/counter.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }
    
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        bpf_object__close(obj);
        return 1;
    }
    
    prog = bpf_object__find_program_by_name(obj, "counter_prog");
    if (!prog) {
        fprintf(stderr, "Failed to find program\n");
        bpf_object__close(obj);
        return 1;
    }
    
    prog_fd = bpf_program__fd(prog);
    if (prog_fd < 0) {
        fprintf(stderr, "Failed to get program fd\n");
        bpf_object__close(obj);
        return 1;
    }
    
    printf("Program loaded successfully (fd=%d)\n", prog_fd);
    
    // Test with JIT disabled
    printf("\n--- Testing with INTERPRETER mode ---\n");
    set_jit_mode(0);
    sleep(1); // Let sysctl take effect
    
    // Reload program for interpreter mode
    bpf_object__close(obj);
    obj = bpf_object__open_file("build/counter.bpf.o", NULL);
    bpf_object__load(obj);
    prog = bpf_object__find_program_by_name(obj, "counter_prog");
    prog_fd = bpf_program__fd(prog);
    
    int jit_status = check_jit_enabled();
    printf("JIT status: %s\n", jit_status == 0 ? "DISABLED" : "ENABLED");
    
    benchmark_program(prog_fd, "INTERPRETER");
    
    // Test with JIT enabled
    printf("\n--- Testing with JIT mode ---\n");
    set_jit_mode(1);
    sleep(1);
    
    // Reload program for JIT mode
    bpf_object__close(obj);
    obj = bpf_object__open_file("build/counter.bpf.o", NULL);
    bpf_object__load(obj);
    prog = bpf_object__find_program_by_name(obj, "counter_prog");
    prog_fd = bpf_program__fd(prog);
    
    jit_status = check_jit_enabled();
    printf("JIT status: %s\n", jit_status == 1 ? "ENABLED" : "DISABLED");
    
    benchmark_program(prog_fd, "JIT");
    
    printf("=================================================\n");
    printf(" Benchmark Complete\n");
    printf("=================================================\n");
    
    bpf_object__close(obj);
    
    return 0;
}
