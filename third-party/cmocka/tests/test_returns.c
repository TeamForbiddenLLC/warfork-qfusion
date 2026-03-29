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

intmax_t mock_function_int(void);
intmax_t mock_function_int(void)
{
    return mock_int();
}

uintmax_t mock_function_uint(void);
uintmax_t mock_function_uint(void)
{
    return mock_uint();
}

uint64_t mock_function_uint64(void);
uint64_t mock_function_uint64(void)
{
    return (uint64_t)mock_uint();
}

double mock_function_float(void);
double mock_function_float(void)
{
    return mock_float();
}

const char *mock_function_ptr(void);
const char *mock_function_ptr(void)
{
    return mock_ptr_type(const char *);
}

void mock_function_call_times(size_t times, int expectedValue)
{
    size_t i;
    for (i = 0u; i < times; ++i)
    {
        assert_int_equal(expectedValue, mock_function());
    }
}

static void test_will_return_maybe_for_no_calls(void **state)
{
    (void) state;

    will_return_maybe(mock_function, 32);
}

static void test_will_return_maybe_for_one_mock_call(void **state)
{
    int value;

    (void) state;

    value = rand();
    will_return_maybe(mock_function, value);
    mock_function_call_times(1u, value);
}

static void test_will_return_maybe_for_more_than_one_call(void **state)
{
    int value;
    size_t numberOfCalls;
    (void)state;

    value = rand();
    numberOfCalls = (size_t) ((rand()) % 20 + 2);
    will_return_maybe(mock_function, value);
    mock_function_call_times(numberOfCalls, value);
}

static void test_will_return_int(void **state)
{
    intmax_t value;

    (void)state; /* unused */

    value = rand();
    will_return_int(mock_function_int, value);
    assert_int_equal(value, mock_function_int());
}

static void test_will_return_uint(void **state)
{
    uintmax_t value;

    (void)state; /* unused */

    value = rand();
    will_return_uint(mock_function_uint, value);
    assert_uint_equal(value, mock_function_uint());
}

static void test_will_return_uint64(void **state)
{
    uint64_t value = 86405000000UL;
    uint64_t ret;

    (void)state; /* unused */

    will_return_uint(mock_function_uint64, value);
    ret = mock_function_uint64();
    assert_uint_equal(ret, 86405000000UL);
}

static void test_will_return_float(void **state)
{
    double value = 1.0;

    (void)state; /* unused */

    will_return_float(mock_function_float, value);
    assert_float_equal(value, mock_function_float(), 0.0);
}

static void test_will_return_ptr(void **state)
{
    const char *value = "What a Wurst!";

    (void)state; /* unused */

    will_return_ptr_type(mock_function_ptr, value, const char *);
    assert_string_equal(value, mock_function_ptr());
}

int main(int argc, char **argv) {
    const struct CMUnitTest alloc_tests[] = {
        cmocka_unit_test(test_will_return_maybe_for_no_calls),
        cmocka_unit_test(test_will_return_maybe_for_one_mock_call),
        cmocka_unit_test(test_will_return_maybe_for_more_than_one_call),
        cmocka_unit_test(test_will_return_int),
        cmocka_unit_test(test_will_return_uint),
        cmocka_unit_test(test_will_return_uint64),
        cmocka_unit_test(test_will_return_float),
        cmocka_unit_test(test_will_return_ptr),
    };

    (void)argc;
    (void)argv;

    return cmocka_run_group_tests(alloc_tests, NULL, NULL);
}
