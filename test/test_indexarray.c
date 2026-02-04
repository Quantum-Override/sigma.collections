/*
 *  Test File: test_indexarray.c
 *  Description: Test cases for IndexArray collection
 */

#include <sigtest/sigtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "collections.h"
#include "indexarray.h"

// Test struct (similar to sc_slab example)
typedef struct test_data {
    int id;
    int value;
} test_data;

//  configure test set
static void set_config(FILE **log_stream) { *log_stream = fopen("logs/test_indexarray.log", "w"); }
static void set_teardown(void) {}

// basic initialization, disposal, and properties
static void test_indexarray_new(void) {
    indexarray ia = IndexArray.new(10, sizeof(test_data));
    Assert.isNotNull(ia, "IndexArray creation failed");
    IndexArray.dispose(ia);
}

static void test_indexarray_dispose(void) {
    indexarray ia = IndexArray.new(10, sizeof(test_data));
    Assert.isNotNull(ia, "IndexArray creation failed");
    IndexArray.dispose(ia);
}

// data manipulation tests
static void test_indexarray_add_value(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));
    test_data data = {.id = 1, .value = 42};

    int handle = IndexArray.add(ia, &data);
    Assert.isTrue(handle >= 0, "IndexArray add failed");

    // retrieve the value back
    test_data retrieved = {0};
    int result = IndexArray.get_at(ia, handle, &retrieved);
    Assert.areEqual(&(int){0}, &result, INT, "IndexArray get_at failed");
    Assert.areEqual(&data.id, &retrieved.id, INT, "ID mismatch");
    Assert.areEqual(&data.value, &retrieved.value, INT, "Value mismatch");

    IndexArray.dispose(ia);
}

static void test_indexarray_get_at(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));
    test_data data = {.id = 99, .value = 123};

    int handle = IndexArray.add(ia, &data);
    Assert.isTrue(handle >= 0, "IndexArray add failed");

    // retrieve value at handle
    test_data retrieved = {0};
    int result = IndexArray.get_at(ia, handle, &retrieved);
    Assert.areEqual(&(int){0}, &result, INT, "IndexArray get_at failed at handle %d", handle);
    Assert.areEqual(&data.id, &retrieved.id, INT, "ID mismatch");
    Assert.areEqual(&data.value, &retrieved.value, INT, "Value mismatch");

    IndexArray.dispose(ia);
}

static void test_indexarray_remove_at(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));
    test_data data = {.id = 42, .value = 100};

    int handle = IndexArray.add(ia, &data);
    Assert.isTrue(handle >= 0, "IndexArray add failed");

    // remove at handle
    int result = IndexArray.remove_at(ia, handle);
    Assert.areEqual(&(int){0}, &result, INT, "IndexArray remove_at failed at handle %d", handle);

    // try to get value at handle, should fail
    test_data retrieved = {0};
    result = IndexArray.get_at(ia, handle, &retrieved);
    Assert.areEqual(&(int){-1}, &result, INT,
                    "IndexArray get_at should fail after remove at handle %d", handle);

    IndexArray.dispose(ia);
}

static void test_indexarray_is_empty_slot(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));

    // Initially all slots should be empty
    Assert.isTrue(IndexArray.is_empty_slot(ia, 0), "Slot 0 should be empty initially");
    Assert.isTrue(IndexArray.is_empty_slot(ia, 2), "Slot 2 should be empty initially");

    // Add to slot
    test_data data = {.id = 1, .value = 10};
    int handle = IndexArray.add(ia, &data);

    // That slot should no longer be empty
    Assert.isFalse(IndexArray.is_empty_slot(ia, handle), "Slot should not be empty after add");

    // Other slots still empty
    int other_slot = (handle + 1) % 5;
    Assert.isTrue(IndexArray.is_empty_slot(ia, other_slot), "Other slots should still be empty");

    // Remove makes it empty again
    IndexArray.remove_at(ia, handle);
    Assert.isTrue(IndexArray.is_empty_slot(ia, handle), "Slot should be empty after remove");

    IndexArray.dispose(ia);
}

