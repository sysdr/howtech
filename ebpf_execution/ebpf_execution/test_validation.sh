#!/bin/bash
echo "=== Validation Test Script ==="
echo "1. Checking files..."
[ -f src/counter.bpf.c ] && echo "  ✓ counter.bpf.c" || echo "  ✗ counter.bpf.c missing"
[ -f src/loader.c ] && echo "  ✓ loader.c" || echo "  ✗ loader.c missing"
[ -f src/monitor.c ] && echo "  ✓ monitor.c" || echo "  ✗ monitor.c missing"
[ -f Makefile ] && echo "  ✓ Makefile" || echo "  ✗ Makefile missing"
[ -f demo.sh ] && echo "  ✓ demo.sh" || echo "  ✗ demo.sh missing"
[ -x demo.sh ] && echo "  ✓ demo.sh executable" || echo "  ✗ demo.sh not executable"

echo "2. Checking for duplicate services..."
ps aux | grep -E "(loader|monitor)" | grep -v grep | grep -v test_validation && echo "  ⚠ Services found" || echo "  ✓ No duplicate services"

echo "3. Script syntax check..."
bash -n demo.sh && echo "  ✓ demo.sh syntax OK" || echo "  ✗ demo.sh syntax errors"
bash -n cleanup.sh && echo "  ✓ cleanup.sh syntax OK" || echo "  ✗ cleanup.sh syntax errors"

echo "=== Validation Complete ==="
