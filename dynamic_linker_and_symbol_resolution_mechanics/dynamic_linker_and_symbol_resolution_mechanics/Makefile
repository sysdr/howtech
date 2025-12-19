CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -lpthread -ldl

all: build/malloc_hook.so build/test_program build/monitor

build/malloc_hook.so: src/malloc_hook.c
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LDFLAGS)
	@echo "Built: $@"

build/test_program: src/test_program.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Built: $@"

build/monitor: src/monitor.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Built: $@"

clean:
	rm -rf build

.PHONY: all clean
