CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11
TARGETS = getpid_bench vdso_inspector monitor

all: $(TARGETS)

getpid_bench: getpid_bench.c
	$(CC) $(CFLAGS) -o $@ $<

vdso_inspector: vdso_inspector.c
	$(CC) $(CFLAGS) -o $@ $<

monitor: monitor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
