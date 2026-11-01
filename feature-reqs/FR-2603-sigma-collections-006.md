# FR-2603-sigma-collections-006: Query — heapless scan mechanism (Query.next / Query.first)

**ID:** FR-2603-sigma-collections-006
**Type:** Feature Request
**Owner:** sigma.collections
**Filed:** 2026-09-22
**Status:** resolved
**Resolved:** 2026-09-22
**Priority:** Medium
**Tags:** query, iteration, anvil-parity

---

## Purpose

Add a uniform, allocation-free way to scan any collection type in this library — FArray,
PArray, Collection, List, Map, SlotArray — via a small POD cursor (`sc_queryable`) each type
produces via its own `as_queryable` (or, for Map, `keys`/`values`). `Query.next` pulls one
element at a time, caller-driven; `Query.first` is a convenience that stops at the first
element satisfying a predicate.

---

## Context

**Problem:** Every collection type in this library exposes its own bespoke way to find a
single matching element (or none): `Iterator`/`SparseIterator` require allocating a stateful
iterator object just to do a linear scan, and there is no shared "find first match" helper —
each caller re-derives the same manual loop-and-break pattern per type.

**Origin:** This design was worked out and implemented independently in Anvil's vendored
`src/sigma/` copy of these collections (`anvil/src/sigma/query.c`,
`anvil/include/sigma/query.h`) before being filed back here. This FR formalizes that design
as the canonical spec for sigma.collections and brings the two implementations back into
parity. See also FR-2603-sigma-collections-001 (the `alloc_use` seam) — Query has no
allocator dependency of its own; it never allocates.

**Solution:** A `sc_queryable` cursor — a raw pointer/index position into the source's own
storage, captured at `as_queryable` time — plus a `sc_query_advance_fn` stepping function
implemented once per storage *kind* (dense array-backed vs. sparse hash-backed), not once per
caller. `Query.next` pulls one element; `Query.first` loops `Query.next` internally against a
predicate and stops at the first match (or exhaustion).

---

## Scope

**In scope:**
- `sc_queryable` struct: `source`, `element_size`, `bound`, `index`, `advance`.
- `sc_predicate_fn` typedef (added to `collection.h`, since it's part of the public
  `collection`-adjacent surface, not query-specific).
- `sc_query_i` interface: `Query.next`, `Query.first`.
- One shared dense-source advance function, `array_base_query_advance`, in
  `internal/array_base.h`/`array_base.c`, reused by every array-backed type (FArray, PArray,
  Collection, and List via its own already-owned collection).
