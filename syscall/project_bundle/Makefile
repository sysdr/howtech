CC = gcc
CFLAGS = -Wall -Wextra -O2 -march=native
BUILD_DIR = build
SRC_DIR = src

TARGETS = $(BUILD_DIR)/syscall_timing $(BUILD_DIR)/syscall_asm $(BUILD_DIR)/syscall_monitor

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/syscall_timing: $(SRC_DIR)/syscall_timing.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/syscall_asm: $(SRC_DIR)/syscall_asm.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/syscall_monitor: $(SRC_DIR)/syscall_monitor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
