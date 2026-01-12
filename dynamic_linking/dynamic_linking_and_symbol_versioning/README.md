# Symbol Versioning Demo

## Quick Start

```bash
./demo.sh
```

## What This Demonstrates

1. **Multiple API versions in one library**: MYLIB_1.0, MYLIB_2.0, MYLIB_3.0
2. **Backward compatibility**: Apps compiled against v1.0 work with v3.0 library
3. **Version symbol resolution**: How ld.so matches versions at runtime
4. **Real-world inspection**: Using readelf, objdump, nm to see versioning
5. **glibc comparison**: How system libraries use versioning

## Files Created

- `libmylib.so.3`: Shared library with 3 API versions
- `app_v1`: Application using v1.0 API
- `app_v2`: Application using v2.0 API
- `monitor`: Real-time version information display

## Manual Inspection Commands

```bash
# See version definitions
readelf -V build/libmylib.so.3

# View versioned symbols
objdump --dynamic-syms build/libmylib.so.3 | grep MYLIB

# Check app requirements
readelf -V build/app_v1

# Trace symbol resolution
LD_DEBUG=symbols LD_LIBRARY_PATH=./build ./build/app_v1

# Run with strace
strace -e openat ./build/app_v1
```

## Key Concepts

- `@@VERSION`: Default symbol version (for new binaries)
- `@VERSION`: Specific version (for backward compatibility)
- `.gnu.version_d`: Version definitions in library
- `.gnu.version_r`: Version requirements in binary
- Symbol versioning is opt-in via version scripts

## Cleanup

```bash
./cleanup.sh
```
