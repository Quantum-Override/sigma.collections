/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: parray.h
 * Description: Standalone malloc-based pointer array (no sigma.memory dependency)
 *
 * NOTE: This is a malloc variant that bypasses sigma.memory/Allocator.
 *       For normal Sigma.* ecosystem code, use sigma.collections.o instead.
 */

#pragma once

#include <sigma.core/types.h>

// Forward declaration of the pointer array structure
struct sc_pointer_array_malloc;
typedef struct sc_pointer_array_malloc *parray;

/* Public interface for malloc-based pointer array operations   */
/* ============================================================ */
typedef struct sc_parray_i {
    /**
     * @brief Initialize a new array with the specified initial capacity.
     * @param capacity Initial array capacity
     */
    parray (*new)(usize);
    /**
     * @brief Initialize an array with the specified capacity.
     * @param arr The array to initialize
     * @param capacity Initial array capacity
     */
    void (*init)(parray *, usize);
    /**
     * @brief Dispose of the array and free associated resources.
     * @param arr The array to dispose of
     */
    void (*dispose)(parray);
    /**
     * @brief Get the current capacity of the array.
     * @param arr The array to query
     * @return Current capacity of the array
     */
    int (*capacity)(parray);
    /**
     * @brief Clear the contents of the array.
     * @param arr The array to clear
     */
    void (*clear)(parray);
    /**
     * @brief Set the value at the specified index in the array.
     * @param arr The array to modify
     * @param index Index at which to set the value
     * @param value Value to set
     * @return 0 on OK; otherwise non-zero
     */
    int (*set)(parray, usize, addr);
    /**
     * @brief Get the value at the specified index in the array.
     * @param arr The array to query
     * @param index Index from which to get the value
     * @param out_value Pointer to store the retrieved value
     * @return 0 on OK; otherwise non-zero
     */
    int (*get)(parray, usize, addr *);
    /**
     * @brief Remove the element at the specified index, setting it to empty without shifting.
     * @param arr The array to modify
     * @param index Index of the element to remove
     * @return 0 on OK; otherwise non-zero
     */
    int (*remove)(parray, usize);
} sc_parray_i;

extern const sc_parray_i PArray;
