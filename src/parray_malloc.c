/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: parray_malloc.c
 * Description: Standalone malloc-based pointer array implementation
 */

#include "parray_malloc.h"
#include <stdlib.h>
#include <string.h>

struct parray_malloc_s {
    size_t capacity;   // Number of pointer slots allocated
    void **data;       // Pointer to pointer storage
};

parray_malloc parray_malloc_create(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    
    // Allocate struct
    parray_malloc arr = (parray_malloc)malloc(sizeof(struct parray_malloc_s));
    if (!arr) {
        return NULL;
    }
    
    // Allocate pointer array
    size_t total_size = sizeof(void *) * capacity;
    arr->data = (void **)malloc(total_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    
    arr->capacity = capacity;
    
    // Initialize all pointers to NULL
    memset(arr->data, 0, total_size);
    
    return arr;
}

void parray_malloc_dispose(parray_malloc arr) {
    if (!arr) {
        return;
    }
    
    if (arr->data) {
        free(arr->data);
    }
    free(arr);
}

size_t parray_malloc_capacity(parray_malloc arr) {
    return arr ? arr->capacity : 0;
}

int parray_malloc_set(parray_malloc arr, size_t index, void *ptr) {
    if (!arr || index >= arr->capacity) {
        return -1;
    }
    
    arr->data[index] = ptr;
    
    return 0;
}

int parray_malloc_get(parray_malloc arr, size_t index, void **out_ptr) {
    if (!arr || !out_ptr || index >= arr->capacity) {
        return -1;
    }
    
    *out_ptr = arr->data[index];
    
    return 0;
}

void parray_malloc_clear(parray_malloc arr) {
    if (!arr || !arr->data) {
        return;
    }
    
    memset(arr->data, 0, sizeof(void *) * arr->capacity);
}

int parray_malloc_remove(parray_malloc arr, size_t index) {
    if (!arr || index >= arr->capacity) {
        return -1;
    }
    
    arr->data[index] = NULL;
    
    return 0;
}
