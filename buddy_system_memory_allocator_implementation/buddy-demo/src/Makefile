CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -lncurses

TARGETS = buddy_monitor alloc_stress

all: $(TARGETS)

buddy_monitor: buddy_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

alloc_stress: alloc_stress.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
