CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -std=gnu11
LDFLAGS = -lncurses

BUILD_DIR = build
SRC_DIR = src

TARGETS = $(BUILD_DIR)/cfs_demo $(BUILD_DIR)/cfs_monitor $(BUILD_DIR)/vruntime_logger

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/cfs_demo: $(SRC_DIR)/cfs_demo.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/cfs_monitor: $(SRC_DIR)/cfs_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/vruntime_logger: $(SRC_DIR)/vruntime_logger.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR) output

.PHONY: all clean
