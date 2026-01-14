#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

METRICS_FILE="metrics.json"
TIMESTAMP=$(date +%s)

# Collect metrics
collect_metrics() {
    echo "{"
    echo "  \"timestamp\": $TIMESTAMP,"
    echo "  \"programs\": {"
    
    # Test race_example
    if [ -f "./race_example" ]; then
        START=$(date +%s%N)
        ./race_example > /tmp/race_metrics.out 2>&1 || true
        END=$(date +%s%N)
        DURATION=$(( (END - START) / 1000000 ))
        FINAL=$(grep "Final counter value" /tmp/race_metrics.out 2>&1 | awk '{print $4}' || echo "0")
        LOST=$(grep "Lost updates" /tmp/race_metrics.out 2>&1 | awk '{print $3}' || echo "0")
        
        echo "    \"race_example\": {"
        echo "      \"duration_ms\": $DURATION,"
        echo "      \"final_value\": ${FINAL:-0},"
        echo "      \"lost_updates\": ${LOST:-0},"
        echo "      \"expected_value\": 400000"
        echo "    },"
    fi
    
    # Test race_fixed
    if [ -f "./race_fixed" ]; then
        START=$(date +%s%N)
        ./race_fixed > /tmp/fixed_metrics.out 2>&1 || true
        END=$(date +%s%N)
        DURATION=$(( (END - START) / 1000000 ))
        FINAL=$(grep "Final counter value" /tmp/fixed_metrics.out 2>&1 | awk '{print $4}' || echo "0")
        
        echo "    \"race_fixed\": {"
        echo "      \"duration_ms\": $DURATION,"
        echo "      \"final_value\": ${FINAL:-0},"
        echo "      \"expected_value\": 400000,"
        echo "      \"correct\": $([ "${FINAL:-0}" -eq 400000 ] && echo "true" || echo "false")"
        echo "    },"
    fi
    
    # Test race_atomic
    if [ -f "./race_atomic" ]; then
        START=$(date +%s%N)
        ./race_atomic > /tmp/atomic_metrics.out 2>&1 || true
        END=$(date +%s%N)
        DURATION=$(( (END - START) / 1000000 ))
        FINAL=$(grep "Final counter value" /tmp/atomic_metrics.out 2>&1 | awk '{print $4}' || echo "0")
        
        echo "    \"race_atomic\": {"
        echo "      \"duration_ms\": $DURATION,"
        echo "      \"final_value\": ${FINAL:-0},"
        echo "      \"expected_value\": 400000,"
        echo "      \"correct\": $([ "${FINAL:-0}" -eq 400000 ] && echo "true" || echo "false")"
        echo "    },"
    fi
    
    # Test TSAN versions
    if [ -f "./race_example_tsan" ]; then
        export TSAN_OPTIONS="halt_on_error=0:exitcode=0"
        START=$(date +%s%N)
        ./race_example_tsan > /tmp/tsan_metrics.out 2>&1 || true
        END=$(date +%s%N)
        DURATION=$(( (END - START) / 1000000 ))
        RACE_DETECTED=$(grep -c "WARNING: ThreadSanitizer\|data race" /tmp/tsan_metrics.out 2>&1 || echo "0")
        
        echo "    \"race_example_tsan\": {"
        echo "      \"duration_ms\": $DURATION,"
        echo "      \"race_detected\": $([ "$RACE_DETECTED" -gt 0 ] && echo "true" || echo "false"),"
        echo "      \"race_count\": $RACE_DETECTED"
        echo "    }"
    fi
    
    echo "  }"
    echo "}"
}

collect_metrics > "$METRICS_FILE.tmp" 2>/dev/null
# Clean up any extra output and validate JSON
if python3 -m json.tool "$METRICS_FILE.tmp" > "$METRICS_FILE" 2>/dev/null; then
    rm -f "$METRICS_FILE.tmp"
else
    # If JSON is invalid, try to fix common issues
    # Remove lines that don't contain JSON structure
    grep -E '^[[:space:]]*[{}:,\[\]"]|^[[:space:]]*[0-9]|^[[:space:]]*(true|false|null)' "$METRICS_FILE.tmp" > "$METRICS_FILE" 2>/dev/null || mv "$METRICS_FILE.tmp" "$METRICS_FILE"
fi
