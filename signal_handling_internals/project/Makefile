CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -lncurses

SRCS = src/signal_demo.c src/signal_monitor.c
TARGETS = build/signal_demo build/signal_monitor

.PHONY: all clean

all: $(TARGETS)

build/signal_demo: src/signal_demo.c | build
	$(CC) $(CFLAGS) -o $@ $<

build/signal_monitor: src/signal_monitor.c | build
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

build:
	mkdir -p build

clean:
	rm -rf build
