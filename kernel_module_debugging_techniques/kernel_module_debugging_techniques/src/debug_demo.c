/*
 * debug_demo.c - Kernel module demonstrating printk levels and debugging
 * 
 * This module demonstrates various kernel debugging techniques:
 * - Different printk log levels
 * - Rate-limited logging
 * - Dynamic debug
 * - Module parameters for runtime control
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("Kernel debugging demonstration module");
MODULE_VERSION("1.0");

/* Module parameters for runtime control */
static int log_level = KERN_INFO;
static int simulate_bug = 0;
static int loop_count = 5;

module_param(log_level, int, 0644);
MODULE_PARM_DESC(log_level, "Logging level (0-7)");
module_param(simulate_bug, int, 0644);
MODULE_PARM_DESC(simulate_bug, "Simulate various bugs (0=none, 1=null ptr, 2=memory leak)");
module_param(loop_count, int, 0644);
MODULE_PARM_DESC(loop_count, "Number of iterations for demonstrations");

/*
 * Demonstrate all printk log levels
 * Each level has specific use cases in production
 */
static void demonstrate_log_levels(void)
{
    ktime_t start, end;
    s64 elapsed_ns;
    
    printk(KERN_INFO "debug_demo: === Demonstrating all log levels ===\n");
    
    start = ktime_get();
    
    /* 0: KERN_EMERG - System is unusable */
    printk(KERN_EMERG "debug_demo: EMERG - System unusable (this is a test)\n");
    
    /* 1: KERN_ALERT - Action must be taken immediately */
    printk(KERN_ALERT "debug_demo: ALERT - Immediate action required (test)\n");
    
    /* 2: KERN_CRIT - Critical conditions */
    printk(KERN_CRIT "debug_demo: CRIT - Critical condition detected (test)\n");
    
    /* 3: KERN_ERR - Error conditions */
    printk(KERN_ERR "debug_demo: ERR - Error condition (test)\n");
    
    /* 4: KERN_WARNING - Warning conditions */
    printk(KERN_WARNING "debug_demo: WARNING - Warning condition (test)\n");
    
    /* 5: KERN_NOTICE - Normal but significant */
    printk(KERN_NOTICE "debug_demo: NOTICE - Normal but significant (test)\n");
    
    /* 6: KERN_INFO - Informational */
    printk(KERN_INFO "debug_demo: INFO - Informational message\n");
    
    /* 7: KERN_DEBUG - Debug-level messages (usually filtered) */
    printk(KERN_DEBUG "debug_demo: DEBUG - Debug message (may not appear)\n");
    
    end = ktime_get();
    elapsed_ns = ktime_to_ns(ktime_sub(end, start));
    
    printk(KERN_INFO "debug_demo: 8 printk() calls took %lld nanoseconds\n", elapsed_ns);
}

/*
 * Demonstrate rate-limited logging
 * Essential for preventing log buffer overflow in hot paths
 */
static void demonstrate_rate_limiting(void)
{
    int i;
    
    printk(KERN_INFO "debug_demo: === Demonstrating rate limiting ===\n");
    printk(KERN_INFO "debug_demo: Generating 100 log messages...\n");
    
    for (i = 0; i < 100; i++) {
        /* Without rate limiting - floods the log buffer */
        if (i < 10) {
            printk(KERN_INFO "debug_demo: Unrestricted log #%d\n", i);
        }
        
        /* With rate limiting - caps at 10 messages per 5 seconds by default */
        printk_ratelimited(KERN_INFO "debug_demo: Rate-limited log #%d\n", i);
        
        /* Small delay to avoid lockup */
        if (i % 10 == 0)
            msleep(1);
    }
    
    printk(KERN_INFO "debug_demo: Check dmesg - only ~10 rate-limited messages appeared\n");
}

/*
 * Demonstrate performance impact of printk
 * Especially with console output enabled
 */
static void measure_printk_overhead(void)
{
    ktime_t start, end;
    s64 elapsed_ns;
    int i;
    const int iterations = 1000;
    
    printk(KERN_INFO "debug_demo: === Measuring printk overhead ===\n");
    
    /* Measure printk() calls */
    start = ktime_get();
    for (i = 0; i < iterations; i++) {
        /* This goes to log buffer but may not hit console due to log level */
        pr_debug("debug_demo: Performance test iteration %d\n", i);
    }
    end = ktime_get();
    elapsed_ns = ktime_to_ns(ktime_sub(end, start));
    
    printk(KERN_INFO "debug_demo: %d pr_debug() calls: %lld ns total, %lld ns avg\n",
           iterations, elapsed_ns, elapsed_ns / iterations);
    
    /* Now with INFO level that might hit console */
    start = ktime_get();
    for (i = 0; i < 10; i++) {  /* Fewer iterations to avoid flooding */
        printk(KERN_INFO "debug_demo: Console output test %d\n", i);
    }
    end = ktime_get();
    elapsed_ns = ktime_to_ns(ktime_sub(end, start));
    
    printk(KERN_INFO "debug_demo: 10 KERN_INFO calls: %lld ns total, %lld ns avg\n",
           elapsed_ns, elapsed_ns / 10);
    printk(KERN_INFO "debug_demo: If console_loglevel allows, console_lock adds ~7ms per message\n");
}

