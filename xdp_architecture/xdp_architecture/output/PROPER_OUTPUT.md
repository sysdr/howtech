# XDP Architecture Demo - Proper Output Summary

## Overview

This document provides a complete reference for the proper output expected from the XDP Architecture Demo project.

## Output Locations

### Build Outputs (`build/` directory)
- `xdp_drop.o` - Compiled XDP BPF program (binary)
- `packet_gen` - Packet generator executable
- `xdp_monitor` - Statistics monitor executable

### Runtime Outputs (`output/` directory)
- `packet_gen.log` - Packet generator log file (created during demo)
- `EXPECTED_OUTPUT.md` - This documentation file
- `packet_gen.log.example` - Example packet generator output

## Proper Output Examples

### 1. Build Process Output

**Command**: `make`

**Expected Output**:
```
clang -target bpf -D__TARGET_ARCH_x86 -I/usr/include/x86_64-linux-gnu -c src/xdp_drop.c -o build/xdp_drop.o
clang -O2 -g -Wall -Wextra -Werror src/packet_gen.c -o build/packet_gen
clang -O2 -g -Wall -Wextra -Werror src/xdp_monitor.c -o build/xdp_monitor -lbpf -lncurses
```

**Success Criteria**: All three binaries created without errors

---

### 2. Validation Output

**Command**: `./validate.sh`

**Expected Output**:
```
========================================
XDP Demo Validation
========================================

Checking source files...
✓ XDP program source exists
✓ Packet generator source exists
✓ Monitor source exists
✓ Makefile exists
✓ Dockerfile exists

Checking scripts...
✓ Setup script exists
✓ Startup script exists
✓ Cleanup script exists
✓ Test script exists
✓ Dashboard script exists
✓ Demo script exists
✓ Dependencies installer exists

Checking build dependencies...
✓ clang installed
✓ make installed
✓ bpftool installed

Checking build outputs...
✓ XDP program built
✓ Packet generator built
✓ Monitor built

Checking for running services...
✓ No packet generator running
✓ No monitor running
⚠ XDP program not loaded (run: sudo ./startup.sh)

Summary:
1 warning(s) - setup may need dependencies or build
```

**Success Criteria**: All files exist, dependencies available, binaries built

---

### 3. Startup Output

**Command**: `sudo ./startup.sh`

**Expected Output**:
```
========================================
XDP Demo Startup
========================================

Loading XDP program on loopback interface...
XDP program loaded successfully!
Statistics map FD: 123

XDP Mode:
    prog/xdp id 456 mode xdpgeneric

Startup complete!
Use ./run_demo.sh to run the demo
Use ./run_dashboard.sh to start the dashboard
Use ./cleanup.sh to unload XDP program
```

**Success Criteria**: 
- XDP program loaded without errors
- Map FD retrieved successfully
- XDP mode displayed

---

### 4. Packet Generator Output

**File**: `output/packet_gen.log`

**Expected Content**:
```
Sending UDP packets to 127.0.0.1:9999 at 100000 pps
Press Ctrl+C to stop

Sent: 10000 packets, Rate: 50000 pps
Sent: 20000 packets, Rate: 66666 pps
Sent: 30000 packets, Rate: 75000 pps
Sent: 40000 packets, Rate: 80000 pps
Sent: 50000 packets, Rate: 83333 pps
Sent: 60000 packets, Rate: 85714 pps
Sent: 70000 packets, Rate: 87500 pps
Sent: 80000 packets, Rate: 88888 pps
Sent: 90000 packets, Rate: 90000 pps
Sent: 100000 packets, Rate: 90909 pps

Total packets sent: 100000
```

**Success Criteria**:
- Shows target IP and port
- Displays packet count and rate updates every 10,000 packets
- Shows total packets sent at the end

---

### 5. Monitor/Dashboard Output

**Command**: `sudo ./run_dashboard.sh` or via `run_demo.sh`

