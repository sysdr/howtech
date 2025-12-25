CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -pthread

all: build/target_program build/perf_monitor

build/target_program: src/target_program.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

build/perf_monitor: src/perf_monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf build

.PHONY: all clean
