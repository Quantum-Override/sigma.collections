# Changelog

All notable changes to sigma.collections will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-03-21 (RC)

### Added
- **Custom Allocation**: New `alloc_use` pattern for module-level custom allocators
  - Each collection interface exposes `alloc_use(sc_alloc_use_t *use)` function
  - Module-level dispatch hooks (`coll_alloc`, `coll_free`, `coll_realloc`) in `arrays.c`
  - Setter `coll_set_alloc_use()` exported from `internal/arrays.h`
  - Direct `malloc`/`free` fallback when no custom allocator is set (zero overhead)
  - Enables seamless integration with sigma.test for leak detection
  - See [Custom Allocation](#custom-allocation) section in User Guide
- **Map Collection**: New string-keyed hash map (FR-2603-sigma-collections-002)
  - FNV-1a 64-bit hashing with open addressing and linear probing
  - Automatic resize at 50% load factor for optimal performance
  - Pointer-based key storage (caller-owned, arena-friendly)
  - Binary-safe keys (supports NULL bytes in key data)
  - O(1) average-case set/get/remove operations
  - Sparse iterator support via `Map.create_iterator()`
  - See [User Guide - Hash Map](docs/USERS_GUIDE.md#hash-map) for details

### Removed
- **Breaking**: Removed dependency on `Allocator` facade from sigma.core
  - Removed `#include <sigma.core/alloc.h>` from all source files
  - Removed all `Allocator.alloc()` / `Allocator.dispose()` / `Allocator.realloc()` calls (35 occurrences)
  - Removed `sigma.core.alloc.o` linker dependency from `config.sh`
  - Replaced with direct `malloc`/`free` calls or custom allocator dispatch

### Changed
- **Internal**: All allocations now route through dispatch helpers:
  - `coll_alloc(usize size)` - Allocates memory
  - `coll_free(void *ptr)` - Frees memory
  - `coll_realloc(void *ptr, usize size)` - Reallocates memory
  - Helpers check module-level `sc_alloc_use_t *s_coll_use` hook first, fallback to libc
- **Build**: Simplified test linker flags (no allocator dependency)
- **Headers**: Added `#include <sigma.core/allocator.h>` to all public headers for `sc_alloc_use_t` type

### Fixed
- Collections now work without sigma.memory dependency (pure libc)
- Test framework can track collection allocations via `sigtest_alloc_use()`

### Documentation
- Added comprehensive [User Guide - Custom Allocation](docs/USERS_GUIDE.md#custom-allocation) section
- Added `alloc_use` function documentation to all interface sections in [API Reference](docs/API_REFERENCE.md)
- Created [Migration Guide v0.2.0](docs/MIGRATION_v0.2.0.md) for upgrading from v0.1.x
- Updated README.md with custom allocation example and migration link

### Migration Notes
See [MIGRATION_v0.2.0.md](docs/MIGRATION_v0.2.0.md) for detailed upgrade instructions.

**Quick Summary**:
- No code changes needed for basic usage (defaults to malloc/free)
- Remove `#include <sigma.core/alloc.h>` if present in your code
- Remove `sigma.core.alloc.o` from build scripts if referenced
- Optionally use `Collections.alloc_use()` for custom allocation

### Testing
- All 102 tests pass (17 map, 17 slotarray, 16 indexarray, 23 list, 13 farray, 13 parray, 1 iterator, 2 allocator)
- Verified zero memory leaks with valgrind
- Verified module builds cleanly without allocator dependency

---

## [0.1.1] - 2026-01-24

### Fixed
- Various bug fixes and stability improvements
- Iterator edge cases
- Sparse collection slot reuse logic

### Documentation
- Improved API documentation
- Added usage examples

---

## [0.1.0] - 2025-12-03

### Added
- Initial release
- Dense collections: FArray, PArray, List
- Sparse collections: SlotArray, IndexArray
- Iterator and SparseIterator interfaces
- Buffer view support
- Dynamic growth for List and IndexArray
- Comprehensive test suite

### Features
- Cache-friendly contiguous memory layouts
- Stable handle semantics for sparse collections
- Automatic slot reuse
- Non-owning buffer views
- Unified collection interface

---

## Version Scheme

- **Major** (0.x.0) - Breaking API changes
- **Minor** (0.x.0) - New features, backwards compatible within major version
- **Patch** (0.0.x) - Bug fixes, no API changes

**RC (Release Candidate)**: Pre-release version undergoing final testing before stable release.

---

## Links

- [Repository](https://github.com/BadKraft/sigma.collections)
- [Issue Tracker](feature-reqs/)
- [User Guide](docs/USERS_GUIDE.md)
- [API Reference](docs/API_REFERENCE.md)
