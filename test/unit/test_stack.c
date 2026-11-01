/*
 *  Test File: test_stack.c
 *  Description: Test cases for the Stack (LIFO collection) interface
 */

#include <sigma.test/sigtest.h>
#include <stdio.h>
#include "stack.h"
#include "fake_arena.h"

//  configure test set
static void set_config(FILE **log_stream) {
    *log_stream = fopen("logs/test_stack.log", "w");
}
static void set_teardown(void) {
}

// ST01 - Stack.new creates a valid, empty stack
static void test_stack_new(void) {
    stack s = Stack.new(4);
    Assert.isNotNull(s, "Stack.new returns non-null");
    Assert.isTrue(Stack.is_empty(s), "new stack is empty");
    Assert.areEqual(&(int){0}, &(int){Stack.count(s)}, INT, "new stack count is 0");
    Stack.dispose(s);
}

// ST02 - push/pop is LIFO order
static void test_stack_push_pop_lifo(void) {
    stack s = Stack.new(4);
    int a = 1, b = 2, c = 3;

    Assert.areEqual(&(int){0}, &(int){Stack.push(s, &a)}, INT, "push a OK");
    Assert.areEqual(&(int){0}, &(int){Stack.push(s, &b)}, INT, "push b OK");
    Assert.areEqual(&(int){0}, &(int){Stack.push(s, &c)}, INT, "push c OK");
    Assert.areEqual(&(int){3}, &(int){Stack.count(s)}, INT, "count is 3 after 3 pushes");

    object out = Stack.pop(s);
    Assert.isTrue(out == &c, "pop returns c (last pushed)");
    out = Stack.pop(s);
    Assert.isTrue(out == &b, "pop returns b");
    out = Stack.pop(s);
    Assert.isTrue(out == &a, "pop returns a (first pushed, popped last)");

    Assert.isTrue(Stack.is_empty(s), "stack empty after popping everything");

    Stack.dispose(s);
}

// ST03 - peek returns the top without removing it
static void test_stack_peek_does_not_mutate(void) {
    stack s = Stack.new(4);
    int a = 1, b = 2;
    Stack.push(s, &a);
    Stack.push(s, &b);

    Assert.isTrue(Stack.peek(s) == &b, "peek returns top (b)");
    Assert.isTrue(Stack.peek(s) == &b, "peek again still returns b");
    Assert.areEqual(&(int){2}, &(int){Stack.count(s)}, INT, "peek does not change count");

    Assert.isTrue(Stack.pop(s) == &b, "pop after peek still returns b");

    Stack.dispose(s);
}

// ST04 - pop/peek on an empty stack return NULL
static void test_stack_pop_peek_empty(void) {
    stack s = Stack.new(4);

    Assert.isNull(Stack.pop(s), "pop on empty stack returns NULL");
    Assert.isNull(Stack.peek(s), "peek on empty stack returns NULL");

    Stack.dispose(s);
}

// ST05 - clear resets the stack to empty
static void test_stack_clear(void) {
    stack s = Stack.new(4);
    int a = 1, b = 2;
    Stack.push(s, &a);
    Stack.push(s, &b);

    Stack.clear(s);

    Assert.isTrue(Stack.is_empty(s), "stack empty after clear");
    Assert.areEqual(&(int){0}, &(int){Stack.count(s)}, INT, "count is 0 after clear");

    Stack.dispose(s);
}

