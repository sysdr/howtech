#!/bin/bash
# Install build dependencies for XDP demo
# Run with: sudo ./install_deps.sh

set -euo pipefail

if [ "$EUID" -ne 0 ]; then 
    echo "This script requires root privileges"
    echo "Please run with: sudo ./install_deps.sh"
    exit 1
fi

echo "Installing build dependencies..."
apt-get update -qq
apt-get install -y -qq \
    build-essential \
    clang \
    llvm \
    libelf-dev \
    libbpf-dev \
    libncurses-dev \
    linux-headers-$(uname -r) \
    bpftool \
    iproute2

echo "Dependencies installed successfully!"
echo "You can now run: make"

