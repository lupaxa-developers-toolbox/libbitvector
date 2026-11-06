/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/bitvector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static int tests_run = 0;
static int tests_failed = 0;
static const char *current_test = NULL;

static void expect(int condition, const char *file, int line, const char *expr)
{
    if (!condition) {
        printf("FAIL %s %s:%d: %s\n",
               current_test != NULL ? current_test : "?",
               file, line, expr);
        fflush(stdout);
        failures++;
    }
}

#define EXPECT(cond) expect((cond), __FILE__, __LINE__, #cond)

static int semver_major_minor(const char *version)
{
    int parts = 0;

    if (version == NULL) {
        return 0;
    }

    while (parts < 2) {
        if (*version < '0' || *version > '9') {
            return 0;
        }
        while (*version >= '0' && *version <= '9') {
            version++;
        }
        parts++;
        if (parts == 2) {
            return 1;
        }
        if (*version != '.') {
            return 0;
        }
        version++;
    }
    return 0;
}

static void test_version_string_is_semver(void)
{
    EXPECT(semver_major_minor(BITVECTOR_VERSION_STRING));
}

static void test_empty_vector(void)
{
    bitvector *bv = NULL;
    char *text = NULL;
    int empty = 7;
    int full = 7;

    EXPECT(bitvector_create(NULL, 0) == BITVECTOR_INVAL);
    EXPECT(bitvector_create(&bv, 0) == BITVECTOR_OK);
    EXPECT(bv != NULL);
    EXPECT(bitvector_length(bv) == 0);
    EXPECT(bitvector_length(NULL) == 0);
    EXPECT(bitvector_is_empty(bv, &empty) == BITVECTOR_OK);
    EXPECT(empty == 1);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 0);
    EXPECT(bitvector_to_string(bv, &text) == BITVECTOR_OK);
    EXPECT(text != NULL);
    EXPECT(strcmp(text, "") == 0);
    free(text);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
    EXPECT(bv == NULL);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
    EXPECT(bitvector_destroy(NULL) == BITVECTOR_INVAL);
}

