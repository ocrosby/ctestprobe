#ifndef CTESTPROBE_H
#define CTESTPROBE_H

typedef enum {
    NOT_RUN,
    PASSED,
    FAILED
} TestStatus;

typedef struct {
    char *name;
    void (*test_func)();
    double execution_time;
    TestStatus status;
} Test;

// Function to initialize the testing framework
void ctestprobe_init();

// Function to run a test
void ctestprobe_run_test(Test *test);

// Function to report the results of the tests to the console.
void ctestprobe_console_report();

#endif // CTESTPROBE_H