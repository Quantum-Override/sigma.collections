# Collections Overview

Complete catalog of collections in sigma.collections library.

## Collection Types

### Dense Collections
Fixed or growing contiguous memory, all elements sequential.

| Collection | Type | Growth | Use Case |
|------------|------|--------|----------|
| **FArray** | Value | Fixed | Fixed-size arrays of values |
| **PArray** | Pointer | Fixed | Fixed-size arrays of pointers |
| **List** | Value | Dynamic | Growing/shrinking sequential data |

### Sparse Collections
Non-contiguous with slot reuse, stable handles, skip empty slots during iteration.

| Collection | Type | Growth | Use Case |
|------------|------|--------|----------|
| **SlotArray** | Pointer | Fixed | Stable handles to objects |
| **IndexArray** | Value | Dynamic | Stable handles to values |

### Hash Map
Key-value store with fast lookups.

| Collection | Key Type | Value Type | Growth | Use Case |
|------------|----------|------------|--------|----------|
| **Map** | String | usize | Dynamic | Fast string-keyed lookups |

---

## Quick Reference

### FArray - Flex Array

```c
#include <sigma.collections/farray.h>
farray arr = FArray.new(capacity, stride);
```

**Features**:
- ✅ Cache-friendly contiguous layout
- ✅ No pointer indirection
- ❌ Fixed capacity (no growth)
- ❌ Must pass stride to every operation

**Best for**: Small fixed collections of values.

---

### PArray - Pointer Array

```c
#include <sigma.collections/parray.h>
parray arr = PArray.new(capacity);
```

**Features**:
- ✅ Stores any pointer type
- ✅ No stride parameter needed
- ❌ Extra indirection (pointer chasing)
- ❌ Caller manages object lifetime

**Best for**: Collections of heap-allocated objects.

---

### List - Dynamic List

```c
#include <sigma.collections/list.h>
list lst = List.new(capacity, stride);
```

**Features**:
- ✅ Automatic growth on append
- ✅ Sequential access patterns
- ❌ Expensive removal (shifts elements)
- ❌ Can over-allocate memory

**Best for**: Growing sequential collections (queues, stacks, logs).

---

### SlotArray - Sparse Pointer Array

```c
#include <sigma.collections/slotarray.h>
slotarray sa = SlotArray.new(capacity);
int handle = SlotArray.add(sa, obj);
```

**Features**:
- ✅ Stable handles (never change)
- ✅ Automatic slot reuse
- ✅ Sparse iteration (skip empty)
- ❌ Fixed capacity
- ❌ Empty slots use memory

**Best for**: Object pools with stable references.

---

### IndexArray - Sparse Value Array

```c
#include <sigma.collections/indexarray.h>
indexarray ia = IndexArray.new(capacity, stride);
int handle = IndexArray.add(ia, &value);
```

**Features**:
- ✅ Stable handles (never change)
- ✅ Automatic growth when full
- ✅ Values stored inline (cache-friendly)
- ✅ Sparse iteration (skip empty)
- ❌ Empty slots use memory
- ❌ Zero-byte pattern = empty

**Best for**: Entity pools, component arrays, handle-based access.

---

### Map - String-Keyed Hash Map

```c
#include <sigma.collections/map.h>
map m = Map.new(capacity);
Map.set(m, key, key_len, value);
```

**Features**:
- ✅ O(1) average lookup/insert/remove
- ✅ No key copying (caller-owned)
- ✅ Auto-resize at 50% load
- ✅ Binary-safe keys (NULL bytes OK)
- ✅ Sparse iteration over entries
- ❌ String keys only
- ❌ Tombstones until resize

**Implementation**: FNV-1a hashing, open addressing, linear probing.

**Best for**: Fast key-value lookups with string keys, arena-allocated keys.

---

## Iterator Support

### Standard Iterator (Dense)
For List and dense collections:

```c
collection coll = List.as_collection(lst);
iterator it = Collections.create_iterator(coll);
while (Iterator.next(it)) {
    object value = Iterator.current(it);
    // process...
}
Iterator.dispose(it);
```

### Sparse Iterator
For SlotArray, IndexArray, and Map (skips empty slots):

```c
sparse_iterator it = IndexArray.create_iterator(ia);
while (SparseIterator.next(it)) {
    my_struct value;
    SparseIterator.current_value(it, &value);
    // process...
}
SparseIterator.dispose(it);
```

**Map Iteration** (access key and value):

```c
sparse_iterator it = Map.create_iterator(m);
while (SparseIterator.next(it)) {
    map_entry *entry;
    SparseIterator.current_value(it, (object *)&entry);
    printf("Key: %.*s, Value: %zu\n", 
           (int)entry->key_len, entry->key, entry->value);
}
SparseIterator.dispose(it);
```

---

## Selection Guide

**Choose FArray when**:
- Fixed small collections
- Value types
- Performance critical (cache-friendly)

**Choose PArray when**:
- Fixed collections of objects
- Reference semantics needed
- Managing existing heap objects

**Choose List when**:
- Growing/shrinking collections
- Sequential access
- Append-heavy workloads

**Choose SlotArray when**:
- Need stable handles
- Fixed capacity acceptable
- Managing heap objects

**Choose IndexArray when**:
- Need stable handles
- Dynamic growth required
- Values stored inline

**Choose Map when**:
- Fast key-value lookups
- String keys
- O(1) access required

---

## Custom Allocation (v0.2.0+)

All collections support custom allocators:

```c
#include <sigma.collections/collections.h>

// Set for all collections
Collections.alloc_use(my_allocator_use());

// Or per-interface
FArray.alloc_use(my_allocator_use());
Map.alloc_use(my_allocator_use());
```

See [User Guide - Custom Allocation](USERS_GUIDE.md#custom-allocation) for details.

---

## Summary Statistics

- **6 Collection Types**: FArray, PArray, List, SlotArray, IndexArray, Map
- **2 Iterator Types**: Iterator, SparseIterator
- **102 Tests**: All passing (17 Map, 17 SlotArray, 16 IndexArray, 23 List, 13 FArray, 13 PArray, 3 other)
- **Zero Memory Leaks**: Verified with valgrind
- **Version**: 0.2.0 (RC)

---

## See Also

- **[User Guide](USERS_GUIDE.md)** - Detailed usage patterns
- **[API Reference](API_REFERENCE.md)** - Complete function reference
- **[Migration Guide v0.2.0](MIGRATION_v0.2.0.md)** - Upgrading from v0.1.x
