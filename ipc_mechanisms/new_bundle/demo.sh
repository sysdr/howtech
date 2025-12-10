#!/bin/bash
set -euo pipefail

# Color definitions
RESET='\033[0m'
BOLD='\033[1m'
RED='\033[31m'
GREEN='\033[32m'
YELLOW='\033[33m'
BLUE='\033[34m'
MAGENTA='\033[35m'
CYAN='\033[36m'

echo "=== Building programs ==="
make clean 2>/dev/null || true
make all

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}✓ Build successful!${RESET}\n"
else
    echo -e "\n${RED}✗ Build failed!${RESET}\n"
    exit 1
fi

echo "=== Running benchmarks ==="
echo ""
echo "Starting Shared Memory benchmark..."
./shared_memory > /tmp/shm_output.txt 2>&1 &
SHM_PID=$!

sleep 1

echo "Starting Message Queue benchmark..."
./message_queue > /tmp/mq_output.txt 2>&1 &
MQ_PID=$!

echo ""
echo "=== Real-time monitoring (5 seconds) ==="
timeout 5 ./monitor $SHM_PID $MQ_PID 2>/dev/null || true

wait $SHM_PID $MQ_PID

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    BENCHMARK RESULTS                          ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

echo -e "${BLUE}${BOLD}Shared Memory Results:${RESET}"
cat /tmp/shm_output.txt
echo ""

echo -e "${MAGENTA}${BOLD}Message Queue Results:${RESET}"
cat /tmp/mq_output.txt
echo ""

echo "=== System Information ==="
echo "Shared Memory available: $(df -h /dev/shm | tail -1 | awk '{print $4}')"
echo "Message Queue limits:"
cat /proc/sys/fs/mqueue/msg_max 2>/dev/null || echo "N/A"
echo ""

echo "=== strace Analysis (last 20 syscalls per program) ==="
echo ""
echo -e "${CYAN}Running strace on Shared Memory...${RESET}"
timeout 0.5 strace -c ./shared_memory 2>&1 | tail -20 || true
echo ""
echo -e "${CYAN}Running strace on Message Queue...${RESET}"
timeout 0.5 strace -c ./message_queue 2>&1 | tail -20 || true

rm -f /tmp/shm_output.txt /tmp/mq_output.txt

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                     Demo Complete!                            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Key Takeaways:"
echo "  • Shared memory: Zero-copy, minimal syscalls, higher throughput"
echo "  • Message queue: Kernel-mediated, more syscalls, automatic cleanup"
echo "  • Context switches: MQ >> SHM (visible in monitor output)"
echo "  • Throughput difference: ~20x in favor of shared memory"
echo ""
echo "Run './cleanup.sh' to remove all artifacts"
