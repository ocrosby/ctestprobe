#define _POSIX_C_SOURCE 200809L

#include "ctestprobe.h"

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>
#include <signal.h>

/* -------- Registry (grows via realloc; no compile-time cap) -------- */

static ctp_test_t *g_tests    = NULL;
static size_t      g_num      = 0;
static size_t      g_cap      = 0;
static ctp_test_t *g_current  = NULL;

/* -------- setjmp buffer for assertion bailout -------- */

static jmp_buf g_test_jmp;
static int     g_jmp_valid = 0;

/* -------- Hooks -------- */

static ctp_hook_t g_global_setup     = NULL; static void *g_global_setup_ud    = NULL;
static ctp_hook_t g_global_teardown  = NULL; static void *g_global_teardown_ud = NULL;
static ctp_hook_t g_each_setup       = NULL; static void *g_each_setup_ud     = NULL;
static ctp_hook_t g_each_teardown    = NULL; static void *g_each_teardown_ud  = NULL;

/* -------- Runner state -------- */

static const char *g_filter = NULL;
static int         g_fork   = 0;
static int         g_color  = -1;   /* -1 auto, 0 off, 1 forced on */

/* -------- Lifecycle -------- */

void ctestprobe_init(void) {
    g_num = 0;
    g_current = NULL;
    g_global_setup    = NULL; g_global_setup_ud    = NULL;
    g_global_teardown = NULL; g_global_teardown_ud = NULL;
    g_each_setup      = NULL; g_each_setup_ud      = NULL;
    g_each_teardown   = NULL; g_each_teardown_ud   = NULL;
    g_filter = NULL;
    g_fork   = 0;
    g_color  = -1;
    g_jmp_valid = 0;
}

void ctestprobe_free(void) {
    free(g_tests);
    g_tests = NULL;
    g_num = 0;
    g_cap = 0;
}

void ctestprobe_register(const char *name, ctp_test_func_t test_func) {
    if (test_func == NULL) {
        fprintf(stderr, "ctestprobe: refusing to register %s: test_func is NULL\n",
                name != NULL ? name : "(unnamed)");
        return;
    }
    if (g_num == g_cap) {
        size_t new_cap = (g_cap == 0) ? 16 : g_cap * 2;
        ctp_test_t *r = (ctp_test_t *)realloc(g_tests, new_cap * sizeof(*r));
        if (r == NULL) {
            fprintf(stderr,
                    "ctestprobe: registry realloc failed at %zu tests: %s\n",
                    g_cap, strerror(errno));
            return;
        }
        g_tests = r;
        g_cap = new_cap;
    }
    g_tests[g_num].name           = name;
    g_tests[g_num].test_func      = test_func;
    g_tests[g_num].execution_time = 0.0;
    g_tests[g_num].status         = CTP_NOT_RUN;
    g_num++;
}

/* -------- Hook setters -------- */

void ctestprobe_set_global_setup(ctp_hook_t h, void *ud)    { g_global_setup    = h; g_global_setup_ud    = ud; }
void ctestprobe_set_global_teardown(ctp_hook_t h, void *ud) { g_global_teardown = h; g_global_teardown_ud = ud; }
void ctestprobe_set_each_setup(ctp_hook_t h, void *ud)      { g_each_setup      = h; g_each_setup_ud      = ud; }
void ctestprobe_set_each_teardown(ctp_hook_t h, void *ud)   { g_each_teardown   = h; g_each_teardown_ud   = ud; }

void ctestprobe_set_filter(const char *substring) { g_filter = substring; }
void ctestprobe_set_fork_isolation(int enable)    { g_fork   = enable; }

/* -------- Helpers -------- */

