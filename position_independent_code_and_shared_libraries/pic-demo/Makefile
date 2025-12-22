CC := gcc
CFLAGS := -Wall -Wextra -Werror -O2 -g
BUILD_DIR := build

.PHONY: all clean pic nopic

all: pic nopic analyzers

# PIC/PIE versions (modern default)
pic: $(BUILD_DIR)/libdemo-pic.so $(BUILD_DIR)/main-pie

$(BUILD_DIR)/libdemo-pic.so: src/libdemo.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(BUILD_DIR)/main-pie: src/main.c $(BUILD_DIR)/libdemo-pic.so
	$(CC) $(CFLAGS) -fPIE -pie -o $@ $< -L$(BUILD_DIR) -ldemo-pic -Wl,-rpath,$(BUILD_DIR)

# Non-PIC versions (old style, text relocations)
# Note: Non-PIC shared libraries cannot be built on modern systems (linker requirement)
nopic: $(BUILD_DIR)/libdemo-nopic.so $(BUILD_DIR)/main-nopie

$(BUILD_DIR)/libdemo-nopic.so: src/libdemo.c
	-$(CC) $(CFLAGS) -fno-pic -shared -o $@ $< 2>&1 | grep -v "warning: requires executable stack" || true

$(BUILD_DIR)/main-nopie: src/main.c
	-@if [ -f $(BUILD_DIR)/libdemo-nopic.so ]; then \
		$(CC) $(CFLAGS) -no-pie -fno-pic -o $@ $< -L$(BUILD_DIR) -ldemo-nopic -Wl,-rpath,$(BUILD_DIR); \
	else \
		echo "Warning: Skipping main-nopie (libdemo-nopic.so not available - non-PIC shared libs not supported)"; \
	fi

# Analysis tools
analyzers: $(BUILD_DIR)/reloc_analyzer $(BUILD_DIR)/got_plt_monitor

$(BUILD_DIR)/reloc_analyzer: src/reloc_analyzer.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/got_plt_monitor: src/got_plt_monitor.c $(BUILD_DIR)/libdemo-pic.so
	$(CC) $(CFLAGS) -fPIE -pie -o $@ $< -L$(BUILD_DIR) -ldemo-pic -Wl,-rpath,$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*
