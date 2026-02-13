// slab_demo.c - Kernel module demonstrating slab allocator behavior
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("SLAB/SLUB allocator demonstration");

#define CACHE_NAME "slab_demo_cache"
#define NUM_ALLOCATIONS 10000
#define OBJECT_SIZE 512

struct demo_object {
    char data[OBJECT_SIZE];
    struct list_head list;
};

static struct kmem_cache *demo_cache = NULL;
static struct demo_object **objects = NULL;
static u64 alloc_time_ns = 0;
static u64 free_time_ns = 0;

// Proc file operations
static int slab_demo_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Slab Allocator Demo Statistics\n");
    seq_printf(m, "================================\n");
    seq_printf(m, "Cache name: %s\n", CACHE_NAME);
    seq_printf(m, "Object size: %d bytes\n", OBJECT_SIZE);
    seq_printf(m, "Allocations: %d\n", NUM_ALLOCATIONS);
    seq_printf(m, "Total alloc time: %llu ns\n", alloc_time_ns);
    seq_printf(m, "Total free time: %llu ns\n", free_time_ns);
    seq_printf(m, "Avg alloc time: %llu ns\n", alloc_time_ns / NUM_ALLOCATIONS);
    seq_printf(m, "Avg free time: %llu ns\n", free_time_ns / NUM_ALLOCATIONS);
    seq_printf(m, "\nCheck /proc/slabinfo for cache details\n");
    return 0;
}

static int slab_demo_open(struct inode *inode, struct file *file)
{
    return single_open(file, slab_demo_show, NULL);
}

static const struct proc_ops slab_demo_ops = {
    .proc_open = slab_demo_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init slab_demo_init(void)
{
    int i;
    ktime_t start, end;
    
    pr_info("SLAB Demo: Initializing\n");
    
    // Create a custom slab cache
    demo_cache = kmem_cache_create(
        CACHE_NAME,
        sizeof(struct demo_object),
        0,                    // alignment (0 = default)
        SLAB_HWCACHE_ALIGN,  // flags: align to cache lines
        NULL                 // constructor
    );
    
    if (!demo_cache) {
        pr_err("SLAB Demo: Failed to create cache\n");
        return -ENOMEM;
    }
    
    // Allocate array to hold object pointers
    objects = kmalloc_array(NUM_ALLOCATIONS, sizeof(struct demo_object *), GFP_KERNEL);
    if (!objects) {
        kmem_cache_destroy(demo_cache);
        return -ENOMEM;
    }
    
    // Benchmark allocations
    start = ktime_get();
    for (i = 0; i < NUM_ALLOCATIONS; i++) {
        objects[i] = kmem_cache_alloc(demo_cache, GFP_KERNEL);
        if (!objects[i]) {
            pr_err("SLAB Demo: Allocation failed at %d\n", i);
            goto cleanup;
        }
        // Touch the memory to ensure it's paged in
        memset(objects[i], 0, sizeof(struct demo_object));
    }
    end = ktime_get();
    alloc_time_ns = ktime_to_ns(ktime_sub(end, start));
    
    pr_info("SLAB Demo: Allocated %d objects in %llu ns\n", 
            NUM_ALLOCATIONS, alloc_time_ns);
    
    // Benchmark frees
    start = ktime_get();
    for (i = 0; i < NUM_ALLOCATIONS; i++) {
        if (objects[i]) {
            kmem_cache_free(demo_cache, objects[i]);
            objects[i] = NULL;
        }
    }
    end = ktime_get();
    free_time_ns = ktime_to_ns(ktime_sub(end, start));
    
    pr_info("SLAB Demo: Freed %d objects in %llu ns\n", 
            NUM_ALLOCATIONS, free_time_ns);
    
    // Create proc entry
    proc_create("slab_demo", 0, NULL, &slab_demo_ops);
    
    pr_info("SLAB Demo: Module loaded. Check /proc/slab_demo and /proc/slabinfo\n");
    return 0;

cleanup:
    for (i = 0; i < NUM_ALLOCATIONS; i++) {
        if (objects[i]) {
            kmem_cache_free(demo_cache, objects[i]);
        }
    }
    kfree(objects);
    kmem_cache_destroy(demo_cache);
    return -ENOMEM;
}

static void __exit slab_demo_exit(void)
{
    remove_proc_entry("slab_demo", NULL);
    kfree(objects);
    kmem_cache_destroy(demo_cache);
    pr_info("SLAB Demo: Module unloaded\n");
}

module_init(slab_demo_init);
module_exit(slab_demo_exit);
