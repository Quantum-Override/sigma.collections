/*
 * SigmaCore
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * ----------------------------------------------
 * File: map.c
 * Description: String-keyed hash map implementation with FNV-1a hashing
 */

#include "map.h"
#include "farray.h"
#include "internal/arrays.h"
#include <string.h>
#include <stdlib.h>

// FNV-1a 64-bit hash constants
#define FNV1A_OFFSET UINT64_C(14695981039346656037)
#define FNV1A_PRIME  UINT64_C(1099511628211)

// Load factor threshold: resize when count / capacity > 0.5
#define LOAD_FACTOR_THRESHOLD 0.5

// Forward declarations - API functions
static map map_new(usize capacity);
static void map_init(map *m, usize capacity);
static void map_dispose(map m);
static int map_set(map m, const char *key, usize len, usize val);
static int map_get(map m, const char *key, usize len, usize *out_val);
static int map_has(map m, const char *key, usize len);
static int map_remove(map m, const char *key, usize len);
static usize map_count(map m);
static usize map_capacity(map m);
static void map_alloc_use(sc_alloc_use_t *use);

// Forward declarations - helper functions
static uint64_t fnv1a_hash(const char *data, usize len);
static usize next_power_of_two(usize n);
static int map_resize(map m, usize new_capacity);
static int map_find_slot(map m, const char *key, usize len, uint64_t hash, usize *out_idx);

/**
 * @brief Map bucket entry
 */
typedef struct {
    uint64_t hash;      // FNV-1a hash; 0 = empty slot, 1 = tombstone
    const char *key;    // Key pointer (caller-owned)
    usize key_len;      // Key length in bytes
    usize value;        // Stored value
} map_bucket;

/**
 * @brief Map structure
 */
struct sc_map_s {
    farray buckets;     // FArray of map_bucket structs
    usize count;        // Number of occupied slots (excludes tombstones)
    usize capacity;     // Current bucket capacity (cached from FArray)
};

// API interface definition
const sc_map_i Map = {
    .new      = map_new,
    .init     = map_init,
    .dispose  = map_dispose,
    .set      = map_set,
    .get      = map_get,
    .has      = map_has,
    .remove   = map_remove,
    .count    = map_count,
    .capacity = map_capacity,
    .alloc_use = map_alloc_use,
};

// Helper/utility function definitions

/**
 * @brief FNV-1a 64-bit hash function
 */
static uint64_t fnv1a_hash(const char *data, usize len) {
    uint64_t hash = FNV1A_OFFSET;
    for (usize i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= FNV1A_PRIME;
    }
    // Ensure hash is never 0 or 1 (reserved for empty/tombstone)
    if (hash == 0 || hash == 1) {
        hash = 2;
    }
    return hash;
}

/**
 * @brief Round up to next power of 2
 */
