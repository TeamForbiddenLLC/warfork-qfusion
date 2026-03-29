#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>
#include <stdio.h>
#include <string.h>


static void mock_test_a(int value)
{
    check_expected(value);
}

int custom_checker(CMockaValueData param, CMockaValueData check);
int custom_checker(CMockaValueData param, CMockaValueData check)
{
    assert_int_equal(param.uint_val, check.uint_val);
    return true;
}

static void test_expect_check(void **state)
{
    (void)state; /* unused */
    expect_check(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0));
    mock_test_a(0);
}

static void test_expect_check_count(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), 2);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_always(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), EXPECT_ALWAYS);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_maybe_1(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), EXPECT_MAYBE);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_maybe_2(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), EXPECT_MAYBE);
}

static void mock_test_ptr(const void *value)
{
    check_expected_ptr(value);
}

static void test_expect_string(void **state)
{
    (void)state; /* unused */
    char string[64];
    snprintf(string, sizeof(string), "hello world");
    assert_ptr_not_equal(string,
                         "hello world"); // should be different memory addresses

    expect_string(mock_test_ptr, value, string);
    mock_test_ptr("hello world");

    expect_not_string(mock_test_ptr, value, string);
    mock_test_ptr("hello world with extra bytes");

    expect_memory(mock_test_ptr, value, string, strlen(string));
    mock_test_ptr("hello world");

    expect_not_memory(mock_test_ptr, value, string, strlen(string));
    mock_test_ptr("different data");
}

static void test_expect_string_count_always(void **state)
{
    (void)state; /* unused */
    char string[64];
    snprintf(string, sizeof(string), "hello world");
    assert_ptr_not_equal(string,
                         "hello world"); // should be different memory addresses

    expect_string_count(mock_test_ptr, value, string, EXPECT_ALWAYS);
    mock_test_ptr("hello world");
    mock_test_ptr("hello world");
}

static void test_expect_string_count_maybe_1(void **state)
{
    (void)state; /* unused */
    char string[64];
    snprintf(string, sizeof(string), "hello world");
    assert_ptr_not_equal(string,
                         "hello world"); // should be different memory addresses

    expect_string_count(mock_test_ptr, value, string, EXPECT_MAYBE);
    mock_test_ptr("hello world");
    mock_test_ptr("hello world");
}

static void test_expect_string_count_maybe_2(void **state)
{
    (void)state; /* unused */
    const char string[] = "hello world";
    expect_string_count(mock_test_ptr, value, string, EXPECT_MAYBE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_expect_check),
        cmocka_unit_test(test_expect_check_count),
        cmocka_unit_test(test_expect_check_count_always),
        cmocka_unit_test(test_expect_check_count_maybe_1),
        cmocka_unit_test(test_expect_check_count_maybe_2),
        cmocka_unit_test(test_expect_string),
        cmocka_unit_test(test_expect_string_count_always),
        cmocka_unit_test(test_expect_string_count_maybe_1),
        cmocka_unit_test(test_expect_string_count_maybe_2)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
