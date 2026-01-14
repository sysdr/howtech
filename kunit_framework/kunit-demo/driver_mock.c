#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <kunit/test.h>
#include "driver_ops.h"

/* Mock state for tracking calls and injecting behavior */
struct mock_state {
    int read_count;
    int write_count;
    int reset_count;
    int inject_read_error;
    int inject_write_error;
    char test_data[4096];
    size_t test_data_len;
};

static struct mock_state mock;

/* Reset mock state between tests */
void mock_reset_state(void)
{
    memset(&mock, 0, sizeof(mock));
}

/* Configure mock to return specific data */
void mock_set_read_data(const char *data, size_t len)
{
    if (len > sizeof(mock.test_data))
        len = sizeof(mock.test_data);
    memcpy(mock.test_data, data, len);
    mock.test_data_len = len;
}

/* Configure mock to inject errors */
void mock_inject_error(int read_error, int write_error)
{
    mock.inject_read_error = read_error;
    mock.inject_write_error = write_error;
}

/* Get call counts for verification */
int mock_get_read_count(void) { return mock.read_count; }
int mock_get_write_count(void) { return mock.write_count; }
int mock_get_reset_count(void) { return mock.reset_count; }

static int mock_read(void *buf, size_t len, loff_t offset)
{
    mock.read_count++;
    
    if (mock.inject_read_error)
        return mock.inject_read_error;
    
    if (offset >= mock.test_data_len)
        return 0;
    
    if (offset + len > mock.test_data_len)
        len = mock.test_data_len - offset;
    
    memcpy(buf, mock.test_data + offset, len);
    return len;
}

static int mock_write(const void *buf, size_t len, loff_t offset)
{
    mock.write_count++;
    
    if (mock.inject_write_error)
        return mock.inject_write_error;
    
    if (offset + len > sizeof(mock.test_data))
        return -ENOSPC;
    
    memcpy(mock.test_data + offset, buf, len);
    if (offset + len > mock.test_data_len)
        mock.test_data_len = offset + len;
    
    return len;
}

static int mock_reset(void)
{
    mock.reset_count++;
    memset(mock.test_data, 0, sizeof(mock.test_data));
    mock.test_data_len = 0;
    return 0;
}

static void mock_cleanup(void)
{
    /* Mock cleanup - nothing to do */
}

const struct device_ops device_mock_ops = {
    .read = mock_read,
    .write = mock_write,
    .reset = mock_reset,
    .cleanup = mock_cleanup,
};

/* Export mock control functions for tests */
EXPORT_SYMBOL_GPL(mock_reset_state);
EXPORT_SYMBOL_GPL(mock_set_read_data);
EXPORT_SYMBOL_GPL(mock_inject_error);
EXPORT_SYMBOL_GPL(mock_get_read_count);
EXPORT_SYMBOL_GPL(mock_get_write_count);
EXPORT_SYMBOL_GPL(mock_get_reset_count);
