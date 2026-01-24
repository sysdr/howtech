# XDP Architecture Demo

An eXpress Data Path (XDP) demonstration project showing high-performance packet processing using BPF programs.

## Project Structure

- `src/xdp_drop.c` - XDP BPF program that drops UDP packets on port 9999
- `src/packet_gen.c` - Packet generator for testing
- `src/xdp_monitor.c` - Real-time statistics monitor with ncurses interface
- `build/` - Build output directory (created by make)
- `output/` - Runtime output files (logs, etc.)

## Quick Start

1. **Install dependencies** (requires root):
   ```bash
   sudo ./install_deps.sh
   ```

2. **Build the project**:
   ```bash
   make
   ```
   Or use the setup script:
   ```bash
   ./setup.sh
   ```

3. **Load the XDP program** (requires root):
   ```bash
   sudo ./startup.sh
   ```

4. **Run the demo** (requires root):
   ```bash
   sudo ./run_demo.sh [duration] [target_ip]
   # Example: sudo ./run_demo.sh 10 127.0.0.1
   ```

5. **View statistics dashboard** (requires root):
   ```bash
   sudo ./run_dashboard.sh
   ```

6. **Run tests**:
   ```bash
   ./run_tests.sh
   ```

7. **Cleanup** (requires root):
   ```bash
   sudo ./cleanup.sh
   ```

## Expected Output

See `output/EXPECTED_OUTPUT.md` for detailed documentation of all expected output formats.

### Key Output Files

- `output/packet_gen.log` - Packet generator log output
- `build/xdp_drop.o` - Compiled XDP BPF program
- `build/packet_gen` - Packet generator binary
- `build/xdp_monitor` - Statistics monitor binary

### Output Formats

1. **Packet Generator**: Shows packets sent and rate (pps)
2. **Monitor Dashboard**: Real-time ncurses interface with:
   - Total packets/bytes received
   - Dropped vs passed packets
   - Packet rate (pps) and bit rate (Mbps)
   - Drop percentage
3. **Test Results**: Pass/fail status for all components

## Requirements

- Linux kernel 4.18+ (for XDP support)
- Root privileges (for loading XDP programs)
- Build dependencies: clang, llvm, libbpf-dev, libelf-dev, libncurses-dev
- Runtime tools: bpftool, iproute2

## Docker Support

Build and run in Docker:
```bash
docker build -t xdp-demo .
docker run --privileged --network host -it xdp-demo
```

## Troubleshooting

- **"clang not found"**: Run `sudo ./install_deps.sh`
- **"Permission denied"**: XDP operations require root - use `sudo`
- **"XDP program not loaded"**: Run `sudo ./startup.sh`
- **"map not found"**: Ensure XDP program is loaded and bpftool is installed

## How It Works

1. The XDP program (`xdp_drop.c`) attaches to the loopback interface
2. It intercepts all packets at the kernel level
3. UDP packets destined for port 9999 are dropped (XDP_DROP)
4. Statistics are tracked in a BPF map
5. The monitor reads statistics and displays them in real-time
6. The packet generator sends test traffic to verify functionality

## License

GPL (as required by Linux kernel BPF programs)

