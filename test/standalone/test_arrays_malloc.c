/*
 * SigmaCore Collections - Malloc Variant Tests
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * Standalone test suite (no sigma.test dependency)
 * Compile: gcc test/standalone/test_arrays_malloc.c -o build/test_arrays_malloc \
 *              -Iinclude -std=c2x -Wall src/farray_malloc.c src/parray_malloc.c
 * Run: ./build/test_arrays_malloc
 * Valgrind: valgrind --leak-check=full ./build/test_arrays_malloc
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "farray_malloc.h"
#include "parray_malloc.h"

// Test counter
static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        printf("Running: %s ... ", #name); \
        fflush(stdout); \
        tests_total++; \
        name(); \
        tests_passed++; \
        printf("✓\n"); \
    } while(0)

//==============================================================================
// FArray Tests
//==============================================================================

void test_farray_create_dispose(void) {
    farray_malloc arr = farray_malloc_create(sizeof(int), 10);
    assert(arr != NULL);
    assert(farray_malloc_capacity(arr) == 10);
    farray_malloc_dispose(arr);
    
    // NULL dispose should not crash
    farray_malloc_dispose(NULL);
}

void test_farray_set_get(void) {
    farray_malloc arr = farray_malloc_create(sizeof(int), 5);
    
    int values[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        assert(farray_malloc_set(arr, i, &values[i]) == 0);
    }
    
    for (size_t i = 0; i < 5; i++) {
        int retrieved;
        assert(farray_malloc_get(arr, i, &retrieved) == 0);
        assert(retrieved == values[i]);
    }
    
    farray_malloc_dispose(arr);
}

void test_farray_bounds_check(void) {
    farray_malloc arr = farray_malloc_create(sizeof(int), 5);
    
    int value = 99;
    int out;
    
    // Out of bounds set should fail
    assert(farray_malloc_set(arr, 10, &value) == -1);
    
    // Out of bounds get should fail
    assert(farray_malloc_get(arr, 10, &out) == -1);
    
    // NULL pointer checks
    assert(farray_malloc_set(arr, 0, NULL) == -1);
    assert(farray_malloc_get(arr, 0, NULL) == -1);
    
    farray_malloc_dispose(arr);
}

void test_farray_clear(void) {
    farray_malloc arr = farray_malloc_create(sizeof(int), 5);
    
    // Fill with non-zero values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 100);
        farray_malloc_set(arr, i, &val);
    }
    
    // Clear and verify all zeros
    farray_malloc_clear(arr);
    
    for (size_t i = 0; i < 5; i++) {
        int val;
        farray_malloc_get(arr, i, &val);
        assert(val == 0);
    }
    
    farray_malloc_dispose(arr);
}

void test_farray_remove(void) {
    farray_malloc arr = farray_malloc_create(sizeof(int), 5);
    
    // Set values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 10);
        farray_malloc_set(arr, i, &val);
    }
    
    // Remove middle element
    assert(farray_malloc_remove(arr, 2) == 0);
    
    // Verify removal (should be zeroed)
    int val;
    farray_malloc_get(arr, 2, &val);
    assert(val == 0);
    
    // Verify others unchanged
    farray_malloc_get(arr, 1, &val);
    assert(val == 11);
    farray_malloc_get(arr, 3, &val);
    assert(val == 13);
    
    farray_malloc_dispose(arr);
}

void test_farray_struct_storage(void) {
    // Test storing complex structs
    typedef struct {
        int id;
        double value;
        char name[16];
    } test_struct;
    
    farray_malloc arr = farray_malloc_create(sizeof(test_struct), 3);
    
    test_struct s1 = {1, 3.14, "first"};
    test_struct s2 = {2, 2.71, "second"};
    test_struct s3 = {3, 1.41, "third"};
    
    farray_malloc_set(arr, 0, &s1);
    farray_malloc_set(arr, 1, &s2);
    farray_malloc_set(arr, 2, &s3);
    
    test_struct retrieved;
    farray_malloc_get(arr, 1, &retrieved);
    assert(retrieved.id == 2);
    assert(retrieved.value == 2.71);
    assert(strcmp(retrieved.name, "second") == 0);
    
    farray_malloc_dispose(arr);
}

//==============================================================================
// PArray Tests
//==============================================================================

void test_parray_create_dispose(void) {
    parray_malloc arr = parray_malloc_create(10);
    assert(arr != NULL);
    assert(parray_malloc_capacity(arr) == 10);
    parray_malloc_dispose(arr);
    
    // NULL dispose should not crash
    parray_malloc_dispose(NULL);
}

void test_parray_set_get(void) {
    parray_malloc arr = parray_malloc_create(5);
    
    // Create some test data (just use stack addresses as test pointers)
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    void *pointers[] = {&a, &b, &c, &d, &e};
    
    // Set pointers
    for (size_t i = 0; i < 5; i++) {
        assert(parray_malloc_set(arr, i, pointers[i]) == 0);
    }
    
    // Get and verify
    for (size_t i = 0; i < 5; i++) {
        void *retrieved;
        assert(parray_malloc_get(arr, i, &retrieved) == 0);
        assert(retrieved == pointers[i]);
    }
    
    parray_malloc_dispose(arr);
}

void test_parray_bounds_check(void) {
    parray_malloc arr = parray_malloc_create(5);
    
    int dummy;
    void *out;
    
    // Out of bounds set should fail
    assert(parray_malloc_set(arr, 10, &dummy) == -1);
    
    // Out of bounds get should fail
    assert(parray_malloc_get(arr, 10, &out) == -1);
    
    // NULL out pointer
    assert(parray_malloc_get(arr, 0, NULL) == -1);
    
    parray_malloc_dispose(arr);
}

void test_parray_clear(void) {
    parray_malloc arr = parray_malloc_create(5);
    
    // Fill with non-NULL pointers
    int a = 1, b = 2, c = 3;
    parray_malloc_set(arr, 0, &a);
    parray_malloc_set(arr, 1, &b);
    parray_malloc_set(arr, 2, &c);
    
    // Clear
    parray_malloc_clear(arr);
    
    // Verify all NULL
    for (size_t i = 0; i < 5; i++) {
        void *ptr;
        parray_malloc_get(arr, i, &ptr);
        assert(ptr == NULL);
    }
    
    parray_malloc_dispose(arr);
}

void test_parray_remove(void) {
    parray_malloc arr = parray_malloc_create(5);
    
    // Set pointers
    int a = 1, b = 2, c = 3;
    parray_malloc_set(arr, 0, &a);
    parray_malloc_set(arr, 1, &b);
    parray_malloc_set(arr, 2, &c);
    
    // Remove middle element
    assert(parray_malloc_remove(arr, 1) == 0);
    
    // Verify removal (should be NULL)
    void *ptr;
    parray_malloc_get(arr, 1, &ptr);
    assert(ptr == NULL);
    
    // Verify others unchanged
    parray_malloc_get(arr, 0, &ptr);
    assert(ptr == &a);
    parray_malloc_get(arr, 2, &ptr);
    assert(ptr == &c);
    
    parray_malloc_dispose(arr);
}

void test_parray_null_storage(void) {
    parray_malloc arr = parray_malloc_create(3);
    
    // Explicitly store NULL
    parray_malloc_set(arr, 1, NULL);
    
    void *ptr;
    parray_malloc_get(arr, 1, &ptr);
    assert(ptr == NULL);
    
    parray_malloc_dispose(arr);
}

//==============================================================================
// Main
//==============================================================================

int main(void) {
    printf("==============================================\n");
    printf("Sigma.Arrays.a Malloc Variant Test Suite\n");
    printf("==============================================\n\n");
    
    printf("--- FArray Tests ---\n");
    TEST(test_farray_create_dispose);
    TEST(test_farray_set_get);
    TEST(test_farray_bounds_check);
    TEST(test_farray_clear);
    TEST(test_farray_remove);
    TEST(test_farray_struct_storage);
    
    printf("\n--- PArray Tests ---\n");
    TEST(test_parray_create_dispose);
    TEST(test_parray_set_get);
    TEST(test_parray_bounds_check);
    TEST(test_parray_clear);
    TEST(test_parray_remove);
    TEST(test_parray_null_storage);
    
    printf("\n==============================================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_total);
    printf("==============================================\n");
    
    if (tests_passed == tests_total) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ Some tests failed!\n");
        return 1;
    }
}
