#include "ctestprobe.h"

#include <stdarg.h>
#include <stdio.h>

#define CTP_MAX_TESTS 256

static Test  g_tests[CTP_MAX_TESTS];
static int   g_num_tests = 0;
static Test *g_current   = NULL;

void ctestprobe_init(void) {
    g_num_tests = 0;
    g_current   = NULL;
}

void ctestprobe_register(const char *name, void (*test_func)(void)) {
    if (test_func == NULL) {
        fprintf(stderr, "ctestprobe: refusing to register %s: test_func is NULL\n",
                name != NULL ? name : "(unnamed)");
        return;
    }
    if (g_num_tests >= CTP_MAX_TESTS) {
        fprintf(stderr, "ctestprobe: registry full (%d tests)\n",
                CTP_MAX_TESTS);
        return;
    }
    g_tests[g_num_tests].name           = (char *)name;
    g_tests[g_num_tests].test_func      = test_func;
    g_tests[g_num_tests].execution_time = 0.0;
    g_tests[g_num_tests].status         = NOT_RUN;
    g_num_tests++;
}

void ctestprobe_run_test(Test *test) {
    g_current = test;
    test->test_func();
    if (test->status == NOT_RUN) {
        test->status = PASSED;
    }
    g_current = NULL;
}

int ctestprobe_run_all(void) {
    int failed = 0;
    for (int i = 0; i < g_num_tests; i++) {
        ctestprobe_run_test(&g_tests[i]);
        if (g_tests[i].status == FAILED) failed++;
    }
    return failed;
}

void ctestprobe_console_report(void) {
    int passed = 0, failed = 0, not_run = 0;
    printf("\nTest Results:\n");
    for (int i = 0; i < g_num_tests; i++) {
        const Test *t = &g_tests[i];
        const char *label;
        if      (t->status == PASSED) { label = "PASS"; passed++;  }
        else if (t->status == FAILED) { label = "FAIL"; failed++;  }
        else                          { label = "SKIP"; not_run++; }
        printf("  [%s] %s\n", label, t->name);
    }
    printf("\n%d run, %d passed, %d failed",
           g_num_tests, passed, failed);
    if (not_run > 0) printf(", %d not run", not_run);
    printf("\n");
}

void ctestprobe_fail(const char *file, int line, const char *fmt, ...) {
    if (g_current != NULL) {
        g_current->status = FAILED;
    }
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "FAIL %s:%d: ", file, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    /* Flush so the diagnostic survives an abnormal exit in a later test. */
    fflush(stderr);
}