static void test_length_stays_when_bits_fit(void)
{
    bitvector *bv = NULL;
    int bit = 7;
    int empty = 7;
    int full = 7;
    size_t i;

    EXPECT(bitvector_create(&bv, 10) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 10);
    EXPECT(bitvector_get(bv, 10, &bit) == BITVECTOR_INVAL);
    EXPECT(bit == 7);
    for (i = 0; i < 10; i++) {
        EXPECT(bitvector_get(bv, i, &bit) == BITVECTOR_OK);
        EXPECT(bit == 0);
    }
    EXPECT(bitvector_is_empty(bv, &empty) == BITVECTOR_OK);
    EXPECT(empty == 1);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 0);
    EXPECT(bitvector_set(bv, 3) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 10);
    EXPECT(bitvector_get(bv, 3, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_is_empty(bv, &empty) == BITVECTOR_OK);
    EXPECT(empty == 0);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void test_fill_unset_and_toggle(void)
{
    bitvector *bv = NULL;
    int full = 0;
    int bit = 0;
    size_t i;

    EXPECT(bitvector_create(&bv, 10) == BITVECTOR_OK);
    for (i = 0; i < 10; i++) {
        EXPECT(bitvector_set(bv, i) == BITVECTOR_OK);
    }
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 1);
    EXPECT(bitvector_unset(bv, 0) == BITVECTOR_OK);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 0);
    EXPECT(bitvector_toggle(bv, 0) == BITVECTOR_OK);
    EXPECT(bitvector_get(bv, 0, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 1);
    EXPECT(bitvector_clear(bv) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 10);
    EXPECT(bitvector_is_empty(bv, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void test_grow_clears_the_gap(void)
{
    bitvector *bv = NULL;
    int bit = 7;
    size_t i;

    EXPECT(bitvector_create(&bv, 1) == BITVECTOR_OK);
    EXPECT(bitvector_set(bv, 0) == BITVECTOR_OK);
    EXPECT(bitvector_set(bv, 70) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 71);
    EXPECT(bitvector_get(bv, 0, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_get(bv, 70, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    for (i = 1; i < 70; i++) {
        EXPECT(bitvector_get(bv, i, &bit) == BITVECTOR_OK);
        EXPECT(bit == 0);
    }
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void test_word_boundary(void)
{
    bitvector *bv = NULL;
    int bit = 0;
    int full = 1;
    size_t i;

    EXPECT(bitvector_create(&bv, 64) == BITVECTOR_OK);
    EXPECT(bitvector_set(bv, 63) == BITVECTOR_OK);
    EXPECT(bitvector_get(bv, 63, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 0);
    EXPECT(bitvector_set(bv, 64) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 65);
    EXPECT(bitvector_get(bv, 64, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    for (i = 0; i < 64; i++) {
        EXPECT(bitvector_set(bv, i) == BITVECTOR_OK);
    }
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 1);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void test_invert_uses_logical_length(void)
{
    bitvector *bv = NULL;
    int bit = 0;
    int full = 0;
    int empty = 0;
    char *text = NULL;

    EXPECT(bitvector_create(&bv, 3) == BITVECTOR_OK);
    EXPECT(bitvector_invert(bv) == BITVECTOR_OK);
    EXPECT(bitvector_is_full(bv, &full) == BITVECTOR_OK);
    EXPECT(full == 1);
    EXPECT(bitvector_to_string(bv, &text) == BITVECTOR_OK);
    EXPECT(strcmp(text, "111") == 0);
    free(text);
    EXPECT(bitvector_invert(bv) == BITVECTOR_OK);
    EXPECT(bitvector_is_empty(bv, &empty) == BITVECTOR_OK);
    EXPECT(empty == 1);
    EXPECT(bitvector_get(bv, 0, &bit) == BITVECTOR_OK);
    EXPECT(bit == 0);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void test_string_keeps_trailing_zeros(void)
{
    bitvector *bv = NULL;
    bitvector *parsed = NULL;
    char *text = NULL;
    int bit = 0;
    bitvector *kept;

    EXPECT(bitvector_from_string("1000", &bv) == BITVECTOR_OK);
    EXPECT(bitvector_length(bv) == 4);
    EXPECT(bitvector_get(bv, 0, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_get(bv, 3, &bit) == BITVECTOR_OK);
    EXPECT(bit == 0);
    EXPECT(bitvector_to_string(bv, &text) == BITVECTOR_OK);
    EXPECT(strcmp(text, "1000") == 0);
    free(text);
    EXPECT(bitvector_from_string("1001", &parsed) == BITVECTOR_OK);
    EXPECT(bitvector_get(parsed, 3, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_destroy(&parsed) == BITVECTOR_OK);
    kept = bv;
    EXPECT(bitvector_from_string("102", &bv) == BITVECTOR_INVAL);
    EXPECT(bv == kept);
    EXPECT(bitvector_from_string("", &parsed) == BITVECTOR_OK);
    EXPECT(bitvector_length(parsed) == 0);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
    EXPECT(bitvector_destroy(&parsed) == BITVECTOR_OK);
}

static void test_clone_is_independent(void)
{
    bitvector *bv = NULL;
    bitvector *copy = NULL;
    int bit = 0;

    EXPECT(bitvector_from_string("101", &bv) == BITVECTOR_OK);
    EXPECT(bitvector_clone(bv, &copy) == BITVECTOR_OK);
    EXPECT(bitvector_length(copy) == 3);
    EXPECT(bitvector_set(copy, 1) == BITVECTOR_OK);
    EXPECT(bitvector_get(bv, 1, &bit) == BITVECTOR_OK);
    EXPECT(bit == 0);
    EXPECT(bitvector_get(copy, 1, &bit) == BITVECTOR_OK);
    EXPECT(bit == 1);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
    EXPECT(bitvector_destroy(&copy) == BITVECTOR_OK);
}

static void test_invalid_arguments(void)
{
    bitvector *bv = NULL;
    int flag = 7;
    char *text = NULL;

    EXPECT(bitvector_create(&bv, 4) == BITVECTOR_OK);
    EXPECT(bitvector_set(NULL, 0) == BITVECTOR_INVAL);
    EXPECT(bitvector_unset(NULL, 0) == BITVECTOR_INVAL);
    EXPECT(bitvector_toggle(NULL, 0) == BITVECTOR_INVAL);
    EXPECT(bitvector_get(NULL, 0, &flag) == BITVECTOR_INVAL);
    EXPECT(bitvector_get(bv, 0, NULL) == BITVECTOR_INVAL);
    EXPECT(flag == 7);
    EXPECT(bitvector_invert(NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_clear(NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_is_empty(NULL, &flag) == BITVECTOR_INVAL);
    EXPECT(bitvector_is_empty(bv, NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_is_full(bv, NULL) == BITVECTOR_INVAL);
    EXPECT(flag == 7);
    EXPECT(bitvector_to_string(bv, NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_from_string(NULL, &bv) == BITVECTOR_INVAL);
    EXPECT(bitvector_from_string("1", NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_clone(NULL, &bv) == BITVECTOR_INVAL);
    EXPECT(bitvector_clone(bv, NULL) == BITVECTOR_INVAL);
    EXPECT(bitvector_to_string(bv, &text) == BITVECTOR_OK);
    EXPECT(strcmp(text, "0000") == 0);
    free(text);
    EXPECT(bitvector_destroy(&bv) == BITVECTOR_OK);
}

static void run_test(const char *name, void (*fn)(void))
{
    int before = failures;

    current_test = name;
    fn();
    tests_run++;
    if (failures == before) {
        printf("tests/test_bitvector.c::%s PASSED\n", name);
    } else {
        tests_failed++;
        printf("tests/test_bitvector.c::%s FAILED\n", name);
    }
    fflush(stdout);
}

int main(void)
{
    int passed;

    run_test("test_version_string_is_semver", test_version_string_is_semver);
    run_test("test_empty_vector", test_empty_vector);
    run_test("test_length_stays_when_bits_fit", test_length_stays_when_bits_fit);
    run_test("test_fill_unset_and_toggle", test_fill_unset_and_toggle);
    run_test("test_grow_clears_the_gap", test_grow_clears_the_gap);
    run_test("test_word_boundary", test_word_boundary);
    run_test("test_invert_uses_logical_length", test_invert_uses_logical_length);
    run_test("test_string_keeps_trailing_zeros", test_string_keeps_trailing_zeros);
    run_test("test_clone_is_independent", test_clone_is_independent);
    run_test("test_invalid_arguments", test_invalid_arguments);

    passed = tests_run - tests_failed;
    if (tests_failed != 0) {
        printf("%d failed, %d passed, %d assertion(s) failed\n",
               tests_failed, passed, failures);
        return 1;
    }
    printf("%d passed\n", passed);
    return 0;
}
