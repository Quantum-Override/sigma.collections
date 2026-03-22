/*
 *  Test File: test_map.c
 *  Description: Test cases for Map collection (string-keyed hash map)
 */

#include <sigtest/sigtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

// Test set configuration
static void set_config(FILE **log_stream) {
    *log_stream = fopen("logs/test_map.log", "w");
}

static void set_teardown(void) {
    // No teardown needed
}

//------------------------------------------------------------------------------
// Basic Operations Tests
//------------------------------------------------------------------------------

static void test_map_new_dispose(void) {
    map m = Map.new(16);
    Assert.isNotNull(m, "Map creation should succeed");
    
    usize cap = Map.capacity(m);
    Assert.isTrue(cap >= 16, "Capacity should be at least requested size");
    Assert.isTrue((cap & (cap - 1)) == 0, "Capacity should be power of 2");
    
    usize count = Map.count(m);
    Assert.isTrue(count == 0, "New map should be empty");
    
    Map.dispose(m);
}

static void test_map_set_get_single(void) {
    map m = Map.new(8);
    Assert.isNotNull(m, "Map creation should succeed");
    
    const char *key = "test_key";
    usize value = 42;
    
    int result = Map.set(m, key, strlen(key), value);
    Assert.isTrue(result == 0, "Set should succeed");
    Assert.isTrue(Map.count(m) == 1, "Count should be 1");
    
    usize retrieved;
    int found = Map.get(m, key, strlen(key), &retrieved);
    Assert.isTrue(found, "Key should be found");
    Assert.isTrue(retrieved == value, "Retrieved value should match");
    
    Map.dispose(m);
}

static void test_map_set_multiple(void) {
    map m = Map.new(8);
    Assert.isNotNull(m, "Map creation should succeed");
    
    Map.set(m, "key1", 4, 100);
    Map.set(m, "key2", 4, 200);
    Map.set(m, "key3", 4, 300);
    
    Assert.isTrue(Map.count(m) == 3, "Count should be 3");
    
    usize val;
    Assert.isTrue(Map.get(m, "key1", 4, &val) && val == 100, "key1 should be 100");
    Assert.isTrue(Map.get(m, "key2", 4, &val) && val == 200, "key2 should be 200");
    Assert.isTrue(Map.get(m, "key3", 4, &val) && val == 300, "key3 should be 300");
    
    Map.dispose(m);
}

static void test_map_update_existing_key(void) {
    map m = Map.new(8);
    
    const char *key = "update_me";
    Map.set(m, key, strlen(key), 1);
    Assert.isTrue(Map.count(m) == 1, "Count should be 1 after insert");
    
    Map.set(m, key, strlen(key), 2);
    Assert.isTrue(Map.count(m) == 1, "Count should still be 1 after update");
    
    usize val;
    Map.get(m, key, strlen(key), &val);
    Assert.isTrue(val == 2, "Value should be updated to 2");
    
    Map.dispose(m);
}

static void test_map_get_nonexistent(void) {
    map m = Map.new(8);
    
    usize val;
    int found = Map.get(m, "missing", 7, &val);
    Assert.isFalse(found, "Nonexistent key should not be found");
    
    Map.dispose(m);
}

//------------------------------------------------------------------------------
// Has/Remove Tests
//------------------------------------------------------------------------------

static void test_map_has(void) {
    map m = Map.new(8);
    
    Map.set(m, "exists", 6, 123);
    
    Assert.isTrue(Map.has(m, "exists", 6), "Key 'exists' should be present");
    Assert.isFalse(Map.has(m, "missing", 7), "Key 'missing' should be absent");
    
    Map.dispose(m);
}

static void test_map_remove(void) {
    map m = Map.new(8);
    
    Map.set(m, "remove_me", 9, 999);
    Assert.isTrue(Map.count(m) == 1, "Count should be 1");
    
    int removed = Map.remove(m, "remove_me", 9);
    Assert.isTrue(removed, "Remove should return 1");
    Assert.isTrue(Map.count(m) == 0, "Count should be 0 after removal");
    
    Assert.isFalse(Map.has(m, "remove_me", 9), "Removed key should not exist");
    
    Map.dispose(m);
}

static void test_map_remove_nonexistent(void) {
    map m = Map.new(8);
    
    int removed = Map.remove(m, "never_existed", 13);
    Assert.isFalse(removed, "Remove nonexistent key should return 0");
    
    Map.dispose(m);
}

