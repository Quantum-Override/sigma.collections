/*
 * Sigma Collections
 * Copyright (c) 2026 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ----------------------------------------------
 * File: indexarray.h
 * Description: Header file for Sigma Collections indexarray definitions and interfaces
 *
 * IndexArray:  A sparse collection structure that stores fixed-size structs/values
 *              directly in contiguous memory, allowing element insertion, removal,
 *              and retrieval by index. Supports slot reuse and dynamic growth.
 *              Unlike SlotArray (pointer-based), IndexArray stores values inline
 *              for better cache performance with small structs.
 */
#pragma once

#include "farray.h"
#include "collection.h"

struct sc_indexarray;
typedef struct sc_indexarray *indexarray;

// forward declaration for sparse_iterator
struct sparse_iterator_s;
typedef struct sparse_iterator_s *sparse_iterator;

/* Public interface for indexarray operations                   */
/* ============================================================ */
typedef struct sc_indexarray_i {
    /**
     * @brief Create a new IndexArray with the specified initial capacity and element stride.
     * @param capacity The initial number of slots to allocate.
     * @param stride The size of each element (struct size).
     * @return A pointer to the newly created IndexArray, or NULL on failure.
     */
    indexarray (*new)(usize capacity, usize stride);
    
    /**
     * @brief Dispose of the IndexArray and free its resources.
     * @param ia The IndexArray to dispose.
     */
    void (*dispose)(indexarray ia);
    
    /**
     * @brief Add a value to the IndexArray, reusing empty slots if available or growing if needed.
     * @param ia The IndexArray to add the value to.
     * @param value Pointer to the value to add (will be copied).
     * @return The index (handle) where the value was added; otherwise -1.
     */
    int (*add)(indexarray ia, object value);
    
    /**
     * @brief Retrieve the value at the specified index (handle) in the IndexArray.
     * @param ia The IndexArray to retrieve the value from.
     * @param index The index (handle) of the value to retrieve.
     * @param out_value Pointer to store the retrieved value (must be at least stride bytes).
     * @return 0 on OK; otherwise non-zero
     */
    int (*get_at)(indexarray ia, usize index, object out_value);
    
    /**
     * @brief Remove the element at the specified index (handle) from the IndexArray.
     * @param ia The IndexArray to remove the element from.
     * @param index The index (handle) of the element to remove.
     * @return 0 on OK; otherwise non-zero
     */
    int (*remove_at)(indexarray ia, usize index);
    
    /**
     * @brief Create an IndexArray from a flex array, copying all elements.
     * @param arr The flex array to copy from.
     * @param stride The size of each element.
     * @return A new IndexArray with the elements, or NULL on failure.
     */
    indexarray (*from_farray)(farray arr, usize stride);
    
    /**
     * @brief Create a non-owning IndexArray view from a raw buffer range.
     * @param buffer Pointer to the start of the buffer.
     * @param end Pointer to one past the end of the buffer.
     * @param stride The size of each element (struct size).
     * @return A new IndexArray view, or NULL on failure.
     * @note The IndexArray does not own the buffer - caller must manage lifetime.
     */
    indexarray (*from_buffer)(void *buffer, void *end, usize stride);
    
    // Introspection
    /**
     * @brief Check if a slot is empty.
     * @param ia The IndexArray to check.
     * @param index The index to check.
     * @return true if slot is empty, false otherwise.
     */
    bool (*is_empty_slot)(indexarray ia, usize index);
    
    /**
     * @brief Get the total capacity (number of slots).
     * @param ia The IndexArray to query.
     * @return The capacity.
     */
    usize (*capacity)(indexarray ia);
    
    /**
     * @brief Get the stride (element size).
     * @param ia The IndexArray to query.
     * @return The stride in bytes.
     */
    usize (*stride)(indexarray ia);
    
    /**
     * @brief Clear all slots in the IndexArray.
     * @param ia The IndexArray to clear.
     */
    void (*clear)(indexarray ia);
    
    /**
     * @brief Create a sparse iterator for the indexarray
     * @param ia The indexarray to iterate over
     * @return New sparse iterator, or NULL on failure
     */
    sparse_iterator (*create_iterator)(indexarray ia);
} sc_indexarray_i;

extern const sc_indexarray_i IndexArray;
