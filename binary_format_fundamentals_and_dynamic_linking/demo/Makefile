CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g
LDFLAGS = -ldl

# Directories
SRC = src
BUILD = build
OUTPUT = output

# Targets
DYNAMIC_DEMO = $(BUILD)/dynamic_demo
INSPECTOR = $(BUILD)/binary_inspector
PLT_MONITOR = $(BUILD)/plt_monitor

LIBDYNAMIC = $(BUILD)/libdynamic.so
LIBRUNTIME = $(BUILD)/libruntime.so

.PHONY: all clean dirs

all: dirs $(DYNAMIC_DEMO) $(LIBDYNAMIC) $(LIBRUNTIME) $(INSPECTOR) $(PLT_MONITOR)
	@echo "Build complete!"

dirs:
	@mkdir -p $(BUILD) $(OUTPUT)

$(LIBDYNAMIC): $(SRC)/libdynamic.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(LIBRUNTIME): $(SRC)/libruntime.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(DYNAMIC_DEMO): $(SRC)/dynamic_demo.c $(LIBDYNAMIC)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILD) -ldynamic -Wl,-rpath,$(BUILD) $(LDFLAGS)

$(INSPECTOR): $(SRC)/binary_inspector.c
	$(CC) $(CFLAGS) -o $@ $<

$(PLT_MONITOR): $(SRC)/plt_monitor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD) $(OUTPUT)
