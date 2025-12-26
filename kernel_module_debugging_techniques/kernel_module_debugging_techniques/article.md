# Kernel Module Debugging Techniques: printk & kgdb

## Overview

This project demonstrates kernel debugging techniques using `printk()` and `kgdb` for kernel module development and troubleshooting.

## Components

### 1. Kernel Module (`debug_demo.c`)

A demonstration kernel module that shows:
- Different `printk()` log levels (KERN_EMERG through KERN_DEBUG)
- Rate-limited logging to prevent log buffer overflow
- Performance measurement of printk overhead
- Common debugging scenarios (null pointers, memory leaks)

### 2. Kernel Log Monitor (`klog_monitor.c`)

A real-time ncurses-based monitor for kernel messages with:
- Color-coded log levels
- Live monitoring of `/dev/kmsg`
- Message statistics

## Usage

### Building

```bash
./setup.sh
```

### Loading the Module

```bash
sudo insmod src/debug_demo.ko
```

### Viewing Logs

```bash
dmesg | grep debug_demo | tail -20
```

### Running the Monitor

```bash
sudo ./src/klog_monitor
```

### Module Parameters

```bash
# Load with custom parameters
sudo insmod src/debug_demo.ko simulate_bug=1 loop_count=10

# View module parameters
cat /sys/module/debug_demo/parameters/*
```

## Debugging Techniques

### printk() Levels

- **KERN_EMERG (0)**: System is unusable
- **KERN_ALERT (1)**: Action must be taken immediately
- **KERN_CRIT (2)**: Critical conditions
- **KERN_ERR (3)**: Error conditions
- **KERN_WARNING (4)**: Warning conditions
- **KERN_NOTICE (5)**: Normal but significant
- **KERN_INFO (6)**: Informational
- **KERN_DEBUG (7)**: Debug-level messages

### Viewing Kernel Logs

```bash
# View all logs
dmesg

# Live monitoring
dmesg -w

# Filter by module
dmesg | grep debug_demo

# Set log level
dmesg -n 8  # Show all levels
```

### kgdb Remote Debugging

1. Build kernel with `CONFIG_KGDB=y`
2. Start QEMU with debugging enabled:
   ```bash
   qemu-system-x86_64 -s -S -kernel bzImage
   ```
3. Connect with GDB:
   ```bash
   gdb vmlinux
   (gdb) target remote :1234
   (gdb) break my_function
   (gdb) continue
   ```

## Best Practices

1. Use appropriate log levels for different message types
2. Rate-limit logs in hot paths to avoid console_lock contention
3. kgdb stops the entire system - not suitable for production
4. Use eBPF/ftrace for production debugging without invasive changes
5. Always free resources in module cleanup functions

## Troubleshooting

### Module Won't Load

- Check kernel version compatibility
- Verify kernel headers are installed
- Check `dmesg` for error messages

### Monitor Can't Access /dev/kmsg

- Run with `sudo` or ensure user has `CAP_SYSLOG` capability
- Check permissions: `ls -l /dev/kmsg`

### Build Failures

- Install kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- Install build tools: `sudo apt-get install build-essential`
- Install ncurses: `sudo apt-get install libncurses5-dev`
