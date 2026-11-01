# Sigma Collections - User Guide

Comprehensive guide to using the sigma.collections library for efficient data structure management in C.

## Table of Contents

- [Overview](#overview)
- [Dense Collections](#dense-collections)
- [Sparse Collections](#sparse-collections)
- [Hash Map](#hash-map)
- [Iterators](#iterators)
- [Query](#query)
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

### Stack - LIFO Collection

**When to use**: LIFO access only — parser/resolver scope tracking, expression evaluation, save-point/rollback patterns.

Composed over [Collection](API_REFERENCE.md#collections) — the same relationship List has to it. `push`/`pop`/`peek`/`clear` are thin wrappers over the underlying collection's add/length operations, so Stack gets automatic growth "for free."

```c
#include <sigma.collections/stack.h>

// Create stack with initial capacity
stack s = Stack.new(16);

// Push/pop/peek
int a = 1, b = 2;
Stack.push(s, &a);
Stack.push(s, &b);

object top = Stack.peek(s);   // &b, does not remove
object popped = Stack.pop(s); // &b, removes

// Save-point / rollback - O(1), no allocation, no zeroing of the
// discarded region
usize checkpoint = Stack.mark(s);
Stack.push(s, &a);
// ... speculative work that might need to be undone ...
Stack.restore(s, checkpoint);  // discards everything pushed since checkpoint

// Cleanup
Stack.dispose(s);
```

**Trade-offs**:
- ✅ O(1) push/pop/peek
- ✅ `mark`/`restore` for O(1) truncation to a saved depth
- ✅ Dynamic growth (inherited from Collection)
- ❌ No random access — top only
- ❌ No iteration or [Query](#query) support (not needed for the stack's usage pattern; would be a trivial addition if a future need arises)

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

## Query

A second, **heapless** way to scan FArray, PArray, Collection, List, SlotArray, or Map — for the common case where `Iterator`/`SparseIterator`'s allocate-then-dispose lifecycle is more than you need. An `sc_queryable` is a plain POD struct held by value, typically on the stack; `Query.next`/`Query.first` never allocate.

**When to reach for `Query` instead of `Iterator`/`SparseIterator`**: a one-shot scan, or "find the first element matching X" — especially inside a hot loop, where allocating (and disposing) an iterator object just to ask one question is real, avoidable overhead. Reach for `Iterator`/`SparseIterator` instead when you genuinely need a held, multi-step iteration session (`reset`, `current`, explicit lifetime).

### Roll-Your-Own Scan (`Query.next`)

```c
#include <sigma.collections/query.h>
#include <sigma.collections/farray.h>

farray arr = FArray.new(4, sizeof(int));
int values[] = {10, 20, 30, 40};
for (int i = 0; i < 4; i++) FArray.set(arr, i, sizeof(int), &values[i]);

sc_queryable q = FArray.as_queryable(arr, sizeof(int));
const void *elem;
long sum = 0;
while (Query.next(&q, &elem, NULL)) {
    sum += *(const int *)elem;
}
FArray.dispose(arr);
```

### Find the First Match (`Query.first`)

```c
bool is_the_value_30(const void *element, usize index, void *userdata) {
    return *(const int *)element == 30;
}

usize idx;
if (Query.first(FArray.as_queryable(arr, sizeof(int)), is_the_value_30, NULL, &idx)) {
    printf("found at index %zu\n", idx);
}

// "any" usage - pass NULL for out_index when you only care whether a
// match exists, not where
if (Query.first(FArray.as_queryable(arr, sizeof(int)), is_the_value_30, NULL, NULL)) {
    printf("contains at least one 30\n");
}
```

### Every Collection Type's `as_queryable`

| Type | Call | Yields |
|---|---|---|
| FArray | `FArray.as_queryable(arr, stride)` | `const void *` (stride-sized element) |
| PArray | `PArray.as_queryable(arr)` | `const addr *` |
| Collection | `Collections.as_queryable(coll)` | `const void *` (stride-sized element) |
| List | `List.as_queryable(lst)` | `const void *` (delegates to Collections) |
| SlotArray | `SlotArray.as_queryable(sa)` | `const addr *`, empty slots skipped |
| Map | `Map.as_queryable(m)` | `const map_entry *`, tombstones skipped |
| Map | `Map.keys(m)` | `const sc_key_view *`, tombstones skipped |
| Map | `Map.values(m)` | `const usize *`, tombstones skipped |

Stack does not have `as_queryable` — its usage pattern (top-only push/pop) never called for a scan; adding one later would be a trivial delegation to `Collections.as_queryable`, same as List.

### Map: Finding an Entry By Value

`Map.get` only looks up by key. `Query.first` over `Map.as_queryable` can find an entry by anything about it, including its **value** — something the key-based API has no way to express:

```c
bool value_is_3(const void *element, usize index, void *userdata) {
    return ((const map_entry *)element)->value == 3;
}
if (Query.first(Map.as_queryable(m), value_is_3, NULL, NULL)) {
    // found an entry whose value is 3, regardless of its key
}
```

`Map.keys`/`Map.values` scan just one half of the pair, when you don't need both:

```c
sc_queryable ks = Map.keys(m);
const void *elem;
while (Query.next(&ks, &elem, NULL)) {
    const sc_key_view *k = elem;  // {ptr, len} — not guaranteed NUL-terminated
    printf("%.*s\n", (int)k->len, k->ptr);
}
```

### ⚠️ The Mutation Invariant

An `sc_queryable` is a **live view into its source's storage, not a snapshot** — it holds a raw pointer/index position captured at `as_queryable`/`keys`/`values` time. Don't add/append/insert/remove/set/clear/resize/grow the source while a queryable over it is still in use: a dense source's buffer may have been reallocated out from under the queryable's saved pointer; a sparse source's slot layout may have shifted under its saved index. This is the same rule `Iterator`/`SparseIterator` already require. Finish pulling from a queryable (or simply stop using it) before mutating its source, and construct a fresh one afterward to scan again.

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

sigma.collections went through several different custom-allocation designs across v0.2.0/v0.2.1/Phase 3A — a module-level `alloc_use` hook was tried and then removed as "vestigial, never fully implemented" (see FR-2603-sigma-collections-001, now marked obsolete). What's described below is the **current, real mechanism**, added in v0.3.0: a **per-instance** override, available on Collection, List, and Stack only.

### Default Behavior

By default, every collection allocates through the global `Allocator`/`Application` facade (from sigma.memory) — there is no module-wide way to swap that out, and no plain `malloc`/`free` fallback:

```c
// Allocates via the global Allocator facade internally
indexarray ia = IndexArray.new(100, sizeof(my_struct));
IndexArray.dispose(ia);  // Disposes via the global Allocator facade
```

This is the only behavior FArray, PArray, SlotArray, IndexArray, and Map have — they don't support any allocator override, past or present.

### Per-Instance Override (Collection, List, Stack)

Collection, List, and Stack additionally support binding *one instance* to its own `sc_alloc_use_t *`, independent of every other instance — including other instances of the same type:

```c
#include <sigma.core/allocator.h>
#include <sigma.collections/list.h>

// Define your allocator callbacks
void *my_alloc(usize size) {
    void *ptr = malloc(size);
    log_allocation(ptr, size);
    return ptr;
}

void my_release(void *ptr) {
    log_deallocation(ptr);
    free(ptr);
}

sc_alloc_use_t my_allocator = {
    .alloc = my_alloc,
    .release = my_release,
    // .resize, .frame_begin, .frame_end — leave NULL if unused
};

// Bind just this one list to my_allocator; every other List.new(...) call
// anywhere else in the program is completely unaffected.
list tracked = List.new_with_allocator(16, sizeof(int), &my_allocator);
List.dispose(tracked);  // Only frees the struct itself — see below
```

The same shape exists on the other two types:

```c
collection coll = Collections.create_with_allocator(capacity, stride, &my_allocator);
stack s = Stack.new_with_allocator(capacity, &my_allocator);
```

**`NULL` is always a plain pass-through** to the type's ordinary constructor (`List.new`, `Stack.new`, `collection_new`) — ​every caller that doesn't opt in behaves exactly as before this feature existed.

### Growth and Disposal Semantics When Bound

Because a bump/arena allocator (the primary motivating use case) has no way to free one specific prior allocation — only bulk-release everything at once — a bound instance's lifecycle differs from the default path:

- **Growth** allocates the new, larger buffer through `use->alloc` and **orphans** the old one (does not call `use->release` on it).
- **Dispose** frees the instance's own struct (always, via the global Allocator — that part is small and short-lived) but **skips freeing the buffer** — the bound allocator is expected to reclaim it as part of its own bulk-release (e.g. resetting/releasing the whole arena).

```c
// Typical arena-scoped pattern: many instances bound to one arena, none
// individually disposed of their buffers — the arena reclaims everything
// at once when the parse (or request, or frame) completes.
list a = List.new_with_allocator(8, sizeof(node *), &parse_arena_use);
list b = List.new_with_allocator(8, sizeof(node *), &parse_arena_use);
// ... use a and b ...
List.dispose(a);  // frees the struct; buffer orphaned
List.dispose(b);  // frees the struct; buffer orphaned
arena_release_all(&parse_arena);  // reclaims both buffers in one shot
```

### Two Different Allocators, Two Live Instances

The reason this is per-instance rather than a single module-wide swap: a module-wide hook can't represent "these two collections are bound to two different allocators, both alive right now." Per-instance binding can:

```c
list request_scoped = List.new_with_allocator(4, sizeof(req_t *), &request_arena_use);
list document_scoped = List.new_with_allocator(4, sizeof(doc_t *), &document_arena_use);
// Both lists are alive simultaneously, each growing/orphaning through its
// own arena, with zero interaction between the two.
```

### Best Practices

**Bind at construction, dispose with the same understanding of ownership**: the allocator you pass to `new_with_allocator`/`create_with_allocator` is fixed for that instance's entire lifetime — there's no way to rebind an existing instance, so there's no "mismatch mid-lifecycle" failure mode to guard against the way a module-wide hook would have had.

**Remember dispose doesn't free the buffer when bound**:
```c
// A bound instance's dispose() only frees its own struct - the buffer is
// the arena's responsibility. Forgetting this and never releasing the
// arena at all is a real leak; calling free() on the buffer yourself is a
// double-free once the arena releases it too.
list scratch = List.new_with_allocator(8, sizeof(int), &parse_arena_use);
// ... use scratch ...
List.dispose(scratch);       // frees the struct only
arena_release(&parse_arena); // this is what actually reclaims the buffer
```

**Thread safety**: an `sc_alloc_use_t` itself carries no locking — if the same arena is bound to instances used from multiple threads, the arena's own `alloc`/`release` callbacks are responsible for any synchronization. Binding a fresh, thread-local arena per thread avoids the question entirely.

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
