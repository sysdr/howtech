/* BPF Scheduler Loader */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

static volatile sig_atomic_t stop = 0;

static void sig_handler(int sig __attribute__((unused))) {
    stop = 1;
}

int main(int argc, char **argv) {
    struct bpf_object *obj;
    struct bpf_link *link = NULL;
    struct bpf_program *prog;
    int err;
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <bpf_object_path>\n", argv[0]);
        return 1;
    }
    
    /* Set libbpf logging */
    libbpf_set_print(NULL);
    
    /* Open and load BPF object */
    obj = bpf_object__open_file(argv[1], NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }
    
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        fprintf(stderr, "Note: This requires kernel >= 6.11 with CONFIG_SCHED_CLASS_EXT=y\n");
        goto cleanup;
    }
    
    /* Find the struct_ops program - try multiple names */
    prog = bpf_object__find_program_by_name(obj, ".struct_ops");
    if (!prog) {
        prog = bpf_object__find_program_by_name(obj, "scx_dsq_demo");
    }
    if (!prog) {
        /* Get the first program in the object as fallback */
        struct bpf_program *p;
        bpf_object__for_each_program(p, obj) {
            prog = p;
            break;
        }
        if (!prog) {
            fprintf(stderr, "No BPF program found in object\n");
            goto cleanup;
        }
    }
    
    /* Attach the scheduler */
    printf("Attaching scheduler...\n");
    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "Failed to attach scheduler\n");
        fprintf(stderr, "Ensure no other sched_ext scheduler is active\n");
        goto cleanup;
    }
    
    printf("✓ Scheduler attached successfully!\n");
    printf("  Press Ctrl+C to detach and exit\n");
    printf("  Run './monitor' in another terminal to see stats\n\n");
    
    /* Wait for signal */
    while (!stop) {
        sleep(1);
    }
    
    printf("\nDetaching scheduler...\n");
    
cleanup:
    if (link)
        bpf_link__destroy(link);
    if (obj)
        bpf_object__close(obj);
    
    printf("Cleanup complete.\n");
    return 0;
}