// ST06 - mark/restore truncates to a saved depth, is a real truncate
static void test_stack_mark_restore(void) {
    stack s = Stack.new(4);
    int a = 1, b = 2, c = 3, d = 4;
    Stack.push(s, &a);
    Stack.push(s, &b);

    usize m = Stack.mark(s);
    Assert.areEqual(&(int){2}, &(int){m}, INT, "mark returns current depth (2)");

    Stack.push(s, &c);
    Assert.areEqual(&(int){3}, &(int){Stack.count(s)}, INT, "count is 3 after pushing past mark");

    Stack.restore(s, m);
    Assert.areEqual(&(int){2}, &(int){Stack.count(s)}, INT, "restore truncates back to marked depth");
    Assert.isTrue(Stack.peek(s) == &b, "top after restore is b (c was discarded)");

    /* proves it's a real truncate, not a cosmetic count: pushing again reuses the slot */
    Stack.push(s, &d);
    Assert.areEqual(&(int){2}, &(int){m}, INT, "original mark value unaffected by reuse");
    Assert.isTrue(Stack.peek(s) == &d, "top after push-after-restore is d");
    Assert.areEqual(&(int){3}, &(int){Stack.count(s)}, INT, "count is 3 after pushing after restore");

    Stack.dispose(s);
}

// ST07 - new_with_allocator(NULL) behaves exactly like Stack.new
// (FR-2603-sigma-collections-007/011)
static void test_stack_new_with_allocator_null_is_default_path(void) {
    stack s = Stack.new_with_allocator(4, NULL);
    Assert.isNotNull(s, "new_with_allocator(NULL) returns non-null");

    int a = 1;
    Stack.push(s, &a);
    Assert.areEqual(&(int){1}, &(int){Stack.count(s)}, INT, "push works via default path");

    Stack.dispose(s);
}

// ST08 - new_with_allocator(&use) grows through the override and orphans
// the old buffer instead of releasing it
static void test_stack_new_with_allocator_grow_orphans(void) {
    fake_arena_reset();

    stack s = Stack.new_with_allocator(2, &fake_arena_use);
    Assert.isNotNull(s, "new_with_allocator(&use) returns non-null");
    Assert.isTrue(fake_arena_alloc_calls >= 1, "initial buffer allocated through override");

    int before_grow_calls = fake_arena_alloc_calls;
    int vals[8];
    for (int i = 0; i < 8; i++) {
        vals[i] = i * 10;
        Assert.areEqual(&(int){0}, &(int){Stack.push(s, &vals[i])}, INT, "push OK while growing");
    }

    Assert.areEqual(&(int){8}, &(int){Stack.count(s)}, INT, "count is 8 after growth");
    Assert.isTrue(fake_arena_alloc_calls > before_grow_calls,
                  "growth allocated new buffer(s) through the override");
    Assert.areEqual(&(int){0}, &fake_arena_release_calls, INT,
                    "growth never released the orphaned buffer through the override");

    Assert.isTrue(Stack.peek(s) == &vals[7], "top is correct after arena-backed growth");

    Stack.dispose(s);
}

// ST09 - dispose skips releasing the bucket through the override - the
// bound allocator owns bulk-reclaiming it
static void test_stack_dispose_skips_release_when_overridden(void) {
    fake_arena_reset();

    stack s = Stack.new_with_allocator(4, &fake_arena_use);
    int a = 1;
    Stack.push(s, &a);

    Stack.dispose(s);

    Assert.areEqual(&(int){0}, &fake_arena_release_calls, INT,
                    "dispose never released the buffer through the override");
}

//  register test cases
static void register_stack_tests(void) {
    testset("core_stack_set", set_config, set_teardown);
    DebugLogger.log("Test Source: %s", __FILE__);

    testcase("stack_new", test_stack_new);
    testcase("stack_push_pop_lifo", test_stack_push_pop_lifo);
    testcase("stack_peek_does_not_mutate", test_stack_peek_does_not_mutate);
    testcase("stack_pop_peek_empty", test_stack_pop_peek_empty);
    testcase("stack_clear", test_stack_clear);
    testcase("stack_mark_restore", test_stack_mark_restore);
    testcase("stack_new_with_allocator_null_is_default_path",
             test_stack_new_with_allocator_null_is_default_path);
    testcase("stack_new_with_allocator_grow_orphans", test_stack_new_with_allocator_grow_orphans);
    testcase("stack_dispose_skips_release_when_overridden",
             test_stack_dispose_skips_release_when_overridden);
}
__attribute__((constructor)) static void enqueue_stack_tests(void) {
    Tests.enqueue(register_stack_tests);
}
