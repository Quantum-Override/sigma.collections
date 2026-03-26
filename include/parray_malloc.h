/*
 * SigmaCore Collections - Malloc Variant
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * ----------------------------------------------
 * File: parray_malloc.h
 * Description: Standalone malloc-based pointer array (zero dependencies)
 * 
 * NOTE: This is a malloc variant for zero-dependency usage.
 *       For Sigma.* ecosystem code, use sigma.collections.o instead.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Opaque pointer array handle
 */
typedef struct parray_malloc_s *parray_malloc;

/**
 * @brief Create a new pointer array with system malloc
 * 
 * @param capacity Initial capacity (number of pointer slots)
 * @return New parray or NULL on allocation failure
 */
parray_malloc parray_malloc_create(size_t capacity);

/**
 * @brief Dispose of array and free all resources
 * 
 * @param arr Array to dispose (safe to pass NULL)
 */
void parray_malloc_dispose(parray_malloc arr);

/**
 * @brief Get current capacity
 * 
 * @param arr Array to query
 * @return Capacity in number of pointer slots, or 0 if arr is NULL
 */
size_t parray_malloc_capacity(parray_malloc arr);

/**
 * @brief Set pointer at index
 * 
 * @param arr   Array to modify
 * @param index Index to set (must be < capacity)
 * @param ptr   Pointer value to store
 * @return 0 on success, -1 on error (NULL arr, out of bounds)
 */
int parray_malloc_set(parray_malloc arr, size_t index, void *ptr);

/**
 * @brief Get pointer at index
 * 
 * @param arr       Array to query
 * @param index     Index to get (must be < capacity)
 * @param out_ptr   Pointer to receive pointer value
 * @return 0 on success, -1 on error (NULL arr, out of bounds, NULL out_ptr)
 */
int parray_malloc_get(parray_malloc arr, size_t index, void **out_ptr);

/**
 * @brief Clear all pointers (set to NULL)
 * 
 * @param arr Array to clear
 */
void parray_malloc_clear(parray_malloc arr);

/**
 * @brief Remove pointer at index (set to NULL)
 * 
 * @param arr   Array to modify
 * @param index Index to remove (must be < capacity)
 * @return 0 on success, -1 on error
 */
int parray_malloc_remove(parray_malloc arr, size_t index);
