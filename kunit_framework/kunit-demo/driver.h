#ifndef DRIVER_H
#define DRIVER_H

#include <linux/cdev.h>
#include <linux/device.h>
#include "driver_ops.h"

#define DEVICE_NAME "kunit_demo"
#define BUFFER_SIZE 4096

/* Driver private data structure */
struct demo_device {
    struct cdev cdev;
    const struct device_ops *ops;
    void *private_data;
    size_t data_len;
};

/* Exported for testing */
int demo_read_from_device(struct demo_device *dev, void *buf, size_t len);
int demo_write_to_device(struct demo_device *dev, const void *buf, size_t len);
int demo_reset_device(struct demo_device *dev);

#endif /* DRIVER_H */