static usize next_power_of_two(usize n) {
    if (n == 0) return 1;
    if ((n & (n - 1)) == 0) return n;  // Already power of 2
    
    usize power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

/**
 * @brief Find slot for key (for get/set/remove operations)
 * @param m The map
 * @param key Key bytes
 * @param len Key length
 * @param hash Pre-computed FNV-1a hash
 * @param out_idx Receives the slot index
 * @return 1 if key found; 0 if not found (out_idx = first empty/tombstone)
 */
static int map_find_slot(map m, const char *key, usize len, uint64_t hash, usize *out_idx) {
    usize stride = sizeof(map_bucket);
    usize mask = m->capacity - 1;
    usize idx = hash & mask;
    usize first_tombstone = m->capacity;  // Invalid index
    
    for (usize probe = 0; probe < m->capacity; probe++) {
        map_bucket bucket;
        FArray.get(m->buckets, idx, stride, &bucket);
        
        // Empty slot - key not found
        if (bucket.hash == 0) {
            // Return tombstone if we saw one, else this empty slot
            *out_idx = (first_tombstone < m->capacity) ? first_tombstone : idx;
            return 0;
        }
        
        // Tombstone - remember first one for insertion
        if (bucket.hash == 1) {
            if (first_tombstone >= m->capacity) {
                first_tombstone = idx;
            }
            idx = (idx + 1) & mask;
            continue;
        }
        
        // Occupied slot - check if it matches our key
        if (bucket.hash == hash && bucket.key_len == len && 
            memcmp(bucket.key, key, len) == 0) {
            *out_idx = idx;
            return 1;  // Found
        }
        
        idx = (idx + 1) & mask;
    }
    
    // Table full - return tombstone if we saw one
    *out_idx = (first_tombstone < m->capacity) ? first_tombstone : 0;
    return 0;
}

/**
 * @brief Resize map to new capacity
 * @param m The map
 * @param new_capacity New bucket count (must be power of 2)
 * @return 0 on success; -1 on allocation failure
 */
static int map_resize(map m, usize new_capacity) {
    usize stride = sizeof(map_bucket);
    farray old_buckets = m->buckets;
    usize old_capacity = m->capacity;
    
    // Create new bucket array
    m->buckets = FArray.new(new_capacity, stride);
    if (!m->buckets) {
        m->buckets = old_buckets;  // Restore on failure
        return ERR;
    }
    
    m->capacity = new_capacity;
    m->count = 0;
    
    // Clear new buckets (all zeros = empty)
    map_bucket empty = {0};
    for (usize i = 0; i < new_capacity; i++) {
        FArray.set(m->buckets, i, stride, &empty);
    }
    
    // Rehash all entries from old table
    for (usize i = 0; i < old_capacity; i++) {
        map_bucket bucket;
        FArray.get(old_buckets, i, stride, &bucket);
        
        // Skip empty and tombstone slots
        if (bucket.hash > 1) {
            // Re-insert into new table
            usize idx;
            map_find_slot(m, bucket.key, bucket.key_len, bucket.hash, &idx);
            FArray.set(m->buckets, idx, stride, &bucket);
            m->count++;
        }
    }
    
    // Dispose old bucket array
    FArray.dispose(old_buckets);
    return OK;
}

// API function definitions

/**
 * @brief Create a new map
 */
static map map_new(usize capacity) {
    map m = coll_alloc(sizeof(struct sc_map_s));
    if (!m) {
        return NULL;
    }
    
    map_init(&m, capacity);
    return m;
}

/**
 * @brief Initialize map in-place
 */
static void map_init(map *m_ptr, usize capacity) {
    map m = *m_ptr;
    usize stride = sizeof(map_bucket);
    
    // Round up to power of 2, minimum 8
    capacity = next_power_of_two(capacity);
    if (capacity < 8) {
        capacity = 8;
    }
    
    m->buckets = FArray.new(capacity, stride);
    m->count = 0;
    m->capacity = capacity;
    
    // Clear all buckets (hash = 0 means empty)
    map_bucket empty = {0};
    for (usize i = 0; i < capacity; i++) {
        FArray.set(m->buckets, i, stride, &empty);
    }
}

/**
 * @brief Dispose of map
 */
static void map_dispose(map m) {
    if (!m) {
        return;
    }
    
    if (m->buckets) {
        FArray.dispose(m->buckets);
    }
    
    coll_free(m);
}

/**
 * @brief Insert or update entry
 */
static int map_set(map m, const char *key, usize len, usize val) {
    if (!m || !key) {
        return ERR;
    }
    
    usize stride = sizeof(map_bucket);
    uint64_t hash = fnv1a_hash(key, len);
    
    // Check if we need to resize
    double load = (double)(m->count + 1) / m->capacity;
    if (load > LOAD_FACTOR_THRESHOLD) {
        if (map_resize(m, m->capacity * 2) != OK) {
            return ERR;
        }
    }
    
    // Find insertion slot
    usize idx;
    int found = map_find_slot(m, key, len, hash, &idx);
    
    map_bucket bucket = {
        .hash = hash,
        .key = key,
        .key_len = len,
        .value = val
    };
    
    FArray.set(m->buckets, idx, stride, &bucket);
    
    // Only increment count if this is a new entry
    if (!found) {
        m->count++;
    }
    
    return OK;
}

/**
 * @brief Look up entry
 */
static int map_get(map m, const char *key, usize len, usize *out_val) {
    if (!m || !key || !out_val) {
        return 0;
    }
    
    usize stride = sizeof(map_bucket);
    uint64_t hash = fnv1a_hash(key, len);
    
    usize idx;
    if (map_find_slot(m, key, len, hash, &idx)) {
        map_bucket bucket;
        FArray.get(m->buckets, idx, stride, &bucket);
        *out_val = bucket.value;
        return 1;
    }
    
    return 0;
}

/**
 * @brief Check if key exists
 */
static int map_has(map m, const char *key, usize len) {
    if (!m || !key) {
        return 0;
    }
    
    uint64_t hash = fnv1a_hash(key, len);
    usize idx;
    return map_find_slot(m, key, len, hash, &idx);
}

/**
 * @brief Remove entry
 */
static int map_remove(map m, const char *key, usize len) {
    if (!m || !key) {
        return 0;
    }
    
    usize stride = sizeof(map_bucket);
    uint64_t hash = fnv1a_hash(key, len);
    
    usize idx;
    if (map_find_slot(m, key, len, hash, &idx)) {
        // Mark as tombstone (hash = 1)
        map_bucket tombstone = {.hash = 1};
        FArray.set(m->buckets, idx, stride, &tombstone);
        m->count--;
        return 1;
    }
    
    return 0;
}

/**
 * @brief Get entry count
 */
static usize map_count(map m) {
    return m ? m->count : 0;
}

/**
 * @brief Get bucket capacity
 */
static usize map_capacity(map m) {
    return m ? m->capacity : 0;
}

/**
 * @brief Set custom allocator (delegates to collections module)
 */
static void map_alloc_use(sc_alloc_use_t *use) {
    coll_set_alloc_use(use);
}
