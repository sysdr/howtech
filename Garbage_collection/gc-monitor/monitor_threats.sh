#!/bin/bash

echo "🔍 Monitoring GC threat levels in real-time..."
echo "Press Ctrl+C to stop monitoring"
echo ""

while true; do
    response=$(curl -s http://localhost:8080/metrics)
    if [ $? -eq 0 ]; then
        # Parse all JSON data in one Python call to avoid scope issues
        eval $(echo $response | python3 -c "
import sys, json
data = json.load(sys.stdin)
print(f'timestamp={data[\"timestamp\"]}')
print(f'heap_used={data[\"heap_used\"]}')
print(f'heap_size={data[\"heap_size\"]}')
print(f'threat_level=\"{data[\"threat_level\"]}\"')
print(f'allocation_rate={data[\"allocation_rate\"]}')
print(f'heap_objects={data[\"heap_objects\"]}')
")
        
        heap_mb=$((heap_used / 1024 / 1024))
        heap_util=$(python3 -c "print('{:.1f}'.format($heap_used / $heap_size * 100))")
        alloc_mb=$(python3 -c "print('{:.1f}'.format($allocation_rate / 1024 / 1024))")
        
        case $threat_level in
            "GREEN")
                color="🟢"
                ;;
            "YELLOW")
                color="🟡"
                ;;
            "ORANGE")
                color="🟠"
                ;;
            "RED")
                color="🔴"
                ;;
            *)
                color="⚪"
                ;;
        esac
        
        printf "\r%s %s | Heap: %dMB (%.1f%%) | Allocation: %.1fMB/s | Objects: %s" \
            "$color" "$threat_level" "$heap_mb" "$heap_util" "$alloc_mb" "$heap_objects"
    else
        printf "\r❌ Connection failed"
    fi
    
    sleep 0.5
done
