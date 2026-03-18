CC ?= gcc
CFLAGS ?= -O2 -g -Wall -Wextra -Wshadow -Wformat=2 -Wno-unused-parameter
LDFLAGS ?=

NCURSES_CFLAGS := $(shell pkg-config --cflags ncurses 2>/dev/null)
NCURSES_LIBS   := $(shell pkg-config --libs ncurses 2>/dev/null)

ifeq ($(strip $(NCURSES_LIBS)),)
  NCURSES_LIBS := -lncurses
endif

BINARIES := scx_task_migration scx_revert_monitor

.PHONY: all clean

all: $(BINARIES)

scx_task_migration: scx_task_migration.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

scx_revert_monitor: scx_revert_monitor.c
	$(CC) $(CFLAGS) $(NCURSES_CFLAGS) -o $@ $< $(LDFLAGS) $(NCURSES_LIBS)

clean:
	rm -f $(BINARIES)
