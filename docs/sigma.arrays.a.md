# Sigma.Arrays.a - Standalone Malloc Variant

## Overview

`sigma.arrays.a` is a **zero sigma.memory dependency** array library using vtable interfaces compatible with `sigma.collections.o`. The malloc variants bypass the Allocator system and use standard C library (`malloc`/`free`) directly. This is **NOT** for normal Sigma.* ecosystem code - if you're writing Sigma code with Allocators, use `sigma.collections.o` instead.

**Purpose**: For profiling tools that track sigma.memory itself, embedded environments where Allocator overhead is unacceptable, or minimal utilities that need arrays without the full collections framework.

**Key Features**:
- Zero sigma.memory/Allocator dependency (uses malloc/free directly)
- **ABI-compatible** vtable interfaces matching FArray/PArray
- Two array types: FArrayMalloc (flexible element size), PArrayMalloc (pointer-specific)
- Uses sigma.core/types.h (usize, addr, object) for type compatibility
- Global interfaces: `FArrayMalloc` and `PArrayMalloc`
- Bounds-checked operations with OK/ERR returns
- NULL-safe with explicit error handling

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

### FArrayMalloc - Flexible Array (Vtable Interface)

Stores elements of any fixed size (like `sizeof(int)`, `sizeof(struct)`). ABI-compatible with `FArray` from sigma.collections.

```c
#include "farray_malloc.h"

// Create array for 10 integers using vtable interface
farray_malloc arr = FArrayMalloc.new(10, sizeof(int));

// Set element at index 5
int value = 42;
FArrayMalloc.set(arr, 5, sizeof(int), &value);

// Get element at index 5
int retrieved;
FArrayMalloc.get(arr, 5, sizeof(int), &retrieved);

// Clear all elements to zero
FArrayMalloc.clear(arr, sizeof(int));

// Remove (zero) specific element
FArrayMalloc.remove(arr, 5, sizeof(int));

// Query capacity
int cap = FArrayMalloc.capacity(arr, sizeof(int));

// Dispose when done
FArrayMalloc.dispose(arr);
```

**Interface: `sc_farray_malloc_i`**:
- `new(capacity, stride)` - Allocate array for `capacity` elements of `stride` bytes
- `init(farray_malloc *arr, capacity, stride)` - Initialize pre-allocated struct
- `dispose(arr)` - Free all memory (NULL-safe)
- `capacity(arr, stride)` - Get capacity (returns int)
- `clear(arr, stride)` - Zero all elements
- `set(arr, index, stride, elem)` - Copy `elem` to `arr[index]` (return OK or ERR)
- `get(arr, index, stride, out_elem)` - Copy `arr[index]` to `out_elem` (return OK or ERR)
- `remove(arr, index, stride)` - Zero element at `index` (return OK or ERR)

**Note**: `stride` parameter provided for ABI compatibility with FArray. Implementation uses stored `elem_size` internally for efficiency.

### PArrayMalloc - Pointer Array (Vtable Interface)

Stores pointers (`addr`/`void*`) - optimized for pointer-specific operations. ABI-compatible with `PArray` from sigma.collections.

```c
#include "parray_malloc.h"

// Create array for 10 pointers using vtable interface
parray_malloc arr = PArrayMalloc.new(10);

// Set pointer at index 3
void *my_data = some_allocation();
PArrayMalloc.set(arr, 3, (addr)my_data);

// Get pointer at index 3
addr retrieved;
PArrayMalloc.get(arr, 3, &retrieved);

// Clear all pointers to NULL
PArrayMalloc.clear(arr);

// Remove (NULL) specific pointer
PArrayMalloc.remove(arr, 3);

// Query capacity
int cap = PArrayMalloc.capacity(arr);

// Dispose when done
PArrayMalloc.dispose(arr);
```

**Interface: `sc_parray_malloc_i`**:
- `new(capacity)` - Allocate array for `capacity` pointers
- `init(parray_malloc *arr, capacity)` - Initialize pre-allocated struct
- `dispose(arr)` - Free all memory (NULL-safe)
- `capacity(arr)` - Get capacity (returns int)
- `clear(arr)` - Set all pointers to NULL
- `set(arr, index, ptr)` - Store `ptr` at `arr[index]` (return OK or ERR)
- `get(arr, index, out_ptr)` - Retrieve `arr[index]` to `out_ptr` (return OK or ERR)
- `remove(arr, index)` - Set pointer at `index` to NULL (return OK or ERR)

**Error Handling**:
- Returns `ERR` (-1) for out-of-bounds, NULL output parameter
- Returns `NULL` for allocation failures
- Storing `NULL` pointers is valid (unlike error returns)

## Usage Examples

### Example 1: Store Structs

```c
typedef struct {
    int id;
    char name[32];
} Person;

farray_malloc people = FArrayMalloc.new(100, sizeof(Person));

Person p = {1, "Alice"};
FArrayMalloc.set(people, 0, sizeof(Person), &p);

Person retrieved;
FArrayMalloc.get(people, 0, sizeof(Person), &retrieved);

FArrayMalloc.dispose(people);
```

### Example 2: Pointer Array for Dynamic Data

```c
parray_malloc buffers = PArrayMalloc.new(10);

// Allocate and store buffers
for (usize i = 0; i < 10; i++) {
    void *buf = malloc(1024);
    PArrayMalloc.set(buffers, i, (addr)buf);
}

// Retrieve and use
addr first_buffer;
PArrayMalloc.get(buffers, 0, &first_buffer);

// Cleanup (you must free stored pointers yourself)
for (usize i = 0; i < 10; i++) {
    addr buf;
    PArrayMalloc.get(buffers, i, &buf);
    free((void *)buf);
}

PArrayMalloc.dispose(buffers);
```

### Example 3: ABI Compatibility (Advanced)

The vtable structure allows pointer substitution when you need to switch between malloc and Allocator-based implementations:

```c
// Both interfaces have the same function signatures (except handle types)
// This allows building generic code:

typedef struct {
    void *handle;
    int (*capacity_fn)(void *, usize);  // Different first param types
    // ... other function pointers
} generic_array_ops;

// Could switch between FArrayMalloc and FArray vtables at runtime
// (with appropriate type casting for the handle)
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

### ABI Compatibility

The malloc variants use **identical vtable structures** to FArray/PArray from sigma.collections:
- Same function pointer layout and signatures
- Uses sigma.core/types.h for type compatibility (usize, addr, object)
- Only difference: handle types (farray_malloc vs farray)

This allows:
- Drop-in substitution of vtable pointers in generic code
- Binary-level interface compatibility
- Testing collections code with malloc-based implementations
- Building tools that work with either implementation

**Note**: `stride` parameter appears in all FArrayMalloc operations for ABI compatibility, but the implementation stores `elem_size` at creation for efficiency. The stride parameter is marked `(void)stride` to avoid warnings.

### Why Separate Implementation?

The FArray/PArray implementations in `sigma.collections.o` depend on:
- `coll_alloc()` dispatch (ties to Allocator framework via `alloc_use`)
- Collections infrastructure (iterators, collection views, interfaces)
- Future: **Phase 3A** will remove `malloc` fallback from collections entirely, requiring Allocator usage

**Phase 3A** removes the `malloc` fallback entirely from collections. This malloc variant serves a different purpose:
- **Collections**: Sophisticated, Allocator-aware, feature-rich (maps, iterators, collection views)
- **Malloc Variant**: Minimal, standalone, pure malloc/free forever, ABI-compatible

They are **different products** with **different futures** but **compatible interfaces**.

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
