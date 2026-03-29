#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

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

static void test_no_expects_fail(void **state)
{
    (void)state; /* unused */
    mock_test_a(0);
}

static void test_expect_check_count_remaining_fail(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), 2);
    mock_test_a(0);
}

static void test_expect_check_count_always_remaining_fail(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, cast_int_to_cmocka_value(0), EXPECT_ALWAYS);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_no_expects_fail),
        cmocka_unit_test(test_expect_check_count_remaining_fail),
        cmocka_unit_test(test_expect_check_count_always_remaining_fail),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
