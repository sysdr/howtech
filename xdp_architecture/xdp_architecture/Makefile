CC = clang
CFLAGS = -O2 -g -Wall -Wextra -Werror
BPF_CFLAGS = -target bpf -D__TARGET_ARCH_x86 -I/usr/include/x86_64-linux-gnu

all: build/xdp_drop.o build/packet_gen build/xdp_monitor

build/xdp_drop.o: src/xdp_drop.c
	@mkdir -p build
	$(CC) $(BPF_CFLAGS) -c $< -o $@

build/packet_gen: src/packet_gen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@

build/xdp_monitor: src/xdp_monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ -lbpf -lncurses

clean:
	rm -rf build

.PHONY: all clean
