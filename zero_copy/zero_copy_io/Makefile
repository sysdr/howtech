CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=gnu11
LDFLAGS = -pthread

SRCDIR = src
BUILDDIR = build

TARGETS = $(BUILDDIR)/traditional_server \
          $(BUILDDIR)/sendfile_server \
          $(BUILDDIR)/splice_server \
          $(BUILDDIR)/client \
          $(BUILDDIR)/monitor

all: $(BUILDDIR) $(TARGETS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/traditional_server: $(SRCDIR)/traditional_server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/sendfile_server: $(SRCDIR)/sendfile_server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/splice_server: $(SRCDIR)/splice_server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/client: $(SRCDIR)/client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/monitor: $(SRCDIR)/monitor.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
