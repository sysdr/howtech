#!/usr/bin/env bash
# =============================================================================
# cleanup.sh — Remove build artifacts for Linux Kernel 6.17 Security Hardening Demo
# =============================================================================
set -euo pipefail

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${DEMO_DIR}/build"
IMAGE_NAME="kernel-hardening:demo"

echo "Cleaning Linux Kernel 6.17 Security Hardening demo artifacts..."

if [ -d "${BUILD_DIR}" ]; then
    rm -rf "${BUILD_DIR}"
    echo "  Removed ${BUILD_DIR}"
fi

if [ -f "${DEMO_DIR}/.docker-build.log" ]; then
    rm -f "${DEMO_DIR}/.docker-build.log"
    echo "  Removed .docker-build.log"
fi

if command -v docker &>/dev/null && docker image inspect "${IMAGE_NAME}" &>/dev/null 2>&1; then
    docker rmi "${IMAGE_NAME}" 2>/dev/null && echo "  Removed Docker image ${IMAGE_NAME}" || true
fi

echo "Cleanup complete."
