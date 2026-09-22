/*
 *  Test File: test_collections.c
 *  Description: Test cases for the Collections (generic collection) interface
 */

#include <sigma.test/sigtest.h>
#include <stdio.h>
#include "collections.h"
#include "farray.h"
#include "query.h"

//  configure test set
static void set_config(FILE **log_stream) {
    *log_stream = fopen("logs/test_collections.log", "w");
}
static void set_teardown(void) {
}

// Collections.create_view/count reflect the source array
static void test_collections_create_view_count(void) {
    farray arr = FArray.new(3, sizeof(int));
    int a = 1, b = 2, c = 3;
    FArray.set(arr, 0, sizeof(int), &a);
    FArray.set(arr, 1, sizeof(int), &b);
    FArray.set(arr, 2, sizeof(int), &c);

    collection view = FArray.as_collection(arr, sizeof(int));
    Assert.isNotNull(view, "as_collection returns non-null");
    Assert.areEqual(&(int){3}, &(int){Collections.count(view)}, INT, "count matches source");

    Collections.dispose(view);
    FArray.dispose(arr);
}

// Collections.as_queryable: Query.first finds a match
static bool collections_is_value_30(const void *element, usize index, void *userdata) {
    (void)index;
    (void)userdata;
    return *(const int *)element == 30;
}

static void test_collections_as_queryable_first(void) {
    farray arr = FArray.new(4, sizeof(int));
    int values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        FArray.set(arr, i, sizeof(int), &values[i]);
    }
    collection view = FArray.as_collection(arr, sizeof(int));

    sc_queryable q = Collections.as_queryable(view);
    usize idx = 999;
    Assert.isTrue(Query.first(q, collections_is_value_30, NULL, &idx), "Query.first finds 30");
    Assert.areEqual(&(usize){2}, &idx, INT, "30 is at index 2");

    Collections.dispose(view);
    FArray.dispose(arr);
}

// Collections.as_queryable: Query.next visits every element
static void test_collections_as_queryable_next(void) {
    farray arr = FArray.new(3, sizeof(int));
    int values[] = {5, 6, 7};
    for (int i = 0; i < 3; i++) {
        FArray.set(arr, i, sizeof(int), &values[i]);
    }
    collection view = FArray.as_collection(arr, sizeof(int));

    sc_queryable q = Collections.as_queryable(view);
    const void *elem;
    int seen = 0;
    while (Query.next(&q, &elem, NULL)) {
        seen++;
    }
    Assert.areEqual(&(int){3}, &seen, INT, "Query.next visits all 3 elements");

    Collections.dispose(view);
    FArray.dispose(arr);
}

//  register test cases
static void register_collections_tests(void) {
    testset("core_collections_set", set_config, set_teardown);
    DebugLogger.log("Test Source: %s", __FILE__);

    testcase("collections_create_view_count", test_collections_create_view_count);
    testcase("collections_as_queryable_first", test_collections_as_queryable_first);
    testcase("collections_as_queryable_next", test_collections_as_queryable_next);
}
__attribute__((constructor)) static void enqueue_collections_tests(void) {
    Tests.enqueue(register_collections_tests);
}