static double now_seconds(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int matches_filter(const char *name) {
    if (g_filter == NULL || g_filter[0] == '\0') return 1;
    return strstr(name, g_filter) != NULL;
}

static int color_enabled(FILE *fp) {
    if (g_color == 0) return 0;
    if (g_color == 1) return 1;
    if (getenv("NO_COLOR") != NULL) return 0;
    return isatty(fileno(fp));
}

static void run_test_inline(ctp_test_t *test) {
    if (g_each_setup) g_each_setup(g_each_setup_ud);
    g_current = test;
    double t0 = now_seconds();
    if (setjmp(g_test_jmp) == 0) {
        g_jmp_valid = 1;
        test->test_func();
        if (test->status == CTP_NOT_RUN) test->status = CTP_PASSED;
    }
    g_jmp_valid = 0;
    test->execution_time = now_seconds() - t0;
    g_current = NULL;
    if (g_each_teardown) g_each_teardown(g_each_teardown_ud);
}

/* Fork-based execution: exit code carries the pass/fail signal. Any
 * termination via signal (SIGSEGV etc.) is treated as FAIL. */
static void run_test_forked(ctp_test_t *test) {
    double t0 = now_seconds();
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "ctestprobe: fork failed for %s: %s\n",
                test->name, strerror(errno));
        test->status = CTP_FAILED;
        test->execution_time = now_seconds() - t0;
        return;
    }
    if (pid == 0) {
        /* Child: run and _exit with a pass/fail code. */
        run_test_inline(test);
        _exit(test->status == CTP_PASSED ? 0 : 1);
    }
    int wstatus;
    while (waitpid(pid, &wstatus, 0) < 0 && errno == EINTR) { /* retry */ }
    test->execution_time = now_seconds() - t0;
    if (WIFEXITED(wstatus)) {
        test->status = (WEXITSTATUS(wstatus) == 0) ? CTP_PASSED : CTP_FAILED;
    } else if (WIFSIGNALED(wstatus)) {
        int sig = WTERMSIG(wstatus);
        fprintf(stderr, "FAIL %s: died with signal %d (%s)\n",
                test->name, sig, strsignal(sig));
        fflush(stderr);
        test->status = CTP_FAILED;
    } else {
        test->status = CTP_FAILED;
    }
}

void ctestprobe_run_test(ctp_test_t *test) {
    if (g_fork) run_test_forked(test);
    else        run_test_inline(test);
}

int ctestprobe_run_all(void) {
    if (g_global_setup) g_global_setup(g_global_setup_ud);
    int failed = 0;
    for (size_t i = 0; i < g_num; i++) {
        if (!matches_filter(g_tests[i].name)) {
            g_tests[i].status = CTP_SKIPPED;
            continue;
        }
        ctestprobe_run_test(&g_tests[i]);
        if (g_tests[i].status == CTP_FAILED) failed++;
    }
    if (g_global_teardown) g_global_teardown(g_global_teardown_ud);
    return failed;
}

/* -------- Reporters -------- */

void ctestprobe_console_report(void) {
    int passed = 0, failed = 0, skipped = 0, not_run = 0;
    int use_color = color_enabled(stdout);
    printf("\nTest Results:\n");
    for (size_t i = 0; i < g_num; i++) {
        const ctp_test_t *t = &g_tests[i];
        const char *label;
        const char *color_on = "";
        const char *color_off = "";
        switch (t->status) {
        case CTP_PASSED:  label = "PASS";
            if (use_color) { color_on = "\x1b[32m"; color_off = "\x1b[0m"; }
            passed++; break;
        case CTP_FAILED:  label = "FAIL";
            if (use_color) { color_on = "\x1b[31m"; color_off = "\x1b[0m"; }
            failed++; break;
        case CTP_SKIPPED: label = "SKIP";
            if (use_color) { color_on = "\x1b[33m"; color_off = "\x1b[0m"; }
            skipped++; break;
        default:          label = "----";
            not_run++; break;
        }
        printf("  %s[%s]%s %-40s %.3f ms\n",
               color_on, label, color_off, t->name,
               t->execution_time * 1000.0);
    }
    printf("\n%zu registered, %d passed, %d failed",
           g_num, passed, failed);
    if (skipped) printf(", %d skipped", skipped);
    if (not_run) printf(", %d not run", not_run);
    printf("\n");
}