static void test_map_remove_and_reinsert(void) {
    map m = Map.new(8);
    
    Map.set(m, "key", 3, 1);
    Map.remove(m, "key", 3);
    
    // Tombstone should allow re-insertion
    Map.set(m, "key", 3, 2);
    Assert.isTrue(Map.count(m) == 1, "Count should be 1 after re-insert");
    
    usize val;
    Map.get(m, "key", 3, &val);
    Assert.isTrue(val == 2, "Re-inserted value should be 2");
    
    Map.dispose(m);
}

//------------------------------------------------------------------------------
// Resize/Growth Tests
//------------------------------------------------------------------------------

static void test_map_automatic_resize(void) {
    map m = Map.new(8);
    usize initial_cap = Map.capacity(m);
    
    // Allocate keys on heap
    char **keys = malloc(10 * sizeof(char *));
    
    // Insert enough to trigger resize (>50% load factor)
    for (int i = 0; i < 10; i++) {
        keys[i] = malloc(16);
        snprintf(keys[i], 16, "key%d", i);
        Map.set(m, keys[i], strlen(keys[i]), (usize)i);
    }
    
    usize final_cap = Map.capacity(m);
    Assert.isTrue(final_cap > initial_cap, "Capacity should have increased");
    Assert.isTrue(Map.count(m) == 10, "All 10 entries should be present");
    
    // Verify all keys still accessible after resize
    for (int i = 0; i < 10; i++) {
        usize val;
        Assert.isTrue(Map.get(m, keys[i], strlen(keys[i]), &val), "Key should still exist");
        Assert.isTrue(val == (usize)i, "Value should be preserved");
    }
    
    Map.dispose(m);
    
    // Free keys
    for (int i = 0; i < 10; i++) {
        free(keys[i]);
    }
    free(keys);
}

static void test_map_large_insertion(void) {
    map m = Map.new(16);
    
    // Allocate keys on heap (map doesn't own them, so we track them)
    char **keys = malloc(100 * sizeof(char *));
    
    // Insert 100 entries
    for (int i = 0; i < 100; i++) {
        keys[i] = malloc(32);
        snprintf(keys[i], 32, "entry_%d", i);
        Map.set(m, keys[i], strlen(keys[i]), (usize)(i * 10));
    }
    
    Assert.isTrue(Map.count(m) == 100, "Should have 100 entries");
    
    // Spot check some entries
    usize val;
    Assert.isTrue(Map.get(m, "entry_0", 7, &val) && val == 0, "entry_0 correct");
    Assert.isTrue(Map.get(m, "entry_50", 8, &val) && val == 500, "entry_50 correct");
    Assert.isTrue(Map.get(m, "entry_99", 8, &val) && val == 990, "entry_99 correct");
    
    Map.dispose(m);
    
    // Free keys
    for (int i = 0; i < 100; i++) {
        free(keys[i]);
    }
    free(keys);
}

//------------------------------------------------------------------------------
// Edge Cases
//------------------------------------------------------------------------------

static void test_map_empty_key(void) {
    map m = Map.new(8);
    
    // Zero-length key should work
    int result = Map.set(m, "", 0, 42);
    Assert.isTrue(result == 0, "Empty key set should succeed");
    
    usize val;
    Assert.isTrue(Map.get(m, "", 0, &val), "Empty key should be found");
    Assert.isTrue(val == 42, "Empty key value should be correct");
    
    Map.dispose(m);
}

static void test_map_key_with_nulls(void) {
    map m = Map.new(8);
    
    // Key containing null bytes (not null-terminated string)
    const char key_with_nulls[] = {'a', '\0', 'b', '\0', 'c'};
    usize key_len = 5;
    
    Map.set(m, key_with_nulls, key_len, 123);
    
    usize val;
    Assert.isTrue(Map.get(m, key_with_nulls, key_len, &val), "Key with nulls should be found");
    Assert.isTrue(val == 123, "Value should be correct");
    
    Map.dispose(m);
}

