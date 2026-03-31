# FR-2603-sigma-collections-002: Map collection — string-keyed hash map with alloc_use support

**ID:** FR-2603-sigma-collections-002  
**Type:** Feature Request  
**Owner:** sigma.collections  
**Filed:** 2026-03-21  
**Status:** resolved  
**Resolved:** 2026-03-31  
**Tags:** sigma-collections, map, hash-map, allocator, alloc-use, anvil  

---

## Summary

Add a `Map` collection type: a string-keyed open-addressing hash map with a configurable load
factor, FNV-1a hashing, and `alloc_use` hookability. The immediate consumer is `anvil`, which
currently maintains four separate hand-rolled string→index lookup structures across its core
modules. A single battle-tested `Map` primitive replaces all of them.

---

## Background

### The problem in `anvil`

Four distinct string-keyed lookup structures exist in `anvil/src/`:

| Site | Implementation | Characteristic |
|------|---------------|----------------|
| `resolver.c` — `anvl_id_map_t` | FNV-1a, open-addressing, linear probe, load ≤ 0.5, pre-sized to statement count | ~90 lines of boilerplate with no reuse |
| `import.c` — `str_map_find` | Linear `strcmp` scan | Adequate today; import graphs are small |
| `schema.c` — `schema_get_type` | Linear `strcmp` scan | Validation hot path if schemas grow |
| `schema.c` — `find_own_field` | Linear `memcmp` scan | Called per-field during validation |

A fifth lookup — `Context.get_field_by_name` — is being designed as part of the E3 query path
primitives (not yet implemented). A `Map` backing it from the start avoids a linear scan on
object field lookup.

### Why existing collection types don't cover this

`farray` and `parray` are index-keyed. `list` is ordered by insertion. None provide O(1) keyed
access by string. The `anvl_id_map_t` in resolver.c is already the correct algorithm — the
request is to promote it to a library primitive so the other three sites can also use it.

### Why `alloc_use` is a hard requirement

`anvil`'s parse context allocates from a bump arena (`ctx->arena`). Object field builds and
statement list lookups both occur during parsing, so any Map used there must route through the
same bump arena. With an `alloc_use` hook, `Map.init` is called with the bump's allocator
hook set — old backing arrays are abandoned in the bump on resize (they are reclaimed together
at arena teardown), matching the exact semantics of the existing manual grow-arrays.

For sites that live outside the arena (schema ruleset, import graph), the default
`malloc`/`free` path is used with no configuration needed.

---

## Requested Interface

```c
// map.h

struct sc_map;
typedef struct sc_map *map;

typedef struct sc_map_i {
    /**
     * @brief Create a new map with the given initial capacity.
     *        Capacity is rounded up to the next power of two.
     *        Load factor ceiling: 0.5 (same as anvil resolver convention).
     * @param capacity Initial bucket count hint
     */
    map  (*new)(usize capacity);

    /**
     * @brief Initialise an already-allocated map slot.
     * @param m     Pointer to map slot to initialise
     * @param capacity Initial bucket count hint
     */
    void (*init)(map *, usize capacity);

    /**
     * @brief Free all resources associated with the map.
     * @param m The map to dispose of
     */
    void (*dispose)(map);

    /**
     * @brief Set the allocator hook used for internal backing arrays.
     *        Must be called before the first insert if arena routing is required.
     * @param m   The map to configure
     * @param use Allocator hook (alloc / release / resize); NULL restores malloc/free
     */
    void (*alloc_use)(map, sc_alloc_use_t *);

    /**
     * @brief Insert or update a string-keyed entry.
     * @param m    The map
     * @param key  Key bytes (not required to be NUL-terminated)
     * @param len  Key length in bytes
     * @param val  Value to store (pointer-sized; caller interprets)
     * @return 0 on success; non-zero on allocation failure
     */
    int  (*set)(map, const char *key, usize len, usize val);

    /**
     * @brief Look up a string-keyed entry.
     * @param m       The map
     * @param key     Key bytes
     * @param len     Key length in bytes
     * @param out_val Receives the stored value on success
     * @return 1 if found (out_val set); 0 if not found
     */
    int  (*get)(map, const char *key, usize len, usize *out_val);

    /**
     * @brief Test whether a key is present without retrieving the value.
     * @return 1 if present; 0 if absent
     */
    int  (*has)(map, const char *key, usize len);

    /**
     * @brief Remove a key from the map.
     * @return 1 if key was present and removed; 0 if key was absent
     */
    int  (*remove)(map, const char *key, usize len);

    /**
     * @brief Return the number of entries currently stored.
     */
    usize (*count)(map);

    /**
     * @brief Return the current bucket capacity.
     */
    usize (*capacity)(map);

} sc_map_i;

extern const sc_map_i Map;
```

