#!/usr/bin/env bash
# =============================================================================
# startup.sh — Run demos and optional security monitor (full-path safe)
# Run after setup.sh. Uses Docker if image exists, else native build/binaries.
# =============================================================================
set -euo pipefail

RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${DEMO_DIR}/build"
IMAGE_NAME="kernel-hardening:demo"

# Ensure we run from project root (full path)
cd "${DEMO_DIR}"

run_cmd=""
if command -v docker &>/dev/null && docker image inspect "${IMAGE_NAME}" &>/dev/null 2>&1; then
    run_cmd="docker run --rm --privileged ${IMAGE_NAME}"
    bindir="/demo"
else
    bindir="${BUILD_DIR}"
    if [ ! -x "${BUILD_DIR}/hardening_probe" ] && [ -d "${BUILD_DIR}" ]; then
        echo -e "${YELLOW}Binaries not found. Running native build...${RESET}"
        if command -v make &>/dev/null; then
            ( cd "${BUILD_DIR}" && make all ) || true
        fi
    fi
fi

# Check for duplicate monitor processes (only relevant for native run)
if [ -z "${run_cmd}" ] && [ -x "${BUILD_DIR}/monitor" ]; then
    existing=$(pgrep -f "${BUILD_DIR}/monitor" 2>/dev/null || true)
    if [ -n "${existing}" ]; then
        echo -e "${YELLOW}Warning: monitor already running (PIDs: ${existing}). Kill first if you want a single instance.${RESET}"
    fi
fi

echo -e "${BOLD}${CYAN}=== Kernel 6.17 Security Hardening — Startup ===${RESET}\n"

# Run demos
for name in hardening_probe fortify_demo kaslr_probe seccomp_demo; do
    echo -e "${CYAN}Running ${name}...${RESET}"
    if [ -n "${run_cmd}" ]; then
        ${run_cmd} "${bindir}/${name}" 2>&1 | head -40
    elif [ -x "${bindir}/${name}" ]; then
        "${bindir}/${name}" 2>&1 | head -40
    else
        echo -e "  ${YELLOW}Binary not found, skip.${RESET}"
    fi
    echo ""
done

echo -e "${GREEN}Demos finished.${RESET}"
echo -e "To run the interactive security monitor (dashboard):"
if [ -n "${run_cmd}" ]; then
    echo -e "  ${BOLD}docker run --rm -it --privileged ${IMAGE_NAME} /demo/monitor${RESET}"
else
    echo -e "  ${BOLD}${BUILD_DIR}/monitor${RESET}"
fi
echo -e "  (Press 'q' in the monitor to quit.)"
