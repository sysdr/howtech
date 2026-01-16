CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -std=gnu11
LDFLAGS = -pthread

SRCDIR = src
BUILDDIR = build

TARGETS = $(BUILDDIR)/fd_leak $(BUILDDIR)/fd_monitor

all: $(BUILDDIR) $(TARGETS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/fd_leak: $(SRCDIR)/fd_leak.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/fd_monitor: $(SRCDIR)/fd_monitor.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR) /tmp/leaked_file_* /tmp/deleted_but_open.txt

.PHONY: all clean
