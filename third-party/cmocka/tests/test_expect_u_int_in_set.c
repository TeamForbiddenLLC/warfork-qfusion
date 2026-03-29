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


static void mock_test_int(intmax_t value)
{
    check_expected(value);
}

static void mock_test_uint(uintmax_t value)
{
    check_expected(value);
}

static void test_expect_int_in_set_count(void **state)
{
    intmax_t set[] = { -1, 0, 1 };
    (void)state; /* unused */

    expect_int_in_set_count(mock_test_int, value, set, 1);

    mock_test_int(-1);
}

static void test_expect_uint_in_set_count(void **state)
{
    uintmax_t set[] = { 0, 1, 42, UINTMAX_MAX };
    (void)state; /* unused */

    expect_uint_in_set_count(mock_test_uint, value, set, 1);

    mock_test_uint(42);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_expect_int_in_set_count),
        cmocka_unit_test(test_expect_uint_in_set_count)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
