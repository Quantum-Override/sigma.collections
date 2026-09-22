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
| **Stack** | Pointer | Dynamic | LIFO push/pop, save-point rollback |

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
- ✅ Queryable via `as_queryable` ([Query.next/Query.first](#query-support), zero allocation)
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
- ✅ Queryable via `as_queryable` ([Query.next/Query.first](#query-support), zero allocation)
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
- ✅ Queryable via `as_queryable` ([Query.next/Query.first](#query-support), zero allocation)
- ✅ `new_with_allocator` for per-instance arena binding (see [Custom Allocation](#custom-allocation))
- ❌ Expensive removal (shifts elements)
- ❌ Can over-allocate memory

**Best for**: Growing sequential collections (queues, logs).

---

### Stack - LIFO Collection

```c
#include <sigma.collections/stack.h>
stack s = Stack.new(capacity);
Stack.push(s, value);
object top = Stack.pop(s);
```

Composed over [Collection](API_REFERENCE.md#collections) — the same relationship List has to it. `push`/`pop`/`peek`/`clear` are thin wrappers over the underlying collection's add/length operations.

**Features**:
- ✅ O(1) push/pop/peek
- ✅ `mark`/`restore` — O(1) truncation back to a saved depth (no allocation, no zeroing), useful for save-point/rollback patterns like speculative parsing
- ✅ `new_with_allocator` for per-instance arena binding (see [Custom Allocation](#custom-allocation))
- ❌ No random access (top-only), no iteration/query support

**Best for**: LIFO workloads — parser/resolver scope stacks, undo-adjacent save-points, expression evaluation.

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
- ✅ Queryable via `as_queryable` ([Query.next/Query.first](#query-support), zero allocation, skips empty slots)
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
- ✅ Queryable via `as_queryable`/`keys`/`values` ([Query.next/Query.first](#query-support), zero allocation) — `as_queryable` can find an entry *by value*, which `Map.get` can't do
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

## Query Support

FArray, PArray, Collection, List, SlotArray, and Map all support a second, **heapless** way to scan — `as_queryable` (or, for Map, `keys`/`values`) — for the common case where `Iterator`/`SparseIterator`'s allocate-then-dispose lifecycle is more than you need. An `sc_queryable` is a plain struct held by value, typically on the stack; `Query.next`/`Query.first` never allocate.

**Roll-your-own scan** (`Query.next`):

```c
sc_queryable q = FArray.as_queryable(arr, sizeof(int));
const void *elem;
long sum = 0;
while (Query.next(&q, &elem, NULL)) {
    sum += *(const int *)elem;
}
```

**Find the first match** (`Query.first`) — this is the one Iterator can't do in a single call, and the one Map.get can't do *by value* rather than by key:

```c
bool value_is_3(const void *element, usize index, void *userdata) {
    return ((const map_entry *)element)->value == 3;
}
if (Query.first(Map.as_queryable(m), value_is_3, NULL, NULL)) {
    // found an entry whose value is 3 — Map.get can only look up by key
}
```

**When to reach for which**:

| | `Iterator`/`SparseIterator` | `Query` |
|---|---|---|
| Allocates | Yes (create/dispose) | No — heapless, POD cursor by value |
| Fits | A held, multi-step iteration session (`reset`, `current`) | A one-shot scan or "find first match" check |
| Cost of "just check one thing" | Allocate, scan, dispose | Zero setup |

**⚠️ Same mutation rule as Iterator**: an `sc_queryable` is a live view into its source's storage, not a snapshot. Don't add/remove/set/clear/grow the source while a queryable over it is still in use — see `query.h`'s INVARIANT note (and [API Reference — Query](API_REFERENCE.md#query) for the full contract).

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

**Choose Stack when**:
- LIFO access only (top-in, top-out)
- Save-point/rollback semantics via `mark`/`restore`
- Parser/resolver scope tracking, expression evaluation

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

## Custom Allocation

By default, every collection allocates through the global `Allocator`/`Application` facade (from sigma.memory) — there is no way to swap that out module-wide. Since v0.3.0, Collection/List/Stack support a **per-instance** override instead: `Collections.create_with_allocator` / `List.new_with_allocator` / `Stack.new_with_allocator` bind *one instance* to its own `sc_alloc_use_t *`, leaving every other instance (and every other type — FArray, PArray, SlotArray, Map still always use the global facade) unaffected. This is what lets two collections be alive at once, bound to two different allocators — e.g. a parse-scoped bump arena versus a longer-lived, document-scoped one:

```c
#include <sigma.collections/list.h>

// Bound to a bump arena that gets bulk-freed in one shot when the parse
// finishes — no per-list dispose bookkeeping needed.
list scratch = List.new_with_allocator(16, sizeof(node *), &parse_arena_use);
```

`NULL` in place of `use` is always a plain pass-through to the ordinary constructor. When an instance is bound, growth allocates through `use->alloc` and *orphans* the old buffer rather than freeing it — a bump/arena allocator has no way to free one specific prior allocation, only bulk-release the whole arena — and `dispose` on that instance likewise skips freeing the buffer.

See [User Guide - Custom Allocation](USERS_GUIDE.md#custom-allocation) for details.

---

## Summary Statistics

- **7 Collection Types**: FArray, PArray, List, Stack, SlotArray, IndexArray, Map
- **2 Iterator Types**: Iterator, SparseIterator
- **1 Query Mechanism**: heapless `Query.next`/`Query.first`, via `as_queryable`/`keys`/`values` on FArray, PArray, Collection, List, SlotArray, Map
- **141 Registered Test Cases**: 28 List, 21 Map, 19 SlotArray, 16 FArray, 16 IndexArray, 15 PArray, 9 Stack, 8 Query, 6 Collections, 2 Collections-allocator, 1 Iterator
- **Zero Memory Leaks**: Verified with valgrind on the pieces checked; one pre-existing, unrelated test (`slotarray_from_value_array`) currently crashes the SlotArray suite before reaching the leak-check step — see its own test file for the note
- **Version**: 0.3.0

---

## See Also

- **[User Guide](USERS_GUIDE.md)** - Detailed usage patterns
- **[API Reference](API_REFERENCE.md)** - Complete function reference
- **[Migration Guide v0.2.0](MIGRATION_v0.2.0.md)** - Upgrading from v0.1.x