static void test_indexarray_capacity(void) {
    indexarray ia = IndexArray.new(7, sizeof(test_data));

    usize cap = IndexArray.capacity(ia);
    Assert.areEqual(&(int){7}, &(int){cap}, INT, "Capacity should be 7");

    IndexArray.dispose(ia);
}

static void test_indexarray_clear(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));

    // Add some values
    for (int i = 0; i < 3; i++) {
        test_data data = {.id = i, .value = i * 10};
        IndexArray.add(ia, &data);
    }

    // Clear all
    IndexArray.clear(ia);

    // All slots should now be empty
    for (usize i = 0; i < 5; i++) {
        Assert.isTrue(IndexArray.is_empty_slot(ia, i), "Slot %zu should be empty after clear", i);
    }

    IndexArray.dispose(ia);
}

static void test_indexarray_slot_reuse(void) {
    indexarray ia = IndexArray.new(3, sizeof(test_data));

    // Fill all slots
    test_data d1 = {.id = 1, .value = 100};
    test_data d2 = {.id = 2, .value = 200};
    test_data d3 = {.id = 3, .value = 300};

    int h1 = IndexArray.add(ia, &d1);
    int h2 = IndexArray.add(ia, &d2);
    int h3 = IndexArray.add(ia, &d3);

    Assert.isTrue(h1 >= 0 && h2 >= 0 && h3 >= 0, "All adds should succeed");

    // Remove middle one
    IndexArray.remove_at(ia, h2);

    // Add new value - should reuse h2 slot
    test_data d4 = {.id = 4, .value = 400};
    int h4 = IndexArray.add(ia, &d4);
    Assert.areEqual(&h2, &h4, INT, "Should reuse removed slot");

    // Verify data
    test_data retrieved = {0};
    IndexArray.get_at(ia, h4, &retrieved);
    Assert.areEqual(&d4.id, &retrieved.id, INT, "Reused slot should have new data");

    IndexArray.dispose(ia);
}

// test dynamic growth
static void test_indexarray_growth(void) {
    indexarray ia = IndexArray.new(3, sizeof(test_data));

    // Fill all slots
    test_data d1 = {.id = 1, .value = 100};
    test_data d2 = {.id = 2, .value = 200};
    test_data d3 = {.id = 3, .value = 300};

    int h1 = IndexArray.add(ia, &d1);
    int h2 = IndexArray.add(ia, &d2);
    int h3 = IndexArray.add(ia, &d3);

    Assert.isTrue(h1 >= 0 && h2 >= 0 && h3 >= 0, "Initial adds should succeed");

    usize initial_cap = IndexArray.capacity(ia);
    Assert.areEqual(&(int){3}, &(int){initial_cap}, INT, "Initial capacity should be 3");

    // Add fourth - should trigger growth
    test_data d4 = {.id = 4, .value = 400};
    int h4 = IndexArray.add(ia, &d4);
    Assert.isTrue(h4 >= 0, "Add after growth should succeed");

    usize new_cap = IndexArray.capacity(ia);
    Assert.isTrue(new_cap > initial_cap, "Capacity should have grown");

    // Verify all values are still accessible
    test_data retrieved = {0};
    Assert.areEqual(&(int){OK}, &(int){IndexArray.get_at(ia, h1, &retrieved)}, INT,
                    "h1 should be accessible");
    Assert.areEqual(&d1.id, &retrieved.id, INT, "h1 value should match");

    Assert.areEqual(&(int){OK}, &(int){IndexArray.get_at(ia, h4, &retrieved)}, INT,
                    "h4 should be accessible");
    Assert.areEqual(&d4.id, &retrieved.id, INT, "h4 value should match");

    IndexArray.dispose(ia);
}

