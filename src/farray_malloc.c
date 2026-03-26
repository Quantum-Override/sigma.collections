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

struct sc_flex_array_malloc {
    usize elem_size;  // Size of each element in bytes
    usize capacity;   // Number of elements allocated
    object data;      // Pointer to element storage
};

// Forward declarations
static farray_malloc farray_malloc_new(usize capacity, usize stride);
static void farray_malloc_init_fn(farray_malloc *arr, usize capacity, usize stride);
static void farray_malloc_dispose_fn(farray_malloc arr);
static int farray_malloc_capacity_fn(farray_malloc arr, usize stride);
static void farray_malloc_clear_fn(farray_malloc arr, usize stride);
static int farray_malloc_set_fn(farray_malloc arr, usize index, usize stride, object elem);
static int farray_malloc_get_fn(farray_malloc arr, usize index, usize stride, object out_elem);
static int farray_malloc_remove_fn(farray_malloc arr, usize index, usize stride);

// API implementations

static farray_malloc farray_malloc_new(usize capacity, usize stride) {
    if (stride == 0 || capacity == 0) {
        return NULL;
    }

    // Allocate struct
    farray_malloc arr = (farray_malloc)malloc(sizeof(struct sc_flex_array_malloc));
    if (!arr) {
        return NULL;
    }

    // Allocate data buffer
    usize total_size = stride * capacity;
    arr->data = malloc(total_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }

    arr->elem_size = stride;
    arr->capacity = capacity;

    // Zero initialize
    memset(arr->data, 0, total_size);

    return arr;
}

static void farray_malloc_init_fn(farray_malloc *arr, usize capacity, usize stride) {
    if (!arr) {
        return;
    }
    *arr = farray_malloc_new(capacity, stride);
}

static void farray_malloc_dispose_fn(farray_malloc arr) {
    if (!arr) {
        return;
    }

    if (arr->data) {
        free(arr->data);
    }
    free(arr);
}

static int farray_malloc_capacity_fn(farray_malloc arr, usize stride) {
    (void)stride;  // Stored in struct, parameter for ABI compatibility
    return arr ? (int)arr->capacity : 0;
}

static int farray_malloc_set_fn(farray_malloc arr, usize index, usize stride, object elem) {
    if (!arr || !elem || index >= arr->capacity) {
        return ERR;
    }

    // Use stored elem_size, but stride provided for ABI compatibility
    (void)stride;
    object dest = (char *)arr->data + (index * arr->elem_size);
    memcpy(dest, elem, arr->elem_size);

    return OK;
}

static int farray_malloc_get_fn(farray_malloc arr, usize index, usize stride, object out_elem) {
    if (!arr || !out_elem || index >= arr->capacity) {
        return ERR;
    }

    // Use stored elem_size, but stride provided for ABI compatibility
    (void)stride;
    object src = (char *)arr->data + (index * arr->elem_size);
    memcpy(out_elem, src, arr->elem_size);

    return OK;
}

static void farray_malloc_clear_fn(farray_malloc arr, usize stride) {
    if (!arr || !arr->data) {
        return;
    }

    // Use stored elem_size, but stride provided for ABI compatibility
    (void)stride;
    memset(arr->data, 0, arr->elem_size * arr->capacity);
}

static int farray_malloc_remove_fn(farray_malloc arr, usize index, usize stride) {
    if (!arr || index >= arr->capacity) {
        return ERR;
    }

    // Use stored elem_size, but stride provided for ABI compatibility
    (void)stride;
    object dest = (char *)arr->data + (index * arr->elem_size);
    memset(dest, 0, arr->elem_size);

    return OK;
}

// Global interface instance
const sc_farray_malloc_i FArrayMalloc = {
    .new = farray_malloc_new,
    .init = farray_malloc_init_fn,
    .dispose = farray_malloc_dispose_fn,
    .capacity = farray_malloc_capacity_fn,
    .clear = farray_malloc_clear_fn,
    .set = farray_malloc_set_fn,
    .get = farray_malloc_get_fn,
    .remove = farray_malloc_remove_fn,
};
