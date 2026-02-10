# Makefile for multi-level FIFO scheduler

CLANG ?= clang
LLC ?= llc
BPFTOOL ?= bpftool
CC = gcc

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
INCLUDES := -I/usr/include -I./include -I/usr/include/$(shell uname -m)-linux-gnu
BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH)
CFLAGS := -Wall -Wextra -Wno-unused-parameter -O2 -g $(INCLUDES)
LDFLAGS := -lbpf -lelf -lz

BPF_OBJ := multi_level_sched.bpf.o
VMLINUX := include/vmlinux.h

PROGS := mlq_loader mlq_monitor workload_gen

.PHONY: all clean vmlinux

all: vmlinux $(PROGS)
	@-$(MAKE) $(BPF_OBJ) 2>/dev/null || echo "Note: BPF build skipped (requires kernel BTF support)"

vmlinux:
	@echo "Generating vmlinux.h..."
	@if command -v bpftool >/dev/null 2>&1 && [ -f /sys/kernel/btf/vmlinux ]; then \
		bpftool btf dump file /sys/kernel/btf/vmlinux format c > $(VMLINUX) 2>/dev/null || \
		echo "/* vmlinux.h placeholder - BTF not available */" > $(VMLINUX); \
	else \
		echo "/* vmlinux.h placeholder - bpftool not available */" > $(VMLINUX); \
	fi

$(BPF_OBJ): src/multi_level_sched.bpf.c $(VMLINUX)
	@echo "Building BPF object (may fail without proper kernel BTF)..."
	@$(CLANG) $(BPF_CFLAGS) $(INCLUDES) -c $< -o $@ 2>&1 || (echo "BPF build failed - this is expected on systems without kernel BTF" && exit 0)

mlq_loader: src/mlq_loader.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

mlq_monitor: src/mlq_monitor.c
	$(CC) $(CFLAGS) $< -o $@ -lncurses $(LDFLAGS)

workload_gen: src/workload_gen.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BPF_OBJ) $(PROGS) $(VMLINUX)
	rm -rf build/*
