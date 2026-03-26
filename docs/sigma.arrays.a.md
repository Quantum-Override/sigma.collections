# Sigma.Arrays.a - Standalone Malloc Variant

## Overview

`sigma.arrays.a` is a **zero-dependency** array library providing flexible and pointer arrays using only standard C library (`malloc`/`free`/`memcpy`/`memset`). This is **NOT** for the Sigma.* ecosystem - if you're writing Sigma code, use `sigma.collections.o` instead.

**Purpose**: For embedded environments, profiling tools, minimal utilities, and contexts where sigma.memory/Allocator cannot be used.

**Key Features**:
- Zero sigma.* dependencies (libc only)
- Two array types: FArray (flexible element size), PArray (pointer-specific)
- Simple API: create, dispose, set, get, clear, remove
- Bounds-checked operations
- NULL-safe with explicit error returns

## When to Use This

✅ **Use sigma.arrays.a if**:
- Profiling tools that track sigma.memory itself
- Embedded environments without sigma.core
- Minimal utilities (awk-like tools, parsers)
- Zero-dependency requirement

❌ **Use sigma.collections.o if**:
- Writing normal Sigma.* code
- Need advanced features (iteration, maps, slotarrays)
- Want Allocator/arena integration
- Part of SigmaCore ecosystem

## Build Instructions

### Compile Object Files
```bash
gcc -c src/farray_malloc.c -Iinclude -std=c2x -Wall -o farray_malloc.o
gcc -c src/parray_malloc.c -Iinclude -std=c2x -Wall -o parray_malloc.o
```

### Create Static Library
```bash
ar rcs sigma.arrays.a farray_malloc.o parray_malloc.o
```

### Verify Zero Dependencies
```bash
nm sigma.arrays.a | grep -E '(Allocator|coll_|sigma_)'
# Should produce no output
```

### Link with Your Code
```bash
gcc your_tool.c sigma.arrays.a -Iinclude -o your_tool
```

## API Reference

### FArray - Flexible Array

Stores elements of any fixed size (like `sizeof(int)`, `sizeof(struct)`).

```c
#include "farray_malloc.h"

// Create array for 10 integers
farray_malloc arr = farray_malloc_create(sizeof(int), 10);

// Set element at index 5
int value = 42;
farray_malloc_set(arr, 5, &value);

// Get element at index 5
int retrieved;
farray_malloc_get(arr, 5, &retrieved);

// Clear all elements to zero
farray_malloc_clear(arr);

// Remove (zero) specific element
farray_malloc_remove(arr, 5);

// Query capacity
size_t cap = farray_malloc_capacity(arr);

// Dispose when done
farray_malloc_dispose(arr);
```

**Functions**:
- `farray_malloc_create(elem_size, capacity)` - Allocate array for `capacity` elements of `elem_size` bytes
- `farray_malloc_dispose(arr)` - Free all memory (NULL-safe)
- `farray_malloc_capacity(arr)` - Get capacity (return `size_t`)
- `farray_malloc_set(arr, index, elem)` - Copy `elem` to `arr[index]` (return 0 or -1)
- `farray_malloc_get(arr, index, out_elem)` - Copy `arr[index]` to `out_elem` (return 0 or -1)
- `farray_malloc_clear(arr)` - Zero all elements
- `farray_malloc_remove(arr, index)` - Zero element at `index` (return 0 or -1)

**Error Handling**:
- Returns `-1` for out-of-bounds, NULL parameters
- Returns `NULL` for allocation failures

### PArray - Pointer Array

Stores pointers (`void*`) - optimized for pointer-specific operations.

```c
#include "parray_malloc.h"

// Create array for 10 pointers
parray_malloc arr = parray_malloc_create(10);

// Set pointer at index 3
void *my_data = some_allocation();
parray_malloc_set(arr, 3, my_data);

// Get pointer at index 3
void *retrieved;
parray_malloc_get(arr, 3, &retrieved);

// Clear all pointers to NULL
parray_malloc_clear(arr);

// Remove (NULL) specific pointer
parray_malloc_remove(arr, 3);

// Query capacity
size_t cap = parray_malloc_capacity(arr);

// Dispose when done
parray_malloc_dispose(arr);
```

**Functions**:
- `parray_malloc_create(capacity)` - Allocate array for `capacity` pointers
- `parray_malloc_dispose(arr)` - Free all memory (NULL-safe)
- `parray_malloc_capacity(arr)` - Get capacity (return `size_t`)
- `parray_malloc_set(arr, index, ptr)` - Store `ptr` at `arr[index]` (return 0 or -1)
- `parray_malloc_get(arr, index, out_ptr)` - Retrieve `arr[index]` to `out_ptr` (return 0 or -1)
- `parray_malloc_clear(arr)` - Set all pointers to NULL
- `parray_malloc_remove(arr, index)` - Set pointer at `index` to NULL (return 0 or -1)

