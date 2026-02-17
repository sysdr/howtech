CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -O2 -g -D_GNU_SOURCE
LDFLAGS :=

SRCDIR  := src
BINDIR  := build

SRCS    := $(SRCDIR)/thp_demo.c $(SRCDIR)/monitor.c
BINS    := $(BINDIR)/thp_demo $(BINDIR)/monitor

.PHONY: all clean check

all: $(BINDIR) $(BINS)

$(BINDIR):
	mkdir -p $@

$(BINDIR)/thp_demo: $(SRCDIR)/thp_demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(BINDIR)/monitor: $(SRCDIR)/monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lncurses

clean:
	rm -rf $(BINDIR)

check: $(BINDIR)/thp_demo
	valgrind --error-exitcode=1 --leak-check=full \
	         --show-leak-kinds=all $(BINDIR)/thp_demo
