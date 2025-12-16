# GOT/PLT Dynamic Linking Demo

This demo shows how the Global Offset Table (GOT) and Procedure Linkage Table (PLT) 
implement lazy binding for shared libraries, and compares different security modes.

## Quick Start

```bash
./demo.sh    # Build everything
```

## What Gets Built

1. **test-lazy** - PIE + Partial RELRO + lazy binding (default)
2. **test-eager** - PIE + Full RELRO + BIND_NOW (all symbols resolved at startup)
3. **test-no-pie** - No PIE, for comparison
4. **monitor** - Interactive ncurses monitor showing GOT resolution
5. **measure** - Precise timing measurements

## Running the Tests

### Basic execution:
```bash
export LD_LIBRARY_PATH=./build:$LD_LIBRARY_PATH
./output/test-lazy
./output/test-eager
./output/test-no-pie
```

### With ltrace (see resolver calls):
```bash
ltrace -C ./output/test-lazy 2>&1 | grep -A5 "example_function"
ltrace -C ./output/test-eager 2>&1 | grep -A5 "example_function"
```

### Interactive monitor:
```bash
./output/monitor
# Press 'c' to call functions and watch GOT get updated
# Press 'q' to quit
```

### Examine binary structure:
```bash
# Show PLT entries
objdump -d -j .plt ./output/test-lazy | less

# Show GOT relocations
readelf -r ./output/test-lazy | grep JUMP_SLOT

# Check RELRO status
readelf -l ./output/test-lazy | grep GNU_RELRO
readelf -d ./output/test-lazy | grep BIND_NOW

# Compare security features
checksec --file=./output/test-lazy
checksec --file=./output/test-eager
checksec --file=./output/test-no-pie
```

## What to Look For

### In test-lazy (Partial RELRO):
- First call to `example_function` goes through resolver (~1-2μs overhead)
- Subsequent calls are fast (direct jump)
- .got.plt is writable during execution
- ltrace shows _dl_runtime_resolve on first call

### In test-eager (Full RELRO):
- ALL symbols resolved at startup
- Slower startup time
- .got.plt is read-only (check with `readelf -l`)
- No resolver calls in ltrace
- More secure - immune to GOT overwrites

### In test-no-pie:
- Fixed base address (check /proc/PID/maps)
- Code always loads at 0x400000
- Less ASLR protection
- Same GOT/PLT mechanism but predictable addresses

## Understanding the Output

The monitor shows:
- **RED**: First call (resolver overhead)
- **GREEN**: Resolved (fast path)
- **YELLOW**: Not yet called

Cycle counts are approximate (assumes 2.5GHz CPU).
First call is ~2-5x slower than subsequent calls due to resolver.

## Docker Build

```bash
docker build -t got-plt-demo .
docker run -it --rm got-plt-demo
```