**Error Handling**:
- Returns `-1` for out-of-bounds, NULL output parameter
- Returns `NULL` for allocation failures
- Storing `NULL` pointers is valid (unlike error returns)

## Usage Examples

### Example 1: Store Structs

```c
typedef struct {
    int id;
    char name[32];
} Person;

farray_malloc people = farray_malloc_create(sizeof(Person), 100);

Person p = {1, "Alice"};
farray_malloc_set(people, 0, &p);

Person retrieved;
farray_malloc_get(people, 0, &retrieved);

farray_malloc_dispose(people);
```

### Example 2: Pointer Array for Dynamic Data

```c
parray_malloc buffers = parray_malloc_create(10);

// Allocate and store buffers
for (size_t i = 0; i < 10; i++) {
    void *buf = malloc(1024);
    parray_malloc_set(buffers, i, buf);
}

// Retrieve and use
void *first_buffer;
parray_malloc_get(buffers, 0, &first_buffer);

// Cleanup (you must free stored pointers yourself)
for (size_t i = 0; i < 10; i++) {
    void *buf;
    parray_malloc_get(buffers, i, &buf);
    free(buf);
}

parray_malloc_dispose(buffers);
```

## Testing

### Run Test Suite
```bash
# Compile tests
gcc test/standalone/test_arrays_malloc.c \
    src/farray_malloc.c src/parray_malloc.c \
    -Iinclude -std=c2x -Wall -o test_arrays_malloc

# Run tests
./test_arrays_malloc

# Valgrind check
valgrind --leak-check=full ./test_arrays_malloc
```

Expected output:
```
==============================================
Sigma.Arrays.a Malloc Variant Test Suite
==============================================

--- FArray Tests ---
Running: test_farray_create_dispose ... ✓
Running: test_farray_set_get ... ✓
...

Tests: 12/12 passed
✓ All tests passed!

==XXXXX== All heap blocks were freed -- no leaks are possible
```

## Installation

**NOTE**: Installation is managed by the toolsmith via [FR-2603-toolchain-007](../q-or/registry/FR-2603-toolchain-007.md). The steps below are for reference only.

```bash
# Build library
ar rcs sigma.arrays.a farray_malloc.o parray_malloc.o

# Install to system (requires sudo)
sudo cp sigma.arrays.a /usr/local/lib/
sudo cp include/farray_malloc.h /usr/local/include/
sudo cp include/parray_malloc.h /usr/local/include/
sudo ldconfig
```

## Design Notes

### Why Duplicate Code?

The FArray/PArray implementations in `sigma.collections.o` depend on:
- `coll_alloc()` dispatch (ties to Allocator framework)
- sigma.core types (`addr`, `object`, `bool`)
- Collections infrastructure (iterators, interfaces)

**Phase 3A** will remove `malloc` fallback from collections entirely, requiring all collections to use explicit Allocators. This malloc variant serves a different purpose:
- **Collections**: Sophisticated, Allocator-aware, feature-rich (maps, iterators, etc.)
- **Malloc Variant**: Minimal, standalone, pure libc forever

They are **different products** with **different futures**.

### Memory Management

- **FArray**: Allocates `struct` + data buffer (`elem_size * capacity`)
- **PArray**: Allocates `struct` + pointer array (`sizeof(void*) * capacity`)
- Both use `memset(0)` for initialization
- No reallocation support (fixed capacity at creation)
- Caller must free stored pointers in PArray before `dispose()`

### Performance Characteristics

- **FArray**: `O(1)` set/get with `memcpy` overhead
- **PArray**: `O(1)` set/get with direct assignment (faster than FArray for pointers)
- No bounds expansion - preallocate sufficient capacity
- `clear()` is `O(n)` with `memset`
- `remove()` is `O(1)` (just zeros/NULLs the slot)

## Related Documentation

- [Collections Overview](COLLECTIONS_OVERVIEW.md) - For sigma.collections.o features
- [API Reference](API_REFERENCE.md) - Full Sigma.* collection types
- [FR-2603-sigma-collections-005](../q-or/registry/FR-2603-sigma-collections-005.md) - Feature request for this library

## License

Copyright (c) 2025 David Boarman (BadKraft) and contributors  
QuantumOverride [Q|]  
See LICENSE file for terms.
