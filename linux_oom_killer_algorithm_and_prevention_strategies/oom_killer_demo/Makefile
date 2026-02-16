CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -g
LDMON   = -lncurses

.PHONY: all clean

all: bin/oom_demo bin/oom_monitor

bin/oom_demo: src/oom_demo.c | bin
	$(CC) $(CFLAGS) -o $@ $<
	@echo "  Built: $@"

bin/oom_monitor: src/oom_monitor.c | bin
	$(CC) $(CFLAGS) -o $@ $< $(LDMON)
	@echo "  Built: $@"

bin:
	mkdir -p bin

clean:
	rm -f bin/oom_demo bin/oom_monitor
