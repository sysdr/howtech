CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -pthread
TARGETS = build/spinlock_test build/mutex_test build/monitor

all: $(TARGETS)

build/spinlock_test: src/spinlock_test.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

build/mutex_test: src/mutex_test.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

build/monitor: src/monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build logs

.PHONY: all clean
