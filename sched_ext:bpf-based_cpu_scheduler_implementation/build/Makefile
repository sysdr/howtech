CLANG ?= clang
LLC ?= llc
BPFTOOL ?= bpftool

BPF_CFLAGS = -g -O2 -target bpf -D__TARGET_ARCH_x86 -I./include -I./src
CFLAGS = -g -O2 -Wall -Wextra

LIBBPF_CFLAGS = $(shell pkg-config --cflags libbpf 2>/dev/null || echo "-I/usr/include")
LIBBPF_LDFLAGS = $(shell pkg-config --libs libbpf 2>/dev/null || echo "-lbpf -lelf -lz")

BPF_OBJ = scx_dsq_demo.bpf.o
VMLINUX = vmlinux.h

all: $(BPF_OBJ) loader monitor

vmlinux.h:
	@echo "Generating vmlinux.h..."
	@if command -v $(BPFTOOL) >/dev/null 2>&1 && [ -f /sys/kernel/btf/vmlinux ]; then \
		$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > src/$(VMLINUX) 2>/dev/null && \
		echo "Generated vmlinux.h from BTF"; \
	else \
		echo "Creating fallback vmlinux.h..."; \
		echo "/* Fallback vmlinux.h - Basic type definitions for BPF compilation */" > src/$(VMLINUX); \
		echo "#ifndef __VMLINUX_H__" >> src/$(VMLINUX); \
		echo "#define __VMLINUX_H__" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "/* Standard integer types */" >> src/$(VMLINUX); \
		echo "typedef unsigned char __u8;" >> src/$(VMLINUX); \
		echo "typedef signed char __s8;" >> src/$(VMLINUX); \
		echo "typedef short int __s16;" >> src/$(VMLINUX); \
		echo "typedef short unsigned int __u16;" >> src/$(VMLINUX); \
		echo "typedef int __s32;" >> src/$(VMLINUX); \
		echo "typedef unsigned int __u32;" >> src/$(VMLINUX); \
		echo "typedef long long int __s64;" >> src/$(VMLINUX); \
		echo "typedef long long unsigned int __u64;" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "/* BPF-friendly type aliases */" >> src/$(VMLINUX); \
		echo "typedef __u8 u8;" >> src/$(VMLINUX); \
		echo "typedef __s8 s8;" >> src/$(VMLINUX); \
		echo "typedef __s16 s16;" >> src/$(VMLINUX); \
		echo "typedef __u16 u16;" >> src/$(VMLINUX); \
		echo "typedef __s32 s32;" >> src/$(VMLINUX); \
		echo "typedef __u32 u32;" >> src/$(VMLINUX); \
		echo "typedef __s64 s64;" >> src/$(VMLINUX); \
		echo "typedef __u64 u64;" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "/* Network-related types (needed by BPF helpers) */" >> src/$(VMLINUX); \
		echo "typedef __u16 __be16;" >> src/$(VMLINUX); \
		echo "typedef __u32 __be32;" >> src/$(VMLINUX); \
		echo "typedef __u32 __wsum;" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "/* Forward declarations */" >> src/$(VMLINUX); \
		echo "struct task_struct {" >> src/$(VMLINUX); \
		echo "	__u32 pid;" >> src/$(VMLINUX); \
		echo "	int static_prio;" >> src/$(VMLINUX); \
		echo "	void *cpus_ptr;" >> src/$(VMLINUX); \
		echo "};" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "struct scx_exit_info {" >> src/$(VMLINUX); \
		echo "	char *msg;" >> src/$(VMLINUX); \
		echo "	int kind;" >> src/$(VMLINUX); \
		echo "};" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "#define SCX_DSQ_GLOBAL 0xffffffffffffffffULL" >> src/$(VMLINUX); \
		echo "" >> src/$(VMLINUX); \
		echo "#endif /* __VMLINUX_H__ */" >> src/$(VMLINUX); \
	fi

$(BPF_OBJ): vmlinux.h src/scx_dsq_demo.bpf.c
	$(CLANG) $(BPF_CFLAGS) -c src/scx_dsq_demo.bpf.c -o $(BPF_OBJ)

loader: src/loader.c
	$(CC) $(CFLAGS) $(LIBBPF_CFLAGS) -o loader src/loader.c $(LIBBPF_LDFLAGS)

monitor: src/monitor.c
	$(CC) $(CFLAGS) $(LIBBPF_CFLAGS) -o monitor src/monitor.c $(LIBBPF_LDFLAGS) -lncurses

clean:
	rm -f $(BPF_OBJ) loader monitor src/$(VMLINUX)

.PHONY: all clean
