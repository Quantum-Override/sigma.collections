/*
 *  Test File: test_collections.c
 *  Description: Test cases for the Collections (generic collection) interface
 */

#include <sigma.test/sigtest.h>
#include <stdio.h>
#include "collections.h"
#include "farray.h"
#include "query.h"
#include "fake_arena.h"

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

// create_with_allocator(NULL) behaves exactly like the plain
// global-Allocator-backed path (FR-2603-sigma-collections-007)
static void test_collections_create_with_allocator_null_is_default_path(void) {
    collection coll = Collections.create_with_allocator(4, sizeof(int), NULL);
    Assert.isNotNull(coll, "create_with_allocator(NULL) returns non-null");

    int a = 1, b = 2;
    Assert.areEqual(&(int){0}, &(int){Collections.add(coll, &a)}, INT, "add OK");
    Assert.areEqual(&(int){0}, &(int){Collections.add(coll, &b)}, INT, "add OK");
    Assert.areEqual(&(int){2}, &(int){Collections.count(coll)}, INT, "count is 2");

    Collections.dispose(coll);
}

// create_with_allocator(&use) grows through the override and orphans the
// old buffer instead of releasing it
static void test_collections_create_with_allocator_grow_orphans(void) {
    fake_arena_reset();

    collection coll = Collections.create_with_allocator(2, sizeof(int), &fake_arena_use);
    Assert.isNotNull(coll, "create_with_allocator(&use) returns non-null");
    Assert.isTrue(fake_arena_alloc_calls >= 1, "initial buffer allocated through override");

    int before_grow_calls = fake_arena_alloc_calls;
    int values[8];
    for (int i = 0; i < 8; i++) {
        values[i] = i * 10;
        Assert.areEqual(&(int){0}, &(int){Collections.add(coll, &values[i])}, INT, "add OK while growing");
    }

    Assert.areEqual(&(int){8}, &(int){Collections.count(coll)}, INT, "count is 8 after growth");
    Assert.isTrue(fake_arena_alloc_calls > before_grow_calls,
                  "growth allocated new buffer(s) through the override");
    Assert.areEqual(&(int){0}, &fake_arena_release_calls, INT,
                    "growth never released the orphaned buffer through the override");

    Collections.dispose(coll);
}

// collection_dispose skips releasing the bucket through the override -
// the bound allocator owns bulk-reclaiming it
static void test_collections_dispose_skips_release_when_overridden(void) {
    fake_arena_reset();

    collection coll = Collections.create_with_allocator(4, sizeof(int), &fake_arena_use);
    int a = 1;
    Collections.add(coll, &a);

    Collections.dispose(coll);

    Assert.areEqual(&(int){0}, &fake_arena_release_calls, INT,
                    "dispose never released the buffer through the override");
}

//  register test cases
static void register_collections_tests(void) {
    testset("core_collections_set", set_config, set_teardown);
    DebugLogger.log("Test Source: %s", __FILE__);

    testcase("collections_create_view_count", test_collections_create_view_count);
    testcase("collections_as_queryable_first", test_collections_as_queryable_first);
    testcase("collections_as_queryable_next", test_collections_as_queryable_next);

    testcase("collections_create_with_allocator_null_is_default_path",
             test_collections_create_with_allocator_null_is_default_path);
    testcase("collections_create_with_allocator_grow_orphans",
             test_collections_create_with_allocator_grow_orphans);
    testcase("collections_dispose_skips_release_when_overridden",
             test_collections_dispose_skips_release_when_overridden);
}
__attribute__((constructor)) static void enqueue_collections_tests(void) {
    Tests.enqueue(register_collections_tests);
}
