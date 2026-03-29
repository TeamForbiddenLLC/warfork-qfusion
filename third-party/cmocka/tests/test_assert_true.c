#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>
#include <cmocka_private.h>

static void test_assert_true(void **state)
{
    int x = 10;

    (void)state; /* unused */
    assert_true(x == 10);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_assert_true),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
