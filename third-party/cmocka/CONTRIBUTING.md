CONTRIBUTING TO CMOCKA
======================

## Merge requests

cmocka uses gitlab for contributing code and code reviews. You can find the
gitlab repository [here](https://gitlab.com/cmocka/cmocka).

## Coding conventions

### Quick Start

Coding style guidelines are about reducing the number of unnecessary
reformatting patches and making things easier for developers to work together.

You don't have to like them or even agree with them, but once put in place we
all have to abide by them (or vote to change them). However, coding style should
never outweigh coding itself and so the guidelines described here should be good
enough to follow as they are common and supported by tools and editors.

The basic style for C code is the Linux Kernel coding style (See
Documentation/CodingStyle in the Kernel source tree). This closely matches what
cmocka developers use already anyways, with a few exceptions as mentioned below.

To save you the trouble of reading the Linux kernel style guide, here are the
highlights.

* Maximum Line Width is 80 Characters
  The reason is not about people with low-res screens but rather sticking to 80
  columns prevents you from nesting more than one level of if statements or
  other code blocks.

* Use 4 spaces to indent

* No trailing spaces
  Clean up your files before committing.

* Follow the K&R guidelines. We won't go through all of them here. You have a
  copy of "The C Programming Language" anyways right?


### Editor Hints

The projects provides a configuration file for [editorconf](https://editorconfig.org/).

Most modern code editor provide support for editorconfig either natively or via
a plugin. You should make use of it!

### FAQ & Statement Reference

#### Comments

Comments should always use the standard C syntax. C++ style comments are not
encouraged.

The lines before a comment should be empty. If the comment directly belongs to
the following code, there should be no empty line after the comment, except if
the comment contains a summary of multiple following code blocks.

This is good:

```c
    ...
    int i;

    /*
     * This is a multi line comment,
     * which explains the logical steps we have to do:
     *
     * 1. We need to set i=5, because...
     * 2. We need to call complex_fn1
     */

    /* This is a one line comment about i = 5. */
    i = 5;

    /*
     * This is a multi line comment,
     * explaining the call to complex_fn1()
     */
    ret = complex_fn1();
    if (ret != 0) {
    ...

    /**
     * @brief This is a doxygen comment.
     *
     * This is a more detailed explanation of
     * this simple function.
     *
     * @param[in]   param1  The parameter value of the function.
     *
     * @param[out]  result1 The result value of the function.
     *
     * @return              0 on success and -1 on error.
     */
    int example(int param1, int *result1);
```

This is bad:

```c
    ...
    int i;
    /*
     * This is a multi line comment,
     * which explains the logical steps we have to do:
     *
     * 1. We need to set i=5, because...
     * 2. We need to call complex_fn1
     */
    /* This is a one line comment about i = 5. */
    i = 5;
    /*
     * This is a multi line comment,
     * explaining the call to complex_fn1()
     */
    ret = complex_fn1();
    if (ret != 0) {
    ...

    /*This is a one line comment.*/

    /* This is a multi line comment,
       with some more words...*/

    /*
     * This is a multi line comment,
     * with some more words...*/
```

### Indentation, spaces and 80 columns

To avoid confusion, indentations have to be 4 spaces. Do not use tabs!

When wrapping parameters for function calls, align the parameter list with the
first parameter on the previous line. For example:

```c
    var1 = foo(long_long_long_long_long_long_long_arg_name1,
               long_long_long_long_long_long_long_arg_name2,
               long_long_long_long_long_long_long_arg_name3);
```

The previous example is intended to illustrate alignment of function parameters
across lines and not as encourage for gratuitous line splitting. Never split a
line before columns 70 - 79 unless you have a really good reason. Be smart
about formatting.


### If, switch and code blocks

Always follow an `if` keyword with a space but don't include additional
spaces following or preceding the parentheses in the conditional.

Examples:

```c
    // This is good
    if (x == 1)

    // This is bad
    if ( x == 1 )

    // This is also bad
    if (x==1)
```

Yes we have a lot of code that uses the second and third form and we are trying
to clean it up without being overly intrusive.

Note that this is a rule about parentheses following keywords and not functions.
Don't insert a space between the name and left parentheses when invoking
functions.

Braces for code blocks used by `for`, `if`, `switch`, `while`, `do..while`, etc should
begin on the same line as the statement keyword and end on a line of their own.
You should always include braces, even if the block only contains one statement.

**NOTE**: Functions are different and the beginning left brace should be located
in the first column on the next line.

If the beginning statement has to be broken across lines due to length, the
beginning brace should be on a line of its own.

The exception to the ending rule is when the closing brace is followed by
another language keyword such as else or the closing while in a `do..while` loop.

Good examples:

```c
    if (x == 1) {
        printf("good\n");
    }

    for (x = 1; x < 10; x++) {
        print("%d\n", x);
    }

    for (really_really_really_really_long_var_name = 0;
         really_really_really_really_long_var_name < 10;
         really_really_really_really_long_var_name++)
    {
        print("%d\n", really_really_really_really_long_var_name);
    }

    do {
        printf("also good\n");
    } while (1);
```

Bad examples:

```c
    while (1)
    {
        print("I'm in a loop!\n"); }

    for (x=1;
         x<10;
         x++)
    {
        print("no good\n");
    }

    if (i < 10)
        print("I should be in braces.\n");
```

### Goto

While many people have been academically taught that goto's are fundamentally
evil, they can greatly enhance readability and reduce memory leaks when used as
the single exit point from a function. But in no world what so ever is a goto
outside of a function or block of code a good idea.

Good Examples:

```c
    int function foo(int y)
    {
        int *z = NULL;
        int rc = 0;

        if (y < 10) {
            z = malloc(sizeof(int)*y);
            if (z == NULL) {
                rc = 1;
                goto done;
            }
        }

        print("Allocated %d elements.\n", y);

    done:
        if (z != NULL) {
            free(z);
        }

        return rc;
    }
```

### Initialize pointers

All pointer variables **MUST** be initialized to `NULL`. History has
demonstrated that uninitialized pointer variables have lead to various bugs and
security issues.

Pointers **MUST** be initialized even if the assignment directly follows the
declaration, like pointer2 in the example below, because the instructions
sequence may change over time.

Good Example:

```c
    char *pointer1 = NULL;
    char *pointer2 = NULL;

    pointer2 = some_func2();

    ...

    pointer1 = some_func1();
```

### Typedefs

cmocka tries to avoid `typedef struct { .. } x_t;` so we do always try to use
`struct x { .. };`. We know there are still such typedefs in the code, but for
new code, please don't do that anymore.

### Make use of helper variables (aka debugging UX)

Please try to avoid passing function calls as function parameters in new code.
This makes the code much easier to read and it's also easier to use the "step"
command within gdb.

Good Example:

```c
    char *name = NULL;

    name = get_some_name();
    if (name == NULL) {
        ...
    }

    rc = some_function_my_name(name);
    ...
```


Bad Example:

```c
    rc = some_function_my_name(get_some_name());
    ...
```

Please try to avoid passing function return values to if- or while-conditions.
The reason for this is better handling of code under a debugger.

Good example:

```c
    x = malloc(sizeof(short) * 10);
    if (x == NULL) {
        fprintf(stderr, "Unable to alloc memory!\n");
    }
```

Bad example:

```c
    if ((x = malloc(sizeof(short)*10)) == NULL ) {
        fprintf(stderr, "Unable to alloc memory!\n");
    }
```

There are exceptions to this rule. One example is walking a data structure in
an iterator style:

```c
    while ((opt = poptGetNextOpt(pc)) != -1) {
        ... do something with opt ...
    }
```

In general, please try to avoid this pattern.

### Control-Flow changing macros

Macros like `STATUS_NOT_OK_RETURN` that change control flow (return/goto/etc)
from within the macro are considered bad, because they look like function calls
that never change control flow. Please do not introduce them.

### Switch/case indentation

The `case` should not be indented to avoid wasting too much horizontal space.
When the case block contains local variables that need to be wrapped in braces,
they should not be indented again either.

Good example:

```c
    switch (x) {
    case 0:
        do_stuff();
        break;
    case 1: {
        int y;
        do_stuff();
        break;
    }
    default:
        do_other_stuff();
        break;
    }
```

Bad example:

```c
    switch (x) {
        case 0:
            do_stuff();
            break;
        case 1:
            {
                int y;
                do_stuff();
                break;
            }
        default:
            do_other_stuff();
            break;
    }
```
