# Quick Reference: Proper Output Guide

## What Output Should You See?

### ✅ Successful Build
```
$ make
clang -target bpf ... -c src/xdp_drop.c -o build/xdp_drop.o
clang ... src/packet_gen.c -o build/packet_gen
clang ... src/xdp_monitor.c -o build/xdp_monitor -lbpf -lncurses
```

### ✅ Successful Validation
```
$ ./validate.sh
[Shows ✓ for all checks]
Summary: 0 error(s), X warning(s)
```

### ✅ Successful Startup
```
$ sudo ./startup.sh
Loading XDP program on loopback interface...
XDP program loaded successfully!
Statistics map FD: [number]
```

### ✅ Packet Generator Output
**File**: `output/packet_gen.log`
- Shows "Sending UDP packets to..."
- Updates every 10,000 packets with count and rate
- Ends with "Total packets sent: [number]"

### ✅ Monitor Output
**Real-time display showing**:
- Total Packets RX: [number]
- Total Bytes RX: [number]
- Dropped (XDP_DROP): [number] (should be 100% for UDP port 9999)
- Passed (XDP_PASS): [number]
- Average Packet Rate: [number] pps
- Average Bit Rate: [number] Mbps
- Drop Percentage: 100.00 %

### ✅ Test Output
```
$ ./run_tests.sh
Testing XDP program binary exists... PASS
Testing Packet generator exists... PASS
Testing Monitor exists... PASS
...
All tests passed!
```

## Output Files Location

- **Build outputs**: `build/` directory
- **Runtime logs**: `output/` directory
- **Example outputs**: `output/*.example` and `output/*.md`

## For Detailed Documentation

See:
- `output/EXPECTED_OUTPUT.md` - Detailed output examples
- `output/PROPER_OUTPUT.md` - Complete output reference
- `README.md` - Project overview and usage

