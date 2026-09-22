/*
 *  Test File: test_query.c
 *  Description: Test cases for the Query heapless scan mechanism
 *               (Query.next / Query.first) — FR-2603-sigma-collections-006
 *
 *  Tests the generic sc_queryable/Query engine in isolation, via a small
 *  hand-rolled advance function over a plain int[] — independent of any
 *  real collection type. FArray/PArray/Collections/List/Map/SlotArray each
 *  verify their own as_queryable/keys/values wiring in their own test files;
 *  this file only proves the shared mechanism itself is correct.
 */

#include <sigma.test/sigtest.h>
#include <stdio.h>
#include "query.h"

//  configure test set
static void set_config(FILE **log_stream) {
    *log_stream = fopen("logs/test_query.log", "w");
}
static void set_teardown(void) {
}

/* ----------------------------------------------------------------- */
/* A minimal dense source: a plain int[] with an explicit count.      */
/* ----------------------------------------------------------------- */
typedef struct {
    int *values;
    usize count;
} int_source;

static bool int_source_advance(sc_queryable *self, const void **out_element, usize *out_index) {
    int_source *src = (int_source *)self->source;
    if (self->index >= src->count) {
        return false;
    }
    usize i = self->index++;
    *out_element = &src->values[i];
    if (out_index) {
        *out_index = i;
    }
    return true;
}

static sc_queryable int_source_as_queryable(int_source *src) {
    return (sc_queryable){
        .source = src, .element_size = sizeof(int), .bound = src->count, .index = 0,
        .advance = int_source_advance};
}

static bool is_even(const void *element, usize index, void *userdata) {
    (void)index;
    (void)userdata;
    return (*(const int *)element % 2) == 0;
}

static bool always_false(const void *element, usize index, void *userdata) {
    (void)element;
    (void)index;
    (void)userdata;
    return false;
}

// QRY01 - Query.next visits every element in order, then exhausts
static void test_qry01_next_visits_in_order(void) {
    int values[] = {10, 20, 30};
    int_source src = {values, 3};
    sc_queryable q = int_source_as_queryable(&src);

    const void *elem;
    usize idx;

    Assert.isTrue(Query.next(&q, &elem, &idx), "QRY01: first next succeeds");
    Assert.areEqual(&(int){10}, (int *)elem, INT, "QRY01: first element is 10");
    Assert.areEqual(&(usize){0}, &idx, INT, "QRY01: first index is 0");

    Assert.isTrue(Query.next(&q, &elem, &idx), "QRY01: second next succeeds");
    Assert.areEqual(&(int){20}, (int *)elem, INT, "QRY01: second element is 20");
    Assert.areEqual(&(usize){1}, &idx, INT, "QRY01: second index is 1");

    Assert.isTrue(Query.next(&q, &elem, &idx), "QRY01: third next succeeds");
    Assert.areEqual(&(int){30}, (int *)elem, INT, "QRY01: third element is 30");
    Assert.areEqual(&(usize){2}, &idx, INT, "QRY01: third index is 2");

    Assert.isFalse(Query.next(&q, &elem, &idx), "QRY01: fourth next exhausts the source");
}

// QRY02 - Query.first finds a match and reports its index
static void test_qry02_first_finds_match(void) {
    int values[] = {1, 3, 4, 7};
    int_source src = {values, 4};
    sc_queryable q = int_source_as_queryable(&src);

    usize idx = 999;
    bool found = Query.first(q, is_even, NULL, &idx);

    Assert.isTrue(found, "QRY02: first finds the even element");
    Assert.areEqual(&(usize){2}, &idx, INT, "QRY02: out_index is 2 (value 4)");
}

// QRY03 - Query.first returns false when no element matches
static void test_qry03_first_no_match(void) {
    int values[] = {1, 3, 5, 7};
    int_source src = {values, 4};
    sc_queryable q = int_source_as_queryable(&src);

    usize idx = 999;
    bool found = Query.first(q, is_even, NULL, &idx);

    Assert.isFalse(found, "QRY03: first returns false when nothing matches");
    Assert.areEqual(&(usize){999}, &idx, INT, "QRY03: out_index left untouched on no match");
}

