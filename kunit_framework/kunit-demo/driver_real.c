#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "driver_ops.h"

/* Simulated hardware buffer (in real driver, this would be device registers) */
static char hardware_buffer[4096];
static size_t hw_buffer_len = 0;

static int real_read(void *buf, size_t len, loff_t offset)
{
    if (offset >= hw_buffer_len)
        return 0;
    
    if (offset + len > hw_buffer_len)
        len = hw_buffer_len - offset;
    
    /* In real driver: ioread32(device_base + offset) */
    memcpy(buf, hardware_buffer + offset, len);
    
    pr_debug("Real HW read: %zu bytes at offset %lld\n", len, offset);
    return len;
}

static int real_write(const void *buf, size_t len, loff_t offset)
{
    if (offset + len > sizeof(hardware_buffer))
        return -ENOSPC;
    
    /* In real driver: iowrite32(val, device_base + offset) */
    memcpy(hardware_buffer + offset, buf, len);
    if (offset + len > hw_buffer_len)
        hw_buffer_len = offset + len;
    
    pr_debug("Real HW write: %zu bytes at offset %lld\n", len, offset);
    return len;
}

static int real_reset(void)
{
    /* In real driver: write to device reset register */
    memset(hardware_buffer, 0, sizeof(hardware_buffer));
    hw_buffer_len = 0;
    
    pr_debug("Real HW reset\n");
    return 0;
}

static void real_cleanup(void)
{
    /* In real driver: free DMA buffers, release IRQs, etc. */
    pr_debug("Real HW cleanup\n");
}

const struct device_ops device_real_ops = {
    .read = real_read,
    .write = real_write,
    .reset = real_reset,
    .cleanup = real_cleanup,
};
