CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu11
LDFLAGS = -lncurses

SRCDIR = src
BUILDDIR = build

TARGETS = $(BUILDDIR)/workload $(BUILDDIR)/ftrace_controller \
          $(BUILDDIR)/analyzer $(BUILDDIR)/monitor

all: $(BUILDDIR) $(TARGETS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/workload: $(SRCDIR)/workload.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/ftrace_controller: $(SRCDIR)/ftrace_controller.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/analyzer: $(SRCDIR)/analyzer.c
	$(CC) $(CFLAGS) -o $@ $< -lm

$(BUILDDIR)/monitor: $(SRCDIR)/monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)
	rm -f traces/*.out

.PHONY: all clean
