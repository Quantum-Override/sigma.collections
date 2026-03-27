# Standalone Usage Guide - Sigma Arrays (Malloc Variant)

## Quick Start

The malloc variant provides `farray` and `parray` types using direct `malloc`/`free` - perfect for standalone tools that don't need the full Sigma.* ecosystem.

### Minimal Example

```c
#include "farray_malloc.h"
#include <stdio.h>

int main(void) {
    // Create array for 10 integers
    farray arr = FArray.new(10, sizeof(int));
    
    // Store values
    for (int i = 0; i < 10; i++) {
        FArray.set(arr, i, sizeof(int), &i);
    }
    
    // Retrieve and print
    int value;
    FArray.get(arr, 5, sizeof(int), &value);
    printf("arr[5] = %d\n", value);
    
    // Cleanup
    FArray.dispose(arr);
    return 0;
}
```

**Compile:**
```bash
gcc example.c src/farray_malloc.c -Iinclude -I/usr/local/include -o example
./example
# Output: arr[5] = 5
```

## Installation

### Option 1: Source Files (Recommended for Portability)

Just copy the source files into your project:
```bash
# Copy headers
cp include/farray_malloc.h your_project/include/
cp include/parray_malloc.h your_project/include/

# Copy implementations  
cp src/farray_malloc.c your_project/src/
cp src/parray_malloc.c your_project/src/

# Also need sigma.core types
cp /usr/local/include/sigma.core/types.h your_project/include/sigma.core/
```

**Build with your project:**
```bash
gcc your_tool.c farray_malloc.c parray_malloc.c -Iinclude -o your_tool
```

### Option 2: Static Library

Build and install the static library:
```bash
# Build library
gcc -c src/farray_malloc.c -Iinclude -I/usr/local/include -std=c2x -Wall -o farray_malloc.o
gcc -c src/parray_malloc.c -Iinclude -I/usr/local/include -std=c2x -Wall -o parray_malloc.o
ar rcs libsigma_arrays_malloc.a farray_malloc.o parray_malloc.o

# Install (requires sudo)
sudo mkdir -p /usr/local/lib /usr/local/include
sudo cp libsigma_arrays_malloc.a /usr/local/lib/
sudo cp include/farray_malloc.h /usr/local/include/
sudo cp include/parray_malloc.h /usr/local/include/
sudo ldconfig
```

**Link against it:**
```bash
gcc your_tool.c -lsigma_arrays_malloc -o your_tool
```

## Complete Examples

### Example 1: Integer Array

```c
#include "farray_malloc.h"
#include <stdio.h>

int main(void) {
    // Create array
    farray numbers = FArray.new(5, sizeof(int));
    
    // Store data
    int values[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        FArray.set(numbers, i, sizeof(int), &values[i]);
    }
    
    // Read data
    printf("Array contents: ");
    for (size_t i = 0; i < 5; i++) {
        int val;
        FArray.get(numbers, i, sizeof(int), &val);
        printf("%d ", val);
    }
    printf("\n");
    
    // Clear all values
    FArray.clear(numbers, sizeof(int));
    
    // Cleanup
    FArray.dispose(numbers);
    return 0;
}
```

### Example 2: Struct Array

```c
#include "farray_malloc.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
    double score;
} Student;

int main(void) {
    // Create array for 100 students
    farray students = FArray.new(100, sizeof(Student));
    
    // Add students
    Student s1 = {1, "Alice", 95.5};
    Student s2 = {2, "Bob", 87.3};
    Student s3 = {3, "Carol", 92.1};
    
    FArray.set(students, 0, sizeof(Student), &s1);
    FArray.set(students, 1, sizeof(Student), &s2);
    FArray.set(students, 2, sizeof(Student), &s3);
    
    // Retrieve and display
    Student retrieved;
    FArray.get(students, 1, sizeof(Student), &retrieved);
    printf("Student %d: %s (%.1f)\n", 
           retrieved.id, retrieved.name, retrieved.score);
    
    // Remove middle student (zeros the entry)
    FArray.remove(students, 1, sizeof(Student));
    
    FArray.dispose(students);
    return 0;
}
```

### Example 3: Pointer Array

