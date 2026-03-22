# sigma.collections

High-performance C collection library providing unified interfaces for array-based data structures with support for both dense and sparse collections.

**Version**: 0.2.0 (RC)  
**License**: BSD-3-Clause

## Features

- **Dense Collections**: FArray, PArray, List
- **Sparse Collections**: SlotArray (pointer-based), IndexArray (value-based)
- **Hash Map**: Map (string-keyed hash map with FNV-1a hashing)
- **Iterators**: Standard Iterator and SparseIterator for unified traversal
- **Dynamic Growth**: Automatic resizing for List, IndexArray, and Map
- **Buffer Views**: Non-owning views from pre-allocated memory
- **Memory Efficient**: Cache-friendly contiguous layouts
- **Custom Allocation**: `alloc_use` hook for custom allocators (v0.2.0+)
- **Testing Integration**: Native support for sigma.test leak detection

## Quick Start

### Installation

```bash
# Build and test
cbuild
ctest

# Package and publish
cpack collection
cpub collection
```

Library installed to: `/usr/local/packages/sigma.collections.o`  
Headers installed to: `/usr/local/include/sigma.collections/`

### Basic Usage

```c
#include <sigma.collections/indexarray.h>

// Create sparse collection
indexarray ia = IndexArray.new(100, sizeof(my_struct));

// Add elements (reuses empty slots)
my_struct data = {.id = 1, .value = 100};
int handle = IndexArray.add(ia, &data);

// Retrieve by handle
my_struct retrieved;
IndexArray.get_at(ia, handle, &retrieved);

// Iterate over occupied slots only
sparse_iterator it = IndexArray.create_iterator(ia);
while (SparseIterator.next(it)) {
    my_struct value;
    SparseIterator.current_value(it, &value);
    // process value...
}
SparseIterator.dispose(it);
IndexArray.dispose(ia);
```

### String-Keyed Hash Map

```c
#include <sigma.collections/map.h>

// Create hash map
map m = Map.new(16);

// Set key-value pairs (keys are caller-owned)
Map.set(m, "username", 8, (usize)user_ptr);
Map.set(m, "user_id", 7, 12345);

// Retrieve values
usize value;
if (Map.get(m, "username", 8, &value)) {
    user_t *user = (user_t *)value;
}

// Check existence and remove
if (Map.has(m, "user_id", 7)) {
    Map.remove(m, "user_id", 7);
}

// Iterate over entries
sparse_iterator it = Map.create_iterator(m);
while (SparseIterator.next(it)) {
    map_entry *entry;
    SparseIterator.current_value(it, (object *)&entry);
    // Access entry->key, entry->key_len, entry->value
}
SparseIterator.dispose(it);
Map.dispose(m);
```

### Working with Pre-allocated Buffers

```c
// Wrap existing buffer as sparse collection
my_struct *buffer = malloc(100 * sizeof(my_struct));
indexarray ia = IndexArray.from_buffer(buffer, buffer + 100, sizeof(my_struct));

// Use normally - modifications affect original buffer
IndexArray.add(ia, &data);
IndexArray.dispose(ia);  // View disposed, buffer remains valid
free(buffer);
```

### Custom Allocation (v0.2.0+)

Wire in custom allocators for testing or specialized memory management:

```c
#include <sigtest/sigtest.h>

static void register_tests(void) {
    // Track all collection allocations
    Collections.alloc_use(sigtest_alloc_use());
    
    testset("my_tests", NULL, NULL);
    testcase("test_no_leaks", test_lifecycle);
}

static void test_lifecycle(void) {
    indexarray ia = IndexArray.new(100, sizeof(int));
    IndexArray.dispose(ia);
    // Test framework reports: Total mallocs: 1, Total frees: 1
}
```

See [User Guide - Custom Allocation](docs/USERS_GUIDE.md#custom-allocation) for details.

## Documentation

- **[User Guide](docs/USERS_GUIDE.md)** - Detailed usage patterns and examples
- **[API Reference](docs/API_REFERENCE.md)** - Complete function reference
- **[Migration Guide v0.2.0](docs/MIGRATION_v0.2.0.md)** - Upgrading from v0.1.x

## Testing

```bash
# Run all tests
ctest

# Run specific collection tests
ctest slotarray
ctest indexarray

# Memory leak checking
ctest indexarray --valgrind
```

**Test Coverage**: 102 total tests - 17 Map, 17 SlotArray, 16 IndexArray, 23 List, 13 FArray, 13 PArray, and more - all passing

## Directory Structure

- `src/` - Source implementation files
- `include/` - Public header files
- `test/` - Test suites with SigmaTest
- `build/` - Build artifacts
- `logs/` - Test and build logs
- `docs/` - Documentation

## License

See source file headers for license information.
