/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: farray_malloc.h
 * Description: Standalone malloc-based flexible array (no sigma.memory dependency)
 *
 * NOTE: This is a malloc variant that bypasses sigma.memory/Allocator.
 *       For normal Sigma.* ecosystem code, use sigma.collections.o instead.
 */

#pragma once

#include <sigma.core/types.h>

// Forward declaration of the flex array structure
struct sc_flex_array_malloc;
typedef struct sc_flex_array_malloc *farray;

/* Public interface for malloc-based array operations           */
/* ============================================================ */
typedef struct sc_farray_malloc_i {
    /**
     * @brief Initialize a new array with the specified initial capacity.
     * @param capacity Initial array capacity
     * @param stride Size of each element in the array
     */
    farray (*new)(usize, usize);
    /**
     * @brief Initialize an array with the specified capacity.
     * @param arr The array to initialize
     * @param capacity Initial array capacity
     * @param stride Size of each element in the array
     */
    void (*init)(farray *, usize, usize);
    /**
     * @brief Dispose of the array and free associated resources.
     * @param arr The array to dispose of
     */
    void (*dispose)(farray);
    /**
     * @brief Get the current capacity of the array.
     * @param arr The array to query
     * @param stride Size of each element in the array
     * @return Current capacity of the array
     */
    int (*capacity)(farray, usize);
    /**
     * @brief Clear the contents of the array.
     * @param arr The array to clear
     * @param stride Size of each element in the array
     */
    void (*clear)(farray, usize);
    /**
     * @brief Set the value at the specified index in the array.
     * @param arr The array to modify
     * @param index Index at which to set the value
     * @param stride Size of each element in the array
     * @param value Pointer to the value to set (must be at least stride bytes)
     * @return 0 on OK; otherwise non-zero
     */
    int (*set)(farray, usize, usize, object);
    /**
     * @brief Get the value at the specified index in the array.
     * @param arr The array to query
     * @param index Index from which to get the value
     * @param stride Size of each element in the array
     * @param out_value Pointer to store the retrieved value (must be at least stride bytes)
     * @return 0 on OK; otherwise non-zero
     */
    int (*get)(farray, usize, usize, object);
    /**
     * @brief Remove the element at the specified index, setting it to empty without shifting.
     * @param arr The array to modify
     * @param index Index of the element to remove
     * @param stride Size of each element in the array
     * @return 0 on OK; otherwise non-zero
     */
    int (*remove)(farray, usize, usize);
} sc_farray_malloc_i;

extern const sc_farray_malloc_i FArray;
