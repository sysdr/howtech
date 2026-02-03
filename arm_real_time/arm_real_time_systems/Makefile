CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -pthread
LDFLAGS = -lrt -lm

TARGETS = build/latency_test build/irq_monitor build/cache_polluter

all: $(TARGETS)

build/latency_test: src/latency_test.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/irq_monitor: src/irq_monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/cache_polluter: src/cache_polluter.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf build results

.PHONY: all clean
