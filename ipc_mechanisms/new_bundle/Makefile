CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -pthread
LDFLAGS = -lrt -pthread

TARGETS = shared_memory message_queue monitor

all: $(TARGETS)

shared_memory: src/shared_memory.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

message_queue: src/message_queue.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

monitor: src/monitor.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f /dev/shm/ipc_benchmark_*
	rm -f /dev/mqueue/ipc_benchmark_*

.PHONY: all clean
