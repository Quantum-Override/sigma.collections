#!/bin/bash
# Build script for sigma.arrays.a (malloc variant)
# Standalone library with no sigma.memory dependency

set -e

# Source configuration
if [ -f "./config.sh" ]; then
    source ./config.sh
fi

BUILD_DIR=${BUILD_DIR:-build}
MALLOC_BUILD_DIR="$BUILD_DIR/malloc"
MALLOC_LIB="$BUILD_DIR/sigma.arrays.a"

# Create build directory
mkdir -p "$MALLOC_BUILD_DIR"

echo "Building sigma.arrays.a (malloc variant)..."

# Compile farray.c
echo "Compiling src/malloc/farray.c"
gcc -c src/malloc/farray.c -o "$MALLOC_BUILD_DIR/farray.o" \
    -Iinclude -I/usr/local/include -std=c2x -Wall -Wextra -g

# Compile parray.c
echo "Compiling src/malloc/parray.c"
gcc -c src/malloc/parray.c -o "$MALLOC_BUILD_DIR/parray.o" \
    -Iinclude -I/usr/local/include -std=c2x -Wall -Wextra -g

# Create static library
echo "Creating static library: $MALLOC_LIB"
ar rcs "$MALLOC_LIB" "$MALLOC_BUILD_DIR/farray.o" "$MALLOC_BUILD_DIR/parray.o"

# Verify zero dependencies
echo "Verifying zero sigma.* dependencies..."
if nm "$MALLOC_LIB" | grep -E '(Allocator|coll_|sigma_)'; then
    echo "ERROR: Found sigma.* dependencies in malloc variant!"
    exit 1
fi

echo "✓ sigma.arrays.a built successfully"
echo "  Location: $MALLOC_LIB"
echo "  Size: $(ls -lh $MALLOC_LIB | awk '{print $5}')"

# Optionally build test
if [ "$1" == "--test" ]; then
    echo ""
    echo "Building malloc variant test..."
    mkdir -p "$BUILD_DIR"
    gcc test/malloc/test_arrays.c \
        -o "$BUILD_DIR/test_arrays_malloc" \
        "$MALLOC_LIB" \
        -Iinclude -I/usr/local/include -std=c2x -Wall -Wextra -g
    
    echo "✓ Test built: $BUILD_DIR/test_arrays_malloc"
    echo ""
    echo "Run tests:"
    echo "  ./$BUILD_DIR/test_arrays_malloc"
    echo "  valgrind --leak-check=full ./$BUILD_DIR/test_arrays_malloc"
fi
