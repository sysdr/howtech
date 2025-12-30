CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -pthread
LDFLAGS = -pthread

SRCS = src/crash_test.c src/core_monitor.c
OBJS = $(SRCS:src/%.c=build/%.o)
BINS = build/crash_test build/core_monitor

all: $(BINS)

build/crash_test: build/crash_test.o
	$(CC) $(LDFLAGS) -o $@ $^

build/core_monitor: build/core_monitor.o
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build cores logs *.core core.* /tmp/core_analysis.txt

.PHONY: all clean
