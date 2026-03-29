# Migration Guide: v0.1.x to v0.2.0

This guide covers breaking changes and migration steps for upgrading from sigma.collections v0.1.x to v0.2.0.

## Table of Contents

- [Overview](#overview)
- [Breaking Changes](#breaking-changes)
- [Migration Steps](#migration-steps)
- [New Features](#new-features)
- [FAQ](#faq)

## Overview

Version 0.2.0 removes the deprecated `Allocator` facade dependency and introduces the `alloc_use` pattern for custom allocation. This change:

- **Removes** dependency on `sigma.core/alloc.h` and the `Allocator` global facade
- **Adds** module-level `alloc_use` hook for custom allocators
- **Defaults** to standard `malloc`/`free` when no custom allocator is set
- **Enables** seamless integration with sigma.test for leak detection

## Breaking Changes

### 1. Allocator Facade Removed

**Before (v0.1.x)**:
```c
#include <sigma.core/alloc.h>  // Required for Allocator facade

indexarray ia = IndexArray.new(100, sizeof(int));
// Internally called Allocator.alloc()
```

**After (v0.2.0)**:
```c
// No special header needed - uses malloc/free by default
indexarray ia = IndexArray.new(100, sizeof(int));
// Internally calls malloc() directly
```

### 2. Build Dependency Removed

**Before (v0.1.x)**:
```bash
# config.sh included sigma.core.alloc.o
TST_LDFLAGS="-lstest -L/usr/lib -L/usr/local/packages -l:sigma.core.alloc.o"
```

**After (v0.2.0)**:
```bash
# No allocator linker flag needed
TST_LDFLAGS="-lstest -L/usr/lib -L/usr/local/packages"
```

### 3. Header Changes

**Removed**: `#include <sigma.core/alloc.h>` from all source files  
**Added**: `#include <sigma.core/allocator.h>` to all public headers (for `sc_alloc_use_t` type)

**Impact**: If your code directly included `<sigma.core/alloc.h>`, remove it. The collections headers now provide everything needed.

## Migration Steps

### Step 1: Update Your Build

If you have a custom build system that links `sigma.core.alloc.o`, remove that dependency:

```bash
# Remove from LDFLAGS
-l:sigma.core.alloc.o  # DELETE THIS
```

### Step 2: Remove Explicit Allocator Includes

Search for and remove any direct includes of the old allocator header:

```bash
# Find files that include the old header
grep -r "sigma.core/alloc.h" your_project/

# Remove the include line from each file
```

### Step 3: Rebuild Your Application

```bash
cbuild clean
cbuild
```

Your code should compile without changes. All allocations now use `malloc`/`free` by default.

### Step 4: (Optional) Add Custom Allocator

If you need custom allocation for testing or specialized memory management:

```c
#include <sigma.core/allocator.h>

// Define your allocator
sc_alloc_use_t my_allocator = {
    .alloc = my_malloc,
    .release = my_free,
    .resize = my_realloc
};

// Set once at startup
int main(void) {
    Collections.alloc_use(&my_allocator);
    
    // All subsequent collection allocations use your allocator
    indexarray ia = IndexArray.new(100, sizeof(int));
    
    return 0;
}
```

## New Features

### Custom Allocation Hook

All collection interfaces now expose an `alloc_use` function:

```c
Collections.alloc_use(&my_allocator);  // Module-wide setting
FArray.alloc_use(&my_allocator);       // Same effect
List.alloc_use(&my_allocator);         // Same effect
// ... etc
```

**Note**: All interfaces share the same module-level hook. Use `Collections.alloc_use()` for clarity.

### Integration with sigma.test

Track all collection allocations in tests:

```c
#include <sigtest/sigtest.h>

static void register_tests(void) {
    // Wire collections into test framework
    Collections.alloc_use(sigtest_alloc_use());
    
    testset("my_tests", NULL, NULL);
    testcase("test_no_leaks", test_collection_lifecycle);
}

static void test_collection_lifecycle(void) {
    // All allocations tracked
    indexarray ia = IndexArray.new(100, sizeof(int));
    
    // ... perform operations ...
    
    IndexArray.dispose(ia);
    
    // Test framework reports:
    // "Total mallocs: 1"
    // "Total frees: 1"
}
```

### Malloc/Free Fallback

No allocator dependency when using defaults:

```c
// Works with just libc - no sigma.memory needed
indexarray ia = IndexArray.new(100, sizeof(int));
IndexArray.dispose(ia);
```

## FAQ

### Q: Do I need to change my existing code?

**A**: No. If you were just using collections without direct `Allocator` references, your code works unchanged. Just rebuild.

### Q: What if I was using `Allocator.alloc()` directly?

**A**: Replace those calls with:
- `Collections.alloc_use(&allocator)` to set module-level allocator
- Or use `malloc()`/`free()` directly if you don't need collections' allocator

### Q: Can I use different allocators for different collection types?

**A**: No. All collection types share one module-level allocator. This is by design to keep allocation behavior predictable.

### Q: Is the change thread-safe?

**A**: The `alloc_use` setter is not thread-safe. Call it once at startup before spawning threads. The dispatch itself (once set) is safe for concurrent reads.

### Q: What happens if I call `alloc_use()` multiple times?

**A**: The last call wins. However, **do not** change allocators while collections are active:

```c
// BAD: Allocator mismatch
Collections.alloc_use(&allocator_a);
indexarray ia = IndexArray.new(100, sizeof(int));  // Uses allocator_a

Collections.alloc_use(&allocator_b);
IndexArray.dispose(ia);  // Uses allocator_b - MISMATCH!

// GOOD: Consistent allocator
Collections.alloc_use(&allocator_a);
indexarray ia = IndexArray.new(100, sizeof(int));
IndexArray.dispose(ia);  // Both use allocator_a
```

### Q: How do I verify no memory leaks?

**A**: Use valgrind or sigma.test:

```bash
# With valgrind
ctest my_test --valgrind

# Or use sigma.test alloc tracking
Collections.alloc_use(sigtest_alloc_use());
```

### Q: Does this affect performance?

**A**: Default behavior (NULL allocator) has **no overhead** - direct `malloc`/`free` calls. Custom allocators add one pointer check per allocation.

### Q: Can I restore the old Allocator behavior?

**A**: No. The `Allocator` facade is deprecated and being removed from sigma.core. Use `alloc_use` instead.

### Q: What if I need sigma.memory integration?

**A**: sigma.memory 2.0 will provide its own `sc_alloc_use_t` implementation. Wire it in via:

```c
Collections.alloc_use(memory_alloc_provider());
```

This is part of the design - no hard dependency on sigma.memory.

## Support

For issues or questions:
- File an issue: `feature-reqs/`
- Check [User Guide](USERS_GUIDE.md#custom-allocation)
- Check [API Reference](API_REFERENCE.md#collections)

---

**Migration Checklist**:
- [ ] Remove `sigma.core.alloc.o` from build files
- [ ] Remove `#include <sigma.core/alloc.h>` from source files
- [ ] Rebuild with `cbuild clean && cbuild`
- [ ] Run tests with `ctest`
- [ ] (Optional) Add custom allocator with `Collections.alloc_use()`
- [ ] (Optional) Integrate with sigma.test for leak tracking