// test from_farray
static void test_indexarray_from_farray(void) {
    farray arr = FArray.new(5, sizeof(test_data));

    // Add some values to farray
    test_data d1 = {.id = 10, .value = 1000};
    test_data d2 = {.id = 20, .value = 2000};

    FArray.set(arr, 0, sizeof(test_data), &d1);
    FArray.set(arr, 2, sizeof(test_data), &d2);

    // Create indexarray from farray
    indexarray ia = IndexArray.from_farray(arr, sizeof(test_data));
    Assert.isNotNull(ia, "from_farray should succeed");

    usize cap = IndexArray.capacity(ia);
    Assert.areEqual(&(int){5}, &(int){cap}, INT, "Capacity should match source");

    // Count non-empty slots
    int found = 0;
    for (usize i = 0; i < cap; i++) {
        if (!IndexArray.is_empty_slot(ia, i)) {
            found++;
        }
    }
    Assert.areEqual(&(int){2}, &found, INT, "Should have 2 non-empty slots");

    IndexArray.dispose(ia);
    FArray.dispose(arr);
}

// test from_buffer with raw buffer range
static void test_indexarray_from_buffer(void) {
    // Allocate a buffer for 5 test_data structs
    usize stride = sizeof(test_data);
    usize capacity = 5;
    test_data *buffer = malloc(stride * capacity);
    test_data *end = buffer + capacity;

    // Zero out buffer
    memset(buffer, 0, stride * capacity);

    // Manually set some values in the buffer
    buffer[0] = (test_data){.id = 10, .value = 100};
    buffer[2] = (test_data){.id = 20, .value = 200};
    buffer[4] = (test_data){.id = 30, .value = 300};

    // Create indexarray view from buffer
    indexarray ia = IndexArray.from_buffer(buffer, end, stride);
    Assert.isNotNull(ia, "from_buffer should succeed");

    // Check capacity
    usize cap = IndexArray.capacity(ia);
    Assert.areEqual(&(int){5}, &(int){cap}, INT, "Capacity should be 5");

    // Verify we can read the pre-existing values
    test_data retrieved = {0};
    Assert.areEqual(&(int){OK}, &(int){IndexArray.get_at(ia, 0, &retrieved)}, INT,
                    "Should get value at 0");
    Assert.areEqual(&(int){10}, &retrieved.id, INT, "ID at 0 should be 10");

    Assert.areEqual(&(int){OK}, &(int){IndexArray.get_at(ia, 2, &retrieved)}, INT,
                    "Should get value at 2");
    Assert.areEqual(&(int){20}, &retrieved.id, INT, "ID at 2 should be 20");

    // Verify empty slots
    Assert.isTrue(IndexArray.is_empty_slot(ia, 1), "Slot 1 should be empty");
    Assert.isTrue(IndexArray.is_empty_slot(ia, 3), "Slot 3 should be empty");
    Assert.isFalse(IndexArray.is_empty_slot(ia, 4), "Slot 4 should not be empty");

    // Can add to empty slots (view doesn't own buffer, but can modify)
    test_data new_data = {.id = 40, .value = 400};
    int handle = IndexArray.add(ia, &new_data);
    Assert.areEqual(&(int){1}, &handle, INT, "Should add to slot 1");

    // Verify the buffer was actually modified
    Assert.areEqual(&(int){40}, &buffer[1].id, INT, "Buffer should be modified");

    // Dispose indexarray (should not free buffer since it's non-owning)
    IndexArray.dispose(ia);

    // Buffer should still be valid and accessible
    Assert.areEqual(&(int){40}, &buffer[1].id, INT, "Buffer should still be valid after dispose");

    free(buffer);
}

// iterator tests
static void test_indexarray_create_iterator(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));
    Assert.isNotNull(ia, "IndexArray creation failed");

    sparse_iterator it = IndexArray.create_iterator(ia);
    Assert.isNotNull(it, "SparseIterator creation failed");

    SparseIterator.dispose(it);
    IndexArray.dispose(ia);
}

static void test_indexarray_iterator_empty(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));

    sparse_iterator it = IndexArray.create_iterator(ia);
    Assert.isNotNull(it, "SparseIterator creation failed");

    // Empty indexarray should have no items to iterate
    bool has_next = SparseIterator.next(it);
    Assert.isFalse(has_next, "Empty indexarray should have no items");

    SparseIterator.dispose(it);
    IndexArray.dispose(ia);
}

