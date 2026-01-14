#include <kunit/test.h>
#include "driver.h"
#include "driver_ops.h"

/* External mock control functions */
extern void mock_reset_state(void);
extern void mock_set_read_data(const char *data, size_t len);
extern void mock_inject_error(int read_error, int write_error);
extern int mock_get_read_count(void);
extern int mock_get_write_count(void);
extern int mock_get_reset_count(void);

/* Test fixture */
struct demo_test_context {
    struct demo_device *dev;
};

static int demo_test_init(struct kunit *test)
{
    struct demo_test_context *ctx;
    
    ctx = kunit_kzalloc(test, sizeof(*ctx), GFP_KERNEL);
    KUNIT_ASSERT_NOT_NULL(test, ctx);
    
    ctx->dev = kunit_kzalloc(test, sizeof(*ctx->dev), GFP_KERNEL);
    KUNIT_ASSERT_NOT_NULL(test, ctx->dev);
    
    /* Use mock ops for testing */
    ctx->dev->ops = &device_mock_ops;
    
    /* Reset mock state */
    mock_reset_state();
    
    test->priv = ctx;
    return 0;
}

/* Test: Successful read operation */
static void test_read_success(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    char buf[64];
    const char *test_data = "KUnit testing rocks!";
    int ret;
    
    /* Setup mock to return test data */
    mock_set_read_data(test_data, strlen(test_data));
    
    /* Perform read */
    ret = demo_read_from_device(ctx->dev, buf, sizeof(buf));
    
    /* Verify results */
    KUNIT_EXPECT_EQ(test, ret, (int)strlen(test_data));
    KUNIT_EXPECT_EQ(test, mock_get_read_count(), 1);
    KUNIT_EXPECT_MEMEQ(test, buf, test_data, strlen(test_data));
}

/* Test: Read with error injection */
static void test_read_error_handling(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    char buf[64];
    int ret;
    
    /* Inject read error */
    mock_inject_error(-EIO, 0);
    
    /* Perform read */
    ret = demo_read_from_device(ctx->dev, buf, sizeof(buf));
    
    /* Verify error is propagated */
    KUNIT_EXPECT_EQ(test, ret, -EIO);
    KUNIT_EXPECT_EQ(test, mock_get_read_count(), 1);
}

/* Test: Successful write operation */
static void test_write_success(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    const char *test_data = "Writing kernel data";
    int ret;
    
    /* Perform write */
    ret = demo_write_to_device(ctx->dev, test_data, strlen(test_data));
    
    /* Verify results */
    KUNIT_EXPECT_EQ(test, ret, (int)strlen(test_data));
    KUNIT_EXPECT_EQ(test, mock_get_write_count(), 1);
}

/* Test: Write with error injection */
static void test_write_error_handling(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    const char *test_data = "This will fail";
    int ret;
    
    /* Inject write error */
    mock_inject_error(0, -ENOSPC);
    
    /* Perform write */
    ret = demo_write_to_device(ctx->dev, test_data, strlen(test_data));
    
    /* Verify error is propagated */
    KUNIT_EXPECT_EQ(test, ret, -ENOSPC);
    KUNIT_EXPECT_EQ(test, mock_get_write_count(), 1);
}

/* Test: Device reset */
static void test_device_reset(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    int ret;
    
    /* Perform reset */
    ret = demo_reset_device(ctx->dev);
    
    /* Verify success */
    KUNIT_EXPECT_EQ(test, ret, 0);
    KUNIT_EXPECT_EQ(test, mock_get_reset_count(), 1);
}

/* Test: NULL pointer handling */
static void test_null_device_handling(struct kunit *test)
{
    char buf[64];
    int ret;
    
    /* Try operations with NULL device */
    ret = demo_read_from_device(NULL, buf, sizeof(buf));
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);
    
    ret = demo_write_to_device(NULL, buf, sizeof(buf));
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);
    
    ret = demo_reset_device(NULL);
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

/* Test: Read-write-read cycle */
static void test_read_write_cycle(struct kunit *test)
{
    struct demo_test_context *ctx = test->priv;
    const char *write_data = "Cycle test data";
    char read_buf[64];
    int ret;
    
    /* Write data */
    ret = demo_write_to_device(ctx->dev, write_data, strlen(write_data));
    KUNIT_EXPECT_EQ(test, ret, (int)strlen(write_data));
    
    /* Read it back */
    ret = demo_read_from_device(ctx->dev, read_buf, sizeof(read_buf));
    KUNIT_EXPECT_EQ(test, ret, (int)strlen(write_data));
    
    /* Verify data matches */
    KUNIT_EXPECT_MEMEQ(test, read_buf, write_data, strlen(write_data));
    
    /* Verify call counts */
    KUNIT_EXPECT_EQ(test, mock_get_write_count(), 1);
    KUNIT_EXPECT_EQ(test, mock_get_read_count(), 1);
}

/* Register test cases */
static struct kunit_case demo_test_cases[] = {
    KUNIT_CASE(test_read_success),
    KUNIT_CASE(test_read_error_handling),
    KUNIT_CASE(test_write_success),
    KUNIT_CASE(test_write_error_handling),
    KUNIT_CASE(test_device_reset),
    KUNIT_CASE(test_null_device_handling),
    KUNIT_CASE(test_read_write_cycle),
    {}
};

/* Define test suite */
static struct kunit_suite demo_test_suite = {
    .name = "kunit_demo_driver",
    .init = demo_test_init,
    .test_cases = demo_test_cases,
};

kunit_test_suites(&demo_test_suite);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Systems Programming Deep Dive");
MODULE_DESCRIPTION("KUnit tests for demo driver");
