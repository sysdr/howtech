#!/bin/bash

# Terminal-based device enumeration monitor
# Shows real-time stats about device queries

# Colors for output
NC='\033[0m' # No Color
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'

clear

cat << "BANNER"
╔════════════════════════════════════════════════════════════════╗
║     Windows Device Tree Enumeration Monitor                    ║
║                                                                ║
║  Simulated monitoring of device property queries              ║
╚════════════════════════════════════════════════════════════════╝
BANNER

echo ""
echo "Monitoring Configuration:"
echo "  • Target OS: Windows 10/11 x64"
echo "  • API Layer: SetupAPI + Configuration Manager"
echo "  • Property Store: Modern DEVPROPKEY model"
echo ""

sleep 1

# Simulate device enumeration
devices=(
    "PCI\\VEN_8086&DEV_9D3D|Intel USB 3.1 Controller"
    "USB\\VID_046D&PID_C52B|Logitech Webcam"
    "ACPI\\PNP0C0C|Power Button"
    "ROOT\\SYSTEM|System Board"
    "HDAUDIO\\FUNC_01|Realtek Audio"
    "SCSI\\DISK|Samsung SSD"
    "ROOT\\NET|Network Adapter"
    "USB\\VID_045E&PID_07A5|Microsoft Mouse"
)

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Device Property Query Timeline                                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

total_time=0
total_queries=0
min_time=999999
max_time=0

for device in "${devices[@]}"; do
    IFS='|' read -r id name <<< "$device"
    
    # Simulate query timing (random 50-500 microseconds)
    query_time=$((50 + RANDOM % 450))
    
    total_queries=$((total_queries + 7))  # 7 properties per device
    total_time=$((total_time + query_time * 7))
    
    if [ $query_time -lt $min_time ]; then
        min_time=$query_time
    fi
    if [ $query_time -gt $max_time ]; then
        max_time=$query_time
    fi
    
    # Color code based on timing
    if [ $query_time -lt 100 ]; then
        color="${GREEN}"  # Green - cached
        source="[CACHED]"
    elif [ $query_time -lt 300 ]; then
        color="${YELLOW}"  # Yellow - partial cache
        source="[HYBRID]"
    else
        color="${RED}"  # Red - registry read
        source="[REGISTRY]"
    fi
    
    echo -e "${color}[${id}]${NC}"
    echo "  Device: $name"
    echo -e "  Query time: ${query_time}µs $source"
    echo "  Properties: DeviceDesc, InstanceID, HardwareIDs, Class, ClassGUID, Driver, Manufacturer"
    echo ""
    
    sleep 0.3
done

avg_time=$((total_time / total_queries))

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Performance Summary                                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "  Total Devices Enumerated: ${#devices[@]}"
echo "  Total Property Queries: $total_queries"
echo "  Total Time: ${total_time}µs (${total_time}us)"
echo "  Average Query Time: ${avg_time}µs"
echo "  Min Query Time: ${min_time}µs (cached property)"
echo "  Max Query Time: ${max_time}µs (registry read)"
echo ""

# Show cache effectiveness
cached_percent=$((100 - (total_time * 100 / (total_queries * max_time))))
echo "  Cache Effectiveness: ~${cached_percent}%"
echo ""

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Key Observations                                              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "  • Cached queries: 50-100µs (property store hit)"
echo "  • Registry reads: 300-500µs (blocking I/O)"
echo "  • 15x performance difference between cache/registry"
echo "  • Boot delays scale linearly with device count"
echo "  • Phantom devices add overhead without benefit"
echo ""