static void test_indexarray_iterator_sparse(void) {
    indexarray ia = IndexArray.new(10, sizeof(test_data));

    // Add items at non-contiguous positions
    test_data d1 = {.id = 1, .value = 100};
    test_data d2 = {.id = 2, .value = 200};
    test_data d3 = {.id = 3, .value = 300};

    IndexArray.add(ia, &d1);           // Index 0
    int h2 = IndexArray.add(ia, &d2);  // Index 1
    IndexArray.add(ia, &d3);           // Index 2

    // Remove middle element to create sparse array
    IndexArray.remove_at(ia, h2);

    sparse_iterator it = IndexArray.create_iterator(ia);
    Assert.isNotNull(it, "SparseIterator creation failed");

    // Should iterate over indices 0 and 2, skipping removed index 1
    int count = 0;
    int found_ids[2] = {0};
    test_data value = {0};
    while (SparseIterator.next(it)) {
        int result = SparseIterator.current_value(it, &value);
        Assert.areEqual(&(int){0}, &result, INT, "current_value failed");

        found_ids[count] = value.id;
        count++;
    }

    Assert.areEqual(&(int){2}, &count, INT, "Should find 2 items");
    Assert.areEqual(&(int){1}, &found_ids[0], INT, "First ID mismatch");
    Assert.areEqual(&(int){3}, &found_ids[1], INT, "Second ID mismatch");

    SparseIterator.dispose(it);
    IndexArray.dispose(ia);
}

static void test_indexarray_iterator_full(void) {
    indexarray ia = IndexArray.new(5, sizeof(test_data));

    // Fill all slots
    for (int i = 0; i < 5; i++) {
        test_data data = {.id = i + 1, .value = (i + 1) * 10};
        int h = IndexArray.add(ia, &data);
        Assert.isTrue(h >= 0, "Add failed at index %d", i);
    }

    sparse_iterator it = IndexArray.create_iterator(ia);
    Assert.isNotNull(it, "SparseIterator creation failed");

    // Should iterate over all 5 items
    int count = 0;
    while (SparseIterator.next(it)) {
        count++;
    }

    Assert.areEqual(&(int){5}, &count, INT, "Should iterate over all 5 items");

    // Test reset
    SparseIterator.reset(it);
    count = 0;
    while (SparseIterator.next(it)) {
        count++;
    }
    Assert.areEqual(&(int){5}, &count, INT, "After reset should iterate over all 5 items again");

    SparseIterator.dispose(it);
    IndexArray.dispose(ia);
}

//  register test cases
__attribute__((constructor)) void init_indexarray_tests(void) {
    testset("core_indexarray_set", set_config, set_teardown);

    testcase("indexarray_creation", test_indexarray_new);
    testcase("indexarray_dispose", test_indexarray_dispose);
    testcase("indexarray_add_value", test_indexarray_add_value);
    testcase("indexarray_get_at", test_indexarray_get_at);
    testcase("indexarray_remove_at", test_indexarray_remove_at);
    testcase("indexarray_is_empty_slot", test_indexarray_is_empty_slot);
    testcase("indexarray_capacity", test_indexarray_capacity);
    testcase("indexarray_clear", test_indexarray_clear);
    testcase("indexarray_slot_reuse", test_indexarray_slot_reuse);
    testcase("indexarray_growth", test_indexarray_growth);
    testcase("indexarray_from_farray", test_indexarray_from_farray);
    testcase("indexarray_from_buffer", test_indexarray_from_buffer);
    testcase("indexarray_create_iterator", test_indexarray_create_iterator);
    testcase("indexarray_iterator_empty", test_indexarray_iterator_empty);
    testcase("indexarray_iterator_sparse", test_indexarray_iterator_sparse);
    testcase("indexarray_iterator_full", test_indexarray_iterator_full);
}