void ctestprobe_tap_report(void) {
    printf("TAP version 14\n");
    printf("1..%zu\n", g_num);
    for (size_t i = 0; i < g_num; i++) {
        const ctp_test_t *t = &g_tests[i];
        const char *ok = (t->status == CTP_PASSED)  ? "ok" :
                        (t->status == CTP_SKIPPED) ? "ok" : "not ok";
        printf("%s %zu - %s", ok, i + 1, t->name);
        if (t->status == CTP_SKIPPED) printf(" # SKIP filter mismatch");
        printf(" # time=%.3fms\n", t->execution_time * 1000.0);
    }
}

/* -------- Failure hooks -------- */

void ctestprobe_fail(const char *file, int line, const char *fmt, ...) {
    if (g_current != NULL) g_current->status = CTP_FAILED;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "FAIL %s:%d: ", file, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
    if (g_jmp_valid) longjmp(g_test_jmp, 1);
    /* No active test frame — the assertion was invoked outside a running
     * test. Abort loudly rather than silently mis-reporting. */
    abort();
}

void ctestprobe_expect_fail(const char *file, int line, const char *fmt, ...) {
    if (g_current != NULL) g_current->status = CTP_FAILED;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "EXPECT %s:%d: ", file, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
    /* Non-fatal: no longjmp. */
}

/* -------- CLI -------- */

static void print_help(FILE *fp, const char *prog) {
    fprintf(fp,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  --list                 List registered tests and exit\n"
        "  --filter=SUBSTR        Run only tests whose name contains SUBSTR\n"
        "  --tap                  Emit TAP output instead of the console report\n"
        "  --fork                 Run each test in a forked process (crash-safe)\n"
        "  --no-color             Disable ANSI color in the console report\n"
        "  -h, --help             Show this message\n"
        "\n"
        "Environment:\n"
        "  CTP_FILTER=SUBSTR      Equivalent to --filter\n"
        "  CTP_FORK_TESTS=1       Equivalent to --fork\n"
        "  NO_COLOR=1             Equivalent to --no-color\n",
        prog);
}

int ctestprobe_main(int argc, char **argv) {
    int use_tap = 0;
    int list_only = 0;

    /* Environment-derived defaults (overridden by flags below). */
    const char *env_filter = getenv("CTP_FILTER");
    if (env_filter != NULL && env_filter[0] != '\0') g_filter = env_filter;
    if (getenv("CTP_FORK_TESTS") != NULL) g_fork = 1;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if      (strcmp(arg, "--list")     == 0) { list_only = 1; }
        else if (strcmp(arg, "--tap")      == 0) { use_tap   = 1; }
        else if (strcmp(arg, "--fork")     == 0) { g_fork    = 1; }
        else if (strcmp(arg, "--no-color") == 0) { g_color   = 0; }
        else if (strncmp(arg, "--filter=", 9) == 0) { g_filter = arg + 9; }
        else if (strcmp(arg, "--filter") == 0 && i + 1 < argc) { g_filter = argv[++i]; }
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_help(stdout, argv[0]);
            return 0;
        } else {
            fprintf(stderr, "ctestprobe: unknown option: %s\n", arg);
            print_help(stderr, argv[0]);
            return 2;
        }
    }

    if (list_only) {
        for (size_t i = 0; i < g_num; i++) {
            if (matches_filter(g_tests[i].name)) {
                printf("%s\n", g_tests[i].name);
            }
        }
        return 0;
    }

    int failed = ctestprobe_run_all();
    if (use_tap) ctestprobe_tap_report();
    else         ctestprobe_console_report();
    return failed == 0 ? 0 : 1;
}
