# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_KUNIT_DEMO_DRIVER) += kunit_demo_driver.o
kunit_demo_driver-y := driver.o driver_real.o driver_mock.o

obj-$(CONFIG_KUNIT_DEMO_TESTS) += kunit_demo_tests.o
kunit_demo_tests-y := driver_test.o

# For building outside kernel tree (not used with kunit.py)
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

help:
	@echo "make              - Build the modules"
	@echo "make clean        - Clean build artifacts"
