#ifndef CTESTPROBE_H
#define CTESTPROBE_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTESTPROBE_VERSION_MAJOR 2
#define CTESTPROBE_VERSION_MINOR 2
#define CTESTPROBE_VERSION_PATCH 0

/*
 * ctestprobe — a minimal unit-testing framework for C.
 *
 * Consumer usage:
 *
 *     #include "ctestprobe.h"
 *
 *     static void test_addition(void) {
 *         CTP_ASSERT_EQ_INT(2 + 2, 4);
 *     }
 *
 *     int main(int argc, char **argv) {
 *         ctestprobe_init();
 *         ctestprobe_register("addition", test_addition);
 *         return ctestprobe_main(argc, argv);
 *     }
 *
 * Assertion macros use setjmp/longjmp for failure bailout, so they work
 * from any call depth inside a running test — not only the top-level
 * test function. Non-fatal CTP_EXPECT_* variants record the failure and
 * continue.
 */

typedef enum {
    CTP_NOT_RUN = 0,
    CTP_PASSED,
    CTP_FAILED,
    CTP_SKIPPED
} ctp_test_status_t;

typedef void (*ctp_test_func_t)(void);
typedef void (*ctp_hook_t)(void *user_data);

/* Cap on the formatted failure message we retain per test. Longer messages
 * still print to stderr in full; only the JUnit-report snapshot is bounded. */
#define CTP_FAILURE_MSG_MAX 512

typedef struct ctp_test_s {
    const char       *name;
    ctp_test_func_t   test_func;
    double            execution_time;   /* seconds; populated after run */
    ctp_test_status_t status;
    char              last_failure_msg[CTP_FAILURE_MSG_MAX];
} ctp_test_t;

/* -------- Registry lifecycle -------- */

/* Reset the internal registry. Safe to call multiple times. */
void ctestprobe_init(void);

/* Release heap-owned registry storage. Optional; useful for LeakSanitizer. */
void ctestprobe_free(void);

/* Register a test by name. `name` must remain valid for the run. */
void ctestprobe_register(const char *name, ctp_test_func_t test_func);

/* -------- Hooks -------- */

/* Called once before any test in ctestprobe_run_all. */
void ctestprobe_set_global_setup(ctp_hook_t hook, void *user_data);
/* Called once after every test in ctestprobe_run_all completes. */
void ctestprobe_set_global_teardown(ctp_hook_t hook, void *user_data);
/* Called before each test. */
void ctestprobe_set_each_setup(ctp_hook_t hook, void *user_data);
/* Called after each test, including failing ones. */
void ctestprobe_set_each_teardown(ctp_hook_t hook, void *user_data);

/* -------- Runner controls -------- */

/* Run only tests whose name contains `substring`. NULL / "" = run everything. */
void ctestprobe_set_filter(const char *substring);

/* When enabled, each test runs in a forked child process. Crashes report
 * as FAIL instead of aborting the runner. POSIX only. */
void ctestprobe_set_fork_isolation(int enable);

/* Set the name used in the JUnit XML report — both the
 * <testsuite name="..."> element and every <testcase classname="...">
 * attribute. NULL or "" resets to the default "ctestprobe". The pointer
 * must outlive ctestprobe_junit_report; passing a string literal is
 * typical. Useful when a single CI job aggregates JUnit XML from
 * multiple binaries whose test names would otherwise collide. */
void ctestprobe_set_suite_name(const char *name);

/* -------- Runners -------- */

/* Run one test in place. Rarely called directly; use ctestprobe_run_all
 * (or ctestprobe_main). Populates test->status and test->execution_time. */
void ctestprobe_run_test(ctp_test_t *test);

/* Run every registered test. Returns the number that failed. */
int  ctestprobe_run_all(void);

/* -------- Reporters -------- */

void ctestprobe_console_report(void);
void ctestprobe_tap_report(void);
/* Emit JUnit XML (Surefire dialect) to `fp`. Consumed by GitHub Actions,
 * Jenkins, CircleCI, GitLab, Buildkite, and any test-result parser that
 * understands the Surefire schema. */
void ctestprobe_junit_report(FILE *fp);

/* -------- CLI entry point --------
 *
 * Recognized flags:
 *   --list                 Print registered test names and exit 0
 *   --filter=SUBSTR        Run only tests whose name contains SUBSTR
 *   --tap                  Emit TAP output instead of the console report
 *   --junit=PATH           Write JUnit XML (Surefire dialect) to PATH
 *   --suite=NAME           Set the JUnit suite name (default: ctestprobe)
 *   --fork                 Fork each test (POSIX; crash-safe)
 *   --no-color             Disable ANSI color in the console report
 *   -h, --help             Show usage
 *
 * Recognized environment variables:
 *   CTP_FILTER=SUBSTR
 *   CTP_FORK_TESTS=1
 *   CTP_JUNIT=PATH
 *   CTP_SUITE=NAME
 *   NO_COLOR=1
 *
 * Returns 0 if all tests passed, 1 if any failed, 2 on argv error.
 */
