CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -pthread -g
LDFLAGS = -lncurses -lpthread -lm

TARGETS = counter_bench percpu_monitor

.PHONY: all clean

all: $(TARGETS)

counter_bench: src/counter_bench.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

percpu_monitor: src/percpu_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
