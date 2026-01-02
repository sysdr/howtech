#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("Custom syscall demonstration module");

/* Our custom syscall implementation */
static long custom_operation(unsigned long arg1, unsigned long arg2, char __user *buffer, size_t len)
{
    char *kbuf;
    int ret;
    unsigned long result;
    
    /* Input validation */
    if (len == 0 || len > 4096) {
        return -EINVAL;
    }
    
    if (!buffer) {
        return -EFAULT;
    }
    
    /* Allocate kernel buffer - use GFP_KERNEL since we can sleep */
    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf) {
        return -ENOMEM;
    }
    
    /* CRITICAL: Must use copy_from_user, not direct dereference */
    ret = copy_from_user(kbuf, buffer, len);
    if (ret != 0) {
        pr_err("custom_syscall: copy_from_user failed, %d bytes not copied\n", ret);
        kfree(kbuf);
        return -EFAULT;
    }
    
    /* Perform our "operation" - simple calculation for demo */
    result = arg1 + arg2;
    
    pr_info("custom_syscall: arg1=%lu, arg2=%lu, result=%lu, buffer=\"%s\"\n",
            arg1, arg2, result, kbuf);
    
    /* Simulate some work */
    snprintf(kbuf, len, "Result: %lu (args: %lu + %lu)", result, arg1, arg2);
    
    /* CRITICAL: Must use copy_to_user to write back */
    ret = copy_to_user(buffer, kbuf, len);
    if (ret != 0) {
        pr_err("custom_syscall: copy_to_user failed\n");
        kfree(kbuf);
        return -EFAULT;
    }
    
    kfree(kbuf);
    
    /* Return negative errno on error, positive value on success */
    return result;
}

/* Device file operations for demonstration */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct {
        unsigned long arg1;
        unsigned long arg2;
        char buffer[256];
        size_t len;
    } params;
    
    /* Copy parameters from userspace */
    if (copy_from_user(&params, (void __user *)arg, sizeof(params))) {
        return -EFAULT;
    }
    
    /* Call our syscall-like function */
    return custom_operation(params.arg1, params.arg2, params.buffer, params.len);
}

static int device_open(struct inode *inode, struct file *file)
{
    pr_info("custom_syscall: device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("custom_syscall: device closed\n");
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static int major_number;

static int __init custom_syscall_init(void)
{
    pr_info("custom_syscall: Module loading\n");
    
    /* Register character device (alternative to syscall table modification) */
    major_number = register_chrdev(0, "custom_syscall", &fops);
    if (major_number < 0) {
        pr_err("custom_syscall: Failed to register device\n");
        return major_number;
    }
    
    pr_info("custom_syscall: Registered with major number %d\n", major_number);
    pr_info("custom_syscall: Create device with: mknod /dev/custom_syscall c %d 0\n", major_number);
    
    return 0;
}

static void __exit custom_syscall_exit(void)
{
    unregister_chrdev(major_number, "custom_syscall");
    pr_info("custom_syscall: Module unloaded\n");
}

module_init(custom_syscall_init);
module_exit(custom_syscall_exit);
