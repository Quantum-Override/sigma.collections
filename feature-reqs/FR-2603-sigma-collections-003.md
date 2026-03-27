# FR-2603-sigma-collections-003: Remove alloc_use Pattern (Simplification)

**ID:** FR-2603-sigma-collections-003  
**Type:** Feature Request  
**Owner:** sigma.collections  
**Filed:** 2026-03-27  
**Status:** open  
**Tags:** sigma-collections, allocator, alloc-use, phase-3a, orchestration, BR-2603-q-or-001  
**Depends on:** FR-2603-sigma-core-005 (sc_alloc_use_t.ctrl field), FR-2603-sigma-memory-002 (Application delegation)  

---

## Summary

Remove the `alloc_use` pattern from sigma.collections entirely. All internal allocations will use `Allocator.alloc()` / `Allocator.dispose()` directly, which delegates through `Application.get_allocator()`. This eliminates the per-module override layer and simplifies the allocation path.

**Decision:** sigma.collections.o **always uses `Allocator.alloc/dispose`** — no per-module allocator override needed. The Application-level allocator is sufficient for all use cases.

---

## Background

### Phase 1 Foundation Complete

FR-2603-sigma-core-004/005/006 established the Application allocator API and the new controller architecture with `sc_alloc_use_t.ctrl` field at offset 0. The `Allocator` facade now delegates through `Application.get_allocator()` (pending FR-2603-sigma-memory-002 implementation, currently uses direct slb0_* calls).

### alloc_use Pattern is Redundant

sigma.collections currently has:
- `static sc_alloc_use_t *s_coll_use` module-level global
- `coll_set_alloc_use(sc_alloc_use_t *use)` setter function
- `.alloc_use` vtable slots in all collection interfaces (`sc_farray_i`, `sc_parray_i`, `sc_list_i`, `sc_map_i`)
- Dispatch helpers (`coll_alloc`, `coll_free`, `coll_realloc`) that check `s_coll_use` before calling `Allocator.*`

This pattern was originally introduced to allow per-module allocator customization and to break circular dependencies with sigma.memory. However, the Phase 1 Application allocator architecture provides a superior solution:

**Application.set_allocator()** allows setting a global custom allocator that affects **all modules** — including sigma.collections, sigma.memory's internal controllers, sigma.core String/StringBuilder, etc.

**Per-module overrides are no longer needed** because:
1. The Application allocator is set once at initialization (before any modules allocate)
2. Test frameworks (sigma.test) can inject tracked allocators via `Application.set_allocator()` globally
3. Advanced use cases (arena allocation, frame scoping) are handled by custom controllers, not per-module hooks

### sigma.core Precedent: BR-2603-sigma-core-003

During Phase 1 integration, BR-2603-sigma-core-003 removed the equivalent `alloc_use` pattern from sigma.core's String/StringBuilder implementations:
- Removed: `s_string_use`, `s_sb_use`, `s_io_use` static hooks
- Removed: `tx_alloc/tx_free/tx_realloc` dispatch helpers
- Result: 29 call sites converted to direct `Allocator.*` calls
- **Phase 4 work completed early** due to segfault fixes (NULL static hooks incompatible with ctrl field architecture)

sigma.collections should follow the same simplification path.

---

## Requested Changes

### 1. Remove Module-Level Hook Infrastructure

**File:** `src/arrays.c` (and any other source files with the pattern)

**Remove:**
```c
static sc_alloc_use_t *s_coll_use = NULL;

void coll_set_alloc_use(sc_alloc_use_t *use) { 
    s_coll_use = use; 
}
```

### 2. Simplify Dispatch Helpers (or Inline Entirely)

**Option A — Simplify to Direct Calls:**
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

**Option B — Inline Completely (Recommended):**
Replace all `coll_alloc(size)` → `Allocator.alloc(size)` call sites directly.  
Replace all `coll_free(ptr)` → `Allocator.dispose(ptr)` call sites directly.  
Replace all `coll_realloc(ptr, size)` → `Allocator.realloc(ptr, size)` call sites directly.  

Delete the dispatch helper functions entirely.

### 3. Remove alloc_use Slots from Collection Interfaces

**Files:** 
- `include/farray.h` (sc_farray_i)
- `include/parray.h` (sc_parray_i)
- `include/list.h` (sc_list_i)
- `include/map.h` (sc_map_i, when implemented)

**Remove from all collection interface vtables:**
```c
void (*alloc_use)(sc_alloc_use_t *use);  // DELETE THIS
```

**Update all vtable initializations** to remove the `.alloc_use = coll_set_alloc_use` entry.

### 4. Update Headers

**Remove:**
- `#include <sigma.core/allocator.h>` (if only used for `sc_alloc_use_t`, still needed for `sc_ctrl_base_s`)
- Any forward declarations of `coll_set_alloc_use()`

**Ensure:**
- `#include <sigma.core/alloc.h>` is present (for `Allocator` facade)

### 5. Update Documentation

**Files:**
- `README.md` — Remove any references to `.alloc_use()` customization
- `docs/COLLECTIONS_IMPLEMENTATION.md` — Update allocation strategy section

