# Sigma Collections - User Guide

Comprehensive guide to using the sigma.collections library for efficient data structure management in C.

## Table of Contents

- [Overview](#overview)
- [Dense Collections](#dense-collections)
- [Sparse Collections](#sparse-collections)
- [Hash Map](#hash-map)
- [Iterators](#iterators)
- [Buffer Management](#buffer-management)
- [Custom Allocation](#custom-allocation)
- [Performance Considerations](#performance-considerations)
- [Best Practices](#best-practices)

## Overview

Sigma.collections provides a unified interface for working with different types of collections, optimized for various use cases:

- **Value storage** (FArray, IndexArray) - Stores data inline
- **Reference storage** (PArray, SlotArray) - Stores pointers to data
- **Dense collections** - Contiguous, sequential data
- **Sparse collections** - Non-contiguous with slot reuse

## Dense Collections

### FArray - Flex Array

**When to use**: Small, fixed-size value types that won't grow.

```c
#include <sigma.collections/farray.h>

// Create array for 10 integers
farray arr = FArray.new(10, sizeof(int));

// Set values
int value = 42;
FArray.set(arr, 0, sizeof(int), &value);

// Get values
int retrieved;
FArray.get(arr, 0, sizeof(int), &retrieved);

// Clear all elements
FArray.clear(arr, sizeof(int));

// Cleanup
FArray.dispose(arr);
```

**Trade-offs**:
- ✅ Cache-friendly (contiguous memory)
- ✅ No pointer indirection
- ❌ Fixed capacity (no growth)
- ❌ Must pass stride to every operation

### PArray - Pointer Array

**When to use**: Collections of pointers or when you need reference semantics.

```c
#include <sigma.collections/parray.h>

// Create array for 10 pointers
parray arr = PArray.new(10);

// Store pointer
my_struct *obj = malloc(sizeof(my_struct));
PArray.set(arr, 0, (addr)obj);

// Retrieve pointer
addr ptr;
PArray.get(arr, 0, &ptr);
my_struct *retrieved = (my_struct *)ptr;

// Cleanup (doesn't free pointed-to objects)
PArray.dispose(arr);
free(obj);
```

**Trade-offs**:
- ✅ Flexible - can store any pointer type
- ✅ No stride parameter needed
- ❌ Extra indirection (pointer chasing)
- ❌ You manage pointed-to object lifetime

### List - Dynamic List

**When to use**: Growing/shrinking sequential collections.

```c
#include <sigma.collections/list.h>

// Create list with initial capacity
list lst = List.new(10, sizeof(my_struct));

// Append elements (automatically grows)
my_struct data = {.id = 1, .value = 100};
List.append(lst, &data);

// Get by index
my_struct retrieved;
List.get_at(lst, 0, &retrieved);

// Remove at index (shifts elements)
List.remove_at(lst, 0);

// Check size
usize size = List.size(lst);

// Cleanup
List.dispose(lst);
```

**Trade-offs**:
- ✅ Dynamic growth
- ✅ Sequential access patterns
- ❌ Expensive removal (shifts elements)
- ❌ Can over-allocate memory

## Sparse Collections

Sparse collections are designed for scenarios where:
- Elements are frequently added/removed
- Indices must remain stable (handles don't change)
- Iteration only needs to visit occupied slots
- Memory for unused slots is acceptable

### SlotArray - Sparse Pointer Array

**When to use**: Managing collections of objects where handles must be stable.

```c
#include <sigma.collections/slotarray.h>

// Create with capacity
slotarray sa = SlotArray.new(100);

// Add elements (returns stable handle)
my_struct *obj = malloc(sizeof(my_struct));
int handle = SlotArray.add(sa, obj);

// Retrieve by handle
object retrieved;
SlotArray.get_at(sa, handle, &retrieved);
my_struct *data = (my_struct *)retrieved;

// Remove (marks slot empty, doesn't shift)
SlotArray.remove_at(sa, handle);

// Add again - reuses empty slot
my_struct *obj2 = malloc(sizeof(my_struct));
int handle2 = SlotArray.add(sa, obj2);  // Likely reuses handle

// Check if slot is occupied
bool empty = SlotArray.is_empty_slot(sa, handle);

// Cleanup
SlotArray.dispose(sa);
free(obj);
free(obj2);
```

**Trade-offs**:
- ✅ Stable handles (indices never change)
- ✅ O(1) removal (no shifting)
- ✅ Automatic slot reuse
- ❌ Fixed capacity (no growth)
- ❌ You manage pointed-to object lifetime
- ❌ Wasted memory for empty slots

### IndexArray - Sparse Value Array

**When to use**: Sparse collections of small/medium structs with dynamic growth.

```c
#include <sigma.collections/indexarray.h>

// Create with initial capacity and struct size
typedef struct {
    int id;
    float value;
    char name[32];
} my_data;

indexarray ia = IndexArray.new(100, sizeof(my_data));

// Add elements (returns stable handle)
my_data data = {.id = 1, .value = 3.14f, .name = "test"};
int handle = IndexArray.add(ia, &data);

// Retrieve by handle
my_data retrieved;
if (IndexArray.get_at(ia, handle, &retrieved) == OK) {
    // Use retrieved data
}

// Remove (zeroes slot)
IndexArray.remove_at(ia, handle);

// Add more - automatically grows if needed
for (int i = 0; i < 200; i++) {
    my_data item = {.id = i, .value = i * 1.5f};
    IndexArray.add(ia, &item);
}

// Check capacity (may have grown)
usize capacity = IndexArray.capacity(ia);

// Get element size
usize stride = IndexArray.stride(ia);

// Clear all slots
IndexArray.clear(ia);

// Cleanup
IndexArray.dispose(ia);
```

**Trade-offs**:
- ✅ Stable handles (indices never change)
- ✅ Dynamic growth when full
- ✅ Values stored inline (cache-friendly)
- ✅ Automatic slot reuse
- ✅ No manual memory management for elements
- ❌ Empty slots use memory
- ❌ Zero-byte pattern determines "empty"

## Hash Map

### Map - String-Keyed Hash Map

**When to use**: Fast key-value lookups with string keys.

```c
#include <sigma.collections/map.h>

// Create map with initial capacity
map m = Map.new(16);

// Store key-value pairs (keys are caller-owned)
const char *key1 = "username";
Map.set(m, key1, strlen(key1), (usize)user_ptr);

// Numeric values work too
Map.set(m, "user_id", 7, 12345);

// Retrieve value
usize value;
if (Map.get(m, "username", 8, &value)) {
    user_t *user = (user_t *)value;
    // use user...
}

// Check existence
if (Map.has(m, "user_id", 7)) {
    // key exists
}

// Remove entry
Map.remove(m, "username", 8);

// Get statistics
usize count = Map.count(m);      // Number of entries
usize capacity = Map.capacity(m); // Bucket capacity

// Iterate over entries
sparse_iterator it = Map.create_iterator(m);
while (SparseIterator.next(it)) {
    map_entry *entry;
    SparseIterator.current_value(it, (object *)&entry);
    printf("Key: %.*s, Value: %zu\n", 
           (int)entry->key_len, entry->key, entry->value);
}
SparseIterator.dispose(it);

// Cleanup (doesn't free keys or values)
Map.dispose(m);
```

**Arena Pattern** (recommended for performance):
```c
// Keys allocated from arena - map stores pointers
char arena[4096];
usize arena_offset = 0;

map m = Map.new(32);

// "Allocate" key in arena
const char *key = arena + arena_offset;
strcpy(arena + arena_offset, "field_name");
arena_offset += strlen("field_name") + 1;

// Map stores pointer to arena memory
Map.set(m, key, strlen(key), value);

// Map disposed, arena outlives it - no key copying overhead
Map.dispose(m);
```

**Implementation Details**:
- **Hash Algorithm**: FNV-1a 64-bit (fast, good distribution)
- **Collision Resolution**: Open addressing with linear probing
- **Load Factor**: Auto-resize at 50% capacity
- **Key Storage**: Pointer-based (caller owns keys)
- **Tombstones**: Removed entries marked but not reclaimed until resize

**Trade-offs**:
- ✅ O(1) average lookup, insert, remove
- ✅ No key copying (arena-friendly)
- ✅ Automatic growth at 50% load
- ✅ Iterate over entries with sparse iterator
- ✅ Keys can contain NULL bytes (binary keys)
- ❌ Caller manages key lifetime
- ❌ String keys only (binary-safe, but no integer keys)
- ❌ Tombstones accumulate until resize
- ❌ Memory not reclaimed on remove (only on resize)

## Iterators

### Standard Iterator (Dense Collections)

For List and other dense collections:

```c
#include <sigma.collections/collections.h>

list lst = List.new(10, sizeof(int));
// ... add elements ...

// Create iterator from collection view
collection coll = List.as_collection(lst);
iterator it = Collections.create_iterator(coll);

// Iterate all elements
while (Iterator.next(it)) {
    object current = Iterator.current(it);
    int *value = (int *)current;
    // Process value
}

// Reset to beginning
Iterator.reset(it);

// Cleanup
Iterator.dispose(it);
Collections.dispose(coll);
```

### SparseIterator (Sparse Collections)

Unified iterator for SlotArray and IndexArray - only visits occupied slots:

```c
#include <sigma.collections/indexarray.h>
#include <sigma.collections/collections.h>

indexarray ia = IndexArray.new(100, sizeof(my_struct));
// ... add/remove elements ...

// Create sparse iterator
sparse_iterator it = IndexArray.create_iterator(ia);

// Iterate only occupied slots
while (SparseIterator.next(it)) {
    // Get current slot index
    usize index = SparseIterator.current_index(it);
    
    // Get current value
    my_struct value;
    if (SparseIterator.current_value(it, &value) == OK) {
        printf("Slot %zu: id=%d\n", index, value.id);
    }
}

// Reset to beginning
SparseIterator.reset(it);

// Cleanup
SparseIterator.dispose(it);
IndexArray.dispose(ia);
```

**SparseIterator works identically for SlotArray**:

```c
slotarray sa = SlotArray.new(100);
// ... add/remove elements ...

sparse_iterator it = SlotArray.create_iterator(sa);
while (SparseIterator.next(it)) {
    usize index = SparseIterator.current_index(it);
    object value;
    SparseIterator.current_value(it, &value);
    // Process pointer value
}
SparseIterator.dispose(it);
```

## Buffer Management

### Non-Owning Views

Create collections that wrap existing memory without taking ownership:

```c
// Pre-allocated buffer
my_struct *buffer = malloc(100 * sizeof(my_struct));
my_struct *end = buffer + 100;

// Initialize buffer
memset(buffer, 0, 100 * sizeof(my_struct));

// Create non-owning indexarray view
indexarray ia = IndexArray.from_buffer(buffer, end, sizeof(my_struct));

// Use normally - modifications affect original buffer
my_struct data = {.id = 1, .value = 100};
int handle = IndexArray.add(ia, &data);

// Verify buffer was modified
assert(buffer[handle].id == 1);

// Dispose view (buffer NOT freed)
IndexArray.dispose(ia);

// Buffer still valid - you manage lifetime
assert(buffer[handle].id == 1);
free(buffer);
```

**Use cases**:
- Memory pools managed externally
- Shared memory regions
- Stack-allocated buffers
- Memory-mapped files

### Converting Between Types

```c
// PArray to SlotArray (non-owning view)
parray pa = PArray.new(10);
slotarray sa = PArray.as_slotarray(pa);
// sa is a view - disposing it doesn't free pa

// FArray to Collection
farray fa = FArray.new(10, sizeof(int));
collection coll = FArray.as_collection(fa, sizeof(int));
// coll is a view of fa

// FArray to IndexArray (copying)
indexarray ia = IndexArray.from_farray(fa, sizeof(int));
// ia is independent - owns its own buffer
```

## Custom Allocation

Starting with v0.2.0, sigma.collections supports custom allocation through the `alloc_use` pattern. This allows you to:
- Route allocations through tracking/debugging allocators
- Use custom memory pools or arenas
- Integrate with testing frameworks for leak detection
- Implement domain-specific allocation strategies

### Default Behavior

By default, all collections use standard `malloc`/`free`:

```c
// Uses malloc/free internally
indexarray ia = IndexArray.new(100, sizeof(my_struct));
IndexArray.dispose(ia);  // Calls free internally
```

This requires no setup and works without any dependencies beyond libc.

### Setting Custom Allocator

Wire in a custom allocator using the `alloc_use` function on any collection interface:

```c
#include <sigma.core/allocator.h>

// Define your allocator callbacks
void *my_alloc(usize size) {
    void *ptr = malloc(size);
    log_allocation(ptr, size);
    return ptr;
}

void my_free(void *ptr) {
    log_deallocation(ptr);
    free(ptr);
}

void *my_realloc(void *ptr, usize size) {
    void *new_ptr = realloc(ptr, size);
    log_reallocation(ptr, new_ptr, size);
    return new_ptr;
}

// Set up the allocator struct
sc_alloc_use_t my_allocator = {
    .alloc = my_alloc,
    .release = my_free,
    .resize = my_realloc
};

// Wire into collections (one-time module-level call)
Collections.alloc_use(&my_allocator);

// Now all collection allocations use your callbacks
indexarray ia = IndexArray.new(100, sizeof(int));  // Calls my_alloc
IndexArray.dispose(ia);                             // Calls my_free
```

### Per-Collection Type

Each collection interface has its own `alloc_use` setter, but all share the same module-level hook:

```c
// All of these set the same module-level allocator
Collections.alloc_use(&my_allocator);
FArray.alloc_use(&my_allocator);
List.alloc_use(&my_allocator);
SlotArray.alloc_use(&my_allocator);
IndexArray.alloc_use(&my_allocator);
PArray.alloc_use(&my_allocator);

// Typically just use Collections.alloc_use() - it affects all types
```

### Integration with sigma.test

The primary use case is integration with the sigma.test framework for leak detection:

```c
#include <sigtest/sigtest.h>

static void register_myapp_tests(void) {
    // Wire collections into test framework allocator
    Collections.alloc_use(sigtest_alloc_use());
    
    testset("myapp_tests", setup_fn, teardown_fn);
    testcase("test_creates_indexarray", test_indexarray_lifecycle);
    testcase("test_list_operations", test_list_ops);
}

static void test_indexarray_lifecycle(void) {
    // All allocations tracked by sigma.test
    indexarray ia = IndexArray.new(100, sizeof(int));
    
    // ... perform operations ...
    
    IndexArray.dispose(ia);
    
    // Test framework will report:
    // "Total mallocs: 1"
    // "Total frees: 1"
    // Leak detection happens automatically
}
```

### Resetting to Default

Pass `NULL` to restore default `malloc`/`free` behavior:

```c
Collections.alloc_use(NULL);  // Back to malloc/free

indexarray ia = IndexArray.new(100, sizeof(int));  // Uses malloc again
```

### Implementation Details

The `alloc_use` pattern uses module-level dispatch:

1. All collection types share a single `sc_alloc_use_t *` hook
2. Internal helpers (`coll_alloc`, `coll_free`, `coll_realloc`) check the hook:
   - If hook is NULL → call `malloc`/`free`/`realloc` directly
   - If hook is set → call hook's function pointers
3. No overhead when using default allocation (direct to libc)

### Migration from Allocator Facade

Prior to v0.2.0, collections used the deprecated `Allocator` facade from sigma.core. This has been removed:

**Old pattern (< v0.2.0)**:
```c
#include <sigma.core/alloc.h>

// Allocator facade was global and mandatory
indexarray ia = IndexArray.new(100, sizeof(int));
```

**New pattern (>= v0.2.0)**:
```c
// No change needed - malloc/free is the new default
indexarray ia = IndexArray.new(100, sizeof(int));

// Or wire in custom allocator if needed
Collections.alloc_use(&my_allocator);
```

**Breaking change**: If your code included `<sigma.core/alloc.h>` directly, remove it. The collections headers now include `<sigma.core/allocator.h>` for the `sc_alloc_use_t` type definition only.

### Best Practices

**Set allocator once at startup**:
```c
int main(void) {
    // One-time setup
    Collections.alloc_use(my_allocator_provider());
    
    // Rest of application...
    return 0;
}
```

**For testing, set per test set**:
```c
static void register_tests(void) {
    Collections.alloc_use(sigtest_alloc_use());
    testset("my_tests", NULL, NULL);
    // All tests in this set tracked
}
```

**Don't mix allocators**:
```c
// BAD: Switching allocators mid-lifecycle
Collections.alloc_use(&allocator_a);
indexarray ia = IndexArray.new(100, sizeof(int));  // Uses allocator_a

Collections.alloc_use(&allocator_b);
IndexArray.dispose(ia);  // Uses allocator_b - MISMATCH!

// GOOD: Consistent allocator for object lifetime
Collections.alloc_use(&allocator_a);
indexarray ia = IndexArray.new(100, sizeof(int));
IndexArray.dispose(ia);  // Both use allocator_a
```

**Thread safety**: The module-level hook is not thread-safe. Set it once before spawning threads, or use thread-local storage for per-thread allocators.

## Performance Considerations

### Memory Overhead

| Type | Per-Element Overhead | Notes |
|------|---------------------|-------|
| FArray | 0 bytes | Pure data |
| PArray | 8 bytes (pointer) | Plus pointed-to object |
| List | 0 bytes + growth | Grows by 2x |
| SlotArray | 8 bytes (pointer) | Plus pointed-to object |
| IndexArray | 0 bytes (inline) | But allocates capacity |

### Sparse Collection Growth

**IndexArray** doubles capacity when full:
- Initial: 100 slots
- After growth: 200 slots (100 old + 100 new)
- After growth: 400 slots
- Memory = capacity × stride (empty slots included)

### Cache Performance

**Best to Worst**:
1. FArray - Contiguous values, no indirection
2. IndexArray - Contiguous values, skip empty slots
3. List - Contiguous values, packed dense
4. PArray - Contiguous pointers, one indirection
5. SlotArray - Contiguous pointers, skip empty slots

### Iteration Performance

**Dense iteration** (List, FArray):
- O(n) where n = element count
- Sequential memory access
- High cache hit rate

**Sparse iteration** (SlotArray, IndexArray):
- O(capacity) worst case
- Skips empty slots
- May have cache misses on sparse data

## Best Practices

### Choosing Collection Types

**Use FArray when**:
- Fixed capacity known upfront
- Small value types (int, float, small structs)
- Sequential access patterns
- No removals needed

**Use PArray when**:
- Need to store different object types
- Large objects (avoid copying)
- Reference semantics required

**Use List when**:
- Need dynamic growth
- Mostly append operations
- Removals are rare
- Want automatic memory management

**Use SlotArray when**:
- Handles must be stable
- Frequent add/remove operations
- Large objects (pointers avoid copying)
- Capacity can be estimated

**Use IndexArray when**:
- Handles must be stable
- Frequent add/remove operations
- Small-to-medium structs (< 256 bytes)
- Need dynamic growth
- Cache performance matters

### Memory Management

**Always dispose collections**:
```c
indexarray ia = IndexArray.new(10, sizeof(int));
// ... use collection ...
IndexArray.dispose(ia);  // REQUIRED
```

**For pointer-based collections, free elements**:
```c
slotarray sa = SlotArray.new(10);
my_struct *obj = malloc(sizeof(my_struct));
SlotArray.add(sa, obj);

// Before disposing collection
object retrieved;
for (usize i = 0; i < SlotArray.capacity(sa); i++) {
    if (SlotArray.get_at(sa, i, &retrieved) == OK) {
        free(retrieved);  // Free each object
    }
}
SlotArray.dispose(sa);
```

**Use valgrind for testing**:
```bash
ctest indexarray --valgrind
```

### Error Handling

Most functions return:
- `0` (OK) on success
- `-1` (ERR) on failure
- `-1` for invalid handles/indices

Always check return values:
```c
int handle = IndexArray.add(ia, &data);
if (handle < 0) {
    // Handle error - array might be full (shouldn't happen with growth)
}

my_struct retrieved;
if (IndexArray.get_at(ia, handle, &retrieved) != OK) {
    // Handle error - index invalid or slot empty
}
```

### Thread Safety

⚠️ **Not thread-safe** - Sigma.collections provides no internal locking:

- Don't share collections across threads without external synchronization
- Use mutexes/locks if multi-threaded access needed
- Consider per-thread collections for parallel processing

### Capacity Planning

**For SlotArray** (fixed capacity):
```c
// Estimate capacity based on usage
usize max_concurrent = 1000;
slotarray sa = SlotArray.new(max_concurrent);
```

**For IndexArray** (dynamic):
```c
// Start with reasonable initial capacity
// Growth is automatic but expensive
indexarray ia = IndexArray.new(100, sizeof(my_struct));
// Will grow to 200, 400, 800... as needed
```

### Zero-Byte Pattern Caveat

**IndexArray** uses all-zeros as "empty" marker:

```c
// This struct works fine
typedef struct {
    int id;        // Non-zero values
    float value;
} good_struct;

// This struct is problematic if all-zero is a valid state
typedef struct {
    int flags;     // If flags=0 is valid, conflicts with indexarray
} bad_struct;

// Solution: Use an "initialized" flag
typedef struct {
    bool initialized;  // true = valid, false = empty
    int flags;         // Can be 0 when initialized=true
} better_struct;

// Or use sentinel values
typedef struct {
    int magic;     // Set to non-zero constant (e.g., 0xDEADBEEF) for valid slots
    int flags;
} sentinel_struct;
```

Or check explicitly:
```c
if (IndexArray.is_empty_slot(ia, index)) {
    // Slot is empty (all zeros)
}
```
