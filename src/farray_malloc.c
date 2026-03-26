/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: farray_malloc.c
 * Description: Standalone malloc-based flexible array implementation
 */

#include "farray_malloc.h"
#include <stdlib.h>
#include <string.h>

struct farray_malloc_s {
    size_t elem_size;  // Size of each element in bytes
    size_t capacity;   // Number of elements allocated
    void *data;        // Pointer to element storage
};

farray_malloc farray_malloc_create(size_t elem_size, size_t capacity) {
    if (elem_size == 0 || capacity == 0) {
        return NULL;
    }
    
    // Allocate struct
    farray_malloc arr = (farray_malloc)malloc(sizeof(struct farray_malloc_s));
    if (!arr) {
        return NULL;
    }
    
    // Allocate data buffer
    size_t total_size = elem_size * capacity;
    arr->data = malloc(total_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    
    arr->elem_size = elem_size;
    arr->capacity = capacity;
    
    // Zero initialize
    memset(arr->data, 0, total_size);
    
    return arr;
}

void farray_malloc_dispose(farray_malloc arr) {
    if (!arr) {
        return;
    }
    
    if (arr->data) {
        free(arr->data);
    }
    free(arr);
}

size_t farray_malloc_capacity(farray_malloc arr) {
    return arr ? arr->capacity : 0;
}

int farray_malloc_set(farray_malloc arr, size_t index, const void *elem) {
    if (!arr || !elem || index >= arr->capacity) {
        return -1;
    }
    
    void *dest = (char *)arr->data + (index * arr->elem_size);
    memcpy(dest, elem, arr->elem_size);
    
    return 0;
}

int farray_malloc_get(farray_malloc arr, size_t index, void *out_elem) {
    if (!arr || !out_elem || index >= arr->capacity) {
        return -1;
    }
    
    const void *src = (const char *)arr->data + (index * arr->elem_size);
    memcpy(out_elem, src, arr->elem_size);
    
    return 0;
}

void farray_malloc_clear(farray_malloc arr) {
    if (!arr || !arr->data) {
        return;
    }
    
    memset(arr->data, 0, arr->elem_size * arr->capacity);
}

int farray_malloc_remove(farray_malloc arr, size_t index) {
    if (!arr || index >= arr->capacity) {
        return -1;
    }
    
    void *dest = (char *)arr->data + (index * arr->elem_size);
    memset(dest, 0, arr->elem_size);
    
    return 0;
}
