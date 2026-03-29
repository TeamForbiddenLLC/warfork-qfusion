#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

#include <stdlib.h>

int mock_function(void);
void mock_function_call_times(size_t times, int expectedValue);

int mock_function(void)
{
  return (int) mock();
}

uintmax_t mock_function_uint(void);
uintmax_t mock_function_uint(void)
{
    return mock_uint();
}

void *mock_function_ptr(void);
void *mock_function_ptr(void)
{
    return mock_ptr_type(void *);
}


void mock_function_call_times(size_t times, int expectedValue)
{
    size_t i;
    for (i = 0u; i < times; ++i)
    {
        assert_int_equal(expectedValue, mock_function());
    }
}

static void test_will_return_fails_for_no_calls(void **state)
{
    (void) state;

    will_return(mock_function, 32);
}

static void test_will_return_count_fails_for_unreturned_items(void **state)
{
    int value;
    size_t numberOfCalls;

    (void) state;

    value = rand();
    numberOfCalls = (size_t) ((rand()) % 20 + 2);

    will_return_count(mock_function, value, numberOfCalls);
    mock_function_call_times(numberOfCalls - 1u, value);
}

static void test_will_return_always_fails_for_no_calls(void **state)
{
    int value;

    (void) state;

    value = rand();

    will_return_always(mock_function, value);
}

static int teardown(void **state) {
    free(*state);

    return 0;
}

static void test_will_return_int_type_mismatch(void **state)
{
    intmax_t value = rand();

    (void) state;

    will_return_int(mock_function_uint, value);
    mock_function_uint();
}

static void test_will_return_ptr_type_mismatch(void **state)
{
    const char *value = "What a Wurst!";

    (void) state;

    will_return_ptr_type(mock_function_uint, value, const char *);
    mock_function_ptr();
}

int main(void)
{
    const struct CMUnitTest will_return_mock_tests[] = {
        cmocka_unit_test_teardown(test_will_return_fails_for_no_calls, teardown),
        cmocka_unit_test_teardown(test_will_return_count_fails_for_unreturned_items, teardown),
        cmocka_unit_test_teardown(test_will_return_always_fails_for_no_calls, teardown),
        cmocka_unit_test_teardown(test_will_return_fails_for_no_calls, teardown),
        cmocka_unit_test(test_will_return_int_type_mismatch),
        cmocka_unit_test(test_will_return_ptr_type_mismatch),
    };

    return cmocka_run_group_tests(will_return_mock_tests, NULL, NULL);
}
