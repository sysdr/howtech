CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -D_GNU_SOURCE -Wno-unused-variable -Wno-unused-parameter
LDFLAGS = -pthread -lncurses

TARGETS = irq_monitor irq_stress

all: $(TARGETS)

irq_monitor: src/irq_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

irq_stress: src/irq_stress.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

clean:
	rm -f $(TARGETS)

.PHONY: all clean
