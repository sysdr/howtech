# BPF uses kernel UAPI headers (not glibc multiarch) to avoid gnu/stubs-32.h pulls.

UNAME_R  := $(shell uname -r)
KBUILD   := /lib/modules/$(UNAME_R)/build
ifeq ($(wildcard $(KBUILD)/Makefile),)
  KBUILD := $(shell ls -dv /usr/src/linux-headers-* 2>/dev/null | tail -1)
endif
ifeq ($(wildcard $(KBUILD)/Makefile),)
  $(error Install kernel headers, e.g. linux-headers-$(UNAME_R) or linux-headers-generic)
endif

ARCH_CPU := $(shell uname -m)
ARCH     := $(shell echo $(ARCH_CPU) | sed -e 's/x86_64/x86/' -e 's/aarch64/arm64/')
CLANG    := clang
CC       := gcc

# UAPI first so <linux/*.h> resolves to uapi; append $(KBUILD)/include for compiler_types.h only.
BPF_CFLAGS := -O2 -g -target bpf \
              -D__TARGET_ARCH_$(ARCH) \
              -I$(KBUILD)/arch/$(ARCH)/include/uapi \
              -I$(KBUILD)/arch/$(ARCH)/include/generated/uapi \
              -I$(KBUILD)/include/uapi \
              -I$(KBUILD)/include/generated/uapi \
              -I$(KBUILD)/include \
              -I$(KBUILD)/arch/$(ARCH)/include/generated \
              -I$(KBUILD)/arch/$(ARCH)/include \
              -Wall -Wno-unused-value -Wno-pointer-sign \
              -Wno-compare-distinct-pointer-types

CFLAGS := -Wall -Wextra -Werror -O2 -g
LDLIBS := -lbpf -lncursesw -lpthread

.PHONY: all clean

all: policy_engine.bpf.o policy_engine

policy_engine.bpf.o: policy_engine.bpf.c
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

policy_engine: policy_engine.c policy_engine.bpf.o
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f policy_engine.bpf.o policy_engine
