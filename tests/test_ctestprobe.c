#include <stdio.h>

#include "ctestprobe.h"

/*
 * Meta-tests for ctestprobe itself.
 *
 * These drive the framework by hand — using ctestprobe_run_test directly
 * and inspecting Test.status — rather than routing through
 * ctestprobe_run_all. If the framework is broken, we cannot trust
 * ctestprobe_run_all to report reliably; direct inspection is the
 * bootstrap.
 *
 * Failing cases deliberately assert false; the FAIL lines they print to
 * stderr are part of the test. The verdict is the summary printed at
 * the end.
 */

static void case_passing(void)         { CTP_ASSERT(1 == 1); }
static void case_failing(void)         { CTP_ASSERT(1 == 2); }
static void case_int_equal(void)       { CTP_ASSERT_EQ_INT(5, 5); }
static void case_int_unequal(void)     { CTP_ASSERT_EQ_INT(5, 6); }
static void case_str_equal(void)       { CTP_ASSERT_EQ_STR("a", "a"); }
static void case_str_unequal(void)     { CTP_ASSERT_EQ_STR("a", "b"); }

static void case_bytes_equal(void) {
    unsigned char a[] = {1, 2, 3};
    unsigned char b[] = {1, 2, 3};
    CTP_ASSERT_EQ_BYTES(a, sizeof a, b, sizeof b);
}
static void case_bytes_diff_length(void) {
    unsigned char a[] = {1, 2, 3};
    unsigned char b[] = {1, 2};
    CTP_ASSERT_EQ_BYTES(a, sizeof a, b, sizeof b);
}
static void case_bytes_diff_content(void) {
    unsigned char a[] = {1, 2, 3};
    unsigned char b[] = {1, 2, 4};
    CTP_ASSERT_EQ_BYTES(a, sizeof a, b, sizeof b);
}

struct meta_case {
    const char *name;
    void      (*fn)(void);
    TestStatus  expect;
};

static const struct meta_case cases[] = {
    { "passing_assert",     case_passing,            PASSED },
    { "failing_assert",     case_failing,            FAILED },
    { "int_equal",          case_int_equal,          PASSED },
    { "int_unequal",        case_int_unequal,        FAILED },
    { "str_equal",          case_str_equal,          PASSED },
    { "str_unequal",        case_str_unequal,        FAILED },
    { "bytes_equal",        case_bytes_equal,        PASSED },
    { "bytes_diff_length",  case_bytes_diff_length,  FAILED },
    { "bytes_diff_content", case_bytes_diff_content, FAILED },
};

int main(void) {
    ctestprobe_init();
    fprintf(stderr,
            "(the FAIL lines below are deliberate — meta-tests "
            "verifying ctestprobe detects failure)\n");

    const size_t n = sizeof cases / sizeof cases[0];
    int mismatched = 0;
    for (size_t i = 0; i < n; i++) {
        Test t = {
            .name           = (char *)cases[i].name,
            .test_func      = cases[i].fn,
            .execution_time = 0.0,
            .status         = NOT_RUN,
        };
        ctestprobe_run_test(&t);
        int ok = (t.status == cases[i].expect);
        if (!ok) mismatched++;
        printf("[%s] %-20s expected=%d got=%d\n",
               ok ? "OK" : "MISMATCH",
               cases[i].name,
               (int)cases[i].expect,
               (int)t.status);
    }
    printf("\nctestprobe self-test: %zu run, %d mismatched\n", n, mismatched);
    return mismatched == 0 ? 0 : 1;
}