static void test_map_collision_handling(void) {
    map m = Map.new(8);
    
    // Insert multiple keys that may cause collisions
    const char *keys[] = {"abc", "bca", "cab", "def", "fed", "efd"};
    usize num_keys = 6;
    
    for (usize i = 0; i < num_keys; i++) {
        Map.set(m, keys[i], 3, (usize)(i + 1));
    }
    
    Assert.isTrue(Map.count(m) == num_keys, "All keys should be inserted");
    
    // Verify all keys are distinct and retrievable
    for (usize i = 0; i < num_keys; i++) {
        usize val;
        Assert.isTrue(Map.get(m, keys[i], 3, &val), "Key should be found");
        Assert.isTrue(val == (usize)(i + 1), "Value should match");
    }
    
    Map.dispose(m);
}

static void test_map_null_safety(void) {
    usize val;
    
    // NULL map operations should not crash
    Assert.isTrue(Map.set(NULL, "key", 3, 1) == -1, "Set on NULL should fail");
    Assert.isFalse(Map.get(NULL, "key", 3, &val), "Get on NULL should return 0");
    Assert.isFalse(Map.has(NULL, "key", 3), "Has on NULL should return 0");
    Assert.isFalse(Map.remove(NULL, "key", 3), "Remove on NULL should return 0");
    Assert.isTrue(Map.count(NULL) == 0, "Count on NULL should be 0");
    Assert.isTrue(Map.capacity(NULL) == 0, "Capacity on NULL should be 0");
    
    // NULL key operations
    map m = Map.new(8);
    Assert.isTrue(Map.set(m, NULL, 3, 1) == -1, "Set with NULL key should fail");
    Assert.isFalse(Map.get(m, NULL, 3, &val), "Get with NULL key should return 0");
    
    Map.dispose(m);
    Map.dispose(NULL);  // Should not crash
}

//------------------------------------------------------------------------------
// Anvil-like Usage Pattern (arena-allocated keys)
//------------------------------------------------------------------------------

static void test_map_arena_pattern(void) {
    map m = Map.new(16);
    
    // Simulate arena-allocated string keys
    char arena[256];
    usize arena_offset = 0;
    
    // "Allocate" keys in arena
    const char *key1 = arena + arena_offset;
    strcpy(arena + arena_offset, "field_name");
    arena_offset += strlen("field_name") + 1;
    
    const char *key2 = arena + arena_offset;
    strcpy(arena + arena_offset, "type");
    arena_offset += strlen("type") + 1;
    
    const char *key3 = arena + arena_offset;
    strcpy(arena + arena_offset, "schema_id");
    arena_offset += strlen("schema_id") + 1;
    
    // Use map with arena keys
    Map.set(m, key1, strlen(key1), 100);
    Map.set(m, key2, strlen(key2), 200);
    Map.set(m, key3, strlen(key3), 300);
    
    usize val;
    Assert.isTrue(Map.get(m, "field_name", 10, &val) && val == 100, "field_name found");
    Assert.isTrue(Map.get(m, "type", 4, &val) && val == 200, "type found");
    Assert.isTrue(Map.get(m, "schema_id", 9, &val) && val == 300, "schema_id found");
    
    Map.dispose(m);
    // Arena outlives map - this is the intended pattern
}

//------------------------------------------------------------------------------
// Test Registration
//------------------------------------------------------------------------------

__attribute__((constructor)) void init_map_tests(void) {
    testset("core_map_set", set_config, set_teardown);
    
    // Basic operations
    testcase("map_new_dispose", test_map_new_dispose);
    testcase("map_set_get_single", test_map_set_get_single);
    testcase("map_set_multiple", test_map_set_multiple);
    testcase("map_update_existing_key", test_map_update_existing_key);
    testcase("map_get_nonexistent", test_map_get_nonexistent);
    
    // Has/Remove
    testcase("map_has", test_map_has);
    testcase("map_remove", test_map_remove);
    testcase("map_remove_nonexistent", test_map_remove_nonexistent);
    testcase("map_remove_and_reinsert", test_map_remove_and_reinsert);
    
    // Resize/Growth
    testcase("map_automatic_resize", test_map_automatic_resize);
    testcase("map_large_insertion", test_map_large_insertion);
    
    // Edge cases
    testcase("map_empty_key", test_map_empty_key);
    testcase("map_key_with_nulls", test_map_key_with_nulls);
    testcase("map_collision_handling", test_map_collision_handling);
    testcase("map_null_safety", test_map_null_safety);
    
    // Usage patterns
    testcase("map_arena_pattern", test_map_arena_pattern);
}
