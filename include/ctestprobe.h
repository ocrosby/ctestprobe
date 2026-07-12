#ifndef CTESTPROBE_H
#define CTESTPROBE_H

#include <stddef.h>
#include <string.h>

/*
 * ctestprobe — a minimal unit-testing framework for C.
 *
 * Usage:
 *   #include "ctestprobe.h"
 *
 *   static void test_addition(void) {
 *       CTP_ASSERT_EQ_INT(2 + 2, 4);
 *   }
 *
 *   int main(void) {
 *       ctestprobe_init();
 *       ctestprobe_register("addition", test_addition);
 *       int failed = ctestprobe_run_all();
 *       ctestprobe_console_report();
 *       return failed == 0 ? 0 : 1;
 *   }
 *
 * Assertion macros return from the current function on failure, so they
 * can only be used inside a `void`-returning test. On failure they mark
 * the current test as FAILED and print a FAIL <file>:<line> diagnostic
 * on stderr.
 */

typedef enum {
    NOT_RUN,
    PASSED,
    FAILED
} TestStatus;

typedef struct {
    char       *name;
    void      (*test_func)(void);
    double      execution_time;
    TestStatus  status;
} Test;

/* Reset the internal registry. Safe to call multiple times. */
void ctestprobe_init(void);

/* Register a test by name. `name` must remain valid for the run. */
void ctestprobe_register(const char *name, void (*test_func)(void));

/* Run one test in place, updating its status. */
void ctestprobe_run_test(Test *test);

/* Run every registered test. Returns the number that failed. */
int  ctestprobe_run_all(void);

/* Print a per-test summary to stdout. */
void ctestprobe_console_report(void);

/* Record a failure on the currently-running test. Called by the assertion
 * macros; consumers rarely invoke it directly. */
void ctestprobe_fail(const char *file, int line, const char *fmt, ...);

#define CTP_ASSERT(cond)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ctestprobe_fail(__FILE__, __LINE__,                              \
                            "assertion failed: %s", #cond);                  \
            return;                                                          \
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
            return;                                                          \
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
            return;                                                          \
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
            return;                                                          \
        }                                                                    \
    } while (0)

#endif /* CTESTPROBE_H */