```c
#include "parray_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    // Create array for 10 string pointers
    parray strings = PArray.new(10);
    
    // Store strings (strdup allocates)
    PArray.set(strings, 0, (uintptr_t)strdup("Hello"));
    PArray.set(strings, 1, (uintptr_t)strdup("World"));
    PArray.set(strings, 2, (uintptr_t)strdup("Example"));
    
    // Retrieve and print
    for (size_t i = 0; i < 3; i++) {
        uintptr_t ptr;
        if (PArray.get(strings, i, &ptr) == 0 && ptr != 0) {
            printf("%zu: %s\n", i, (char *)ptr);
        }
    }
    
    // Cleanup: free stored strings first
    for (size_t i = 0; i < 3; i++) {
        uintptr_t ptr;
        if (PArray.get(strings, i, &ptr) == 0 && ptr != 0) {
            free((void *)ptr);
        }
    }
    
    PArray.dispose(strings);
    return 0;
}
```

### Example 4: Dynamic Buffer Management

```c
#include "parray_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define NUM_BUFFERS 5

int main(void) {
    parray buffers = PArray.new(NUM_BUFFERS);
    
    // Allocate buffers
    for (size_t i = 0; i < NUM_BUFFERS; i++) {
        void *buf = malloc(BUFFER_SIZE);
        if (buf) {
            memset(buf, 0, BUFFER_SIZE);
            PArray.set(buffers, i, (uintptr_t)buf);
            printf("Allocated buffer %zu\n", i);
        }
    }
    
    // Use a buffer
    uintptr_t buf_addr;
    PArray.get(buffers, 2, &buf_addr);
    char *buffer = (char *)buf_addr;
    snprintf(buffer, BUFFER_SIZE, "Data in buffer 2");
    printf("Buffer 2 contains: %s\n", buffer);
    
    // Free all buffers
    for (size_t i = 0; i < NUM_BUFFERS; i++) {
        uintptr_t addr;
        if (PArray.get(buffers, i, &addr) == 0 && addr != 0) {
            free((void *)addr);
        }
    }
    
    PArray.dispose(buffers);
    return 0;
}
```

## API Quick Reference

### FArray - Flexible Array

```c
farray FArray.new(capacity, stride)          // Create array
void FArray.init(farray *arr, cap, stride)   // Init pre-allocated
void FArray.dispose(farray arr)              // Free resources
int FArray.capacity(farray arr, stride)      // Get capacity
void FArray.clear(farray arr, stride)        // Zero all elements
int FArray.set(arr, index, stride, elem)     // Store element (0=OK, -1=ERR)
int FArray.get(arr, index, stride, out_elem) // Retrieve element (0=OK, -1=ERR)
int FArray.remove(arr, index, stride)        // Zero element (0=OK, -1=ERR)
```

**Key Points:**
- `stride` = `sizeof(your_type)` 
- Elements copied with `memcpy`
- Returns `0` (OK) or `-1` (ERR)
- `NULL`-safe operations

### PArray - Pointer Array

```c
parray PArray.new(capacity)                  // Create array
void PArray.init(parray *arr, capacity)      // Init pre-allocated
void PArray.dispose(parray arr)              // Free resources
int PArray.capacity(parray arr)              // Get capacity
void PArray.clear(parray arr)                // NULL all pointers
int PArray.set(arr, index, ptr)              // Store pointer (0=OK, -1=ERR)
int PArray.get(arr, index, &out_ptr)         // Retrieve pointer (0=OK, -1=ERR)
int PArray.remove(arr, index)                // NULL pointer (0=OK, -1=ERR)
```

**Key Points:**
- Stores `uintptr_t` (cast from/to pointers)
- Does NOT manage pointed-to memory
- You must `free()` stored allocations
- `NULL` is a valid value to store

## Error Handling

```c
farray arr = FArray.new(10, sizeof(int));
if (!arr) {
    fprintf(stderr, "Allocation failed\n");
    return 1;
}

int value = 42;
if (FArray.set(arr, 5, sizeof(int), &value) != 0) {
    fprintf(stderr, "Set failed (out of bounds or NULL)\n");
}

int retrieved;
if (FArray.get(arr, 5, sizeof(int), &retrieved) != 0) {
    fprintf(stderr, "Get failed (out of bounds or NULL)\n");
}

FArray.dispose(arr);
```

**Error Codes:**
- `NULL` return = allocation failure
- `0` (OK) = success
- `-1` (ERR) = failure (bounds, NULL param)

## Memory Management

### FArray Memory Layout
```
[struct sc_flex_array_malloc]
  ├─ elem_size: size of each element
  ├─ capacity: number of elements
  └─ data: ───→ [elem0][elem1][elem2]...[elemN]
```

### PArray Memory Layout  
```
[struct sc_pointer_array_malloc]
  ├─ capacity: number of pointer slots
  └─ data: ───→ [ptr0][ptr1][ptr2]...[ptrN]
                  │     │     │
                  ↓     ↓     ↓
                (your managed memory)
```

**Important:** PArray does NOT free the memory pointed to by stored pointers. You must manage those allocations yourself.

