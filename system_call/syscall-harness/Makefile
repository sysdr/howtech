CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -std=gnu11
LDFLAGS = -pthread

TARGETS = syscall_test monitor

all: $(TARGETS)

syscall_test: src/syscall_test.c
	$(CC) $(CFLAGS) $< -o build/$@ $(LDFLAGS)

monitor: src/monitor.c
	$(CC) $(CFLAGS) $< -o build/$@ -lncurses

clean:
	rm -f build/*

.PHONY: all clean

