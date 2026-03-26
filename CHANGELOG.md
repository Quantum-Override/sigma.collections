# Changelog

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
