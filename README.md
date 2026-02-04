# sigma.collections

High-performance C collection library providing unified interfaces for array-based data structures with support for both dense and sparse collections.

## Features

- **Dense Collections**: FArray, PArray, List
- **Sparse Collections**: SlotArray (pointer-based), IndexArray (value-based)
- **Iterators**: Standard Iterator and SparseIterator for unified traversal
- **Dynamic Growth**: Automatic resizing for List and IndexArray
- **Buffer Views**: Non-owning views from pre-allocated memory
- **Memory Efficient**: Cache-friendly contiguous layouts

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

## Documentation

- **[User Guide](docs/USERS_GUIDE.md)** - Detailed usage patterns and examples
- **[API Reference](docs/API_REFERENCE.md)** - Complete function reference

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

**Test Coverage**: 17 SlotArray tests, 16 IndexArray tests - all passing

## Directory Structure

- `src/` - Source implementation files
- `include/` - Public header files
- `test/` - Test suites with SigmaTest
- `build/` - Build artifacts
- `logs/` - Test and build logs
- `docs/` - Documentation

## License

See source file headers for license information.
