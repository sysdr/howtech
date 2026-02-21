CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -std=c11
TARGETS = swappiness_probe lru_monitor

all: $(TARGETS)

swappiness_probe: src/swappiness_probe.c
	$(CC) $(CFLAGS) -o $@ $<

lru_monitor: src/lru_monitor.c
	$(CC) $(CFLAGS) -o $@ $< -lncurses

clean:
	rm -f $(TARGETS)