/*
 * Demonstrate common debugging scenarios
 * These show what kgdb would help debug
 */
static void demonstrate_debugging_scenarios(void)
{
    void *ptr = NULL;
    void *leak = NULL;
    
    printk(KERN_INFO "debug_demo: === Debugging scenarios ===\n");
    
    switch (simulate_bug) {
    case 0:
        printk(KERN_INFO "debug_demo: No bugs simulated (simulate_bug=0)\n");
        printk(KERN_INFO "debug_demo: Set simulate_bug=1 or 2 to test error conditions\n");
        break;
        
    case 1:
        /* Null pointer dereference - would need kgdb to debug */
        printk(KERN_WARNING "debug_demo: Simulating null pointer scenario...\n");
        printk(KERN_WARNING "debug_demo: In real code, this would crash. kgdb would show:\n");
        printk(KERN_WARNING "debug_demo:   - Exact line where crash occurred\n");
        printk(KERN_WARNING "debug_demo:   - Full backtrace\n");
        printk(KERN_WARNING "debug_demo:   - Register state at crash\n");
        printk(KERN_INFO "debug_demo: (Not actually dereferencing to avoid crash)\n");
        /* In real scenario: value = *(int *)ptr;  <- Would crash here */
        break;
        
    case 2:
        /* Memory leak - would use kgdb watchpoints to track */
        printk(KERN_WARNING "debug_demo: Simulating memory leak...\n");
        leak = kmalloc(1024, GFP_KERNEL);
        if (leak) {
            printk(KERN_WARNING "debug_demo: Allocated 1KB at %p (not freeing!)\n", leak);
            printk(KERN_WARNING "debug_demo: kgdb watchpoint on this address would show:\n");
            printk(KERN_WARNING "debug_demo:   - When allocation occurred\n");
            printk(KERN_WARNING "debug_demo:   - Call stack at allocation\n");
            printk(KERN_WARNING "debug_demo:   - Why free() was never called\n");
            /* Intentionally not freeing to demonstrate leak */
        }
        break;
        
    default:
        printk(KERN_ERR "debug_demo: Invalid simulate_bug value: %d\n", simulate_bug);
    }
}

/*
 * Module initialization
 */
static int __init debug_demo_init(void)
{
    ktime_t module_start, module_end;
    s64 init_time_ns;
    
    module_start = ktime_get();
    
    printk(KERN_INFO "╔════════════════════════════════════════════════════════════╗\n");
    printk(KERN_INFO "║  Kernel Debugging Demo Module Loading                    ║\n");
    printk(KERN_INFO "╚════════════════════════════════════════════════════════════╝\n");
    
    printk(KERN_INFO "debug_demo: Module parameters:\n");
    printk(KERN_INFO "debug_demo:   log_level=%d, simulate_bug=%d, loop_count=%d\n",
           log_level, simulate_bug, loop_count);
    
    /* Run demonstrations */
    demonstrate_log_levels();
    demonstrate_rate_limiting();
    measure_printk_overhead();
    demonstrate_debugging_scenarios();
    
    module_end = ktime_get();
    init_time_ns = ktime_to_ns(ktime_sub(module_end, module_start));
    
    printk(KERN_INFO "debug_demo: Module initialization completed in %lld ns\n", init_time_ns);
    printk(KERN_INFO "debug_demo: Check 'dmesg | tail -50' to see all output\n");
    printk(KERN_INFO "debug_demo: Use 'cat /proc/kmsg' for live monitoring\n");
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit debug_demo_exit(void)
{
    printk(KERN_INFO "debug_demo: Module unloading...\n");
    printk(KERN_INFO "debug_demo: In production, ensure all resources are freed\n");
    printk(KERN_INFO "debug_demo: Use kgdb to verify cleanup in complex modules\n");
    printk(KERN_INFO "╚════════════════════════════════════════════════════════════╝\n");
}

module_init(debug_demo_init);
module_exit(debug_demo_exit);