## Best Practices

### 1. Always Check Allocation
```c
farray arr = FArray.new(1000, sizeof(MyType));
if (!arr) {
    // Handle allocation failure
    return -1;
}
```

### 2. Match Set/Get Stride
```c
typedef struct { int x, y; } Point;

farray points = FArray.new(10, sizeof(Point));

// ALWAYS use same stride in set/get/remove
Point p = {1, 2};
FArray.set(points, 0, sizeof(Point), &p);  // ✓ Correct
FArray.get(points, 0, sizeof(Point), &p);  // ✓ Correct
// FArray.get(points, 0, sizeof(int), &p); // ✗ WRONG!
```

### 3. Free PArray Contents Before Dispose
```c
parray allocations = PArray.new(10);

// Store allocated memory
for (int i = 0; i < 10; i++) {
    void *mem = malloc(256);
    PArray.set(allocations, i, (uintptr_t)mem);
}

// FREE FIRST
for (int i = 0; i < 10; i++) {
    uintptr_t addr;
    PArray.get(allocations, i, &addr);
    free((void *)addr);
}

// THEN dispose
PArray.dispose(allocations);
```

### 4. Initialize to Known State
```c
farray arr = FArray.new(100, sizeof(int));
FArray.clear(arr, sizeof(int));  // Zero all elements

// Now all elements are 0
int val;
FArray.get(arr, 50, sizeof(int), &val);
assert(val == 0);
```

## Performance Characteristics

| Operation | FArray | PArray | Notes |
|-----------|--------|--------|-------|
| Create | O(n) | O(n) | Allocates and zeros memory |
| Dispose | O(1) | O(1) | Does NOT free PArray contents |
| Set | O(1) | O(1) | FArray uses memcpy |
| Get | O(1) | O(1) | FArray uses memcpy |
| Clear | O(n) | O(n) | Zeros/NULLs all slots |
| Remove | O(1) | O(1) | Zeros/NULLs one slot |

**Memory Overhead:**
- FArray: `sizeof(struct) + (capacity * stride)` 
- PArray: `sizeof(struct) + (capacity * sizeof(uintptr_t))`

## Testing

Run the included test suite:
```bash
gcc test/standalone/test_arrays_malloc.c \
    src/farray_malloc.c src/parray_malloc.c \
    -Iinclude -I/usr/local/include -std=c2x -o test_arrays

./test_arrays
# Should output: Tests: 12/12 passed ✓

# Check for memory leaks
valgrind --leak-check=full ./test_arrays
# Should report: All heap blocks were freed -- no leaks are possible
```

## Troubleshooting

### "undefined reference to FArray"
You forgot to link the source files or library:
```bash
# Add source files
gcc your_tool.c farray_malloc.c parray_malloc.c -Iinclude -o your_tool

# Or link library
gcc your_tool.c -lsigma_arrays_malloc -o your_tool
```

### "sigma.core/types.h: No such file"
Install sigma.core headers or copy types.h to your include path:
```bash
sudo cp /usr/local/include/sigma.core/types.h /usr/local/include/sigma.core/
# Or add -I flag pointing to sigma.core location
```

### "invalid use of undefined type"
You included the wrong header. Use:
```c
#include "farray_malloc.h"  // Not "farray.h"
#include "parray_malloc.h"  // Not "parray.h"
```

### Segmentation Fault
Common causes:
- Out-of-bounds access (check index < capacity)
- Using disposed array
- NULL pointer in get/set operations
- Mismatched stride in FArray operations

## Drop-In Compatibility

The malloc variants use the **same type names** as sigma.collections. To switch implementations, just change the include:

```c
// Option 1: sigma.collections (with Allocator)
#include "farray.h"
#include "parray.h"

// Option 2: malloc variant (no Allocator)
#include "farray_malloc.h"
#include "parray_malloc.h"

// Same code works with either:
farray arr = FArray.new(10, sizeof(int));
FArray.set(arr, 0, sizeof(int), &value);
FArray.dispose(arr);
```

This is intentional - choose the right implementation for your use case without changing code.

## License

Copyright (c) 2025 David Boarman (BadKraft) and contributors  
QuantumOverride [Q|]

MIT License - See LICENSE file for full terms.

## Further Reading

- [sigma.arrays.a.md](sigma.arrays.a.md) - Complete technical documentation
- [Sigma Collections Overview](COLLECTIONS_OVERVIEW.md) - Full ecosystem documentation
- [API Reference](API_REFERENCE.md) - Detailed API for all collection types

## Support

For bugs, questions, or contributions, see the main Sigma.Collections project.
