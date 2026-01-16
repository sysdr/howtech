CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu11
LDFLAGS = -lncurses

SRCS = src/openocd_validator.c src/jtag_simulator.c src/openocd_monitor.c
OBJS = $(SRCS:.c=.o)
BINS = openocd_validator jtag_simulator openocd_monitor

.PHONY: all clean

all: $(BINS)

openocd_validator: src/openocd_validator.o
	$(CC) $(CFLAGS) -o $@ $^

jtag_simulator: src/jtag_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

openocd_monitor: src/openocd_monitor.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)