### Hashing

Use FNV-1a 64-bit (matching the algorithm already in `anvil/src/resolver/resolver.c`):

```c
#define FNV1A_OFFSET UINT64_C(14695981039346656037)
#define FNV1A_PRIME  UINT64_C(1099511628211)

static uint64_t fnv1a(const char *data, usize len) {
    uint64_t h = FNV1A_OFFSET;
    for (usize i = 0; i < len; i++) { h ^= (uint8_t)data[i]; h *= FNV1A_PRIME; }
    return h;
}
```

### Collision resolution

Open addressing with linear probing. This matches the resolver's existing design and is
cache-friendly for the small-to-medium maps Anvil uses in practice.

### Load factor

Trigger a resize (double capacity) when `count / capacity > 0.5`. This matches the resolver's
pre-sizing convention (`cap = 2 × n` where `n` = statement count).

### Resize semantics with bump allocator

When resizing under a bump `alloc_use` hook:
1. Allocate new backing arrays from the hook's `alloc`
2. Copy live entries (re-hash)
3. Call `release` on the old arrays — if the hook is a bump, `release` is a no-op; keys/values
   already live in the arena

This is the same pattern the existing `ci_ensure_*_capacity` functions use, and is correct.

---

## Intended Replacements in `anvil`

Once `Map` is available, the following will be replaced:

| File | Current | Replacement |
|------|---------|-------------|
| `src/resolver/resolver.c` | `anvl_id_map_t` + `fnv1a` + `id_map_*` (~90 lines) | `Map` with arena `alloc_use` |
| `src/import/import.c` | `str_map_find` linear scan | `Map` (default malloc; import graphs are small) |
| `src/schema/schema.c` | `schema_get_type` + `find_own_field` linear scans | `Map` per ruleset; keyed by type name |
| `src/core/context.c` (future E3) | not yet implemented | `Map` per object context for field-by-name |

---

## Priority and Dependencies

- **Depends on:** FR-2603-sigma-collections-001 (alloc_use seam). `Map` without `alloc_use`
  cannot be used in Anvil's parse context.
- **Priority:** High — blocks E3 query path primitives (`Context.get_field_by_name`) and will
  remove ~90 lines of hash table boilerplate from the resolver on adoption.
- **Blocking milestone:** Anvil v1.0.0-rel (E3 is a post-v1.0 enhancement, but Map is useful
  now; resolver cleanup can happen immediately after `alloc_use` lands).

---

## Resolution

**Resolved:** 2026-03-31  
**Commit:** Implemented in v0.2.0-rc2 (2026-03-21-25), included in v0.2.1 (2026-03-27)

### Implementation Summary

Map collection fully implemented with all requested features. Uses `Allocator.alloc/dispose` directly (not alloc_use hooks, as FR-001 was never implemented and later marked obsolete).

### Deliverables

**Source Implementation** ([src/map.c](../src/map.c)):
- FNV-1a 64-bit hashing with standard prime/offset constants
- Open addressing with linear probing collision resolution
- Load factor threshold 0.5 (auto-resize at 50% capacity)
- Automatic resize to next power of two
- Tombstone marking for deleted entries (prevents probe chain breakage)
- Entry structure: `{ uint64_t hash, char *key, usize key_len, usize value, bool occupied }`

**Public Interface** ([include/map.h](../include/map.h)):
```c
typedef struct sc_map_i {
    map  (*new)(usize capacity);
    void (*init)(map *, usize capacity);
    void (*dispose)(map);
    int  (*set)(map, const char *key, usize len, usize val);
    int  (*get)(map, const char *key, usize len, usize *out_val);
    int  (*has)(map, const char *key, usize len);
    int  (*remove)(map, const char *key, usize len);
    usize (*count)(map);
    usize (*capacity)(map);
    sparse_iterator (*create_iterator)(map);
} sc_map_i;
extern const sc_map_i Map;
```

