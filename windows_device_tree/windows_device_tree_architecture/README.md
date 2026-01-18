# Windows Device Tree Analyzer

Comprehensive demonstration of device tree enumeration in Windows kernel mode.

## What's Inside

### 1. KMDF Driver (`src/driver/`)
- `device_enum.c` - Kernel-mode driver demonstrating proper device property queries
- `device_enum.inf` - Driver installation file
- Uses modern `IoGetDevicePropertyData()` API (not deprecated `IoGetDeviceProperty()`)
- Proper IRQL handling and memory management
- Performance timing for each property query

### 2. User-Mode Tool (`src/usermode/`)
- `enum_devices.cpp` - SetupAPI-based device enumerator
- Walks entire device tree
- Measures query performance
- Shows property caching effectiveness

### 3. WinDbg Scripts (`src/windbg/`)
- `device_tree_commands.txt` - Essential debugging commands
- `analyze_phantom_devices.txt` - Finding non-present devices
- Breakpoints for property query analysis

### 4. Monitor (`src/usermode/monitor.sh`)
- Terminal-based visualization
- Simulates device enumeration with realistic timing
- Shows cache vs. registry performance

## Quick Start

### Option 1: Run Demo Monitor (Linux/WSL)
```bash
./demo.sh
```

This will:
1. Create all source files
2. Build user-mode tool (cross-compile for Windows)
3. Run terminal monitor showing enumeration simulation

### Option 2: Build on Windows

**User-Mode Tool:**
```cmd
cd src\usermode
cl /W4 /EHsc /O2 enum_devices.cpp /Fe:enum_devices.exe /link setupapi.lib cfgmgr32.lib
enum_devices.exe
```

**Kernel Driver:**
Requires WDK (Windows Driver Kit):
```cmd
cd src\driver
msbuild /p:Configuration=Release /p:Platform=x64
```

Then install with:
```cmd
devcon install device_enum.inf ROOT\DEVICEENUM
```

### Option 3: Cross-Compile with Docker
```bash
docker build -t device-enum .
docker run -it device-enum
```

## Key Concepts Demonstrated

### Modern Property APIs
- `IoGetDevicePropertyData()` - Kernel mode
- `SetupDiGetDevicePropertyW()` - User mode
- `DEVPROPKEY` structures instead of legacy `SPDRP_*` integers

### Performance Characteristics
- **Cached properties**: ~50µs
- **Registry reads**: 5-15ms (300x slower!)
- **Impact at scale**: 200ms × 100 devices = 20 seconds boot delay

### IRQL Constraints
Property queries require `PASSIVE_LEVEL`:
```c
if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
    return STATUS_INVALID_DEVICE_REQUEST;  // Bugcheck if ignored!
}
```

### Memory Management
Always pair allocations:
```c
buffer = ExAllocatePoolWithTag(PagedPool, size, TAG);
// ... use buffer ...
ExFreePoolWithTag(buffer, TAG);
```

## WinDbg Commands

```
!devnode 0 1                    # Full device tree
!devstack <PDO>                 # Show driver stack
!devnode 0 21                   # Include phantom devices
bp nt!IoGetDevicePropertyData   # Break on property query
dt nt!_DEVICE_OBJECT            # Device structure layout
```

## Files Created

```
article-windows-device-tree/
├── article.md              # Technical article
├── diagram-1.svg/jpg       # Device stack visualization
├── diagram-2.svg/jpg       # Property query flow
├── demo.sh                 # This script
├── cleanup.sh              # Cleanup script
├── Dockerfile              # Cross-compilation container
├── README.md               # This file
└── src/
    ├── driver/             # KMDF driver
    │   ├── device_enum.c
    │   └── device_enum.inf
    ├── usermode/           # User-mode tool
    │   ├── enum_devices.cpp
    │   ├── Makefile
    │   └── monitor.sh
    └── windbg/             # Debugging scripts
        ├── device_tree_commands.txt
        └── analyze_phantom_devices.txt
```

## Common Issues

### Phantom Devices
Clean up non-present devices:
```cmd
devcon removeall @*
```

### Property Query Timeouts
- Check IRQL level
- Verify device is in D0 state
- Ensure registry keys exist

### Driver Won't Load
- Check Event Viewer (System log)
- Verify INF syntax: `InfVerif.exe device_enum.inf`
- Enable Driver Verifier

## Production Tips

1. **Cache Aggressively**: Don't query same property repeatedly
2. **Handle Failures**: Always check `STATUS_*` return codes
3. **Resource Cleanup**: Use `__try/__finally` or RAII patterns
4. **IRQL Awareness**: Profile with Driver Verifier
5. **Async Operations**: Use completion routines, never busy-wait

## References

- [Modern Device Property Model](https://docs.microsoft.com/windows-hardware/drivers/install/devpkey)
- [IoGetDevicePropertyData](https://docs.microsoft.com/windows-hardware/drivers/ddi/wdm/nf-wdm-iogetdevicepropertydata)
- [SetupAPI Functions](https://docs.microsoft.com/windows-hardware/drivers/install/setupapi)

---

Built for "Systems Programming Deep Dive" newsletter
