#include "ctestprobe.h"

static void test_addition_works(void) {
    CTP_ASSERT_EQ_INT(2 + 2, 4);
}

static void test_string_compare(void) {
    CTP_ASSERT_EQ_STR("hello", "hello");
}

static void test_bytes_compare(void) {
    unsigned char a[] = {1, 2, 3, 4};
    unsigned char b[] = {1, 2, 3, 4};
    CTP_ASSERT_EQ_BYTES(a, sizeof a, b, sizeof b);
}

int main(void) {
    ctestprobe_init();
    ctestprobe_register("addition works", test_addition_works);
    ctestprobe_register("string compare", test_string_compare);
    ctestprobe_register("bytes compare",  test_bytes_compare);
    int failed = ctestprobe_run_all();
    ctestprobe_console_report();
    return failed == 0 ? 0 : 1;
}
