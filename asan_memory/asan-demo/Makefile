CC = gcc
CFLAGS = -Wall -Wextra -Wno-stringop-overflow
ASAN_FLAGS = -fsanitize=address -g -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls
TARGETS = heap_overflow use_after_free stack_overflow memory_leak double_free shadow_inspect benchmark monitor

.PHONY: all clean asan normal run-asan run-normal

all: asan

asan: $(addsuffix _asan,$(TARGETS))

normal: $(addsuffix _normal,$(TARGETS))

%_asan: %.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $< -o $@

%_normal: %.c
	$(CC) $(CFLAGS) -O2 $< -o $@

clean:
	rm -f *_asan *_normal *.o