**Document:**
- All collections use `Allocator.alloc/dispose` directly
- Custom allocators are set via `Application.set_allocator()` (affects all modules)
- No per-collection allocator override available (not needed)

---

## Migration Impact

### Breaking Change — Pre-1.0 API Evolution

**Removed API:**
```c
// REMOVED — No longer supported
FArray.alloc_use(&my_custom_alloc_use);
PArray.alloc_use(&my_custom_alloc_use);
List.alloc_use(&my_custom_alloc_use);
```

**Migration Path:**
Use `Application.set_allocator()` at program initialization:
```c
// Before module_init callbacks run
sc_alloc_use_t my_alloc = {
    .ctrl = (sc_ctrl_base_s*)my_controller,
    .alloc = my_allocator_alloc,
    .release = my_allocator_free,
    .resize = my_allocator_realloc,
};
Application.set_allocator(&my_alloc);

// Now all modules (collections, strings, etc.) use custom allocator
sigma_module_init();
```

**Affected Projects:**
- sigma.test — Will use `Application.set_allocator()` instead of `Collections.alloc_use()`
- anvil — Likely no usage, but audit parser allocations
- Any external projects — Pre-1.0 API evolution notice required

### Test Framework Integration

**sigma.test update** (coordinate with FR-2603-sigma-test-001):
```c
// OLD (FR-001 pattern):
coll_set_alloc_use(sigtest_alloc_use());

// NEW (FR-003 pattern):
Application.set_allocator(sigtest_alloc_use());
```

Test coverage for collections remains identical — `tracked_malloc/tracked_free` still capture all allocations, just through the Application layer instead of per-module hooks.

---

## Test Cases

### TC-001: FArray allocation uses Allocator facade
```c
// Setup: Application.set_allocator() with custom allocator
static int alloc_called = 0;
static void *test_alloc(usize size) { alloc_called++; return malloc(size); }
static void test_free(void *ptr) { free(ptr); }

sc_alloc_use_t custom = {
    .ctrl = NULL,
    .alloc = test_alloc,
    .release = test_free,
    .resize = NULL,
};
Application.set_allocator(&custom);

// Test: Create FArray
farray_t arr = FArray.create_with_capacity(10);
assert(alloc_called == 1);  // Internal buffer allocation
FArray.destroy(arr);

// Verify: No coll_set_alloc_use() exists
// (Compile-time check: symbol should not be exported)
```

### TC-002: PArray allocation uses Allocator facade
```c
// Same pattern as TC-001 with PArray.create_with_capacity(10)
// Verify alloc_called incremented through Application.get_allocator() path
```

### TC-003: List allocation uses Allocator facade
```c
// Same pattern as TC-001 with List.create()
// Verify node allocations route through custom allocator
```

### TC-004: Map allocation uses Allocator facade (when Map implemented)
```c
// Same pattern as TC-001 with Map.create()
// Verify bucket allocations route through custom allocator
```

### TC-005: No per-collection override possible
```c
// Compile-time verification:
// FArray.alloc_use() should not exist (vtable entry removed)
// coll_set_alloc_use() should not exist (function removed)

// If user tries to set per-collection allocator:
// - Compilation error (good, early feedback)
// - No runtime fallback (no silent behavior change)
```

### TC-006: Existing test suite passes unchanged
```c
// All 102 existing sigma.collections tests pass
// No behavioral changes to collection algorithms
// Only allocation routing changed (transparent to public API)
```

---

## Acceptance Criteria

- [ ] No `sc_alloc_use_t *s_coll_use` globals in any source file
- [ ] No `coll_set_alloc_use()` function in any source file
- [ ] No `.alloc_use` vtable entries in collection interfaces
- [ ] All allocation call sites use `Allocator.alloc/dispose/realloc` directly (or via helper functions that call Allocator)
- [ ] sigma.collections test suite passes (102 tests, no regressions)
- [ ] sigma.test integration updated (coordinate with sigma.test team)
- [ ] Documentation updated (README.md, implementation guide)
- [ ] Published: sigma.collections v0.3.0 (pre-1.0 breaking change)

---

## Rationale

**Simplification:** Removes 200+ lines of dispatch infrastructure (hooks, setters, vtable entries)

**Consistency:** Matches sigma.core pattern (BR-003 removed equivalent hooks from String/StringBuilder)

**Future-proof:** Application-level allocator + controller architecture is the final design — per-module hooks were a transitional pattern

**Test framework:** sigma.test can still inject tracked allocators via `Application.set_allocator()` — same observability, simpler path

**Performance:** Eliminates branch per allocation (no `if (s_coll_use)` check) — direct vtable call through Allocator

---

## See Also

- **FR-2603-sigma-collections-004** — Update dispatch helpers after alloc_use removal
- **BR-2603-sigma-core-003** — sigma.core facade removal precedent
- **FR-2603-sigma-memory-002** — Application delegation in Allocator facade
- **ORCHESTRATION-BR-2603-q-or-001.md** — Phase 3A specifications
