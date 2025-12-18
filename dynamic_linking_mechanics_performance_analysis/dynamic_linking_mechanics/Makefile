CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS_LAZY = -Wl,-z,lazy
LDFLAGS_NOW = -Wl,-z,now,-z,relro,-z,noexecstack
LIBS = -lm -lpthread -ldl

SRC_DIR = src
BUILD_DIR = build

all: lazy now monitor

lazy: $(BUILD_DIR)/binding_test_lazy

now: $(BUILD_DIR)/binding_test_now

monitor: $(BUILD_DIR)/monitor

$(BUILD_DIR)/binding_test_lazy: $(SRC_DIR)/binding_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS_LAZY) $< -o $@ $(LIBS)

$(BUILD_DIR)/binding_test_now: $(SRC_DIR)/binding_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS_NOW) $< -o $@ $(LIBS)

$(BUILD_DIR)/monitor: $(SRC_DIR)/monitor.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ -lncurses

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all lazy now monitor clean