// QRY04 - Query.first "any" usage: caller passes NULL for out_index
static void test_qry04_first_any_usage(void) {
    int values[] = {2, 4, 6};
    int_source src = {values, 3};
    sc_queryable q = int_source_as_queryable(&src);

    Assert.isTrue(Query.first(q, is_even, NULL, NULL), "QRY04: any-style call with NULL out_index");
}

// QRY05 - Query.first stops scanning at the first match (early break)
static int call_count = 0;
static bool counting_is_even(const void *element, usize index, void *userdata) {
    (void)index;
    (void)userdata;
    call_count++;
    return (*(const int *)element % 2) == 0;
}

static void test_qry05_first_early_break(void) {
    int values[] = {1, 1, 2, 1, 1};
    int_source src = {values, 5};
    sc_queryable q = int_source_as_queryable(&src);

    call_count = 0;
    usize idx = 999;
    bool found = Query.first(q, counting_is_even, NULL, &idx);

    Assert.isTrue(found, "QRY05: match found");
    Assert.areEqual(&(usize){2}, &idx, INT, "QRY05: match at index 2");
    Assert.areEqual(&(int){3}, &call_count, INT,
                    "QRY05: predicate called exactly 3 times, not 5 (early break)");
}

// QRY06 - empty source: next and first both report nothing found
static void test_qry06_empty_source(void) {
    int_source src = {NULL, 0};
    sc_queryable q = int_source_as_queryable(&src);

    const void *elem;
    Assert.isFalse(Query.next(&q, &elem, NULL), "QRY06: next on empty source returns false");

    sc_queryable q2 = int_source_as_queryable(&src);
    Assert.isFalse(Query.first(q2, is_even, NULL, NULL), "QRY06: first on empty source returns false");
}

// QRY07 - NULL-argument invariants
static void test_qry07_null_invariants(void) {
    Assert.isFalse(Query.next(NULL, NULL, NULL), "QRY07: next(NULL, ...) returns false, no crash");

    sc_queryable zeroed = {0};
    const void *elem;
    Assert.isFalse(Query.next(&zeroed, &elem, NULL),
                   "QRY07: next on a zero-initialized queryable (NULL advance) returns false");

    int values[] = {1, 2, 3};
    int_source src = {values, 3};
    sc_queryable q = int_source_as_queryable(&src);
    Assert.isFalse(Query.first(q, NULL, NULL, NULL), "QRY07: first(..., NULL pred, ...) returns false");
}

// QRY08 - always-false predicate exhausts the whole source
static void test_qry08_first_exhausts_on_never_match(void) {
    int values[] = {1, 2, 3, 4, 5};
    int_source src = {values, 5};
    sc_queryable q = int_source_as_queryable(&src);

    Assert.isFalse(Query.first(q, always_false, NULL, NULL),
                   "QRY08: predicate that never matches exhausts the source cleanly");
}

//  register test cases
static void register_query_tests(void) {
    testset("core_query_set", set_config, set_teardown);
    DebugLogger.log("Test Source: %s", __FILE__);

    testcase("query_next_visits_in_order", test_qry01_next_visits_in_order);
    testcase("query_first_finds_match", test_qry02_first_finds_match);
    testcase("query_first_no_match", test_qry03_first_no_match);
    testcase("query_first_any_usage", test_qry04_first_any_usage);
    testcase("query_first_early_break", test_qry05_first_early_break);
    testcase("query_empty_source", test_qry06_empty_source);
    testcase("query_null_invariants", test_qry07_null_invariants);
    testcase("query_first_exhausts_on_never_match", test_qry08_first_exhausts_on_never_match);
}
__attribute__((constructor)) static void enqueue_query_tests(void) {
    Tests.enqueue(register_query_tests);
}
