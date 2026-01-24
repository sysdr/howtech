# Expected Output Documentation

This document describes the proper output format for the XDP Architecture Demo project.

## Build Output

When running `make`, you should see:
```
clang -target bpf -D__TARGET_ARCH_x86 -I/usr/include/x86_64-linux-gnu -c src/xdp_drop.c -o build/xdp_drop.o
clang -O2 -g -Wall -Wextra -Werror src/packet_gen.c -o build/packet_gen
clang -O2 -g -Wall -Wextra -Werror src/xdp_monitor.c -o build/xdp_monitor -lbpf -lncurses
```

## Validation Output

When running `./validate.sh`, you should see:
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

## Startup Output

When running `sudo ./startup.sh`, you should see:
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

## Packet Generator Output

When running the packet generator, output goes to `output/packet_gen.log`:
```
Sending UDP packets to 127.0.0.1:9999 at 100000 pps
Press Ctrl+C to stop

Sent: 10000 packets, Rate: 50000 pps
Sent: 20000 packets, Rate: 66666 pps
...
Total packets sent: 100000
```

## Monitor/Dashboard Output

The XDP monitor displays a real-time ncurses interface showing:
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

When the monitor exits (Ctrl+C), it prints final statistics:
```
Final Statistics:
  RX Packets: 100000
  RX Bytes:   6400000
  Dropped:    100000
  Passed:     0
```

## Demo Output

When running `sudo ./run_demo.sh 10 127.0.0.1`, you should see:
```
========================================
XDP Demo Runner
========================================

Running demo for 10 seconds...
Target IP: 127.0.0.1

[Monitor output appears here]

Demo complete!
```

## Test Output

When running `./run_tests.sh`, you should see:
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

## Cleanup Output

When running `sudo ./cleanup.sh`, you should see:
```
Cleaning up XDP demo...
Unloading XDP program from loopback interface...
XDP program unloaded
Cleanup complete!
```

