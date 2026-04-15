CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O2 -g
LDFLAGS ?=

.PHONY: all clean

all: sched_bench sched_monitor

sched_bench: sched_bench.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

sched_monitor: sched_monitor.c
	$(CC) $(CFLAGS) -o $@ $< -lncurses

clean:
	rm -f sched_bench sched_monitor

