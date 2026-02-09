#!/bin/bash
# Generate test workload with different priorities

echo "Starting test workload (Ctrl+C to stop)..."
echo "Creating tasks with different nice values..."

# High priority tasks (nice -10)
for i in {1..3}; do
    nice -n -10 bash -c 'while true; do :; done' &
    echo "  Started high-priority task (PID $!)"
done

# Normal priority tasks (nice 0)
for i in {1..5}; do
    nice -n 0 bash -c 'while true; do :; done' &
    echo "  Started normal-priority task (PID $!)"
done

# Low priority tasks (nice 10)
for i in {1..3}; do
    nice -n 10 bash -c 'while true; do :; done' &
    echo "  Started low-priority task (PID $!)"
done

echo ""
echo "Workload running. Check monitor to see DSQ behavior."
echo "Press Ctrl+C to stop all tasks."

# Wait and cleanup on exit
trap 'echo "Stopping all tasks..."; kill $(jobs -p) 2>/dev/null; exit' INT TERM
wait
