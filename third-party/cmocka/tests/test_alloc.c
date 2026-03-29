#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void torture_test_calloc(void **state)
{
    void *ptr;

    (void)state; /* unsused */

    ptr = test_calloc(2, SIZE_MAX);
    assert_null(ptr);
    ptr = test_calloc(SIZE_MAX, 2);
    assert_null(ptr);

    /*  overflows to 0  */
    ptr = test_calloc(2, (SIZE_MAX/2)+1);
    assert_null(ptr);

    ptr = test_calloc(3, (SIZE_MAX/2)+42);
    assert_null(ptr);
}

static void torture_test_malloc(void **state)
{
    char *str;
    size_t str_len;
    size_t len;

    (void)state; /* unsused */

    str_len = 12;
    str = (char *)test_malloc(str_len);
    assert_non_null(str);

    len = snprintf(str, str_len, "test string");
    assert_int_equal(len, 11);

    len = strlen(str);
    assert_int_equal(len, 11);

    test_free(str);
}

static void torture_test_realloc(void **state)
{
    char *str;
    char *tmp;
    size_t str_len;
    size_t len;

    (void)state; /* unsused */

    str_len = 16;
    str = (char *)test_malloc(str_len);
    assert_non_null(str);

    len = snprintf(str, str_len, "test string 123");
    assert_int_equal(len, 15);

    len = strlen(str);
    assert_int_equal(len, 15);

    str_len = 20;
    tmp = test_realloc(str, str_len);
    assert_non_null(tmp);

    str = tmp;
    len = strlen(str);
    assert_string_equal(tmp, "test string 123");

    snprintf(str + len, str_len - len, "4567");
    assert_string_equal(tmp, "test string 1234567");

    test_free(str);
}

static void torture_test_realloc_set0(void **state)
{
    char *str;
    size_t str_len;

    (void)state; /* unsused */

    str_len = 16;
    str = (char *)test_malloc(str_len);
    assert_non_null(str);

    /* realloc(ptr, 0) is like a free() */
    str = (char *)test_realloc(str, 0);
    assert_null(str);
}

int main(void) {
    const struct CMUnitTest alloc_tests[] = {
        cmocka_unit_test(torture_test_calloc),
        cmocka_unit_test(torture_test_malloc),
        cmocka_unit_test(torture_test_realloc),
        cmocka_unit_test(torture_test_realloc_set0),
    };

    return cmocka_run_group_tests(alloc_tests, NULL, NULL);
}
