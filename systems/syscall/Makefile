CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11
DEBUGFLAGS = -g -O0

all: syscall_demo monitor

syscall_demo: syscall_demo.c
	$(CC) $(CFLAGS) -o syscall_demo syscall_demo.c

monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c

debug: syscall_demo.c
	$(CC) $(DEBUGFLAGS) -o syscall_demo_debug syscall_demo.c

clean:
	rm -f syscall_demo monitor syscall_demo_debug *.o

.PHONY: all clean debug
