#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "greatest.h"

/* Define a suite, compiled seperately. */
extern SUITE(other_suite);

/* Declare a local suite. */
SUITE(suite);

/* Just test against random ints, to show a variety of results. */
TEST example_test_case(void) {
    int r = 0;
    ASSERT(1 == 1);

    r = rand() % 10;
    if (r == 1) SKIP();
    ASSERT(r >= 1);
    PASS();
}

TEST expect_equal(void) {
    int i = 9;
    ASSERT_EQ(10, i);
    PASS();
}

TEST expect_str_equal(void) {
    const char *foo = "foo";
    ASSERT_STR_EQ("bar", foo);
    PASS();
}

/* A boxed int type, used to show type-specific equality tests. */
typedef struct {
    int i;
} boxed_int;

/* Callback used to check whether two boxed_ints are equal. */
static int boxed_int_equal_cb(const void *exp, const void *got, void *udata) {
    boxed_int *ei = (boxed_int *)exp;
    boxed_int *gi = (boxed_int *)got;

    /* udata is not used here, but could be used to specify a comparison
     * resolution, a string encoding, or any other state that should be
     * passed along to the equal and print callbacks. */
    (void)udata;
    return (ei->i == gi->i);
}

/* Callback to print a boxed_int, used to produce an
 * "Exected X, got Y" failure message. */
static int boxed_int_printf_cb(const void *t, void *udata) {
    boxed_int *bi = (boxed_int *)t;
    (void)udata;
    return printf("{%d}", bi->i);
}

/* The struct that stores the previous two functions' pointers. */
static greatest_type_info boxed_int_type_info = {
    boxed_int_equal_cb,
    boxed_int_printf_cb,
};

TEST expect_boxed_int_equal(void) {
    boxed_int a = {3};
    boxed_int b = {3};
    boxed_int c = {4};
    ASSERT_EQUAL_T(&a, &b, &boxed_int_type_info, NULL);  /* succeeds */
    ASSERT_EQUAL_T(&a, &c, &boxed_int_type_info, NULL);  /* fails */
    PASS();
}

/* Flag, used to confirm that teardown hook is being called. */
static int teardown_was_called = 0;

TEST teardown_example_PASS(void) {
    teardown_was_called = 0;
    PASS();
}

TEST teardown_example_FAIL(void) {
    teardown_was_called = 0;
    FAILm("Using FAIL to trigger teardown callback");
}

TEST teardown_example_SKIP(void) {
    teardown_was_called = 0;
    SKIPm("Using SKIP to trigger teardown callback");
}

/* Example of an ANSI C compatible way to do test cases with
 * arguments: they are passed one argument, a pointer which
 * should be cast back to a struct with the other data. */
TEST parametric_example_c89(void *closure) {
    int arg = *(int *) closure;
    ASSERT(arg > 10);
    PASS();
}

/* If using C99, greatest can also do parametric tests without
 * needing to manually manage a closure. */
#if __STDC_VERSION__ >= 19901L
TEST parametric_example_c99(int arg) {
    ASSERT(arg > 10);
    PASS();
}
#endif

static void trace_setup(void *arg) {
    printf("-- in setup callback\n");
    teardown_was_called = 0;
    (void)arg;
}

static void trace_teardown(void *arg) {
    printf("-- in teardown callback\n");
    teardown_was_called = 1;
    (void)arg;
}

/* Primary test suite. */
SUITE(suite) {
    int i=0;
    printf("\nThis should have some failures:\n");
    for (i=0; i<200; i++) {
        RUN_TEST(example_test_case);
    }
    RUN_TEST(expect_equal);
    printf("\nThis should fail:\n");
    RUN_TEST(expect_str_equal);
    printf("\nThis should fail:\n");
    RUN_TEST(expect_boxed_int_equal);

    /* Set so asserts below won't fail if running in list-only or
     * first-fail modes. (setup() won't be called and clear it.) */
    teardown_was_called = -1;

    /* Add setup/teardown for each test case. */
    GREATEST_SET_SETUP_CB(trace_setup, NULL);
    GREATEST_SET_TEARDOWN_CB(trace_teardown, NULL);

    /* Check that the test-specific teardown hook is called. */
    RUN_TEST(teardown_example_PASS);
    assert(teardown_was_called);

    printf("\nThis should fail:\n");
    RUN_TEST(teardown_example_FAIL);
    assert(teardown_was_called);

    printf("This should be skipped:\n");
    RUN_TEST(teardown_example_SKIP);
    assert(teardown_was_called);

    /* Run a test with one void* argument (which can point to a
     * struct with multiple arguments). */
    printf("\nThis should fail:\n");
    i = 10;
    RUN_TEST1(parametric_example_c89, &i);
    i = 11;
    RUN_TEST1(parametric_example_c89, &i);

    /* Run a test, with arguments. ('p' for "parametric".) */
#if __STDC_VERSION__ >= 19901L
    printf("\nThis should fail:\n");
    RUN_TESTp(parametric_example_c99, 10);
    RUN_TESTp(parametric_example_c99, 11);
#endif
}

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_SUITE(suite);
    RUN_SUITE(other_suite);
    GREATEST_MAIN_END();        /* display results */
}
