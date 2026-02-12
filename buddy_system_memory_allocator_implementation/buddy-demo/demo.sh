#!/bin/bash
cd "$(dirname "$0")"
cd src
if [ ! -f buddy_monitor ]; then
    echo "Error: buddy_monitor not found. Run setup.sh first."
    exit 1
fi
echo "Starting buddy system monitor..."
echo "Press 'q' to quit, 'a' to allocate test memory, 'f' to free"
echo
./buddy_monitor
