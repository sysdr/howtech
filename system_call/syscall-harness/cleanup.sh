#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "Cleaning up generated files..."
rm -rf src build output Makefile Dockerfile README.md demo.sh cleanup.sh
echo "Cleanup complete!"

