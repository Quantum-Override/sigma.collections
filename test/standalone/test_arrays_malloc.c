/*
 * SigmaCore Collections - Malloc Variant Tests
 * Copyright (c) 2025 David Boarman (BadKraft) and contributors
 * QuantumOverride [Q|]
 * ----------------------------------------------
 * Standalone test suite (no sigma.test dependency)
 * Compile: gcc test/standalone/test_arrays_malloc.c -o build/test_arrays_malloc \
 *              -Iinclude -I/usr/local/include -std=c2x -Wall src/farray.c
 * src/parray.c Run: ./build/test_arrays_malloc Valgrind: valgrind --leak-check=full
 * ./build/test_arrays_malloc
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "farray_malloc.h"
#include "parray_malloc.h"

// Test counter
static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name)                         \
    do {                                   \
        printf("Running: %s ... ", #name); \
        fflush(stdout);                    \
        tests_total++;                     \
        name();                            \
        tests_passed++;                    \
        printf("✓\n");                     \
    } while (0)

//==============================================================================
// FArray Tests
//==============================================================================

void test_farray_create_dispose(void) {
    farray arr = FArray.new(10, sizeof(int));
    assert(arr != NULL);
    assert(FArray.capacity(arr, sizeof(int)) == 10);
    FArray.dispose(arr);

    // NULL dispose should not crash
    FArray.dispose(NULL);
}

void test_farray_set_get(void) {
    farray arr = FArray.new(5, sizeof(int));

    int values[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        assert(FArray.set(arr, i, sizeof(int), &values[i]) == 0);
    }

    for (size_t i = 0; i < 5; i++) {
        int retrieved;
        assert(FArray.get(arr, i, sizeof(int), &retrieved) == 0);
        assert(retrieved == values[i]);
    }

    FArray.dispose(arr);
}

void test_farray_bounds_check(void) {
    farray arr = FArray.new(5, sizeof(int));

    int value = 99;
    int out;

    // Out of bounds set should fail
    assert(FArray.set(arr, 10, sizeof(int), &value) == -1);

    // Out of bounds get should fail
    assert(FArray.get(arr, 10, sizeof(int), &out) == -1);

    // NULL pointer checks
    assert(FArray.set(arr, 0, sizeof(int), NULL) == -1);
    assert(FArray.get(arr, 0, sizeof(int), NULL) == -1);

    FArray.dispose(arr);
}

void test_farray_clear(void) {
    farray arr = FArray.new(5, sizeof(int));

    // Fill with non-zero values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 100);
        FArray.set(arr, i, sizeof(int), &val);
    }

    // Clear and verify all zeros
    FArray.clear(arr, sizeof(int));

    for (size_t i = 0; i < 5; i++) {
        int val;
        FArray.get(arr, i, sizeof(int), &val);
        assert(val == 0);
    }

    FArray.dispose(arr);
}

void test_farray_remove(void) {
    farray arr = FArray.new(5, sizeof(int));

    // Set values
    for (size_t i = 0; i < 5; i++) {
        int val = (int)(i + 10);
        FArray.set(arr, i, sizeof(int), &val);
    }

    // Remove middle element
    assert(FArray.remove(arr, 2, sizeof(int)) == 0);

    // Verify removal (should be zeroed)
    int val;
    FArray.get(arr, 2, sizeof(int), &val);
    assert(val == 0);

    // Verify others unchanged
    FArray.get(arr, 1, sizeof(int), &val);
    assert(val == 11);
    FArray.get(arr, 3, sizeof(int), &val);
    assert(val == 13);

    FArray.dispose(arr);
}

void test_farray_struct_storage(void) {
    // Test storing complex structs
    typedef struct {
        int id;
        double value;
        char name[16];
    } test_struct;

    farray arr = FArray.new(3, sizeof(test_struct));

    test_struct s1 = {1, 3.14, "first"};
    test_struct s2 = {2, 2.71, "second"};
    test_struct s3 = {3, 1.41, "third"};

    FArray.set(arr, 0, sizeof(test_struct), &s1);
    FArray.set(arr, 1, sizeof(test_struct), &s2);
    FArray.set(arr, 2, sizeof(test_struct), &s3);

    test_struct retrieved;
    FArray.get(arr, 1, sizeof(test_struct), &retrieved);
    assert(retrieved.id == 2);
    assert(retrieved.value == 2.71);
    assert(strcmp(retrieved.name, "second") == 0);

    FArray.dispose(arr);
}

//==============================================================================
// PArray Tests
//==============================================================================

void test_parray_create_dispose(void) {
    parray arr = PArray.new(10);
    assert(arr != NULL);
    assert(PArray.capacity(arr) == 10);
    PArray.dispose(arr);

    // NULL dispose should not crash
    PArray.dispose(NULL);
}

void test_parray_set_get(void) {
    parray arr = PArray.new(5);

    // Create some test data (just use stack addresses as test pointers)
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    void *pointers[] = {&a, &b, &c, &d, &e};

    // Set pointers
    for (size_t i = 0; i < 5; i++) {
        assert(PArray.set(arr, i, (addr)pointers[i]) == 0);
    }

    // Get and verify
    for (size_t i = 0; i < 5; i++) {
        addr retrieved;
        assert(PArray.get(arr, i, &retrieved) == 0);
        assert((void *)retrieved == pointers[i]);
    }

    PArray.dispose(arr);
}

void test_parray_bounds_check(void) {
    parray arr = PArray.new(5);

    int dummy;
    addr out;

    // Out of bounds set should fail
    assert(PArray.set(arr, 10, (addr)&dummy) == -1);

    // Out of bounds get should fail
    assert(PArray.get(arr, 10, &out) == -1);

    // NULL out pointer
    assert(PArray.get(arr, 0, NULL) == -1);

    PArray.dispose(arr);
}

void test_parray_clear(void) {
    parray arr = PArray.new(5);

    // Fill with non-NULL pointers
    int a = 1, b = 2, c = 3;
    PArray.set(arr, 0, (addr)&a);
    PArray.set(arr, 1, (addr)&b);
    PArray.set(arr, 2, (addr)&c);

    // Clear
    PArray.clear(arr);

    // Verify all NULL
    for (size_t i = 0; i < 5; i++) {
        addr ptr;
        PArray.get(arr, i, &ptr);
        assert(ptr == (addr)NULL);
    }

    PArray.dispose(arr);
}

void test_parray_remove(void) {
    parray arr = PArray.new(5);

    // Set pointers
    int a = 1, b = 2, c = 3;
    PArray.set(arr, 0, (addr)&a);
    PArray.set(arr, 1, (addr)&b);
    PArray.set(arr, 2, (addr)&c);

    // Remove middle element
    assert(PArray.remove(arr, 1) == 0);

    // Verify removal (should be NULL)
    addr ptr;
    PArray.get(arr, 1, &ptr);
    assert(ptr == (addr)NULL);

    // Verify others unchanged
    PArray.get(arr, 0, &ptr);
    assert((void *)ptr == &a);
    PArray.get(arr, 2, &ptr);
    assert((void *)ptr == &c);

    PArray.dispose(arr);
}

void test_parray_null_storage(void) {
    parray arr = PArray.new(3);

    // Explicitly store NULL
    PArray.set(arr, 1, (addr)NULL);

    addr ptr;
    PArray.get(arr, 1, &ptr);
    assert(ptr == (addr)NULL);

    PArray.dispose(arr);
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
