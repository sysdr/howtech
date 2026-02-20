CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -g -D_GNU_SOURCE
LDFLAGS =

all: alloc_demo monitor

alloc_demo: alloc_demo.c
	$(CC) $(CFLAGS) -o ../bin/alloc_demo alloc_demo.c $(LDFLAGS)

monitor: monitor.c
	$(CC) $(CFLAGS) -o ../bin/monitor monitor.c -lncurses $(LDFLAGS)

clean:
	rm -f ../bin/alloc_demo ../bin/monitor

.PHONY: all clean
