#!/bin/bash
# Simple web server to serve the dashboard
# Requires Python 3

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PORT=${1:-8000}

echo "Starting dashboard server on http://localhost:$PORT"
echo "Press Ctrl+C to stop"
echo ""

# Try Python 3 first, then Python 2
if command -v python3 &> /dev/null; then
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    python -m SimpleHTTPServer $PORT
else
    echo "Error: Python not found. Please install Python to run the web server."
    exit 1
fi

