#!/bin/bash

echo "🔍 Simple GC Monitor - Real-time threat levels"
echo "Press Ctrl+C to stop"
echo ""

while true; do
    # Get metrics and parse with jq if available, otherwise use python
    if command -v jq &> /dev/null; then
        response=$(curl -s http://localhost:8080/metrics)
        threat_level=$(echo $response | jq -r '.threat_level')
        heap_used=$(echo $response | jq -r '.heap_used')
        heap_size=$(echo $response | jq -r '.heap_size')
        heap_objects=$(echo $response | jq -r '.heap_objects')
    else
        response=$(curl -s http://localhost:8080/metrics)
        threat_level=$(echo $response | python3 -c "import sys, json; print(json.load(sys.stdin)['threat_level'])")
        heap_used=$(echo $response | python3 -c "import sys, json; print(json.load(sys.stdin)['heap_used'])")
        heap_size=$(echo $response | python3 -c "import sys, json; print(json.load(sys.stdin)['heap_size'])")
        heap_objects=$(echo $response | python3 -c "import sys, json; print(json.load(sys.stdin)['heap_objects'])")
    fi
    
    heap_mb=$((heap_used / 1024 / 1024))
    heap_util=$((heap_used * 100 / heap_size))
    
    case $threat_level in
        "GREEN") color="🟢" ;;
        "YELLOW") color="🟡" ;;
        "ORANGE") color="🟠" ;;
        "RED") color="🔴" ;;
        *) color="⚪" ;;
    esac
    
    printf "\r%s %s | Heap: %dMB (%d%%) | Objects: %s" \
        "$color" "$threat_level" "$heap_mb" "$heap_util" "$heap_objects"
    
    sleep 1
done