**Expected ncurses Interface**:
```
=== XDP Statistics Monitor ===
Uptime: 10 seconds
Press Ctrl+C to exit

Total Packets RX:               100000
Total Bytes RX:                 6400000

Dropped (XDP_DROP):             100000
Passed (XDP_PASS):                   0

Average Packet Rate:             10000 pps
Average Bit Rate:                   5.12 Mbps
Drop Rate:                        10000 pps
Drop Percentage:                 100.00 %

Check XDP mode with: ip link show dev lo
  xdpgeneric = Generic (slow)
  xdpdrv     = Native (fast)
  xdpoffload = Offloaded (SmartNIC)
```

**On Exit (Ctrl+C), Final Statistics**:
```
Final Statistics:
  RX Packets: 100000
  RX Bytes:   6400000
  Dropped:    100000
  Passed:     0
```

**Success Criteria**:
- Real-time statistics update every 500ms
- Accurate packet counts
- Correct rate calculations
- Drop percentage matches expected behavior (100% for UDP port 9999)

---

### 6. Demo Output

**Command**: `sudo ./run_demo.sh 10 127.0.0.1`

**Expected Output**:
```
========================================
XDP Demo Runner
========================================

Running demo for 10 seconds...
Target IP: 127.0.0.1

[Monitor interface appears here, updating in real-time]

Demo complete!
```

**Success Criteria**:
- Demo runs for specified duration
- Monitor displays statistics
- Packet generator creates log file
- Clean exit

---

### 7. Test Output

**Command**: `./run_tests.sh`

**Expected Output** (when run as root):
```
========================================
XDP Demo Tests
========================================

Testing XDP program binary exists... PASS
Testing Packet generator exists... PASS
Testing Monitor exists... PASS
Testing XDP program loaded on lo... PASS
Testing statistics map accessible... PASS
Testing packet generation... PASS

Test Results:
  Passed: 6
  Failed: 0

All tests passed!
```

**Expected Output** (when run as non-root):
```
========================================
XDP Demo Tests
========================================

Testing XDP program binary exists... PASS
Testing Packet generator exists... PASS
Testing Monitor exists... PASS
Skipping root-required tests (not running as root)

Test Results:
  Passed: 3
  Failed: 0

All tests passed!
```

**Success Criteria**: All applicable tests pass

---

### 8. Cleanup Output

**Command**: `sudo ./cleanup.sh`

**Expected Output**:
```
Cleaning up XDP demo...
Unloading XDP program from loopback interface...
XDP program unloaded
Cleanup complete!
```

**Success Criteria**: XDP program unloaded, processes terminated

---

## Output Verification Checklist

- [ ] Build completes without errors
- [ ] All three binaries created in `build/`
- [ ] Validation script shows all checks passing
- [ ] XDP program loads successfully
- [ ] Packet generator creates log file with packet counts
- [ ] Monitor displays real-time statistics
- [ ] Statistics show correct drop/pass counts
- [ ] Rate calculations are accurate
- [ ] Tests pass
- [ ] Cleanup removes XDP program

## Troubleshooting Output Issues

### No output files created
- Ensure `output/` directory exists (created automatically by scripts)
- Check file permissions
- Verify scripts have execute permissions

### Monitor shows zero statistics
- Verify XDP program is loaded: `ip link show dev lo | grep xdp`
- Check map FD is accessible: `bpftool map list | grep xdp_stats`
- Ensure packet generator is running

### Packet generator shows errors
- Check if running as root (required for raw sockets)
- Verify target IP is valid
- Check network interface is up

### Build fails
- Install dependencies: `sudo ./install_deps.sh`
- Verify kernel headers match running kernel
- Check clang/llvm versions are compatible

## Expected File Sizes

- `build/xdp_drop.o`: ~2-5 KB (BPF bytecode)
- `build/packet_gen`: ~20-50 KB (stripped binary)
- `build/xdp_monitor`: ~50-100 KB (with ncurses)
- `output/packet_gen.log`: Varies based on runtime

## Performance Expectations

- Packet generation: 50,000-100,000 pps (depending on system)
- XDP processing: Near wire-speed (millions of pps)
- Monitor update rate: Every 500ms
- Statistics accuracy: Real-time, per-CPU aggregation

