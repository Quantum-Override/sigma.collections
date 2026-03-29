# Sigma Collections - API Reference

Complete function reference for sigma.collections library.

## Table of Contents

- [FArray](#farray)
- [PArray](#parray)
- [List](#list)
- [SlotArray](#slotarray)
- [IndexArray](#indexarray)
- [Map](#map)
- [Iterator](#iterator)
- [SparseIterator](#sparseiterator)
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

#### `FArray.alloc_use`
```c
void FArray.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**:
- Sets module-level allocator affecting all collection types
- See [Custom Allocation](#custom-allocation) section for details
- Pass NULL to restore default malloc/free behavior

**Example**:
```c
Collections.alloc_use(sigtest_alloc_use());  // Route to test framework
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

#### `PArray.alloc_use`
```c
void PArray.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**: Sets module-level allocator shared by all collection types.

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

#### `List.alloc_use`
```c
void List.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**: Sets module-level allocator shared by all collection types.

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

#### `SlotArray.alloc_use`
```c
void SlotArray.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**: Sets module-level allocator shared by all collection types.

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

#### `IndexArray.alloc_use`
```c
void IndexArray.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**: Sets module-level allocator shared by all collection types.

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

#### `Map.alloc_use`
```c
void Map.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**: Sets module-level allocator shared by all collection types.

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

#### `Collections.alloc_use`
```c
void Collections.alloc_use(sc_alloc_use_t *use);
```
Set custom allocator for all collections module-wide.

**Parameters**:
- `use` - Pointer to sc_alloc_use_t or NULL to restore malloc/free

**Notes**:
- Sets module-level allocator affecting all collection types
- Pass NULL to restore default malloc/free behavior
- Typically called once at application startup or test setup
- Not thread-safe - set before spawning threads

**Example**:
```c
// For testing with leak detection
Collections.alloc_use(sigtest_alloc_use());

// For custom memory pool
Collections.alloc_use(&my_allocator);

// Restore default malloc/free
Collections.alloc_use(NULL);
```

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
