# sigma.collections

High-performance C collection library providing unified interfaces for array-based data structures with support for both dense and sparse collections.

**Version**: 0.3.0  
**License**: BSD-3-Clause

## Features

- **Dense Collections**: FArray, PArray, List, Stack (LIFO)
- **Sparse Collections**: SlotArray (pointer-based), IndexArray (value-based)
- **Hash Map**: Map (string-keyed hash map with FNV-1a hashing)
- **Heapless Query**: `Query.next`/`Query.first` — zero-allocation scan/find-first over FArray, PArray, Collection, List, SlotArray, and Map, via each type's `as_queryable` (v0.3.0+)
- **Iterators**: Standard Iterator and SparseIterator for unified traversal
- **Dynamic Growth**: Automatic resizing for List, Stack, IndexArray, and Map
- **Per-Instance Allocator Override**: bind one Collection/List/Stack instance to its own arena, independent of every other instance (v0.3.0+)
- **Buffer Views**: Non-owning views from pre-allocated memory
- **Memory Efficient**: Cache-friendly contiguous layouts
- **Memory Management**: Uses Allocator facade from sigma.memory (v0.2.1+)
- **Testing Integration**: Native support for sigma.test framework

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

### LIFO Stack

```c
#include <sigma.collections/stack.h>

stack s = Stack.new(16);

int a = 1, b = 2;
Stack.push(s, &a);
Stack.push(s, &b);

object top = Stack.pop(s);  // &b — last in, first out

// Save-point / rollback: mark the current depth, restore to discard
// everything pushed after it (O(1), no allocation)
usize checkpoint = Stack.mark(s);
Stack.push(s, &a);
Stack.restore(s, checkpoint);  // a's push is discarded

Stack.dispose(s);
```

### Heapless Query (v0.3.0+)

`Query.next`/`Query.first` scan any collection type — FArray, PArray, Collection, List, SlotArray, Map — via a small POD cursor each type produces through its own `as_queryable`. Nothing here allocates: no iterator object to create or dispose, just for the common case of a one-shot scan or "find the first match."

```c
#include <sigma.collections/query.h>
#include <sigma.collections/farray.h>

farray arr = FArray.new(4, sizeof(int));
int values[] = {10, 20, 30, 40};
for (int i = 0; i < 4; i++) FArray.set(arr, i, sizeof(int), &values[i]);

// Roll your own scan loop:
sc_queryable q = FArray.as_queryable(arr, sizeof(int));
const void *elem;
long sum = 0;
while (Query.next(&q, &elem, NULL)) {
    sum += *(const int *)elem;
}

// Or find the first match in one call:
bool is_30(const void *element, usize index, void *userdata) {
    return *(const int *)element == 30;
}
usize idx;
if (Query.first(FArray.as_queryable(arr, sizeof(int)), is_30, NULL, &idx)) {
    printf("found 30 at index %zu\n", idx);
}

FArray.dispose(arr);
```

Map goes one step further — `Map.as_queryable` can find an entry **by value**, something `Map.get` (key-only lookup) can't do, and `Map.keys`/`Map.values` scan just one half of the pair:

```c
bool value_is_3(const void *element, usize index, void *userdata) {
    return ((const map_entry *)element)->value == 3;
}
Query.first(Map.as_queryable(m), value_is_3, NULL, NULL);
```

**Important**: an `sc_queryable` is a live view into its source, not a snapshot — don't mutate the source (add/remove/set/clear/grow) while a queryable over it is still in use. See [API Reference — Query](docs/API_REFERENCE.md#query) for the full contract, and [Collections Overview — Query Support](docs/COLLECTIONS_OVERVIEW.md#query-support) for when to reach for `Query` versus `Iterator`.

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

### Custom Allocation (v0.3.0+)

By default, every collection allocates through the global `Allocator`/`Application` facade (from sigma.memory) — there's no module-wide way to swap that out. Instead, Collection/List/Stack support a **per-instance** override: bind one instance to its own `sc_alloc_use_t *` via `create_with_allocator`/`new_with_allocator`, leaving every other instance (and every other type — FArray, PArray, SlotArray, Map) on the default path. This is what lets two collections be alive at once, bound to two *different* allocators — e.g. a parse-scoped bump arena versus a longer-lived, document-scoped one:

```c
#include <sigma.collections/list.h>

sc_alloc_use_t parse_arena_use = { .alloc = arena_alloc, .release = arena_release, /* ... */ };

// Bound to the arena instead of the global Allocator; every other List
// you create is unaffected and still uses the default path.
list scratch = List.new_with_allocator(16, sizeof(node *), &parse_arena_use);

// Growth allocates through parse_arena_use and orphans (not frees) the old
// buffer — a bump/arena allocator can't free one specific prior allocation,
// only bulk-release the whole arena. List.dispose(scratch) likewise skips
// freeing the buffer; the arena reclaims it in one shot when it's released.
```

`NULL` in place of `use` is always a plain pass-through to the ordinary constructor (`List.new`, `Stack.new`, `Collections.create_view`-style construction) — every caller that doesn't opt in is unaffected.

See [User Guide - Custom Allocation](docs/USERS_GUIDE.md#custom-allocation) for details.

## Documentation

- **[Collections Overview](docs/COLLECTIONS_OVERVIEW.md)** - Quick reference for all collection types
- **[User Guide](docs/USERS_GUIDE.md)** - Detailed usage patterns and examples
- **[API Reference](docs/API_REFERENCE.md)** - Complete function reference
- **[Migration Guide v0.2.0](docs/MIGRATION_v0.2.0.md)** - Upgrading from v0.1.x
- **[Malloc Variant Sync](docs/MALLOC_VARIANT_SYNC.md)** - Feature parity requirements for standalone malloc arrays

## Malloc Variant (sigma.arrays.a)

For embedded environments, profiling tools, or utilities requiring zero dependencies, sigma.collections provides a **standalone malloc variant** of FArray and PArray:

```bash
# Build standalone library
bash build_malloc.sh

# Build with tests
bash build_malloc.sh --test

# Run tests
./build/test_arrays_malloc
```

**Key Features**:
- Zero dependencies (direct malloc/free, no sigma.memory)
- ABI-compatible vtable interfaces with main collections
- Drop-in replacement via include path change only
- Outputs: `build/sigma.arrays.a` static library

**Usage**:
```c
#include "malloc/farray.h"  // Instead of <sigma.collections/farray.h>

flex_array arr = FArray.create(100, sizeof(int));
// Same API as main collections
FArray.dispose(arr);
```

See [docs/sigma.arrays.a.md](docs/sigma.arrays.a.md) and [docs/MALLOC_VARIANT_SYNC.md](docs/MALLOC_VARIANT_SYNC.md) for details.

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

**Test Coverage**: 141 registered test cases - 28 List, 21 Map, 19 SlotArray, 16 FArray, 16 IndexArray, 15 PArray, 9 Stack, 8 Query, 6 Collections, and more

## Directory Structure

- `src/` - Source implementation files
- `include/` - Public header files
- `test/` - Test suites with SigmaTest
- `build/` - Build artifacts
- `logs/` - Test and build logs
- `docs/` - Documentation

## License

See source file headers for license information.
