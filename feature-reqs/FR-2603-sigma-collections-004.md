# FR-2603-sigma-collections-004: Update Dispatch Helpers Post-alloc_use Removal

**ID:** FR-2603-sigma-collections-004  
**Type:** Feature Request  
**Owner:** sigma.collections  
**Filed:** 2026-03-27  
**Status:** open  
**Tags:** sigma-collections, allocator, refactor, phase-3a, orchestration, BR-2603-q-or-001  
**Depends on:** FR-2603-sigma-collections-003 (alloc_use pattern removal)  
**Blocks:** None (optimization/cleanup)  

---

## Summary

After FR-2603-sigma-collections-003 removes the `alloc_use` pattern, the dispatch helper functions (`coll_alloc`, `coll_free`, `coll_realloc`) become trivial wrappers around `Allocator.*` calls. This FR proposes updating those helpers to direct delegation (removing the branch overhead) **or** inlining them entirely.

**Decision Point:** Keep helpers for potential future instrumentation, or inline for simplicity?

---

## Background

### Current State (Pre-FR-003)

```c
static sc_alloc_use_t *s_coll_use = NULL;

static void *coll_alloc(usize size) {
    if (s_coll_use && s_coll_use->alloc) return s_coll_use->alloc(size);
    return malloc(size);
}

static void coll_free(void *ptr) {
    if (s_coll_use && s_coll_use->release) { s_coll_use->release(ptr); return; }
    free(ptr);
}

static void *coll_realloc(void *ptr, usize size) {
    if (s_coll_use && s_coll_use->resize) return s_coll_use->resize(ptr, size);
    return realloc(ptr, size);
}
```

Each allocation checks `s_coll_use` (branch + dereference overhead).

### After FR-003 (Branch Removed)

```c
static void *coll_alloc(usize size) {
    return Allocator.alloc(size);
}

static void coll_free(void *ptr) {
    Allocator.dispose(ptr);
}

static void *coll_realloc(void *ptr, usize size) {
    return Allocator.realloc(ptr, size);
}
```

Helpers are now **trivial wrappers** — single delegation to `Allocator` facade.

---

## Requested Changes

### Option A: Keep Simplified Helpers (Conservative)

**Rationale:**
- Provides a single point to add debugging/instrumentation later
- Example: `#ifdef DEBUG_COLL_ALLOC` logging without changing 100+ call sites
- Minimal cost (likely inlined by compiler anyway)

**Changes:**
- Update helpers to direct `Allocator.*` delegation (as shown above)
- Keep all call sites using `coll_alloc/coll_free/coll_realloc`
- Mark helpers as `static inline` to ensure zero overhead

**File:** `src/internal/arrays.h` (or wherever helpers are declared)
```c
static inline void *coll_alloc(usize size) {
    return Allocator.alloc(size);
}

static inline void coll_free(void *ptr) {
    Allocator.dispose(ptr);
}

static inline void *coll_realloc(void *ptr, usize size) {
    return Allocator.realloc(ptr, size);
}
```

### Option B: Inline Completely (Aggressive Simplification)

**Rationale:**
- Removes abstraction layer entirely (YAGNI principle)
- Matches sigma.core pattern after BR-003 (direct `Allocator.*` calls)
- Reduces cognitive overhead (fewer indirection levels)

**Changes:**
- Replace all `coll_alloc(size)` → `Allocator.alloc(size)` (100+ call sites)
- Replace all `coll_free(ptr)` → `Allocator.dispose(ptr)` (80+ call sites)
- Replace all `coll_realloc(ptr, size)` → `Allocator.realloc(ptr, size)` (20+ call sites)
- Delete helper functions entirely

**Example (FArray resize logic):**
```c
// BEFORE:
void *new_buf = coll_realloc(arr->data, new_cap * arr->elem_size);
if (!new_buf) return false;

// AFTER:
void *new_buf = Allocator.realloc(arr->data, new_cap * arr->elem_size);
if (!new_buf) return false;
```

