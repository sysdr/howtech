# Makefile for sched_ext CPU Selection Demo

CLANG ?= clang
LLC ?= llc
BPFTOOL ?= bpftool
CC = gcc

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
INCLUDES := -I./include -I/usr/include -I/usr/include/$(shell uname -m)-linux-gnu

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH)
CFLAGS := -Wall -Wextra -O2 -g $(INCLUDES)
LDFLAGS := -lbpf -lncurses -lelf -lz

BPF_OBJ := src/scheduler.bpf.o
PROGS := monitor interactive_workload batch_workload group_manager get_map_fd

.PHONY: all clean vmlinux

all: vmlinux $(BPF_OBJ) $(PROGS)

vmlinux:
	@if [ ! -f include/vmlinux.h ]; then \
		echo "Generating vmlinux.h..."; \
		$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > include/vmlinux.h; \
	fi

$(BPF_OBJ): src/scheduler.bpf.c include/scheduler.h include/vmlinux.h
	$(CLANG) $(BPF_CFLAGS) $(INCLUDES) -c $< -o $@

monitor: src/monitor.c include/scheduler.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

interactive_workload: src/interactive_workload.c
	$(CC) $(CFLAGS) $< -o $@

batch_workload: src/batch_workload.c
	$(CC) $(CFLAGS) $< -o $@

group_manager: src/group_manager.c
	$(CC) $(CFLAGS) $< -o $@ -lbpf

get_map_fd: src/get_map_fd.c
	$(CC) $(CFLAGS) $< -o $@ -lbpf

clean:
	rm -f $(BPF_OBJ) $(PROGS)
	rm -f include/vmlinux.h