int ctestprobe_main(int argc, char **argv);

/* -------- Failure hooks --------
 *
 * Called by the CTP_ASSERT_* / CTP_EXPECT_* macros. Consumers rarely
 * invoke them directly. ctestprobe_fail does not return when called
 * inside a running test — it longjmps back to ctestprobe_run_test. */
void ctestprobe_fail(const char *file, int line, const char *fmt, ...);
void ctestprobe_expect_fail(const char *file, int line, const char *fmt, ...);

/* -------- Fatal assertions -------- */

#define CTP_ASSERT(cond)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "assertion failed: %s", #cond);                  \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_TRUE(cond)  CTP_ASSERT((cond))

#define CTP_ASSERT_FALSE(cond)                                               \
    do {                                                                     \
        if ((cond)) {                                                        \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected false: %s", #cond);                    \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_NULL(ptr)                                                 \
    do {                                                                     \
        const void *_ctp_p = (const void *)(ptr);                            \
        if (_ctp_p != NULL) {                                                \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected NULL, got %p (%s)",                    \
                            _ctp_p, #ptr);                                   \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_NOT_NULL(ptr)                                             \
    do {                                                                     \
        if ((ptr) == NULL) {                                                 \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected non-NULL: %s", #ptr);                  \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_EQ_INT(got, want)                                         \
    do {                                                                     \
        long long _ctp_got  = (long long)(got);                              \
        long long _ctp_want = (long long)(want);                             \
        if (_ctp_got != _ctp_want) {                                         \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected %lld, got %lld (%s)",                  \
                            _ctp_want, _ctp_got, #got);                      \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_EQ_STR(got, want)                                         \
    do {                                                                     \
        const char *_ctp_got  = (got);                                       \
        const char *_ctp_want = (want);                                      \
        if (strcmp(_ctp_got, _ctp_want) != 0) {                              \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected \"%s\", got \"%s\" (%s)",              \
                            _ctp_want, _ctp_got, #got);                      \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_STR_CONTAINS(haystack, needle)                            \
    do {                                                                     \
        const char *_ctp_h = (haystack);                                     \
        const char *_ctp_n = (needle);                                       \
        if (strstr(_ctp_h, _ctp_n) == NULL) {                                \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected \"%s\" to contain \"%s\"",             \
                            _ctp_h, _ctp_n);                                 \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_EQ_BYTES(got, got_len, want, want_len)                    \
    do {                                                                     \
        size_t _ctp_gl = (size_t)(got_len);                                  \
        size_t _ctp_wl = (size_t)(want_len);                                 \
        if (_ctp_gl != _ctp_wl ||                                            \
            memcmp((got), (want), _ctp_wl) != 0) {                           \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "byte arrays differ (got %zu, want %zu)",        \
                            _ctp_gl, _ctp_wl);                               \
        }                                                                    \
    } while (0)

#define CTP_ASSERT_EQ_DOUBLE(got, want, epsilon)                             \
    do {                                                                     \
        double _ctp_g   = (double)(got);                                     \
        double _ctp_w   = (double)(want);                                    \
        double _ctp_eps = (double)(epsilon);                                 \
        double _ctp_d   = _ctp_g - _ctp_w;                                   \
        if (_ctp_d < 0) _ctp_d = -_ctp_d;                                    \
        if (_ctp_d > _ctp_eps) {                                             \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "expected %g +/- %g, got %g (%s)",               \
                            _ctp_w, _ctp_eps, _ctp_g, #got);                 \
        }                                                                    \
    } while (0)

/* -------- Non-fatal expectations -------- */

#define CTP_EXPECT(cond)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ctestprobe_expect_fail(__FILE__, __LINE__,                       \
                                   "expectation failed: %s", #cond);         \
        }                                                                    \
    } while (0)

#define CTP_EXPECT_EQ_INT(got, want)                                         \
    do {                                                                     \
        long long _ctp_got  = (long long)(got);                              \
        long long _ctp_want = (long long)(want);                             \
        if (_ctp_got != _ctp_want) {                                         \
            ctestprobe_expect_fail(__FILE__, __LINE__,                       \
                                   "expected %lld, got %lld (%s)",           \
                                   _ctp_want, _ctp_got, #got);               \
        }                                                                    \
    } while (0)

#define CTP_EXPECT_EQ_STR(got, want)                                         \
    do {                                                                     \
        const char *_ctp_got  = (got);                                       \
        const char *_ctp_want = (want);                                      \
        if (strcmp(_ctp_got, _ctp_want) != 0) {                              \
            ctestprobe_expect_fail(__FILE__, __LINE__,                       \
                                   "expected \"%s\", got \"%s\" (%s)",       \
                                   _ctp_want, _ctp_got, #got);               \
        }                                                                    \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* CTESTPROBE_H */