**Tool-Assisted Refactor:**
```bash
# If chosen, use sed/perl/clang-refactor to batch replace
sed -i 's/coll_alloc(/Allocator.alloc(/g' src/*.c
sed -i 's/coll_free(/Allocator.dispose(/g' src/*.c
sed -i 's/coll_realloc(/Allocator.realloc(/g' src/*.c
```

---

## Recommendation: Option A (Keep Helpers)

**Reasoning:**
1. **Future instrumentation:** sigma.collections may want allocation telemetry (bytes allocated, collection type profiling) without touching every call site
2. **Compiler optimization:** `static inline` ensures zero runtime cost
3. **Consistency:** Other modules (sigma.memory controllers) use similar dispatch patterns
4. **Low risk:** Keeps diff contained to helper bodies (3 functions), not 100+ call sites

**If simplicity is paramount:** Option B is valid, but creates larger diff and loses flexibility.

---

## Test Cases

### TC-001: Helpers delegate to Allocator (Option A)
```c
// Verify coll_alloc calls Allocator.alloc
static int alloc_called = 0;
static void *test_alloc(usize size) { alloc_called++; return malloc(size); }

sc_alloc_use_t custom = {
    .ctrl = NULL,
    .alloc = test_alloc,
    .release = free,
    .resize = NULL,
};
Application.set_allocator(&custom);

void *p = coll_alloc(64);
assert(alloc_called == 1);
coll_free(p);
```

### TC-002: Direct calls work (Option B)
```c
// If helpers inlined, verify Allocator.alloc() called directly
// (Same test as TC-001, just directly using Allocator facade)
static int alloc_called = 0;
Application.set_allocator(&custom);

void *p = Allocator.alloc(64);
assert(alloc_called == 1);
Allocator.dispose(p);
```

### TC-003: No performance regression
```c
// Benchmark: FArray.append() 10,000 elements
// Compare before/after FR-003+004
// Expected: Identical performance (branch removed, inline overhead is zero)
```

### TC-004: Existing test suite passes
```c
// All 102 sigma.collections tests pass
// No behavioral changes
```

---

## Acceptance Criteria

### Option A
- [ ] `coll_alloc/coll_free/coll_realloc` marked `static inline`
- [ ] Helpers delegate directly to `Allocator.*` (no branches)
- [ ] All call sites unchanged (still use `coll_*` helpers)
- [ ] Test suite passes (102 tests, no regressions)

### Option B
- [ ] All `coll_alloc` → `Allocator.alloc` call sites updated
- [ ] All `coll_free` → `Allocator.dispose` call sites updated
- [ ] All `coll_realloc` → `Allocator.realloc` call sites updated
- [ ] Helper functions deleted entirely
- [ ] Test suite passes (102 tests, no regressions)

---

## Implementation Notes

**If Option A chosen:**
- Single-commit change (3 helper functions updated)
- Low risk, easy to review

**If Option B chosen:**
- Multi-file change (8-10 source files, 200+ lines changed)
- Use tool-assisted refactor (sed/clang-refactor) to ensure consistency
- Split into logical commits by collection type (FArray, PArray, List)

**Timeline:**
- Option A: 1 hour
- Option B: 2-3 hours (includes verification)

---

## Rationale

**Why not do this in FR-003?**
FR-003 focuses on **removing the alloc_use pattern** (hooks, vtable entries, setters). FR-004 is a **follow-up optimization** to simplify the dispatch layer. Splitting allows:
- FR-003 to be reviewed/tested independently (critical path)
- FR-004 to be deferred if resources constrained (nice-to-have)

**Why keep helpers at all?**
Provides flexibility for future instrumentation without touching 100+ call sites. Cost is zero (inline optimization).

**Why inline completely?**
Maximum simplicity — matches sigma.core pattern, removes unnecessary indirection.

---

## See Also

- **FR-2603-sigma-collections-003** — Remove alloc_use pattern (prerequisite)
- **BR-2603-sigma-core-003** — sigma.core facade removal (precedent for direct calls)
- **ORCHESTRATION-BR-2603-q-or-001.md** — Phase 3A specifications
