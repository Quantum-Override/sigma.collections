# Changelog

## [0.3.0] — 2026-09-22

### Added
- FR-2603-sigma-collections-006: Query — heapless scan mechanism (Query.next/Query.first) over sc_queryable, ported from Anvil's vendored src/sigma/query.h+.c
- FR-2603-sigma-collections-006: sc_predicate_fn typedef on collection.h, shared array_base_query_advance dense-source stepping function
- FR-2603-sigma-collections-006: as_queryable on FArray, PArray, Collections, List, SlotArray
- FR-2603-sigma-collections-006: as_queryable/keys/values on Map (keys/values skip empty and tombstone slots)
- FR-2603-sigma-collections-007: Collections.create_with_allocator — per-instance sc_alloc_use_t binding for one collection, independent of every other instance
- FR-2603-sigma-collections-007: List.new_with_allocator — thin delegation to Collections.create_with_allocator
- FR-2603-sigma-collections-011: Stack — new LIFO type composed over Collection (push/pop/peek/clear/is_empty/count/clear), plus mark/restore for O(1) truncation to a saved depth; Stack.new_with_allocator via the same allocator-override mechanism
- test/unit/fake_arena.h — malloc-backed sc_alloc_use_t test double, ported from Anvil's test/sigma/fake_arena.h
- docs: Query section in API_REFERENCE.md, COLLECTIONS_OVERVIEW.md, and USERS_GUIDE.md; Stack section in all three plus README.md

### Changed
- struct sc_collection gains an alloc_use field (NULL = global Allocator/Application facade, unchanged for every existing caller)
- collection_grow / collection_dispose branch on alloc_use — when bound, growth orphans (does not free) the old buffer, and dispose skips freeing the buffer; the bound allocator owns bulk-reclaiming it
- docs: corrected README.md, COLLECTIONS_OVERVIEW.md, API_REFERENCE.md, and USERS_GUIDE.md's Custom Allocation sections, which still described the module-level Collections.alloc_use() hook removed in the Phase 3A cleanup (see FR-2603-sigma-collections-001, obsolete) — the per-instance override above is the only custom-allocator mechanism that currently exists
- internal: Collections library version string 1.2.0 -> 1.3.0 (sc_collections_i gained create_with_allocator and as_queryable this release)

### Fixed
- Stray trailing '}' in test_slotarray.c and test_parray.c that broke compilation entirely (pre-existing, unrelated typo, found while adding as_queryable tests to those files)

**Migration:** No code changes needed for basic usage. New: Query.next/Query.first via as_queryable (FArray/PArray/Collections/List/SlotArray) and as_queryable/keys/values (Map) for heapless scanning. New: Collections.create_with_allocator/List.new_with_allocator/Stack.new_with_allocator for per-instance allocator binding — NULL behaves exactly like the existing constructors. New: Stack type. Note: docs previously described a Collections.alloc_use() module-wide hook that does not exist in this or any recent release — if you were relying on documentation referencing it, see the corrected Custom Allocation sections.

---

## [0.2.1] — 2026-03-27

### Added
- Re-coupled to sigma.memory via Allocator facade — all allocations use Allocator.alloc/dispose
- Dependency: sigma.memory for Allocator interface
- Test framework integration: sigma.test included in REQUIRES for Tests.enqueue API

### Changed
- All collection implementations use Allocator.alloc() instead of alloc_use dispatch
- Removed coll_alloc/coll_free/coll_realloc internal dispatch helpers
- Removed alloc_use() from all collection interfaces
- Updated headers: #include <sigma.core/allocator.h> (was alloc.h)
- Test registration: two-function pattern with Tests.enqueue (static register + constructor enqueue)
- Test includes: #include <sigma.test/sigtest.h> (was sigtest/sigtest.h)
- Build: added sigma.test to REQUIRES in config.sh

### Removed
- alloc_use pattern and custom allocator hooks from v0.2.0
- Module-level dispatch mechanisms (coll_alloc/coll_free/coll_realloc)
- sc_alloc_use_t type references
- sigtest_alloc_use() integration path

### Breaking Changes
- API change: removed alloc_use() method from all collection interfaces
- Build change: now requires sigma.memory dependency (Allocator facade)
- Test API change: requires sigma.test with Tests.enqueue support

**Migration:** Collections now use Allocator from sigma.memory. Remove any alloc_use() calls. Add sigma.memory to build dependencies. Update includes to <sigma.core/allocator.h>. Tests require sigma.test with Tests.enqueue API.

---

## [0.2.0] — 2026-03-21

### Added
- FR-2603-sigma-collections-001: alloc_use pattern — module-level custom allocator hook in arrays.c
- FR-2603-sigma-collections-001: alloc_use(sc_alloc_use_t *use) exposed on every collection interface
- FR-2603-sigma-collections-001: dispatch helpers coll_alloc/coll_free/coll_realloc; direct malloc/free fallback when no hook set
- FR-2603-sigma-collections-001: sigtest_alloc_use() integration path for sigma.test leak detection
- FR-2603-sigma-collections-002: Map collection — string-keyed FNV-1a 64-bit hash map with open addressing and linear probing
- FR-2603-sigma-collections-002: Map auto-resize at 50% load factor; pointer-based caller-owned keys; binary-safe
- FR-2603-sigma-collections-002: O(1) average set/get/remove; sparse iterator via Map.create_iterator()
- docs: User Guide — Custom Allocation section
- docs: alloc_use documentation in all interface sections of API Reference
- docs: Migration Guide v0.2.0 (docs/MIGRATION_v0.2.0.md)

### Changed
- internal: all allocations route through coll_alloc/coll_free/coll_realloc dispatch helpers
- build: simplified test linker flags — no allocator object dependency
- headers: added #include <sigma.core/allocator.h> to all public headers for sc_alloc_use_t type

### Fixed
- sigma.collections now works without sigma.memory dependency — pure libc fallback
- test framework can track collection allocations via sigtest_alloc_use()

### Breaking Changes
- Removed Allocator facade dependency — removed #include <sigma.core/alloc.h> and all Allocator.alloc/dispose/realloc calls (35 occurrences)
- Removed sigma.core.alloc.o linker dependency from config.sh

**Migration:** No code changes needed for basic usage (defaults to malloc/free). Remove #include <sigma.core/alloc.h> if present. Remove sigma.core.alloc.o from build scripts if referenced. Optionally use Collections.alloc_use() for custom allocation. See docs/MIGRATION_v0.2.0.md.

---

## [0.1.1] — 2026-01-24

### Changed
- Improved API documentation
- Added usage examples

### Fixed
- Various bug fixes and stability improvements
- Iterator edge cases
- Sparse collection slot reuse logic

---

## [0.1.0] — 2025-12-03

### Added
- Initial release
- Dense collections: FArray, PArray, List
- Sparse collections: SlotArray, IndexArray
- Iterator and SparseIterator interfaces
- Buffer view support
- Dynamic growth for List and IndexArray
- Comprehensive test suite