**Sparse Iterator Support:**
- `Map.create_iterator()` → returns sparse_iterator over occupied entries only
- Iterator yields `map_entry *` with `{ char *key, usize key_len, usize value }`
- Compatible with `SparseIterator.next/current_value/dispose` interface

**Test Coverage** ([test/unit/test_map.c](../test/unit/test_map.c)):
- **17 comprehensive tests**, all passing:
  1. `map_new_dispose` - Lifecycle, capacity initialization
  2. `map_set_get_single` - Single entry set/get
  3. `map_set_multiple` - Multiple distinct keys
  4. `map_update_existing_key` - Value updates
  5. `map_get_nonexistent` - Missing key handling
  6. `map_has` - Presence checking
  7. `map_remove` - Entry removal
  8. `map_remove_nonexistent` - Remove missing key (no-op)
  9. `map_remove_and_reinsert` - Tombstone reuse
  10. `map_automatic_resize` - Load factor triggering
  11. `map_large_insertion` - 100+ entries, stress test
  12. `map_empty_key` - Zero-length keys
  13. `map_key_with_nulls` - Binary keys (embedded NULs)
  14. `map_collision_handling` - Linear probing verification
  15. `map_null_safety` - NULL pointer guards
  16. `map_arena_pattern` - Usage with bump allocators
  17. `map_iterate_keys_values` - Sparse iterator functionality

### Allocation Architecture

**Implementation Uses Allocator.alloc (Not alloc_use):**
- Map uses `Allocator.alloc()` / `Allocator.dispose()` from sigma.memory facade
- Allocator delegates through `Application.get_allocator()` (Phase 2 complete)
- Test frameworks inject custom allocators via `Application.set_allocator()` globally
- **No per-module alloc_use hooks** - Application-level allocator sufficient

**Dependency Note:**
Original FR requested dependency on FR-001 (alloc_use seam). FR-001 marked obsolete 2026-03-31. Map implementation uses superior Application allocator architecture from Phase 2.

**Arena/Frame Allocation:**
Custom controllers (reclaim, frame, etc.) designed into Application allocator. Map automatically benefits from Application-level allocator control without per-instance configuration.

### Acceptance Criteria

- [x] FNV-1a 64-bit hashing implemented ✓
- [x] Open addressing with linear probing ✓
- [x] Load factor 0.5, auto-resize to next power of 2 ✓
- [x] String keys with explicit length (binary-safe) ✓
- [x] Set/get/has/remove operations ✓
- [x] Count/capacity accessors ✓
- [x] Sparse iterator support (iterate occupied entries only) ✓
- [x] Comprehensive test coverage (17/17 tests passing) ✓
- [x] Included in v0.2.1 release ✓

### Usage Example

```c
#include <sigma.collections/map.h>

map m = Map.new(16);  // Initial capacity 16

// Insert entries
Map.set(m, "username", 8, (usize)user_ptr);
Map.set(m, "user_id", 7, 12345);

// Retrieve
usize value;
if (Map.get(m, "username", 8, &value)) {
    user_t *user = (user_t *)value;
}

// Check existence
if (Map.has(m, "user_id", 7)) {
    Map.remove(m, "user_id", 7);
}

// Iterate
sparse_iterator it = Map.create_iterator(m);
while (SparseIterator.next(it)) {
    map_entry *entry;
    SparseIterator.current_value(it, (object *)&entry);
    // Access entry->key, entry->key_len, entry->value
}
SparseIterator.dispose(it);
Map.dispose(m);
```

### Next Actions

- [x] Map implementation complete and tested
- [ ] Anvil integration: Replace `anvl_id_map_t` with `Map` in resolver.c
- [ ] Anvil integration: Replace linear scans in import.c and schema.c with `Map`
- [ ] E3 query path: Use `Map` for `Context.get_field_by_name` implementation

Map is production-ready and available in sigma.collections v0.2.1+.
