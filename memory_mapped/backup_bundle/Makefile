CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu11
LDFLAGS = -lncurses

TARGETS = coherence_test dirty_monitor dirty_generator msync_benchmark

all: $(TARGETS)

coherence_test: coherence_test.c
	$(CC) $(CFLAGS) -o $@ $<

dirty_monitor: dirty_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

dirty_generator: dirty_generator.c
	$(CC) $(CFLAGS) -o $@ $<

msync_benchmark: msync_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.dat *.o

.PHONY: all clean
