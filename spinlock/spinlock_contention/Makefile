CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -pthread -std=gnu11
LDFLAGS = -pthread

TARGETS = build/spinlock_livelock build/rcu_stall build/monitor

all: $(TARGETS)

build/spinlock_livelock: src/spinlock_livelock.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/rcu_stall: src/rcu_stall.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build/monitor: src/monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf build

.PHONY: all clean
