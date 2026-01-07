/*
 * hello_module.c - Minimal kernel module demonstrating:
 * - Module init/exit
 * - Module parameters
 * - Kernel logging (printk)
 * - Module metadata
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

/* Module parameters */
static char *name = "World";
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "Name to greet (default: World)");

static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of times to greet (default: 1)");

/*
 * Module initialization function
 * Called when module is loaded with insmod/modprobe
 * Must return 0 on success, negative errno on failure
 */
static int __init hello_init(void)
{
    int i;
    
    printk(KERN_INFO "hello_module: Initializing...\n");
    printk(KERN_INFO "hello_module: Built against kernel %s\n", UTS_RELEASE);
    printk(KERN_INFO "hello_module: Module address: %p\n", hello_init);
    
    for (i = 0; i < count; i++) {
        printk(KERN_INFO "hello_module: [%d/%d] Hello, %s!\n", 
               i + 1, count, name);
    }
    
    printk(KERN_INFO "hello_module: Loaded successfully\n");
    return 0;
}

/*
 * Module cleanup function
 * Called when module is unloaded with rmmod
 * No return value (void)
 */
static void __exit hello_exit(void)
{
    int i;
    
    printk(KERN_INFO "hello_module: Shutting down...\n");
    
    for (i = 0; i < count; i++) {
        printk(KERN_INFO "hello_module: [%d/%d] Goodbye, %s!\n", 
               i + 1, count, name);
    }
    
    printk(KERN_INFO "hello_module: Unloaded successfully\n");
}

/* Register init and exit functions */
module_init(hello_init);
module_exit(hello_exit);

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("Minimal demonstration kernel module");
MODULE_VERSION("1.0");
