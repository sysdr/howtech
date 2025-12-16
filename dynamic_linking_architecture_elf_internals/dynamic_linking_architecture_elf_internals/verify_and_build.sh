#!/bin/bash
set -e

cd "$(dirname "$0")"

echo "=== Verifying and Building Required Files ==="

# Expected files
EXPECTED=(
    "build/libexample.so"
    "output/test-lazy"
    "output/test-eager"
    "output/test-no-pie"
    "output/monitor"
    "output/measure"
)

MISSING=()
for file in "${EXPECTED[@]}"; do
    if [ ! -f "$file" ]; then
        MISSING+=("$file")
        echo "MISSING: $file"
    else
        echo "OK: $file"
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    echo ""
    echo "Building missing files..."
    cd src
    
    # Build library first
    if [[ " ${MISSING[@]} " =~ " build/libexample.so " ]]; then
        echo "Building libexample.so..."
        gcc -Wall -Wextra -Werror -O2 -g -fPIC -shared -o ../build/libexample.so libexample.c
    fi
    
    # Build binaries
    export LD_LIBRARY_PATH=../build:$LD_LIBRARY_PATH
    
    if [[ " ${MISSING[@]} " =~ " output/test-lazy " ]]; then
        echo "Building test-lazy..."
        gcc -Wall -Wextra -Werror -O2 -g -fPIE -pie -Wl,-z,relro -o ../output/test-lazy main.c -L../build -lexample -ldl
    fi
    
    if [[ " ${MISSING[@]} " =~ " output/test-eager " ]]; then
        echo "Building test-eager..."
        gcc -Wall -Wextra -Werror -O2 -g -fPIE -pie -Wl,-z,relro,-z,now -o ../output/test-eager main.c -L../build -lexample -ldl
    fi
    
    if [[ " ${MISSING[@]} " =~ " output/test-no-pie " ]]; then
        echo "Building test-no-pie..."
        gcc -Wall -Wextra -Werror -O2 -g -no-pie -Wl,-z,relro -o ../output/test-no-pie main.c -L../build -lexample -ldl
    fi
    
    if [[ " ${MISSING[@]} " =~ " output/monitor " ]]; then
        echo "Building monitor..."
        gcc -Wall -Wextra -Werror -O2 -g -fPIE -pie -o ../output/monitor monitor.c -L../build -lexample -lncurses -ldl
    fi
    
    if [[ " ${MISSING[@]} " =~ " output/measure " ]]; then
        echo "Building measure..."
        gcc -Wall -Wextra -Werror -O2 -g -fPIE -pie -o ../output/measure measure.c -L../build -lexample
    fi
    
    cd ..
    
    # Verify again
    echo ""
    echo "=== Final Verification ==="
    ALL_OK=true
    for file in "${EXPECTED[@]}"; do
        if [ ! -f "$file" ]; then
            echo "STILL MISSING: $file"
            ALL_OK=false
        else
            echo "OK: $file"
        fi
    done
    
    if [ "$ALL_OK" = true ]; then
        echo ""
        echo "✓ All files built successfully!"
        exit 0
    else
        echo ""
        echo "✗ Some files are still missing"
        exit 1
    fi
else
    echo ""
    echo "✓ All files already exist!"
    exit 0
fi

