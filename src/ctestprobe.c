
#include "../include/ctestprobe.h"

#include <stdio.h>
#include <stdlib.h>

// Array to store the tests
Test tests[100];

// Number of tests
int num_tests = 0;

// Function to initialize the testing framework
void ctestprobe_init() {
    num_tests = 0;
}

// Function to run a test
void ctestprobe_run_test(Test *test) {
    // Run the test function
    test->test_func();

    // Update the status of the test
    if (test->status == NOT_RUN) {
        test->status = PASSED;
    }
}

// Function to report the results of the tests to the console.
void ctestprobe_console_report() {
    int i;
    int num_passed = 0;
    int num_failed = 0;

    printf("Test Results:\n");

    for (i = 0; i < num_tests; i++) {
        Test test = tests[i];

        printf("Test %s: ", test.name);

        if (test.status == PASSED) {
            printf("PASSED\n");
            num_passed++;
        } else {
            printf("FAILED\n");
            num_failed++;
        }
    }

    printf("Total Tests: %d\n", num_tests);
    printf("Tests Passed: %d\n", num_passed);
    printf("Tests Failed: %d\n", num_failed);
}

// Path: src/ctestprobe.c
