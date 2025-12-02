CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu11
LDFLAGS = -lncurses

BUILDDIR = build
SRCDIR = src

all: $(BUILDDIR)/memexplore $(BUILDDIR)/memmonitor

$(BUILDDIR)/memexplore: $(SRCDIR)/memexplore.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/memmonitor: $(SRCDIR)/memmonitor.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
