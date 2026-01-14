#ifndef DRIVER_OPS_H
#define DRIVER_OPS_H

#include <linux/types.h>

/* Abstract interface for device operations (dependency injection) */
struct device_ops {
    int (*read)(void *buf, size_t len, loff_t offset);
    int (*write)(const void *buf, size_t len, loff_t offset);
    int (*reset)(void);
    void (*cleanup)(void);
};

/* Real hardware implementation */
extern const struct device_ops device_real_ops;

/* Mock implementation for testing */
extern const struct device_ops device_mock_ops;

#endif /* DRIVER_OPS_H */
