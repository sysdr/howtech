CLANG ?= clang
LLVM_STRIP ?= llvm-strip
BPFTOOL ?= bpftool
LIBBPF_DIR = /usr/include
ARCH := $(shell uname -m | sed 's/x86_64/x86/')

INCLUDES := -I$(LIBBPF_DIR) -I.
CFLAGS := -g -Wall -Wextra -Werror
BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH)

.PHONY: all clean

all: netcorr_user test_client monitor

# Generate vmlinux.h (BTF)
vmlinux.h:
	@if [ ! -f /sys/kernel/btf/vmlinux ]; then \
		echo "Warning: BTF not available, using fallback headers"; \
		touch vmlinux.h; \
	elif ! command -v $(BPFTOOL) >/dev/null 2>&1; then \
		echo "Warning: bpftool not found, using fallback headers"; \
		touch vmlinux.h; \
	else \
		$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h || touch vmlinux.h; \
	fi

# Compile BPF program
netcorr.bpf.o: netcorr.bpf.c vmlinux.h
	$(CLANG) $(BPF_CFLAGS) $(INCLUDES) -c netcorr.bpf.c -o netcorr.bpf.o
	$(LLVM_STRIP) -g netcorr.bpf.o

# Generate skeleton
netcorr.skel.h: netcorr.bpf.o
	@if ! command -v $(BPFTOOL) >/dev/null 2>&1; then \
		echo "Error: bpftool is required to generate skeleton. Please install linux-tools-generic or bpftool."; \
		exit 1; \
	fi
	$(BPFTOOL) gen skeleton netcorr.bpf.o > netcorr.skel.h

# Compile userspace program
netcorr_user: netcorr_user.c netcorr.skel.h
	$(CC) $(CFLAGS) $(INCLUDES) netcorr_user.c -o netcorr_user -lbpf -lelf -lz

# Compile test client
test_client: test_client.c
	$(CC) $(CFLAGS) test_client.c -o test_client

# Compile monitor
monitor: monitor.c
	$(CC) $(CFLAGS) $(INCLUDES) monitor.c -o monitor -lncurses -lbpf -lelf -lz

clean:
	rm -f *.o netcorr_user test_client monitor netcorr.skel.h vmlinux.h
