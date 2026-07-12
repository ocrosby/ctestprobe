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

static void test_pointer_not_null(void) {
    int x = 0;
    CTP_ASSERT_NOT_NULL(&x);
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("addition works",  test_addition_works);
    ctestprobe_register("string compare",  test_string_compare);
    ctestprobe_register("bytes compare",   test_bytes_compare);
    ctestprobe_register("pointer not NULL", test_pointer_not_null);
    return ctestprobe_main(argc, argv);
}
