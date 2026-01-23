CLANG := clang
LLC := llc
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

BPF_CFLAGS := -O2 -target bpf -D__TARGET_ARCH_$(ARCH)
BPF_CFLAGS += -Wall -Wextra -Werror

PASSING_SRCS := $(wildcard src/passing/*.c)
FAILING_SRCS := $(wildcard src/failing/*.c)

PASSING_OBJS := $(PASSING_SRCS:src/passing/%.c=build/passing_%.o)
FAILING_OBJS := $(FAILING_SRCS:src/failing/%.c=build/failing_%.o)

.PHONY: all clean passing failing monitor

all: passing failing monitor

passing: $(PASSING_OBJS)

failing: $(FAILING_OBJS)

build/passing_%.o: src/passing/%.c
	@echo "Compiling $< (should PASS verification)..."
	@mkdir -p build
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

build/failing_%.o: src/failing/%.c
	@echo "Compiling $< (should FAIL verification)..."
	@mkdir -p build
	-$(CLANG) $(BPF_CFLAGS) -c $< -o $@ 2>&1 | tee logs/$(notdir $(@:.o=.log)) || true

monitor: src/monitor/log_parser.c
	@echo "Building log parser..."
	gcc -O2 -Wall -Wextra -o build/log_parser src/monitor/log_parser.c

clean:
	rm -rf build logs *.o
