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
 * File: indexarray.c
 * Description: Implementation of IndexArray - sparse collection for fixed-size values
 */

#include "indexarray.h"
#include <sigma.core/alloc.h>
#include <string.h>
#include "internal/collections.h"

// IndexArray struct: uses collection internally
struct sc_indexarray {
    collection coll;  // underlying collection (handles stride, growth, ownership)
    usize next_slot;  // next slot to check for reuse
};

// Forward declarations
static indexarray indexarray_new(usize capacity, usize stride);
static void indexarray_dispose(indexarray ia);
static int indexarray_add(indexarray ia, object value);
static int indexarray_get_at(indexarray ia, usize index, object out_value);
static int indexarray_remove_at(indexarray ia, usize index);
static indexarray indexarray_from_farray(farray arr, usize stride);
static indexarray indexarray_from_buffer(void *buffer, void *end, usize stride);
static bool indexarray_is_empty_slot(indexarray ia, usize index);
static usize indexarray_capacity(indexarray ia);
static usize indexarray_stride(indexarray ia);
static void indexarray_clear(indexarray ia);
static sparse_iterator indexarray_create_iterator(indexarray ia);

// Helper: check if a slot is empty (all zeros)
static bool is_slot_empty(object slot_ptr, usize stride) {
    byte *bytes = (byte *)slot_ptr;
    for (usize i = 0; i < stride; i++) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

// Helper: zero out a slot
static void zero_slot(object slot_ptr, usize stride) { memset(slot_ptr, 0, stride); }

// Create new indexarray with specified capacity and stride
static indexarray indexarray_new(usize capacity, usize stride) {
    indexarray ia = NULL;

    if (stride == 0) {
        goto exit;
    }

    ia = Allocator.alloc(sizeof(struct sc_indexarray));
    if (!ia) {
        goto exit;
    }

    // Create underlying collection with stride
    ia->coll = collection_new(capacity, stride);
    if (!ia->coll) {
        goto cleanup;
    }

    // Zero out all slots initially
    void *buffer = collection_get_buffer(ia->coll);
    if (buffer) {
        memset(buffer, 0, capacity * stride);
    }

    ia->next_slot = 0;
    return ia;

cleanup:
    Allocator.dispose(ia);
    ia = NULL;

exit:
    return ia;
}

// Dispose of the indexarray
static void indexarray_dispose(indexarray ia) {
    if (!ia) {
        return;
    }
    collection_dispose(ia->coll);
    Allocator.dispose(ia);
}

// Add a value to the indexarray
static int indexarray_add(indexarray ia, object value) {
    if (!ia || !value) {
        return ERR;
    }

    usize stride = collection_get_stride(ia->coll);
    usize capacity = indexarray_capacity(ia);
    void *buffer = collection_get_buffer(ia->coll);

    // Try to find an empty slot starting from next_slot
    for (usize i = 0; i < capacity; i++) {
        usize slot_index = (ia->next_slot + i) % capacity;
        void *slot = (char *)buffer + slot_index * stride;

        if (is_slot_empty(slot, stride)) {
            // Found empty slot, copy value
            memcpy(slot, value, stride);
            ia->next_slot = (slot_index + 1) % capacity;
            return (int)slot_index;
        }
    }

    // No empty slot found - need to grow
    if (collection_grow(ia->coll) != OK) {
        return ERR;
    }

    // After growth, buffer and capacity change
    buffer = collection_get_buffer(ia->coll);
    capacity = indexarray_capacity(ia);

    // Zero out new space
    void *new_space = (char *)buffer + (capacity / 2) * stride;
    memset(new_space, 0, (capacity / 2) * stride);

    // Add to first slot in new space
    void *slot = (char *)buffer + (capacity / 2) * stride;
    memcpy(slot, value, stride);
    ia->next_slot = (capacity / 2) + 1;

    return (int)(capacity / 2);
}

// Get value at index
static int indexarray_get_at(indexarray ia, usize index, object out_value) {
    if (!ia || !out_value) {
        return ERR;
    }

    usize capacity = indexarray_capacity(ia);
    if (index >= capacity) {
        return ERR;
    }

    usize stride = collection_get_stride(ia->coll);
    void *buffer = collection_get_buffer(ia->coll);
    void *slot = (char *)buffer + index * stride;

    if (is_slot_empty(slot, stride)) {
        return ERR;
    }

    memcpy(out_value, slot, stride);
    return OK;
}

// Remove element at index
static int indexarray_remove_at(indexarray ia, usize index) {
    if (!ia) {
        return ERR;
    }

    usize capacity = indexarray_capacity(ia);
    if (index >= capacity) {
        return ERR;
    }

    usize stride = collection_get_stride(ia->coll);
    void *buffer = collection_get_buffer(ia->coll);
    void *slot = (char *)buffer + index * stride;

    zero_slot(slot, stride);
    return OK;
}

// Create from farray
static indexarray indexarray_from_farray(farray arr, usize stride) {
    if (!arr) {
        return NULL;
    }

    usize cap = FArray.capacity(arr, stride);
    indexarray ia = IndexArray.new(cap, stride);
    if (!ia) {
        return NULL;
    }

    // Copy non-empty elements
    for (usize i = 0; i < cap; i++) {
        void *value = Allocator.alloc(stride);
        if (!value) {
            IndexArray.dispose(ia);
            return NULL;
        }

        if (FArray.get(arr, i, stride, value) == OK) {
            if (!is_slot_empty(value, stride)) {
                IndexArray.add(ia, value);
            }
        }
        Allocator.dispose(value);
    }

    return ia;
}

// Create from raw buffer range (non-owning view)
static indexarray indexarray_from_buffer(void *buffer, void *end, usize stride) {
    indexarray ia = NULL;

    if (!buffer || !end || stride == 0 || buffer >= end) {
        goto exit;
    }

    ia = Allocator.alloc(sizeof(struct sc_indexarray));
    if (!ia) {
        goto exit;
    }

    // Create a non-owning collection view of the buffer
    ia->coll = Allocator.alloc(sizeof(struct sc_collection));
    if (!ia->coll) {
        goto cleanup;
    }

    ia->coll->array.handle[0] = 'F';
    ia->coll->array.handle[1] = '\0';
    ia->coll->array.bucket = buffer;
    ia->coll->array.end = end;
    ia->coll->stride = stride;
    ia->coll->length = 0;           // Length not used for sparse arrays
    ia->coll->owns_buffer = false;  // Non-owning view

    ia->next_slot = 0;
    return ia;

cleanup:
    Allocator.dispose(ia);
    ia = NULL;

exit:
    return ia;
}

// Check if slot is empty
static bool indexarray_is_empty_slot(indexarray ia, usize index) {
    if (!ia) {
        return true;
    }

    usize capacity = indexarray_capacity(ia);
    if (index >= capacity) {
        return true;
    }

    usize stride = collection_get_stride(ia->coll);
    void *buffer = collection_get_buffer(ia->coll);
    void *slot = (char *)buffer + index * stride;

    return is_slot_empty(slot, stride);
}

// Get capacity
static usize indexarray_capacity(indexarray ia) {
    if (!ia) {
        return 0;
    }
    void *buffer = collection_get_buffer(ia->coll);
    void *end = collection_get_end(ia->coll);
    usize stride = collection_get_stride(ia->coll);

    if (!buffer || !end || stride == 0) {
        return 0;
    }

    return ((char *)end - (char *)buffer) / stride;
}

// Get stride
static usize indexarray_stride(indexarray ia) {
    if (!ia) {
        return 0;
    }
    return collection_get_stride(ia->coll);
}

// Clear all slots
static void indexarray_clear(indexarray ia) {
    if (!ia) {
        return;
    }

    usize capacity = indexarray_capacity(ia);
    usize stride = collection_get_stride(ia->coll);
    void *buffer = collection_get_buffer(ia->coll);

    if (buffer) {
        memset(buffer, 0, capacity * stride);
    }
    ia->next_slot = 0;
}

// Internal ops table for sparse iterator interface
static const sc_sparse_i indexarray_sparse_ops = {
    .is_empty_slot = (bool (*)(object, usize))indexarray_is_empty_slot,
    .capacity = (usize (*)(object))indexarray_capacity,
    .get_at = (int (*)(object, usize, object *))indexarray_get_at};

// Create sparse iterator
static sparse_iterator indexarray_create_iterator(indexarray ia) {
    if (!ia) {
        return NULL;
    }
    return sparse_iterator_new(ia, &indexarray_sparse_ops);
}

// Public interface implementation
const sc_indexarray_i IndexArray = {
    .new = indexarray_new,
    .dispose = indexarray_dispose,
    .add = indexarray_add,
    .get_at = indexarray_get_at,
    .remove_at = indexarray_remove_at,
    .from_farray = indexarray_from_farray,
    .from_buffer = indexarray_from_buffer,
    .is_empty_slot = indexarray_is_empty_slot,
    .capacity = indexarray_capacity,
    .stride = indexarray_stride,
    .clear = indexarray_clear,
    .create_iterator = indexarray_create_iterator,
};
