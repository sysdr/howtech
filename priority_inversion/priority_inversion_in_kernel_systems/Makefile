CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -pthread
LDFLAGS = -pthread

SRCDIR = src
BUILDDIR = build

TARGETS = $(BUILDDIR)/priority_inversion $(BUILDDIR)/rt_monitor

all: $(TARGETS)

$(BUILDDIR)/priority_inversion: $(SRCDIR)/priority_inversion.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/rt_monitor: $(SRCDIR)/rt_monitor.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
