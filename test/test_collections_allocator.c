/*
 *  Test File: test_collections_allocator.c
 *  Description: Test cases for SigmaCore collections with default allocator
 */

// #define SIGMA_MEMORY_AVAILABLE

#include <sigma.core/alloc.h>
#include <sigtest/sigtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

//  configure test set
static void set_config(FILE **log_stream) {
    *log_stream = fopen("logs/test_collections_allocator.log", "w");
    const char *version = Collections.version();
    Assert.isNotNull(version, "Collections version retrieval ERRed");
    fprintf(stdout, "SigmaCore Collections version: %s\n", version);
}

static void set_teardown(void) {
    // No teardown needed - using direct Allocator
}

//  test with direct Allocator usage
static void test_list_with_allocator(void) {
    int initial_capacity = 10;
    list lst = List.new(initial_capacity, sizeof(int));
    Assert.isNotNull(lst, "List creation with allocator ERRed");

    if (lst) {
        List.dispose(lst);
    }
}

static void test_farray_with_allocator(void) {
    farray arr = FArray.new(10, sizeof(int));
    Assert.isNotNull(arr, "FArray creation with allocator ERRed");

    if (arr) {
        FArray.dispose(arr);
    }
}

//  register test cases
__attribute__((constructor)) void init_collections_allocator_tests(void) {
    testset("collections_allocator_set", set_config, set_teardown);

    testcase("list_with_allocator", test_list_with_allocator);
    testcase("farray_with_allocator", test_farray_with_allocator);
}