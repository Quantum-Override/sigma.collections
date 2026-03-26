/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: farray_malloc.h
 * Description: Standalone malloc-based flexible array (zero dependencies)
 * 
 * NOTE: This is a malloc variant for zero-dependency usage.
 *       For Sigma.* ecosystem code, use sigma.collections.o instead.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Opaque flexible array handle
 */
typedef struct farray_malloc_s *farray_malloc;

/**
 * @brief Create a new flexible array with system malloc
 * 
 * @param elem_size Size of each element in bytes
 * @param capacity  Initial capacity (number of elements)
 * @return New farray or NULL on allocation failure
 */
farray_malloc farray_malloc_create(size_t elem_size, size_t capacity);

/**
 * @brief Dispose of array and free all resources
 * 
 * @param arr Array to dispose (safe to pass NULL)
 */
void farray_malloc_dispose(farray_malloc arr);

/**
 * @brief Get current capacity
 * 
 * @param arr Array to query
 * @return Capacity in number of elements, or 0 if arr is NULL
 */
size_t farray_malloc_capacity(farray_malloc arr);

/**
 * @brief Set element at index
 * 
 * @param arr   Array to modify
 * @param index Index to set (must be < capacity)
 * @param elem  Pointer to element data (will be memcpy'd)
 * @return 0 on success, -1 on error (NULL arr, out of bounds, NULL elem)
 */
int farray_malloc_set(farray_malloc arr, size_t index, const void *elem);

/**
 * @brief Get element at index
 * 
 * @param arr       Array to query
 * @param index     Index to get (must be < capacity)
 * @param out_elem  Pointer to receive element data (will be memcpy'd)
 * @return 0 on success, -1 on error (NULL arr, out of bounds, NULL out_elem)
 */
int farray_malloc_get(farray_malloc arr, size_t index, void *out_elem);

/**
 * @brief Clear all elements (zero memory)
 * 
 * @param arr Array to clear
 */
void farray_malloc_clear(farray_malloc arr);

/**
 * @brief Remove element at index (zero memory at that slot)
 * 
 * @param arr   Array to modify
 * @param index Index to remove (must be < capacity)
 * @return 0 on success, -1 on error
 */
int farray_malloc_remove(farray_malloc arr, size_t index);
