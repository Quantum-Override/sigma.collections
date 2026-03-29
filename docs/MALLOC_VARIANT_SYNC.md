# Malloc Variant Feature Parity

## Overview

The sigma.collections project maintains **two implementations** of FArray and PArray:

1. **Main Collections** (`src/farray.c`, `src/parray.c`) - Uses `Allocator.alloc/dispose` from sigma.memory
2. **Malloc Variant** (`src/malloc/farray.c`, `src/malloc/parray.c`) - Uses direct `malloc/free` with **zero dependencies**

Both implementations share identical **ABI-compatible vtable interfaces** (`sc_farray_i`, `sc_parray_i`), enabling drop-in replacement via include path change only.

## Purpose of Malloc Variant

The malloc variant (`sigma.arrays.a`) is used for:
- **Profiling tools** that need to measure sigma.memory allocator performance
- **Embedded environments** with minimal dependencies
- **Standalone utilities** that don't need the full Sigma ecosystem
- **Phase 0 deliverables** where sigma.memory integration isn't available
- **Taste But Don't Feast** where users want to check out what we're doing without buying into the entire ecosystem

## Critical Requirement: Feature Parity

**IMPORTANT**: If any features, bug fixes, or enhancements are implemented in `farray.c` or `parray.c`, those **MUST** be implemented and tested in the malloc variant.

### Synchronization Process

When modifying main collections:

1. **Identify Changes**
   - Algorithm improvements
   - Bug fixes
   - New features
   - Performance optimizations
   - API additions

2. **Port to Malloc Variant**
   - Copy algorithm changes to `src/malloc/farray.c` or `src/malloc/parray.c`
   - Replace `Allocator.alloc()` → `malloc()`
   - Replace `Allocator.dispose()` → `free()`
   - Preserve identical function signatures and behavior

3. **Test Both Implementations**
   - Run main collection tests: `./rtest unit/farray` or `./rtest unit/parray`
   - Run malloc variant tests: `./build/test_arrays_malloc`
   - Verify with valgrind: `valgrind --leak-check=full ./build/test_arrays_malloc`

4. **Update Test Suites**
   - Add corresponding tests to `test/malloc/test_arrays.c`
   - Ensure test coverage matches between implementations

### Files to Keep in Sync

| Main Collection | Malloc Variant | Test (Main) | Test (Malloc) |
|----------------|----------------|-------------|---------------|
| src/farray.c | src/malloc/farray.c | test/test_farray.c | test/malloc/test_arrays.c |
| src/parray.c | src/malloc/parray.c | test/test_parray.c | test/malloc/test_arrays.c |

### Quick Checklist

Before committing changes to farray.c or parray.c:

- [ ] Ported algorithm changes to malloc variant
- [ ] Replaced Allocator calls with malloc/free
- [ ] Added tests to test/malloc/test_arrays.c
- [ ] Run: `bash build_malloc.sh --test`
- [ ] Run: `./build/test_arrays_malloc`
- [ ] Run: `valgrind --leak-check=full ./build/test_arrays_malloc`
- [ ] All tests pass (12/12 for malloc variant)
- [ ] Zero memory leaks reported by valgrind

## Building Malloc Variant

```bash
# Build library only
bash build_malloc.sh

# Build library and test
bash build_malloc.sh --test

# Run tests
./build/test_arrays_malloc

# Check for leaks
valgrind --leak-check=full ./build/test_arrays_malloc
```

## Binary Outputs

- **sigma.collections.o** - Main collection library (depends on sigma.memory)
- **sigma.arrays.a** - Malloc variant static library (zero dependencies)

Both libraries export the same vtable interfaces, enabling binary compatibility.

## Verification Commands

```bash
# Verify zero sigma.* dependencies in malloc variant
nm build/sigma.arrays.a | grep -E '(Allocator|coll_|sigma_)'
# Should return empty (no matches)

# Check exported symbols
nm build/sigma.arrays.a | grep ' T '
# Should show: FArray, PArray vtable symbols
```

## Example: Porting a Feature

**Main Collection Change** (src/farray.c):
```c
void farray_resize(flex_array arr, usize new_capacity) {
    object new_data = Allocator.alloc(new_capacity * arr->elem_size, false);
    // ... copy logic ...
    Allocator.dispose(arr->data);
    arr->data = new_data;
}
```

**Malloc Variant Port** (src/malloc/farray.c):
```c
void farray_resize(flex_array arr, usize new_capacity) {
    object new_data = malloc(new_capacity * arr->elem_size);
    // ... copy logic (identical) ...
    free(arr->data);
    arr->data = new_data;
}
```

**Test Addition** (test/malloc/test_arrays.c):
```c
void test_farray_resize() {
    flex_array arr = FArray.create(10, sizeof(int));
    // ... test resize logic ...
    FArray.dispose(arr);
}
```

## Common Pitfalls

1. **Forgetting to port bug fixes** - Always sync both implementations
2. **Different behavior** - Malloc variant must behave identically to main
3. **Memory leaks** - Always run valgrind after changes
4. **Test coverage gaps** - Keep test suites aligned

## Automation Ideas (Future)

- Git pre-commit hook to detect changes in farray.c/parray.c without corresponding malloc changes
- CI pipeline to run both test suites
- Diff tool to compare algorithm sections between implementations
- Automated sync script with allocation call translation

---

**Last Updated**: 2025-03-28  
**Maintainers**: sigma.collections team
