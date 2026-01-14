#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "driver.h"

static dev_t dev_number;
static struct class *demo_class;
static struct demo_device *demo_dev;

/* Core driver logic - uses ops interface for hardware interaction */
int demo_read_from_device(struct demo_device *dev, void *buf, size_t len)
{
    int ret;
    
    if (!dev || !dev->ops || !dev->ops->read)
        return -EINVAL;
    
    ret = dev->ops->read(buf, len, 0);
    if (ret < 0)
        return ret;
    
    return ret;
}
EXPORT_SYMBOL_GPL(demo_read_from_device);

int demo_write_to_device(struct demo_device *dev, const void *buf, size_t len)
{
    int ret;
    
    if (!dev || !dev->ops || !dev->ops->write)
        return -EINVAL;
    
    ret = dev->ops->write(buf, len, 0);
    if (ret < 0)
        return ret;
    
    return ret;
}
EXPORT_SYMBOL_GPL(demo_write_to_device);

int demo_reset_device(struct demo_device *dev)
{
    if (!dev || !dev->ops || !dev->ops->reset)
        return -EINVAL;
    
    return dev->ops->reset();
}
EXPORT_SYMBOL_GPL(demo_reset_device);

/* Character device operations */
static int demo_open(struct inode *inode, struct file *file)
{
    struct demo_device *dev = container_of(inode->i_cdev, 
                                           struct demo_device, cdev);
    file->private_data = dev;
    return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t demo_read(struct file *file, char __user *buf, 
                         size_t len, loff_t *offset)
{
    struct demo_device *dev = file->private_data;
    char *kbuf;
    int ret;
    
    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;
    
    ret = demo_read_from_device(dev, kbuf, len);
    if (ret < 0)
        goto out;
    
    if (copy_to_user(buf, kbuf, ret)) {
        ret = -EFAULT;
        goto out;
    }
    
out:
    kfree(kbuf);
    return ret;
}

static ssize_t demo_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    struct demo_device *dev = file->private_data;
    char *kbuf;
    int ret;
    
    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;
    
    if (copy_from_user(kbuf, buf, len)) {
        ret = -EFAULT;
        goto out;
    }
    
    ret = demo_write_to_device(dev, kbuf, len);
    
out:
    kfree(kbuf);
    return ret;
}

static const struct file_operations demo_fops = {
    .owner = THIS_MODULE,
    .open = demo_open,
    .release = demo_release,
    .read = demo_read,
    .write = demo_write,
};

static int __init demo_init(void)
{
    int ret;
    struct device *device;
    
    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }
    
    /* Create device class */
    demo_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(demo_class)) {
        ret = PTR_ERR(demo_class);
        goto err_class;
    }
    
    /* Allocate device structure */
    demo_dev = kzalloc(sizeof(*demo_dev), GFP_KERNEL);
    if (!demo_dev) {
        ret = -ENOMEM;
        goto err_alloc;
    }
    
    /* Initialize device with real hardware ops (production) */
    demo_dev->ops = &device_real_ops;
    
    /* Initialize character device */
    cdev_init(&demo_dev->cdev, &demo_fops);
    demo_dev->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&demo_dev->cdev, dev_number, 1);
    if (ret < 0)
        goto err_cdev;
    
    /* Create device node */
    device = device_create(demo_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        ret = PTR_ERR(device);
        goto err_device;
    }
    
    pr_info("KUnit demo driver loaded: /dev/%s\n", DEVICE_NAME);
    return 0;
    
err_device:
    cdev_del(&demo_dev->cdev);
err_cdev:
    kfree(demo_dev);
err_alloc:
    class_destroy(demo_class);
err_class:
    unregister_chrdev_region(dev_number, 1);
    return ret;
}

static void __exit demo_exit(void)
{
    if (demo_dev->ops && demo_dev->ops->cleanup)
        demo_dev->ops->cleanup();
    
    device_destroy(demo_class, dev_number);
    cdev_del(&demo_dev->cdev);
    kfree(demo_dev);
    class_destroy(demo_class);
    unregister_chrdev_region(dev_number, 1);
    
    pr_info("KUnit demo driver unloaded\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("KUnit demo driver with dependency injection");
MODULE_VERSION("1.0");
