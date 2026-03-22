# FR-2603-sigma-collections-001: alloc_use seam for sigma.collections + sigma.test opaque provider

**ID:** FR-2603-sigma-collections-001  
**Type:** Feature Request  
**Owner:** sigma.collections  
**Co-change:** sigma.test  
**Filed:** 2026-03-21  
**Status:** open  
**Tags:** sigma-collections, sigma-test, allocator, alloc-use, testing  

---

## Summary

sigma.collections currently routes all internal allocations through `Allocator.alloc()` /
`Allocator.dispose()` — the `sc_allocator_i` facade, which is being retired. Replace that
dependency with a module-level `sc_alloc_use_t` hook (the same pattern established by
`sigma.core` String and StringBuilder) with a `malloc`/`free` direct fallback when no hook
is set.

As a paired change, sigma.test exposes a static `sc_alloc_use_t *` that wraps its existing
`tracked_malloc` / `tracked_free` instrumentation, providing an opaque allocator provider
with zero sigma.memory dependency. Tests wire the two together with a single setter call.

---

## Background

### The facade retirement

All sigma.collections sources include `<sigma.core/alloc.h>` and call `Allocator.alloc()` /
`Allocator.dispose()`. The `Allocator` global facade is deprecated and being removed. Every
call site in the library needs a replacement before the facade is gone.

### Why not a hard sigma.memory dependency

sigma.memory's Phase 3 roadmap includes pulling in sigma.collections for B-tree / skip-list
internals. A hard dependency in the opposite direction creates a circular link. The `alloc_use`
hook breaks the coupling: sigma.memory can wire in via `sc_trusted_cap_t.alloc_use` at
module init time without sigma.collections knowing anything about slabs or controllers.

### Why sigma.test is involved

sigma.test already has `tracked_malloc` / `tracked_free` and per-set counters
(`total_mallocs`, `total_frees` in `st_summary`). These currently catch libc-level
allocations. Because sigma.collections allocates through the `Allocator` facade today, its
allocations are invisible to the test framework. Once the `alloc_use` seam exists,
`sigtest_alloc_use()` closes that gap completely — no new test infrastructure required.

---

## Requested Changes

### sigma.collections

**1. `internal/arrays.h` — shared allocator dispatch**

Add a setter exported from `arrays.c`:

```c
void coll_set_alloc_use(sc_alloc_use_t *use);
```

**2. `arrays.c` — module-level hook + dispatch helpers**

```c
static sc_alloc_use_t *s_coll_use = NULL;

void coll_set_alloc_use(sc_alloc_use_t *use) { s_coll_use = use; }

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

Replace every `Allocator.alloc` / `Allocator.dispose` / `Allocator.realloc` call site across
all sources with `coll_alloc` / `coll_free` / `coll_realloc`.

Remove `#include <sigma.core/alloc.h>` from all sources.  
Add `#include <sigma.core/allocator.h>` (for `sc_alloc_use_t`) and `#include <stdlib.h>`
(for `malloc`/`free` fallback) to `arrays.c`.

**3. Collection interface structs — add `alloc_use` slot**

Add to `sc_collections_i`, `sc_farray_i`, `sc_parray_i` (and remaining typed interfaces):

```c
void (*alloc_use)(sc_alloc_use_t *use);
```

All implementations call through to `coll_set_alloc_use`.

**4. `config.sh` — drop sys_alloc linker flag**

```diff
-TST_LDFLAGS="-lstest -L/usr/lib -L/usr/local/packages -l:sigma.core.alloc.o"
+TST_LDFLAGS="-lstest -L/usr/lib -L/usr/local/packages"
```

### sigma.test

**5. `sigtest.h` — expose opaque alloc_use provider**

```c
// Returns a static sc_alloc_use_t * backed by tracked_malloc / tracked_free.
// Wire into any alloc_use-capable library to route its allocations through
// sigma.test's per-set counters and on_memory_alloc / on_memory_free hooks.
sc_alloc_use_t *sigtest_alloc_use(void);
```

**6. `sigtest.c` — implement provider**

```c
static sc_alloc_use_t s_test_alloc_use = {
    .alloc   = tracked_malloc,
    .release = tracked_free,
    .resize  = tracked_realloc,   // add tracked_realloc if not already present
};

sc_alloc_use_t *sigtest_alloc_use(void) { return &s_test_alloc_use; }
```

`tracked_realloc` follows the same pattern as `tracked_malloc` — calls `realloc`, increments
`global_allocs` on grow (net alloc) or zero-delta tracking as appropriate.

---

## Usage Pattern (in a test set)

```c
static void register_farray_tests(void) {
    FArray.alloc_use(sigtest_alloc_use());   // one call, entire set tracked

    testset("FArray", NULL, NULL);
    testcase("new/dispose balance", test_farray_alloc_balance);
    // ...
}
```

After the set, `st_summary.total_mallocs == total_frees` is the leak assertion — already
produced by sigma.test's existing summary machinery.

---

## Rationale

- Removes the only remaining `Allocator` facade dependency in sigma.collections.
- `malloc`/`free` fallback keeps collections usable in lean builds and test harnesses that
  do not link sigma.memory.
- `sigtest_alloc_use()` reuses 100% of existing sigma.test instrumentation — no new concepts
  introduced on the test side.
- Per-set alloc/free balance checking becomes available to any library with an `alloc_use`
  seam, not just collections. This is a reusable pattern.
- Groundwork for FR-2603-sigma-test-001 (sandboxed controller) — the `alloc_use` slot is the
  same connection point; only the provider changes.
