#!/bin/bash
# Startup script for zero-copy demonstration servers

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DATA_DIR="${SCRIPT_DIR}/data"
TESTFILE="${DATA_DIR}/testfile.bin"

# Check if binaries exist
if [ ! -f "${BUILD_DIR}/traditional_server" ] || \
   [ ! -f "${BUILD_DIR}/sendfile_server" ] || \
   [ ! -f "${BUILD_DIR}/splice_server" ]; then
    echo "Error: Server binaries not found. Please run setup.sh first."
    exit 1
fi

# Check if test file exists
if [ ! -f "${TESTFILE}" ]; then
    echo "Error: Test file not found: ${TESTFILE}"
    exit 1
fi

# Kill any existing servers
echo "Checking for existing servers..."
pkill -f "traditional_server.*testfile" 2>/dev/null || true
pkill -f "sendfile_server.*testfile" 2>/dev/null || true
pkill -f "splice_server.*testfile" 2>/dev/null || true
sleep 1

# Start servers in background
echo "Starting traditional_server on port 8080..."
"${BUILD_DIR}/traditional_server" "${TESTFILE}" > /tmp/traditional_server.log 2>&1 &
TRAD_PID=$!
sleep 1

echo "Starting sendfile_server on port 8081..."
"${BUILD_DIR}/sendfile_server" "${TESTFILE}" > /tmp/sendfile_server.log 2>&1 &
SENDFILE_PID=$!
sleep 1

echo "Starting splice_server on port 8082..."
"${BUILD_DIR}/splice_server" "${TESTFILE}" > /tmp/splice_server.log 2>&1 &
SPLICE_PID=$!
sleep 1

# Verify servers are running
if ! kill -0 $TRAD_PID 2>/dev/null; then
    echo "Error: traditional_server failed to start"
    cat /tmp/traditional_server.log
    exit 1
fi

if ! kill -0 $SENDFILE_PID 2>/dev/null; then
    echo "Error: sendfile_server failed to start"
    cat /tmp/sendfile_server.log
    exit 1
fi

if ! kill -0 $SPLICE_PID 2>/dev/null; then
    echo "Error: splice_server failed to start"
    cat /tmp/splice_server.log
    exit 1
fi

echo "All servers started successfully!"
echo "  traditional_server: PID $TRAD_PID (port 8080)"
echo "  sendfile_server:    PID $SENDFILE_PID (port 8081)"
echo "  splice_server:      PID $SPLICE_PID (port 8082)"
echo ""
echo "Logs:"
echo "  /tmp/traditional_server.log"
echo "  /tmp/sendfile_server.log"
echo "  /tmp/splice_server.log"
echo ""
echo "To stop servers, run: pkill -f '.*_server.*testfile'"

