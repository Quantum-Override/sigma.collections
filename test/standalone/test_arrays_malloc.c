/*
 * SigmaCore Collections - Malloc Variant Tests
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * Standalone test suite (no sigma.test dependency)
 * Compile: gcc test/standalone/test_arrays_malloc.c -o build/test_arrays_malloc \
 *              -Iinclude -I/usr/local/include -std=c2x -Wall src/farray_malloc.c src/parray_malloc.c
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
    farray_malloc arr = FArrayMalloc.new(10, sizeof(int));
    assert(arr != NULL);
    assert(FArrayMalloc.capacity(arr, sizeof(int)) == 10);
    FArrayMalloc.dispose(arr);
    
    // NULL dispose should not crash
    FArrayMalloc.dispose(NULL);
}

void test_farray_set_get(void) {
    farray_malloc arr = FArrayMalloc.new(5, sizeof(int));
    
    int values[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        assert(FArrayMalloc.set(arr, i, sizeof(int), &values[i]) == 0);
    }
    
    for (size_t i = 0; i < 5; i++) {
        int retrieved;
        assert(FArrayMalloc.get(arr, i, sizeof(int), &retrieved) == 0);
        assert(retrieved == values[i]);
    }
    
    FArrayMalloc.dispose(arr);
}

void test_farray_bounds_check(void) {
    farray_malloc arr = FArrayMalloc.new(5, sizeof(int));
    
    int value = 99;
    int out;
    
    // Out of bounds set should fail
    assert(FArrayMalloc.set(arr, 10, sizeof(int), &value) == -1);
    
    // Out of bounds get should fail
    assert(FArrayMalloc.get(arr, 10, sizeof(int), &out) == -1);
    
    // NULL pointer checks
    assert(FArrayMalloc.set(arr, 0, sizeof(int), NULL) == -1);
    assert(FArrayMalloc.get(arr, 0, sizeof(int), NULL) == -1);
    
    FArrayMalloc.dispose(arr);
}

void test_farray_clear(void) {
    farray_malloc arr = FArrayMalloc.new(5, sizeof(int));
    
    // Fill with non-zero values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 100);
        FArrayMalloc.set(arr, i, sizeof(int), &val);
    }
    
    // Clear and verify all zeros
    FArrayMalloc.clear(arr, sizeof(int));
    
    for (size_t i = 0; i < 5; i++) {
        int val;
        FArrayMalloc.get(arr, i, sizeof(int), &val);
        assert(val == 0);
    }
    
    FArrayMalloc.dispose(arr);
}

void test_farray_remove(void) {
    farray_malloc arr = FArrayMalloc.new(5, sizeof(int));
    
    // Set values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 10);
        FArrayMalloc.set(arr, i, sizeof(int), &val);
    }
    
    // Remove middle element
    assert(FArrayMalloc.remove(arr, 2, sizeof(int)) == 0);
    
    // Verify removal (should be zeroed)
    int val;
    FArrayMalloc.get(arr, 2, sizeof(int), &val);
    assert(val == 0);
    
    // Verify others unchanged
    FArrayMalloc.get(arr, 1, sizeof(int), &val);
    assert(val == 11);
    FArrayMalloc.get(arr, 3, sizeof(int), &val);
    assert(val == 13);
    
    FArrayMalloc.dispose(arr);
}

void test_farray_struct_storage(void) {
    // Test storing complex structs
    typedef struct {
        int id;
        double value;
        char name[16];
    } test_struct;
    
    farray_malloc arr = FArrayMalloc.new(3, sizeof(test_struct));
    
    test_struct s1 = {1, 3.14, "first"};
    test_struct s2 = {2, 2.71, "second"};
    test_struct s3 = {3, 1.41, "third"};
    
    FArrayMalloc.set(arr, 0, sizeof(test_struct), &s1);
    FArrayMalloc.set(arr, 1, sizeof(test_struct), &s2);
    FArrayMalloc.set(arr, 2, sizeof(test_struct), &s3);
    
    test_struct retrieved;
    FArrayMalloc.get(arr, 1, sizeof(test_struct), &retrieved);
    assert(retrieved.id == 2);
    assert(retrieved.value == 2.71);
    assert(strcmp(retrieved.name, "second") == 0);
    
    FArrayMalloc.dispose(arr);
}

//==============================================================================
// PArray Tests
//==============================================================================

void test_parray_create_dispose(void) {
    parray_malloc arr = PArrayMalloc.new(10);
    assert(arr != NULL);
    assert(PArrayMalloc.capacity(arr) == 10);
    PArrayMalloc.dispose(arr);
    
    // NULL dispose should not crash
    PArrayMalloc.dispose(NULL);
}

void test_parray_set_get(void) {
    parray_malloc arr = PArrayMalloc.new(5);
    
    // Create some test data (just use stack addresses as test pointers)
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    void *pointers[] = {&a, &b, &c, &d, &e};
    
    // Set pointers
    for (size_t i = 0; i < 5; i++) {
        assert(PArrayMalloc.set(arr, i, (addr)pointers[i]) == 0);
    }
    
    // Get and verify
    for (size_t i = 0; i < 5; i++) {
        addr retrieved;
        assert(PArrayMalloc.get(arr, i, &retrieved) == 0);
        assert((void *)retrieved == pointers[i]);
    }
    
    PArrayMalloc.dispose(arr);
}

void test_parray_bounds_check(void) {
    parray_malloc arr = PArrayMalloc.new(5);
    
    int dummy;
    addr out;
    
    // Out of bounds set should fail
    assert(PArrayMalloc.set(arr, 10, (addr)&dummy) == -1);
    
    // Out of bounds get should fail
    assert(PArrayMalloc.get(arr, 10, &out) == -1);
    
    // NULL out pointer
    assert(PArrayMalloc.get(arr, 0, NULL) == -1);
    
    PArrayMalloc.dispose(arr);
}

void test_parray_clear(void) {
    parray_malloc arr = PArrayMalloc.new(5);
    
    // Fill with non-NULL pointers
    int a = 1, b = 2, c = 3;
    PArrayMalloc.set(arr, 0, (addr)&a);
    PArrayMalloc.set(arr, 1, (addr)&b);
    PArrayMalloc.set(arr, 2, (addr)&c);
    
    // Clear
    PArrayMalloc.clear(arr);
    
    // Verify all NULL
    for (size_t i = 0; i < 5; i++) {
        addr ptr;
        PArrayMalloc.get(arr, i, &ptr);
        assert(ptr == (addr)NULL);
    }
    
    PArrayMalloc.dispose(arr);
}

void test_parray_remove(void) {
    parray_malloc arr = PArrayMalloc.new(5);
    
    // Set pointers
    int a = 1, b = 2, c = 3;
    PArrayMalloc.set(arr, 0, (addr)&a);
    PArrayMalloc.set(arr, 1, (addr)&b);
    PArrayMalloc.set(arr, 2, (addr)&c);
    
    // Remove middle element
    assert(PArrayMalloc.remove(arr, 1) == 0);
    
    // Verify removal (should be NULL)
    addr ptr;
    PArrayMalloc.get(arr, 1, &ptr);
    assert(ptr == (addr)NULL);
    
    // Verify others unchanged
    PArrayMalloc.get(arr, 0, &ptr);
    assert((void *)ptr == &a);
    PArrayMalloc.get(arr, 2, &ptr);
    assert((void *)ptr == &c);
    
    PArrayMalloc.dispose(arr);
}

void test_parray_null_storage(void) {
    parray_malloc arr = PArrayMalloc.new(3);
    
    // Explicitly store NULL
    PArrayMalloc.set(arr, 1, (addr)NULL);
    
    addr ptr;
    PArrayMalloc.get(arr, 1, &ptr);
    assert(ptr == (addr)NULL);
    
    PArrayMalloc.dispose(arr);
}

//==============================================================================
// Main
//==============================================================================

int main(void) {
    printf("==============================================\n");
    printf("Sigma.Arrays.a Malloc Variant Test Suite\n");
    printf("(Vtable Interface - ABI Compatible)\n");
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
