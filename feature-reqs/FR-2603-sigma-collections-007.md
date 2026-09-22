# FR-2603-sigma-collections-007: Per-instance allocator override (create_with_allocator / new_with_allocator)

**ID:** FR-2603-sigma-collections-007
**Type:** Feature Request
**Owner:** sigma.collections
**Filed:** 2026-09-22
**Status:** resolved
**Resolved:** 2026-09-22
**Priority:** Medium
**Tags:** allocator, arena, anvil-parity

---

## Purpose

Let a single Collection or List instance be bound, at creation time, to a caller-supplied
`sc_alloc_use_t *` — so its buffer is allocated/grown/orphaned through that allocator instead
of the global `Allocator`/`Application` facade — while every existing constructor and every
existing caller keeps behaving exactly as before.

---

## Context

**Problem:** FR-2603-sigma-collections-001 gave sigma.collections an `alloc_use` seam, but
it's a single **module-level static** (`s_coll_use`): `Collections.alloc_use(use)` swaps it
process-wide, for every instance, for the rest of the process's life (or until swapped again).
That's fine when a consumer only ever wants one allocator active at a time. It breaks down the
moment a consumer needs two collections alive *concurrently*, bound to two *different*
allocators — e.g. a parse-scoped bump arena that gets bulk-freed when a parse finishes, versus
a document-scoped allocator that outlives it. A single global can't represent that.

**Origin:** Anvil hit exactly this case (its parser needs arena-scoped collections whose
lifetime is tied to a parse call, coexisting with longer-lived document-scoped ones) and
solved it in its vendored `src/sigma/` fork by adding a *second*, per-instance binding
alongside the existing global one. This FR files that design back as the canonical spec and
brings sigma.collections into parity with it. See `anvil/src/sigma/collections.c`
(`collection_new_with_allocator`), `anvil/src/sigma/list.c` (`list_new_with_allocator`).

**Solution:** `struct sc_collection` gains an `alloc_use` field. `NULL` (every existing
caller, unchanged) means "use the global Allocator/Application facade, exactly as today."
Non-NULL means this instance's buffer is allocated via `use->alloc`, grown via `use->alloc`
again (copy-and-orphan, not realloc — a bump/arena allocator has no way to free one specific
prior allocation), and `collection_dispose` skips freeing the buffer entirely on this path —
the bound allocator's own bulk-release (e.g. `frame_end`) is what reclaims it.

---

## Scope

**In scope:**
- `struct sc_collection` gains `sc_alloc_use_t *alloc_use`.
- `Collections.create_with_allocator(capacity, stride, use)` — `collection_new_with_allocator`.
- `collection_grow` and `collection_dispose` both branch on `coll->alloc_use`.
- `List.new_with_allocator(capacity, elem_size, use)` — `list_new_with_allocator`, a thin
  delegation to `collection_new_with_allocator` (List's struct is just `{ collection coll; }`,
  no changes needed there beyond the new constructor).
