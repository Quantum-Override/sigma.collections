# Sigma Collections - API Reference

Complete function reference for sigma.collections library.

## Table of Contents

- [FArray](#farray)
- [PArray](#parray)
- [List](#list)
- [Stack](#stack)
- [SlotArray](#slotarray)
- [IndexArray](#indexarray)
- [Map](#map)
- [Iterator](#iterator)
- [SparseIterator](#sparseiterator)
- [Query](#query)
- [Collections](#collections)

---

## FArray

**Header**: `<sigma.collections/farray.h>`

Flex array for value types with configurable stride.

### Functions

#### `FArray.new`
```c
farray FArray.new(usize capacity, usize stride);
```
Create a new flex array.

**Parameters**:
- `capacity` - Initial number of elements
- `stride` - Size of each element in bytes

**Returns**: New farray or NULL on failure

**Example**:
```c
farray arr = FArray.new(10, sizeof(int));
```

---

#### `FArray.init`
```c
void FArray.init(farray *arr, usize capacity, usize stride);
```
Initialize an farray in-place.

**Parameters**:
- `arr` - Pointer to farray variable
- `capacity` - Initial capacity
- `stride` - Element size

---

#### `FArray.dispose`
```c
void FArray.dispose(farray arr);
```
Dispose of array and free resources.

**Parameters**:
- `arr` - Array to dispose

---

#### `FArray.capacity`
```c
int FArray.capacity(farray arr, usize stride);
```
Get array capacity.

**Parameters**:
- `arr` - Array to query
- `stride` - Element size

**Returns**: Capacity or 0 on error

---

#### `FArray.set`
```c
int FArray.set(farray arr, usize index, usize stride, object value);
```
Set value at index.

**Parameters**:
- `arr` - Array to modify
- `index` - Index to set
- `stride` - Element size
- `value` - Pointer to value (must be at least stride bytes)

**Returns**: 0 on success, -1 on error

---

#### `FArray.get`
```c
int FArray.get(farray arr, usize index, usize stride, object out_value);
```
Get value at index.

**Parameters**:
- `arr` - Array to query
- `index` - Index to get
- `stride` - Element size
- `out_value` - Pointer to store value (must be at least stride bytes)

**Returns**: 0 on success, -1 on error

---

#### `FArray.remove`
```c
int FArray.remove(farray arr, usize index, usize stride);
```
Remove element at index (zeros slot).

**Parameters**:
- `arr` - Array to modify
- `index` - Index to remove
- `stride` - Element size

**Returns**: 0 on success, -1 on error

---

#### `FArray.clear`
```c
void FArray.clear(farray arr, usize stride);
```
Clear all elements.

**Parameters**:
- `arr` - Array to clear
- `stride` - Element size

---

#### `FArray.as_collection`
```c
collection FArray.as_collection(farray arr, usize stride);
```
Create non-owning collection view.

**Parameters**:
- `arr` - Array to view
- `stride` - Element size

**Returns**: Collection view or NULL on failure

---

#### `FArray.as_queryable`
```c
sc_queryable FArray.as_queryable(farray arr, usize stride);
```
Produce a heapless queryable over the array's elements, for use with [`Query.next`/`Query.first`](#query). No allocation — no collection view is created.

**Parameters**:
- `arr` - Array to query
- `stride` - Element size

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not mutate `arr` while the returned queryable is in use — see [Query](#query)'s invariant note.

**Example**:
```c
sc_queryable q = FArray.as_queryable(arr, sizeof(int));
usize idx;
if (Query.first(q, is_the_value_im_after, NULL, &idx)) {
    printf("found at index %zu\n", idx);
}
```

---

## PArray

**Header**: `<sigma.collections/parray.h>`

Pointer array for reference types.

### Functions

#### `PArray.new`
```c
parray PArray.new(usize capacity);
```
Create a new pointer array.

**Parameters**:
- `capacity` - Initial number of pointer slots

**Returns**: New parray or NULL on failure

---

#### `PArray.dispose`
```c
void PArray.dispose(parray arr);
```
Dispose of array. Does NOT free pointed-to objects.

---

#### `PArray.capacity`
```c
int PArray.capacity(parray arr);
```
Get array capacity.

**Returns**: Capacity or 0 on error

---

#### `PArray.set`
```c
int PArray.set(parray arr, usize index, addr value);
```
Set pointer at index.

**Parameters**:
- `arr` - Array to modify
- `index` - Index to set
- `value` - Pointer value (cast to addr)

**Returns**: 0 on success, -1 on error

---

#### `PArray.get`
```c
int PArray.get(parray arr, usize index, addr *out_value);
```
Get pointer at index.

**Parameters**:
- `arr` - Array to query
- `index` - Index to get
- `out_value` - Pointer to store address

**Returns**: 0 on success, -1 on error

---

#### `PArray.as_slotarray`
```c
slotarray PArray.as_slotarray(parray arr);
```
Create non-owning slotarray view.

**Returns**: SlotArray view or NULL on failure

---

#### `PArray.as_queryable`
```c
sc_queryable PArray.as_queryable(parray arr);
```
Produce a heapless queryable over the array's elements, for use with [`Query.next`/`Query.first`](#query). Yields `const addr *` per element.

**Parameters**:
- `arr` - Array to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not mutate `arr` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

## List

**Header**: `<sigma.collections/list.h>`

Dynamic growable list.

### Functions

#### `List.new`
```c
list List.new(usize capacity, usize stride);
```
Create a new list.

**Parameters**:
- `capacity` - Initial capacity
- `stride` - Element size

**Returns**: New list or NULL on failure

---

#### `List.new_with_allocator`
```c
list List.new_with_allocator(usize capacity, usize elem_size, sc_alloc_use_t *use);
```
Create a new list bound to a caller-supplied allocator-use. The instance grows/orphans through `use` instead of the global Allocator/Application facade.

**Parameters**:
- `capacity` - Initial list capacity
- `elem_size` - Size of each element in the list
- `use` - Allocator-use to bind this instance to, or `NULL` to behave exactly like `List.new`

**Returns**: New list instance, or NULL on failure

**Notes**:
- `NULL` is a plain pass-through to `List.new` — every existing caller is unaffected.
- When bound, growth allocates through `use->alloc` and **orphans** (does not free) the previous buffer — a bump/arena allocator has no way to free one specific prior allocation, only bulk-release the whole arena.
- `List.dispose` on a bound instance likewise skips freeing the buffer; the bound allocator owns reclaiming it.

**Example**:
```c
// A parse-scoped bump arena; every list bound to it is reclaimed in one
// shot when the parse finishes, no per-list dispose bookkeeping needed.
list scratch = List.new_with_allocator(16, sizeof(node *), &parse_arena_use);
```

---

#### `List.dispose`
```c
void List.dispose(list lst);
```
Dispose of list.

---

#### `List.append`
```c
int List.append(list lst, object value);
```
Append element to end (grows automatically).

**Parameters**:
- `lst` - List to modify
- `value` - Pointer to value to append

**Returns**: 0 on success, -1 on error

---

#### `List.get_at`
```c
int List.get_at(list lst, usize index, object *out_value);
```
Get element at index.

**Parameters**:
- `lst` - List to query
- `index` - Index to get
- `out_value` - Pointer to store value

**Returns**: 0 on success, -1 on error

---

#### `List.remove_at`
```c
int List.remove_at(list lst, usize index);
```
Remove element at index (shifts remaining elements).

**Parameters**:
- `lst` - List to modify
- `index` - Index to remove

**Returns**: 0 on success, -1 on error

---

#### `List.size`
```c
usize List.size(list lst);
```
Get number of elements.

**Returns**: Element count

---

#### `List.capacity`
```c
usize List.capacity(list lst);
```
Get current capacity.

**Returns**: Capacity

---

#### `List.as_queryable`
```c
sc_queryable List.as_queryable(list lst);
```
Produce a heapless queryable over the list's elements, for use with [`Query.next`/`Query.first`](#query). Delegates to `Collections.as_queryable` — List owns no storage of its own.

**Parameters**:
- `lst` - List to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not mutate `lst` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

## Stack

**Header**: `<sigma.collections/stack.h>`

LIFO collection composed over [Collection](#collections) — the same relationship List has to it. `push`/`pop`/`peek`/`clear` are thin wrappers over the underlying collection's add/length operations, so Stack inherits arena-aware growth for free via `new_with_allocator`.

### Functions

#### `Stack.new`
```c
stack Stack.new(usize capacity);
```
Create a new stack.

**Parameters**:
- `capacity` - Initial stack capacity

**Returns**: New stack or NULL on failure

---

#### `Stack.new_with_allocator`
```c
stack Stack.new_with_allocator(usize capacity, sc_alloc_use_t *use);
```
Create a new stack bound to a caller-supplied allocator-use. Same allocator-override semantics as [`List.new_with_allocator`](#list) — `NULL` behaves exactly like `Stack.new`; a bound instance grows through `use` and orphans (rather than frees) old buffers.

**Parameters**:
- `capacity` - Initial stack capacity
- `use` - Allocator-use to bind this instance to, or `NULL`

**Returns**: New stack instance, or NULL on failure

---

#### `Stack.dispose`
```c
void Stack.dispose(stack s);
```
Dispose of stack.

---

#### `Stack.push`
```c
int Stack.push(stack s, object value);
```
Push a value onto the top of the stack.

**Parameters**:
- `s` - Stack to push onto
- `value` - Value to push

**Returns**: 0 on success, -1 on error

---

#### `Stack.pop`
```c
object Stack.pop(stack s);
```
Remove and return the value at the top of the stack.

**Returns**: The top value, or `NULL` if the stack is empty

---

#### `Stack.peek`
```c
object Stack.peek(stack s);
```
Return the value at the top of the stack without removing it.

**Returns**: The top value, or `NULL` if the stack is empty

---

#### `Stack.is_empty`
```c
bool Stack.is_empty(stack s);
```
Check whether the stack has no elements.

**Returns**: `true` if empty; otherwise `false`

---

#### `Stack.count`
```c
usize Stack.count(stack s);
```
Get the current number of elements on the stack.

**Returns**: Current element count

---

#### `Stack.clear`
```c
void Stack.clear(stack s);
```
Clear the contents of the stack.

---

#### `Stack.mark`
```c
usize Stack.mark(stack s);
```
Save the stack's current depth as a bookmark, for use with `Stack.restore`.

**Returns**: The current depth (`== Stack.count(s)`)

---

#### `Stack.restore`
```c
void Stack.restore(stack s, usize mark);
```
Truncate the stack back to a previously saved depth, discarding everything pushed after it. O(1) — no allocation, no zeroing of the discarded region.

**Parameters**:
- `s` - Stack to restore
- `mark` - A depth previously returned by `Stack.mark`; a value greater than the current depth is a no-op

**Example** — save-point/rollback pattern (e.g. speculative parsing):
```c
usize checkpoint = Stack.mark(s);
try_speculative_parse(s);
if (parse_failed) {
    Stack.restore(s, checkpoint);  // discard everything pushed since the checkpoint
}
```

---

## SlotArray

**Header**: `<sigma.collections/slotarray.h>`

Sparse pointer array with slot reuse.

### Functions

#### `SlotArray.new`
```c
slotarray SlotArray.new(usize capacity);
```
Create a new slotarray.

**Parameters**:
- `capacity` - Fixed number of slots

**Returns**: New slotarray or NULL on failure

---

#### `SlotArray.dispose`
```c
void SlotArray.dispose(slotarray sa);
```
Dispose of slotarray. Does NOT free pointed-to objects.

---

#### `SlotArray.add`
```c
int SlotArray.add(slotarray sa, object value);
```
Add element, reusing empty slots.

**Parameters**:
- `sa` - SlotArray to modify
- `value` - Pointer to add

**Returns**: Handle (index) or -1 if full

---

#### `SlotArray.get_at`
```c
int SlotArray.get_at(slotarray sa, usize index, object *out_value);
```
Get element at handle.

**Parameters**:
- `sa` - SlotArray to query
- `index` - Handle (index)
- `out_value` - Pointer to store value

**Returns**: 0 on success, -1 if empty or invalid

---

#### `SlotArray.remove_at`
```c
int SlotArray.remove_at(slotarray sa, usize index);
```
Remove element at handle (marks slot empty).

**Parameters**:
- `sa` - SlotArray to modify
- `index` - Handle to remove

**Returns**: 0 on success, -1 on error

---

#### `SlotArray.is_empty_slot`
```c
bool SlotArray.is_empty_slot(slotarray sa, usize index);
```
Check if slot is empty.

**Returns**: true if empty, false if occupied

---

#### `SlotArray.capacity`
```c
usize SlotArray.capacity(slotarray sa);
```
Get total number of slots.

**Returns**: Capacity

---

#### `SlotArray.clear`
```c
void SlotArray.clear(slotarray sa);
```
Mark all slots as empty.

---

#### `SlotArray.create_iterator`
```c
sparse_iterator SlotArray.create_iterator(slotarray sa);
```
Create sparse iterator.

**Returns**: Iterator or NULL on failure

---

#### `SlotArray.as_queryable`
```c
sc_queryable SlotArray.as_queryable(slotarray sa);
```
Produce a heapless queryable over the slotarray's occupied slots, for use with [`Query.next`/`Query.first`](#query). Skips empty slots automatically. Yields `const addr *` per element.

**Parameters**:
- `sa` - SlotArray to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not `add`/`remove_at` on `sa` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

## IndexArray

**Header**: `<sigma.collections/indexarray.h>`

Sparse value array with dynamic growth.

### Functions

#### `IndexArray.new`
```c
indexarray IndexArray.new(usize capacity, usize stride);
```
Create a new indexarray.

**Parameters**:
- `capacity` - Initial capacity
- `stride` - Element size

**Returns**: New indexarray or NULL on failure

---

#### `IndexArray.dispose`
```c
void IndexArray.dispose(indexarray ia);
```
Dispose of indexarray.

---

#### `IndexArray.add`
```c
int IndexArray.add(indexarray ia, object value);
```
Add element, reusing empty slots or growing.

**Parameters**:
- `ia` - IndexArray to modify
- `value` - Pointer to value to copy

**Returns**: Handle (index) or -1 on error

---

#### `IndexArray.get_at`
```c
int IndexArray.get_at(indexarray ia, usize index, object out_value);
```
Get element at handle.

**Parameters**:
- `ia` - IndexArray to query
- `index` - Handle
- `out_value` - Pointer to store value (must be at least stride bytes)

**Returns**: 0 on success, -1 if empty or invalid

---

#### `IndexArray.remove_at`
```c
int IndexArray.remove_at(indexarray ia, usize index);
```
Remove element at handle (zeros slot).

**Parameters**:
- `ia` - IndexArray to modify
- `index` - Handle to remove

**Returns**: 0 on success, -1 on error

---

#### `IndexArray.from_farray`
```c
indexarray IndexArray.from_farray(farray arr, usize stride);
```
Create indexarray by copying from farray.

**Parameters**:
- `arr` - Source farray
- `stride` - Element size

**Returns**: New indexarray or NULL on failure

---

#### `IndexArray.from_buffer`
```c
indexarray IndexArray.from_buffer(void *buffer, void *end, usize stride);
```
Create non-owning indexarray view of buffer.

**Parameters**:
- `buffer` - Start of buffer
- `end` - One past end of buffer
- `stride` - Element size

**Returns**: IndexArray view or NULL on failure

**Note**: IndexArray does not own buffer - caller manages lifetime.

---

#### `IndexArray.is_empty_slot`
```c
bool IndexArray.is_empty_slot(indexarray ia, usize index);
```
Check if slot is empty (all zeros).

**Returns**: true if empty, false if occupied

---

#### `IndexArray.capacity`
```c
usize IndexArray.capacity(indexarray ia);
```
Get current capacity.

**Returns**: Capacity

---

#### `IndexArray.stride`
```c
usize IndexArray.stride(indexarray ia);
```
Get element size.

**Returns**: Stride in bytes

---

#### `IndexArray.clear`
```c
void IndexArray.clear(indexarray ia);
```
Zero all slots.

---

#### `IndexArray.create_iterator`
```c
sparse_iterator IndexArray.create_iterator(indexarray ia);
```
Create sparse iterator.

**Returns**: Iterator or NULL on failure

---

## Map

**Header**: `<sigma.collections/map.h>`

String-keyed hash map with FNV-1a hashing and automatic resizing.

### Functions

#### `Map.new`
```c
map Map.new(usize capacity);
```
Create a new map with specified capacity.

**Parameters**:
- `capacity` - Initial number of buckets (rounded up to power of 2)

**Returns**: New map or NULL on failure

**Example**:
```c
map m = Map.new(16);
```

---

#### `Map.init`
```c
void Map.init(map *m, usize capacity);
```
Initialize a map in-place.

**Parameters**:
- `m` - Pointer to map variable
- `capacity` - Initial capacity

---

#### `Map.dispose`
```c
void Map.dispose(map m);
```
Dispose of map and free resources. Does not free keys or values (caller-owned).

**Parameters**:
- `m` - Map to dispose

---

#### `Map.set`
```c
int Map.set(map m, const char *key, usize key_len, usize value);
```
Set key-value pair. If key exists, updates value. Auto-resizes at 50% load.

**Parameters**:
- `m` - Map to modify
- `key` - Pointer to key bytes (caller retains ownership)
- `key_len` - Length of key in bytes
- `value` - Value to store (usize, typically pointer or integer)

**Returns**: 0 on success, -1 on error

**Example**:
```c
Map.set(m, "name", 4, (usize)name_ptr);
Map.set(m, "id", 2, 12345);
```

---

#### `Map.get`
```c
bool Map.get(map m, const char *key, usize key_len, usize *out_value);
```
Retrieve value by key.

**Parameters**:
- `m` - Map to query
- `key` - Key bytes to search for
- `key_len` - Length of key
- `out_value` - Pointer to store retrieved value

**Returns**: true if found, false if not found

**Example**:
```c
usize value;
if (Map.get(m, "name", 4, &value)) {
    char *name = (char *)value;
}
```

---

#### `Map.has`
```c
bool Map.has(map m, const char *key, usize key_len);
```
Check if key exists in map.

**Parameters**:
- `m` - Map to query
- `key` - Key to search for
- `key_len` - Length of key

**Returns**: true if key exists, false otherwise

---

#### `Map.remove`
```c
bool Map.remove(map m, const char *key, usize key_len);
```
Remove key-value pair from map.

**Parameters**:
- `m` - Map to modify
- `key` - Key to remove
- `key_len` - Length of key

**Returns**: true if removed, false if not found

---

#### `Map.count`
```c
usize Map.count(map m);
```
Get number of entries in map.

**Returns**: Entry count or 0 on error

---

#### `Map.capacity`
```c
usize Map.capacity(map m);
```
Get bucket capacity.

**Returns**: Capacity or 0 on error

---

#### `Map.create_iterator`
```c
sparse_iterator Map.create_iterator(map m);
```
Create sparse iterator for traversing map entries.

**Returns**: Sparse iterator or NULL on error

**Example**:
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

#### `Map.as_queryable`
```c
sc_queryable Map.as_queryable(map m);
```
Produce a heapless queryable over the map's entries (key + key_len + value together), for use with [`Query.next`/`Query.first`](#query). Skips empty and tombstone slots automatically. Yields `const map_entry *` per element.

**Parameters**:
- `m` - Map to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not `set`/`remove` on `m` while the returned queryable is in use — same rule as `create_iterator` above, and see [Query](#query)'s invariant note.

**Example** — something `Map.get` can't do, finding an entry *by value*:
```c
bool value_is_3(const void *element, usize index, void *userdata) {
    return ((const map_entry *)element)->value == 3;
}
Query.first(Map.as_queryable(m), value_is_3, NULL, NULL);
```

---

#### `Map.keys`
```c
sc_queryable Map.keys(map m);
```
Produce a heapless queryable over just the map's keys. Skips empty and tombstone slots automatically. Yields `const sc_key_view *` per element — keys are caller-owned byte spans, not guaranteed NUL-terminated, so this yields a `{ptr, len}` view rather than a bare `const char *`.

**Parameters**:
- `m` - Map to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not `set`/`remove` on `m` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

#### `Map.values`
```c
sc_queryable Map.values(map m);
```
Produce a heapless queryable over just the map's values. Skips empty and tombstone slots automatically. Yields `const usize *` per element.

**Parameters**:
- `m` - Map to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not `set`/`remove` on `m` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

## Iterator

**Header**: `<sigma.collections/collections.h>`

Standard sequential iterator for dense collections.

### Functions

#### `Iterator.next`
```c
bool Iterator.next(iterator it);
```
Advance to next element.

**Returns**: true if element exists, false if end reached

---

#### `Iterator.current`
```c
object Iterator.current(iterator it);
```
Get current element.

**Returns**: Pointer to current element or NULL

---

#### `Iterator.reset`
```c
void Iterator.reset(iterator it);
```
Reset to beginning.

---

#### `Iterator.dispose`
```c
void Iterator.dispose(iterator it);
```
Dispose of iterator.

---

## SparseIterator

**Header**: `<sigma.collections/collections.h>`

Unified iterator for sparse collections (SlotArray, IndexArray).

### Functions

#### `SparseIterator.next`
```c
bool SparseIterator.next(sparse_iterator it);
```
Advance to next occupied slot.

**Returns**: true if occupied slot found, false if no more

---

#### `SparseIterator.current_index`
```c
usize SparseIterator.current_index(sparse_iterator it);
```
Get current slot index.

**Returns**: Current index

---

#### `SparseIterator.current_value`
```c
int SparseIterator.current_value(sparse_iterator it, object *out_value);
```
Get current value and advance.

**Parameters**:
- `it` - Iterator
- `out_value` - Pointer to store value

**Returns**: 0 on success, -1 on error

**Note**: Advances iterator position after retrieval.

---

#### `SparseIterator.reset`
```c
void SparseIterator.reset(sparse_iterator it);
```
Reset to beginning.

---

#### `SparseIterator.dispose`
```c
void SparseIterator.dispose(sparse_iterator it);
```
Dispose of iterator.

---

## Query

**Header**: `<sigma.collections/query.h>`

A uniform, **heapless** way to scan any collection type in this library — FArray, PArray, Collection, List, SlotArray, Map — via a small POD cursor (`sc_queryable`) each type produces through its own `as_queryable` (or, for Map, `keys`/`values`). `Query.next` pulls one element at a time, caller-driven; `Query.first` is a convenience that stops at the first element satisfying a predicate. Nothing in `Query` allocates — an `sc_queryable` is a plain struct you hold by value, typically on the stack, wired to a source's existing storage.

Compare this to [`Iterator`](#iterator)/[`SparseIterator`](#sparseiterator): those require allocating (and later disposing) a stateful iterator object, which is the right cost for a genuine multi-step iteration session you hold onto (`reset`, `current`, explicit lifetime). `Query` is for the cheaper, more common case — "find the first thing matching X" or "walk this once and sum it up" — where paying for a heap allocation just to ask one question is wasted work, especially inside a hot loop.

### Types

```c
typedef struct sc_queryable {
    void *source;                 // The underlying handle (farray/parray/collection/map/slotarray)
    usize element_size;           // Meaningful for dense sources; ignored by sparse ones
    usize bound;                  // Upper bound for index — logical length for dense sources,
                                   // raw slot count for sparse sources
    usize index;                  // Current scan position — mutated by advance()
    sc_query_advance_fn advance;  // The source-kind-specific stepping behavior
} sc_queryable;
```
A heapless, POD cursor over some collection's elements. **Never constructed by hand** — always produced by a collection type's own `as_queryable` (or `keys`/`values`, for Map). Safe to pass by value.

```c
typedef struct sc_key_view {
    const char *ptr;
    usize len;
} sc_key_view;
```
A projected view of a Map key — keys are caller-owned byte spans, not guaranteed NUL-terminated, so `Map.keys` yields this instead of a bare pointer.

```c
typedef bool (*sc_predicate_fn)(const void *element, usize index, void *userdata);
```
Return `true` to stop a scan at this element (it matched); `false` to keep scanning. Declared in `collection.h`, used by `Query.first`.

### ⚠️ INVARIANT — do not mutate a source while it is in a queryable state

`sc_queryable` is a **live view, not a snapshot** — it holds a raw pointer/index position into the source's own storage, taken at `as_queryable`/`keys`/`values` time. Any operation that can change that storage's shape (add/append/insert/remove/set/clear/resize/grow, on the same farray/parray/collection/list/map/slotarray) once a queryable over it exists and is still being pulled from is **undefined**: a dense source's buffer may have been reallocated out from under the queryable's saved pointer; a sparse source's slot layout may have shifted under its saved index.

Finish pulling from a queryable (or simply stop using it) before mutating its source; construct a fresh one afterward to scan again. This is the same rule `Iterator`/`SparseIterator` already require — `Query` does not relax it, and currently has no runtime check that enforces it.

### Functions

#### `Query.next`
```c
bool Query.next(sc_queryable *q, const void **out_element, usize *out_index);
```
Pull the next element from a queryable. No predicate, no allocation, no early-break imposed — "roll your own query loop."

**Parameters**:
- `q` - The queryable to advance (mutated in place)
- `out_element` - Receives a pointer to the yielded element, or undefined if this call returns false
- `out_index` - Optional (may be `NULL`); receives the yielded element's index

**Returns**: `true` if an element was yielded; `false` if exhausted

**Example**:
```c
sc_queryable q = FArray.as_queryable(arr, sizeof(int));
const void *elem;
long sum = 0;
while (Query.next(&q, &elem, NULL)) {
    sum += *(const int *)elem;
}
```

---

#### `Query.first`
```c
bool Query.first(sc_queryable q, sc_predicate_fn pred, void *userdata, usize *out_index);
```
A one-call specialization of `next`: loop internally, stop at the first element `pred` returns true for. Covers "find first" (real `out_index`) and "any" (caller passes `NULL` for `out_index`) with one function.

**Parameters**:
- `q` - The queryable to scan, **passed by value** — this call gets its own cursor, starting fresh from `q`'s current position, leaving the caller's original queryable untouched
- `pred` - The predicate to test each element against
- `userdata` - Passed through to every `pred` call
- `out_index` - Optional (may be `NULL`); receives the matching element's index

**Returns**: `true` if a match was found; `false` if the source was exhausted with no match, or `pred` is `NULL`

**Example** — "any" usage, ignoring the index:
```c
bool is_negative(const void *element, usize index, void *userdata) {
    return *(const int *)element < 0;
}
if (Query.first(FArray.as_queryable(arr, sizeof(int)), is_negative, NULL, NULL)) {
    printf("contains at least one negative value\n");
}
```

---

## Collections

**Header**: `<sigma.collections/collections.h>`

Collection utilities and iterators.

### Functions

#### `Collections.create_iterator`
```c
iterator Collections.create_iterator(collection coll);
```
Create standard iterator for collection.

**Returns**: Iterator or NULL on failure

---

#### `Collections.create_with_allocator`
```c
collection Collections.create_with_allocator(usize capacity, usize stride, sc_alloc_use_t *use);
```
Create a new collection bound to a caller-supplied allocator-use. The instance grows/orphans through `use` instead of the global Allocator/Application facade. This is the only custom-allocator mechanism in the library — every allocation was previously hard-coded to the global `Allocator` facade with no override available at all; there is no module-wide equivalent.

**Parameters**:
- `capacity` - Initial collection capacity
- `stride` - Size of each element in the collection
- `use` - Allocator-use to bind this instance to, or `NULL` to behave exactly like the global-Allocator-backed path

**Returns**: New collection instance, or NULL on failure

**Notes**:
- `NULL` is a plain pass-through — every existing caller is unaffected.
- When bound, growth allocates through `use->alloc` and **orphans** (does not free) the previous buffer — a bump/arena allocator has no way to free one specific prior allocation, only bulk-release the whole arena.
- `Collections.dispose` on a bound instance likewise skips freeing the buffer; the bound allocator owns reclaiming it.
- `List.new_with_allocator` and `Stack.new_with_allocator` are thin wrappers over this.

---

#### `Collections.as_queryable`
```c
sc_queryable Collections.as_queryable(collection coll);
```
Produce a heapless queryable over an existing collection's elements, for use with [`Query.next`/`Query.first`](#query).

**Parameters**:
- `coll` - Collection to query

**Returns**: An `sc_queryable` ready to scan

**Warning**: Do not mutate `coll` while the returned queryable is in use — see [Query](#query)'s invariant note.

---

#### `Collections.count`
```c
usize Collections.count(collection coll);
```
Get number of elements.

**Returns**: Element count

---

#### `Collections.clear`
```c
void Collections.clear(collection coll);
```
Clear collection.

---

#### `Collections.dispose`
```c
void Collections.dispose(collection coll);
```
Dispose of collection.

---

## Type Definitions

```c
typedef void *object;              // Generic object pointer
typedef uintptr_t addr;            // Address type
typedef size_t usize;              // Unsigned size
typedef uint8_t byte;              // Byte type

#define OK 0                       // Success
#define ERR -1                     // Error
#define ADDR_EMPTY ((addr)0)       // Empty address marker
```
