CC = gcc
CFLAGS = -Wall -Wextra -Werror -O1 -g -std=c11
TSAN_FLAGS = -fsanitize=thread -fPIE -pie
LIBS = -lpthread

# Targets
PROGRAMS = race_example race_example_tsan race_fixed race_fixed_tsan race_atomic race_atomic_tsan tsan_monitor

all: $(PROGRAMS)

# Build without TSAN
race_example: race_example.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

race_fixed: race_fixed.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

race_atomic: race_atomic.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

# Build with TSAN
race_example_tsan: race_example.c
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $< -o $@ $(LIBS)

race_fixed_tsan: race_fixed.c
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $< -o $@ $(LIBS)

race_atomic_tsan: race_atomic.c
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $< -o $@ $(LIBS)

# Monitor program
tsan_monitor: tsan_monitor.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o

.PHONY: all clean
