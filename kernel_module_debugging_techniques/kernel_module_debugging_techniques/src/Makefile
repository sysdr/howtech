obj-m += debug_demo.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# For debugging the module itself
debug:
	$(MAKE) -C $(KDIR) M=$(PWD) EXTRA_CFLAGS="-DDEBUG" modules