- Test double: `fake_arena.h` (ported from Anvil's `test/sigma/fake_arena.h`) — a malloc-backed
  `sc_alloc_use_t` stand-in that counts `alloc`/`release` calls and can bulk-free everything it
  handed out, so tests can assert an arena-backed instance never calls `release` on a buffer
  it's supposed to orphan.

**Out of scope:**
- FArray, PArray, SlotArray, Map — none of these grow via a pointer-bucket that this pattern
  applies to in the same way (FArray/PArray are fixed-capacity past their initial allocation
  in the calling patterns that matter here; Map has its own resize/rehash logic and is
  intentionally deferred — see Anvil's `src/sigma/ROADMAP.md`, which plans a shared hash-core
  refactor before extending Map further).
- Stack — covered by FR-2603-sigma-collections-011, which gets this for free by composing over
  Collection (`Stack.new_with_allocator` is a two-line delegation once this FR lands).
- Any change to the *existing* global `alloc_use` seam from FR-001 — the two mechanisms
  coexist; this FR adds a second, narrower one, it doesn't replace the first.

---

## Requirements

### R1: `sc_collection` struct

```c
struct sc_collection {
    sc_array_base array;
    usize stride;
    usize length;
    bool owns_buffer;
    // NULL = global Allocator/Application facade (every existing caller, unchanged
    // behavior); non-NULL = this instance's buffer is allocated/grown/orphaned through
    // `alloc_use` instead, and collection_dispose skips freeing the buffer.
    sc_alloc_use_t *alloc_use;
};
```

`collection_new` sets `alloc_use = NULL` explicitly (previously-undefined field must not be
left uninitialized now that it exists).

### R2: `collection_new_with_allocator`

`use == NULL` → identical to `collection_new(capacity, stride)`. `use != NULL` → allocates the
struct via the global `Allocator` (the struct itself is small and short-lived; only the
*buffer* is arena-bound), allocates the bucket via `use->alloc(stride * capacity)`, and stores
`use` on the instance. Overflow-checks `stride * capacity` the same way `collection_new`'s
caller path already does before it.

### R3: `collection_grow`

```c
void *new_buffer = coll->alloc_use
    ? coll->alloc_use->alloc(coll->stride * new_capacity)
    : Allocator.alloc(coll->stride * new_capacity);
...
if (!coll->alloc_use) {
    Allocator.dispose(coll->array.bucket);
}
// alloc_use set: old buffer is orphaned, not disposed.
```

### R4: `collection_dispose`

```c
if (coll->owns_buffer && coll->array.bucket && !coll->alloc_use) {
    Allocator.dispose(coll->array.bucket);
}
// alloc_use set: buffer reclaim is the bound allocator's responsibility.
Allocator.dispose(coll);  // the struct itself is always Allocator-owned
```

### R5: `List.new_with_allocator`

```c
list list_new_with_allocator(usize capacity, usize elem_size, sc_alloc_use_t *use) {
    if (!use) return list_new(capacity, elem_size);
    struct sc_list *lst = Allocator.alloc(sizeof(struct sc_list));
    if (!lst) return NULL;
    lst->coll = collection_new_with_allocator(capacity, elem_size, use);
    if (!lst->coll) { Allocator.dispose(lst); return NULL; }
    return lst;
}
```

### R6: `fake_arena.h` test double

Malloc-backed `alloc`/`release` pair that: zero-initializes every block it hands out, remembers
every pointer so `fake_arena_reset()` can bulk-free them (keeping valgrind clean between
tests), and counts calls so a test can assert `release` was never called on an orphaned
buffer. `resize`/`frame_begin`/`frame_end` are NULL (unused by this FR's code paths).

---

## Test Cases

Ported from Anvil's `test/sigma/test_collections.c` (IC04–IC06) and `test/sigma/test_list.c`
(IL11–IL13):

- **create_with_allocator(NULL) / new_with_allocator(NULL)** behaves exactly like the plain
  constructor — add/append and count all work normally.
- **create_with_allocator(&use) / new_with_allocator(&use)**: initial buffer is allocated
  through the override (`alloc_calls >= 1`); pushing past capacity grows through the override
  (`alloc_calls` increases) and never calls `release` on the orphaned buffer
  (`release_calls == 0` throughout growth).
- **dispose** with an override bound never calls `release` on the buffer — the bound
  allocator owns bulk-reclaiming it.

---

## Acceptance Criteria

- [ ] `struct sc_collection` has `alloc_use`; `collection_new` initializes it to NULL.
- [ ] `Collections.create_with_allocator` added to `sc_collections_i` and implemented.
- [ ] `collection_grow`/`collection_dispose` branch correctly on `alloc_use` (grow: alloc
      through override, orphan old buffer; dispose: skip freeing buffer, still free the
      struct).
- [ ] `List.new_with_allocator` added to `sc_list_i` and implemented.
- [ ] `test/unit/fake_arena.h` exists; new/extended `test/unit/test_collections_allocator.c`
      and `test/unit/test_list.c` cases (ported IC04–06 / IL11–13 equivalents) all GREEN.
- [ ] No regression in any existing test.

---

## Dependencies

**Blocks:** FR-2603-sigma-collections-011 (Stack's `new_with_allocator` depends on this).
**Blocked by:** None.
**Parallel with:** FR-2603-sigma-collections-006 (Query) — independent axes of the same
Anvil-parity effort, landing on the same branch.

---

## Resolution

**Resolved:** 2026-09-22
**Commits:** bba5195 (Collections.create_with_allocator), 526c363 (List.new_with_allocator) —
both on `feature/FR-2603-sigma-collections-006-anvil-parity`.

All R1–R6 delivered as specced, ported from Anvil's `collection_new_with_allocator` /
`list_new_with_allocator` with no design deviation. TDD throughout: tests written first
against the not-yet-existing methods (confirmed RED), then implemented to GREEN. Verified via
direct compile+link+run against the prebuilt sibling package objects (see FR-006's Resolution
for why `rtest` itself isn't usable in this sandbox). No regressions in any pre-existing test.

FArray/PArray/SlotArray/Map were out of scope per this FR's own text and remain untouched.
