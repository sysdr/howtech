#!/bin/bash

echo "Cleaning up demonstration files..."

# Remove build artifacts
rm -rf build/
rm -rf src/

echo "✓ Cleanup complete!"
echo ""
echo "All generated files have been removed."
echo "Run ./demo.sh again to regenerate everything."