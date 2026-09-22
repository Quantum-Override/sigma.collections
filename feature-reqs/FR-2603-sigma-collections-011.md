# FR-2603-sigma-collections-011: Stack — LIFO collection composed over Collection

**ID:** FR-2603-sigma-collections-011
**Type:** Feature Request
**Owner:** sigma.collections
**Filed:** 2026-09-22
**Status:** open
**Priority:** Medium
**Tags:** new-type, anvil-parity

---

## Purpose

Add a Stack type: a LIFO collection composed over `collection`, the same relationship `List`
already has to it. At zero new growth logic — push/pop/peek/clear are thin wrappers over
`collection_add`/`collection_get_length`/`collection_set_length` — plus arena-aware growth for
free via FR-2603-sigma-collections-007's allocator override, and a `mark`/`restore` pair for
O(1) truncation back to a saved depth.

---

## Context

Anvil needed a stack (its parser/resolver use one for nested-scope bookkeeping) and, having
already vendored `src/sigma/` with Collection available, built it as a ~150-line thin wrapper
rather than pulling in a separate dependency. This FR ports that design back as the canonical
spec. See `anvil/src/sigma/stack.c`, `anvil/include/sigma/stack.h`.

`mark`/`restore` (save the current depth, later truncate back to it, discarding everything
pushed since) is Stack's one piece of behavior beyond the obvious push/pop/peek — useful for
save-point/rollback patterns (e.g. speculative parsing) without paying for real undo storage.

---

## Scope

**In scope:**
- `struct sc_stack { collection coll; }` — same shape as `struct sc_list`.
- `Stack.new(capacity)`, `Stack.new_with_allocator(capacity, use)` (depends on
  FR-2603-sigma-collections-007), `Stack.dispose`.
- `Stack.push`, `Stack.pop`, `Stack.peek`, `Stack.is_empty`, `Stack.count`, `Stack.clear`.
- `Stack.mark` / `Stack.restore` — O(1) truncation via `collection_set_length`, no zeroing of
  the discarded region.
- `package/include/stack.h` mirroring `include/stack.h` per the existing pattern.

**Out of scope:**
- `as_queryable` — not requested by Anvil's Stack usage and not part of this FR; can be added
  trivially later (delegate to `Collections.as_queryable(s->coll)`, same as List) if a need
  arises.
- Iterator/SparseIterator support — a stack is walked top-down via pop, not iterated in place;
  no iterator was built for it in the source implementation either.

---

## Requirements

### R1: `sc_stack_i` interface (stack.h)

```c
typedef struct sc_stack_i {
    stack (*new)(usize capacity);
    stack (*new_with_allocator)(usize capacity, sc_alloc_use_t *use);
    void (*dispose)(stack s);
    int (*push)(stack s, object value);
    object (*pop)(stack s);
    object (*peek)(stack s);
    bool (*is_empty)(stack s);
    usize (*count)(stack s);
    void (*clear)(stack s);
    usize (*mark)(stack s);
    void (*restore)(stack s, usize mark);
} sc_stack_i;
extern const sc_stack_i Stack;
```

### R2: Implementation (stack.c)

- `stack_new`: allocates `struct sc_stack`, backs it with `collection_new(capacity,
  sizeof(object))` (pointer-sized elements — Stack stores `object` values, reference
  semantics, same as Collection's default pointer-bucket mode).
- `stack_new_with_allocator`: `use == NULL` → `stack_new`; else backs the same struct with
  `collection_new_with_allocator(capacity, sizeof(object), use)`.
- `stack_push`: `collection_add(s->coll, value)`.
- `stack_pop`/`stack_peek`: read the last `collection`-buffer slot directly (memcpy out),
  `pop` additionally decrements length via `collection_set_length`; both return `NULL` on an
  empty stack.
- `stack_is_empty`/`stack_count`/`stack_clear`: thin delegations to the matching `collection_*`
  accessor.
- `stack_mark`: returns `collection_get_length(s->coll)` (the current depth).
- `stack_restore`: `collection_set_length(s->coll, mark)` — a `mark` greater than the current
  depth is a no-op (never grows the stack back).

---

## Test Cases

Ported from Anvil's `test/sigma/test_stack.c` (ST01–ST09):

- **ST01** `Stack.new` creates a valid, empty stack (`is_empty` true, `count` 0).
- **ST02** push/pop is LIFO order (push a,b,c → pop returns c, b, a).
- **ST03** `peek` returns the top without removing it or changing count; a subsequent `pop`
  still returns the same value.
- **ST04** `pop`/`peek` on an empty stack both return NULL.
- **ST05** `clear` resets to empty (is_empty true, count 0).
- **ST06** `mark`/`restore` is a real truncate, not a cosmetic count: mark at depth 2, push a
  3rd, restore to the mark → count back to 2, top is the pre-mark value; pushing again reuses
  the slot and the original mark value is unaffected.
- **ST07** `new_with_allocator(NULL)` behaves exactly like `Stack.new`.
- **ST08** `new_with_allocator(&use)` grows through the override (alloc call count increases
  on growth) and never releases the orphaned buffer (`release_calls == 0`).
- **ST09** `dispose` with an override bound never calls `release` on the buffer.

(ST08/09 reuse the `fake_arena.h` test double introduced by FR-2603-sigma-collections-007.)

---

## Acceptance Criteria

- [ ] `include/stack.h` + `src/stack.c` exist and compile cleanly.
- [ ] `package/include/stack.h` mirrors `include/stack.h`.
- [ ] `test/unit/test_stack.c` (ST01–09) all GREEN.
- [ ] No regression in any existing test.

---

## Dependencies

**Blocked by:** FR-2603-sigma-collections-007 (allocator override — `new_with_allocator`
depends on `collection_new_with_allocator` existing).
**Parallel with:** FR-2603-sigma-collections-006 (Query) — independent; Stack does not gain
`as_queryable` under this FR (see Out of scope).
