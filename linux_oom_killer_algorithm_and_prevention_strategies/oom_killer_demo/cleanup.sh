#!/usr/bin/env bash
# Cleanup script for OOM Killer Demo
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$SCRIPT_DIR/oom_killer_demo"
IMAGE_NAME="oom-killer-demo:latest"

echo "Cleaning up OOM Killer Demo..."

# Remove Docker image
if docker image inspect "$IMAGE_NAME" &>/dev/null; then
    echo "Removing Docker image: $IMAGE_NAME"
    docker rmi "$IMAGE_NAME" || true
fi

# Remove work directory
if [ -d "$WORK_DIR" ]; then
    echo "Removing work directory: $WORK_DIR"
    rm -rf "$WORK_DIR"
fi

# Remove generated files
for f in diagram-1.jpg diagram-2.jpg; do
    if [ -f "$SCRIPT_DIR/$f" ]; then
        echo "Removing: $f"
        rm -f "$SCRIPT_DIR/$f"
    fi
done

echo "Cleanup complete."
