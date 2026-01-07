# Kernel module Makefile
# Uses kbuild system for proper kernel module compilation

# Module object name (without .ko extension)
obj-m += hello_module.o

# Get current kernel version
KVERSION := $(shell uname -r)

# Kernel build directory (contains headers and kbuild scripts)
KDIR := /lib/modules/$(KVERSION)/build

# Current directory (where module source is located)
PWD := $(shell pwd)

# Source directory
SRC_DIR := src

# Default target: build the module
all: check-headers
	@echo "Building kernel module..."
	$(MAKE) -C $(KDIR) M=$(PWD)/$(SRC_DIR) modules
	@echo "Build complete!"
	@echo ""
	@echo "Module information:"
	@modinfo $(SRC_DIR)/hello_module.ko
	@echo ""

# Verbose build (shows actual compilation commands)
verbose: check-headers
	@echo "Building kernel module (verbose)..."
	$(MAKE) V=1 -C $(KDIR) M=$(PWD)/$(SRC_DIR) modules

# Check if kernel headers are installed
check-headers:
	@if [ ! -d "$(KDIR)" ]; then \
		echo "Error: Kernel headers not found at $(KDIR)"; \
		echo "Install with: apt install linux-headers-$(KVERSION)"; \
		exit 1; \
	fi

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/$(SRC_DIR) clean
	@echo "Cleaned build artifacts"

# Show module information
info:
	@if [ -f $(SRC_DIR)/hello_module.ko ]; then \
		echo "=== Module Info ==="; \
		modinfo $(SRC_DIR)/hello_module.ko; \
		echo ""; \
		echo "=== Symbol Table ==="; \
		nm $(SRC_DIR)/hello_module.ko | grep -E ' [TtDdBbRr] ' | head -20; \
		echo ""; \
		echo "=== ELF Sections ==="; \
		objdump -h $(SRC_DIR)/hello_module.ko | grep -E '(Idx|\.text|\.data|\.bss|\.modinfo)'; \
	else \
		echo "Module not built yet. Run 'make' first."; \
	fi

# Show kernel build configuration (sample)
kernel-config:
	@echo "=== Kernel Configuration (sample) ==="
	@if [ -f /boot/config-$(KVERSION) ]; then \
		grep -E '(CONFIG_MODULES|CONFIG_MODULE_SIG|CONFIG_MODVERSIONS)' /boot/config-$(KVERSION) || true; \
	elif [ -f /proc/config.gz ]; then \
		zcat /proc/config.gz | grep -E '(CONFIG_MODULES|CONFIG_MODULE_SIG|CONFIG_MODVERSIONS)' || true; \
	else \
		echo "Kernel config not accessible"; \
	fi

.PHONY: all verbose check-headers clean info kernel-config
