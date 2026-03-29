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
static parray parray_new(usize capacity);
static void parray_init_fn(parray *arr, usize capacity);
static void parray_dispose_fn(parray arr);
static int parray_capacity_fn(parray arr);
static void parray_clear_fn(parray arr);
static int parray_set_fn(parray arr, usize index, addr value);
static int parray_get_fn(parray arr, usize index, addr *out_value);
static int parray_remove_fn(parray arr, usize index);

// API implementations

static parray parray_new(usize capacity) {
    if (capacity == 0) {
        return NULL;
    }

    // Allocate struct
    parray arr = (parray)malloc(sizeof(struct sc_pointer_array_malloc));
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

static void parray_init_fn(parray *arr, usize capacity) {
    if (!arr) {
        return;
    }
    *arr = parray_new(capacity);
}

static void parray_dispose_fn(parray arr) {
    if (!arr) {
        return;
    }

    if (arr->data) {
        free(arr->data);
    }
    free(arr);
}

static int parray_capacity_fn(parray arr) { return arr ? (int)arr->capacity : 0; }

static int parray_set_fn(parray arr, usize index, addr value) {
    if (!arr || index >= arr->capacity) {
        return ERR;
    }

    arr->data[index] = value;

    return OK;
}

static int parray_get_fn(parray arr, usize index, addr *out_value) {
    if (!arr || !out_value || index >= arr->capacity) {
        return ERR;
    }

    *out_value = arr->data[index];

    return OK;
}

static void parray_clear_fn(parray arr) {
    if (!arr || !arr->data) {
        return;
    }

    memset(arr->data, 0, sizeof(addr) * arr->capacity);
}

static int parray_remove_fn(parray arr, usize index) {
    if (!arr || index >= arr->capacity) {
        return ERR;
    }

    arr->data[index] = (addr)NULL;

    return OK;
}

// Global interface instance
const sc_parray_i PArray = {
    .new = parray_new,
    .init = parray_init_fn,
    .dispose = parray_dispose_fn,
    .capacity = parray_capacity_fn,
    .clear = parray_clear_fn,
    .set = parray_set_fn,
    .get = parray_get_fn,
    .remove = parray_remove_fn,
};
