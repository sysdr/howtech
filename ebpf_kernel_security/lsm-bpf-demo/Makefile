CLANG := clang
LLC := llc
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
KERNEL_SRC := /lib/modules/$(shell uname -r)/build
INCLUDES := -I$(KERNEL_SRC)/arch/$(ARCH)/include \
	-I$(KERNEL_SRC)/arch/$(ARCH)/include/uapi \
	-I$(KERNEL_SRC)/arch/$(ARCH)/include/generated/uapi \
	-I$(KERNEL_SRC)/include \
	-I$(KERNEL_SRC)/include/uapi \
	-I$(KERNEL_SRC)/include/generated/uapi \
	-I/usr/include \
	-I/usr/include/$(shell uname -m)-linux-gnu

all: build/file_policy.bpf.o build/loader build/test_access build/monitor

build/file_policy.bpf.o: src/file_policy.bpf.c
	@mkdir -p build
	$(CLANG) -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) \
		-D__BPF_TRACING__ \
		$(INCLUDES) \
		-c src/file_policy.bpf.c -o build/file_policy.bpf.o

build/loader: src/loader.c
	$(CLANG) -g -Wall -Wextra -O2 \
		src/loader.c -o build/loader \
		-lbpf -lelf -lz

build/test_access: src/test_access.c
	$(CLANG) -g -Wall -Wextra -O2 \
		src/test_access.c -o build/test_access

build/monitor: src/monitor.c
	$(CLANG) -g -Wall -Wextra -O2 \
		src/monitor.c -o build/monitor \
		-lncurses

clean:
	rm -rf build

.PHONY: all clean
