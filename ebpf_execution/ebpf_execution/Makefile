CLANG := clang
LLC := llc
CC := gcc
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

LIBBPF_CFLAGS := $(shell pkg-config --cflags libbpf 2>/dev/null || echo "-I/usr/include")
LIBBPF_LIBS := $(shell pkg-config --libs libbpf 2>/dev/null || echo "-lbpf -lelf -lz")

# Extra include paths so clang -target bpf can find asm/types.h etc. on Ubuntu/WSL
BPF_SYS_INCLUDES := -I/usr/include -I/usr/include/$(shell gcc -dumpmachine)

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) $(BPF_SYS_INCLUDES)
CFLAGS := -Wall -Wextra -O2 -g $(LIBBPF_CFLAGS)
LDFLAGS := $(LIBBPF_LIBS)
NCURSES_LDFLAGS := -lncurses

.PHONY: all clean check-deps

# Check for required dependencies
check-deps:
	@echo "Checking dependencies..."
	@missing=0; \
	if ! command -v $(CLANG) >/dev/null 2>&1; then \
		echo "ERROR: clang is not installed"; \
		missing=1; \
	fi; \
	if ! command -v $(CC) >/dev/null 2>&1; then \
		echo "ERROR: gcc is not installed"; \
		missing=1; \
	fi; \
	if ! pkg-config --exists libbpf 2>/dev/null && [ ! -f /usr/include/bpf/libbpf.h ]; then \
		echo "WARNING: libbpf-dev may not be installed"; \
	fi; \
	if [ $$missing -eq 1 ]; then \
		echo ""; \
		echo "To install missing dependencies, run:"; \
		echo "  sudo apt-get install -y clang llvm libelf-dev libbpf-dev bpftool libncurses5-dev libncursesw5-dev pkg-config"; \
		echo ""; \
		echo "Or run the demo script which will install them automatically:"; \
		echo "  sudo ./demo.sh"; \
		exit 1; \
	fi; \
	echo "✓ All required dependencies found"

all: check-deps build/counter.bpf.o build/loader build/monitor

build/counter.bpf.o: src/counter.bpf.c
	@mkdir -p build
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

build/loader: src/loader.c build/counter.bpf.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/monitor: src/monitor.c
	$(CC) $(CFLAGS) $< -o $@ $(NCURSES_LDFLAGS)

clean:
	rm -rf build output
