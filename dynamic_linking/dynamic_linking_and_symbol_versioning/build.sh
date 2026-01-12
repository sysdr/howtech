#!/bin/bash
set -e

echo "Building symbol versioning demo..."

# Compile library version 3.0 (includes all previous versions)
gcc -Wall -Wextra -Werror -O2 -fPIC -shared \
    -Wl,--version-script=src/mylib_v3.map \
    -Wl,-soname,libmylib.so.3 \
    -o build/libmylib.so.3.0.0 \
    src/mylib.c

# Create soname symlinks
ln -sf libmylib.so.3.0.0 build/libmylib.so.3
ln -sf libmylib.so.3.0.0 build/libmylib.so

# Compile app_v1 (links against v1.0 API)
gcc -Wall -Wextra -Werror -O2 \
    -o build/app_v1 \
    src/app_v1.c \
    -L./build -lmylib \
    -Wl,-rpath,'$ORIGIN'

# Compile app_v2 (links against v2.0 API)
gcc -Wall -Wextra -Werror -O2 \
    -o build/app_v2 \
    src/app_v2.c \
    -L./build -lmylib \
    -Wl,-rpath,'$ORIGIN'

# Compile monitor
gcc -Wall -Wextra -O2 \
    -o build/monitor \
    src/monitor.c

echo "Build completed successfully!"