- Per-type `as_queryable()` entry points — tracked as their own scope items under this FR's
  sibling work (see FR-2603-sigma-collections-007 for the allocator-override work landing
  alongside; the per-type `as_queryable` wiring for FArray/PArray/SlotArray/Collection/List/Map
  lands as part of this FR's implementation, one commit per type, TDD).

**Out of scope:**
- Any allocator/lifetime changes (that's FR-2603-sigma-collections-007).
- The new Stack type (FR-2603-sigma-collections-011).
- The planned Set type / shared hash-core refactor (Anvil's `src/sigma/ROADMAP.md` — tracked
  informally, not yet filed as its own FR).

---

## Requirements

### R1: `sc_queryable` + `sc_query_i` (query.h / query.c)

```c
typedef bool (*sc_query_advance_fn)(struct sc_queryable *self, const void **out_element,
                                    usize *out_index);

typedef struct sc_queryable {
    void *source;
    usize element_size;
    usize bound;
    usize index;
    sc_query_advance_fn advance;
} sc_queryable;

typedef struct sc_query_i {
    bool (*next)(sc_queryable *q, const void **out_element, usize *out_index);
    bool (*first)(sc_queryable q, sc_predicate_fn pred, void *userdata, usize *out_index);
} sc_query_i;
extern const sc_query_i Query;
```

`Query.next`: advances by exactly one (matching, for sparse sources) element; returns `false`
on a NULL queryable, a NULL `advance`, or exhaustion.

`Query.first`: takes `q` **by value** (its own cursor, starting fresh from `q`'s current
position) so the caller's original queryable is left untouched; loops `next` internally,
stops at the first element `pred` returns `true` for; returns `false` if `pred` is NULL or no
element matches.

### R2: `sc_predicate_fn` (collection.h)

```c
typedef bool (*sc_predicate_fn)(const void *element, usize index, void *userdata);
```

Return `true` to stop a scan at this element (it matched); `false` to keep scanning.

### R3: Shared dense-source advance (`internal/array_base.h` / `array_base.c`)

```c
bool array_base_query_advance(sc_queryable *self, const void **out_element, usize *out_index);
```

Casts `self->source` to `sc_array_base *`, walks it at `self->element_size`, in order, up to
`self->bound`. Reused verbatim by FArray, PArray, and Collection's `as_queryable`.

### R4: Per-type `as_queryable` wiring

| Type | `as_queryable` bound / advance |
|---|---|
| FArray | `bound = capacity`, `advance = array_base_query_advance` |
| PArray | `bound = capacity`, `advance = array_base_query_advance` |
| Collection | `bound = length`, `advance = array_base_query_advance` |
| List | delegates to `Collections.as_queryable(lst->coll)` — List owns no storage of its own |
| SlotArray | own advance fn: walks the underlying parray, skipping `ADDR_EMPTY` slots |
| Map | three variants — `as_queryable` (yields `const map_entry *`), `keys` (yields `const sc_key_view *`), `values` (yields `const usize *`) — each skips empty/tombstone slots |

**`sc_key_view`** (new, `query.h`): `{ const char *ptr; usize len; }` — Map keys are
caller-owned byte spans, not guaranteed NUL-terminated, so `Map.keys` yields this instead of a
bare pointer.

**Map value type note:** Anvil's fork renamed Map's value slot from `usize` to `addr`
(`uintptr_t` — same width, different semantic name). This FR does **not** carry that rename
forward; sigma.collections' Map keeps `usize` values, and `Map.values` yields `const usize *`.
Revisit only if a future consumer needs pointer-typed values.

### R5: The INVARIANT (document in query.h's file header)

`sc_queryable` is a live view, not a snapshot. Any operation that changes a source's storage
shape (add/append/insert/remove/set/clear/resize/grow) while a queryable over it is still
being pulled from is undefined. This is the same rule `Iterator`/`SparseIterator` already
require; Query does not relax it and has no runtime check enforcing it.

---

## Test Cases

Ported from Anvil's `test/sigma/test_query.c` (QRY01–QRY08) plus one new test case per type
added to that type's existing `test/unit/test_*.c` file:

- **QRY01–QRY08** (`test/unit/test_query.c`, new file): mechanism correctness in isolation,
  via a hand-rolled `int[]` source — next visits in order then exhausts; first finds a match
  and reports index; first returns false on no match; first "any" usage (NULL out_index);
  first stops at the first match (predicate call count asserted); empty source; NULL-argument
  invariants; predicate that never matches exhausts cleanly.
- **FArray/PArray/Collection/List**: `as_queryable` + `Query.first` finds a value and reports
  its index; `as_queryable` + `Query.next` visits every element in order; (FArray only)
  no-match exhausts without finding anything.
- **SlotArray**: `as_queryable` skips empty (removed) slots; `as_queryable` + `Query.first`
  finds a match among occupied slots.
- **Map**: `as_queryable` yields entries and skips tombstones; `as_queryable` + `Query.first`
  finds an entry by value (something `Map.get` can't do — it only looks up by key); `keys`
  yields every live key and skips tombstones; `values` yields every live value and skips
  tombstones.

---

## Acceptance Criteria

- [ ] `include/query.h` + `src/query.c` exist; `Query.next`/`Query.first` match R1.
- [ ] `collection.h` gains `sc_predicate_fn`.
- [ ] `internal/array_base.h`/`array_base.c` gain `array_base_query_advance`.
- [ ] `as_queryable()` added to FArray, PArray, Collection, List, SlotArray; `as_queryable()` +
      `keys()` + `values()` added to Map.
- [ ] `package/include/query.h` mirrors `include/query.h` (matching the existing pattern for
      every other header in `package/include/`).
- [ ] `test/unit/test_query.c` (QRY01–08) plus the per-type additions above all GREEN.
- [ ] No regression in any existing test.

---

## Dependencies

**Blocks:** None directly, but FR-2603-sigma-collections-011 (Stack) composes over Collection
and inherits `as_queryable` for free once Collection has it.
**Blocked by:** None.
**Parallel with:** FR-2603-sigma-collections-007 (allocator override) — independent axes of
the same Anvil-parity effort, landing on the same branch.

---

## Notes

**Why not extend `Iterator`/`SparseIterator` instead of adding a new mechanism?** Those
interfaces require an allocated, disposable iterator object even for a single linear scan —
`Query` is heapless by design, and unifies dense and sparse sources behind one calling
convention (`Query.next`/`Query.first`) instead of two.

**FR numbering gap (008–010):** Anvil's headers reference FR-2603-sigma-collections-007 and
-011 by number but nothing in between; whatever 008–010 were (if anything) was never filed
here and isn't reconstructed by this FR.

---

## Resolution

**Resolved:** 2026-09-22
**Commits:** 68a096e (Query mechanism), e450a34 (FArray), 6487ec3 (PArray), bcf16da
(Collection), 36c2c85 (List), d41c3cb (SlotArray), ded8cf5 (Map keys/values) — all on
`feature/FR-2603-sigma-collections-006-anvil-parity`.

All R1–R5 requirements delivered exactly as specced, ported from Anvil's `src/sigma/query.h`
+`.c` and each type's `as_queryable`/`keys`/`values` with no design deviation. Built strict
TDD throughout: each type's test cases were written first against the not-yet-existing method
and confirmed RED before implementing — most RED states surfaced as compile errors, but
List/SlotArray/Map's confirmed RED via a null-function-pointer crash instead, since C
zero-fills an unset field in a designated-initializer struct literal (`const sc_list_i List =
{...}` compiles fine with `.as_queryable` simply absent/NULL) rather than refusing to compile.

One deliberate deviation from the FR text: Map's value type stays `usize` (not Anvil's `addr`
rename) — flagged in R4 as a known, intentional gap, not an oversight.

Two pre-existing, unrelated bugs were found and fixed along the way because they blocked
compiling/running the files this FR needed to touch: a stray trailing `}` in both
`test_slotarray.c` and `test_parray.c` (broke compilation entirely), and — found but
deliberately **not** fixed, since it's out of scope — a `free()` call on `Allocator`-owned
memory in `test_slotarray_from_value_array` (crashes the whole test binary; already broken on
the last real run recorded in `logs/test_slotarray.log` before this FR touched the file).
Recommend filing a BR for that one.

Full acceptance criteria: all GREEN, verified by direct compile+link+run against the prebuilt
sibling `sigma.core`/`sigma.memory`/`sigma.test` package objects (this sandbox has no
`/usr/local/packages` for `rtest` to use, so verification bypassed `rtest` and drove `gcc`
directly with an include-path shim reproducing the same package layout). No regressions in
any pre-existing test.
