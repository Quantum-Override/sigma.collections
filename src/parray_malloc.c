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

struct sc_pointer_array_malloc {
    usize capacity;  // Number of pointer slots allocated
    addr *data;      // Pointer to pointer storage
};

// Forward declarations
static parray_malloc parray_malloc_new(usize capacity);
static void parray_malloc_init_fn(parray_malloc *arr, usize capacity);
static void parray_malloc_dispose_fn(parray_malloc arr);
static int parray_malloc_capacity_fn(parray_malloc arr);
static void parray_malloc_clear_fn(parray_malloc arr);
static int parray_malloc_set_fn(parray_malloc arr, usize index, addr value);
static int parray_malloc_get_fn(parray_malloc arr, usize index, addr *out_value);
static int parray_malloc_remove_fn(parray_malloc arr, usize index);

// API implementations

static parray_malloc parray_malloc_new(usize capacity) {
    if (capacity == 0) {
        return NULL;
    }

    // Allocate struct
    parray_malloc arr = (parray_malloc)malloc(sizeof(struct sc_pointer_array_malloc));
    if (!arr) {
        return NULL;
    }

    // Allocate pointer array
    usize total_size = sizeof(addr) * capacity;
    arr->data = (addr *)malloc(total_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }

    arr->capacity = capacity;

    // Initialize all pointers to NULL
    memset(arr->data, 0, total_size);

    return arr;
}

static void parray_malloc_init_fn(parray_malloc *arr, usize capacity) {
    if (!arr) {
        return;
    }
    *arr = parray_malloc_new(capacity);
}

static void parray_malloc_dispose_fn(parray_malloc arr) {
    if (!arr) {
        return;
    }

    if (arr->data) {
        free(arr->data);
    }
    free(arr);
}

static int parray_malloc_capacity_fn(parray_malloc arr) {
    return arr ? (int)arr->capacity : 0;
}

static int parray_malloc_set_fn(parray_malloc arr, usize index, addr value) {
    if (!arr || index >= arr->capacity) {
        return ERR;
    }

    arr->data[index] = value;

    return OK;
}

static int parray_malloc_get_fn(parray_malloc arr, usize index, addr *out_value) {
    if (!arr || !out_value || index >= arr->capacity) {
        return ERR;
    }

    *out_value = arr->data[index];

    return OK;
}

static void parray_malloc_clear_fn(parray_malloc arr) {
    if (!arr || !arr->data) {
        return;
    }

    memset(arr->data, 0, sizeof(addr) * arr->capacity);
}

static int parray_malloc_remove_fn(parray_malloc arr, usize index) {
    if (!arr || index >= arr->capacity) {
        return ERR;
    }

    arr->data[index] = (addr)NULL;

    return OK;
}

// Global interface instance
const sc_parray_malloc_i PArrayMalloc = {
    .new = parray_malloc_new,
    .init = parray_malloc_init_fn,
    .dispose = parray_malloc_dispose_fn,
    .capacity = parray_malloc_capacity_fn,
    .clear = parray_malloc_clear_fn,
    .set = parray_malloc_set_fn,
    .get = parray_malloc_get_fn,
    .remove = parray_malloc_remove_fn,
};
