CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS =

all: build/failing_app monitor/syscall_monitor

build/failing_app: src/failing_app.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

monitor/syscall_monitor: monitor/syscall_monitor.c
	$(CC) $(CFLAGS) -o $@ $< -lncurses

clean:
	rm -f build/failing_app monitor/syscall_monitor
	rm -rf logs/*

.PHONY: all clean
