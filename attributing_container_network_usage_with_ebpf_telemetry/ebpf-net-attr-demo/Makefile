CLANG    ?= clang
CC       ?= gcc
ARCH_RAW := $(shell uname -m)
ifeq ($(ARCH_RAW),x86_64)
ARCH := x86
else ifeq ($(ARCH_RAW),aarch64)
ARCH := arm64
else
ARCH := x86
endif

BPF_CFLAGS := -O2 -g -Wall -Wextra -target bpf \
              -D__TARGET_ARCH_$(ARCH) \
              -I/usr/include/$(shell uname -m)-linux-gnu

USER_CFLAGS := -O2 -Wall -Wextra -Werror -std=c11
USER_LDFLAGS := -lbpf -lncurses -lpthread

.PHONY: all clean
all: net_attr.bpf.o net_attr

net_attr.bpf.o: net_attr.bpf.c
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

net_attr: net_attr.c
	$(CC) $(USER_CFLAGS) -o $@ $< $(USER_LDFLAGS)

clean:
	rm -f net_attr.bpf.o net_attr
