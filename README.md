# ctestprobe

A minimal unit-testing framework for C.

## Motivation

Writing C after time in modern languages, I miss having a simple unit-testing
harness. C has no reflection, so tests must be registered manually — but the
resulting API can still be light and legible. `ctestprobe` is a small static
library that provides just enough scaffolding: register tests, run them, get
a summary.

## Building

```sh
make          # build lib, self-test, and example
make test     # run the self-test
make example  # build and run the example
make clean
```

Artifacts land in `build/`:

- `build/libctestprobe.a` — the static library
- `build/ctestprobe.o` — the object file
- `build/test_ctestprobe` — self-test binary
- `build/example1` — example binary

## Using it in another project

Add `-I<path-to-ctestprobe>/include` to your compile flags and link against
`build/libctestprobe.a`. Then:

```c
#include "ctestprobe.h"

static void test_addition(void) {
    CTP_ASSERT_EQ_INT(2 + 2, 4);
}

static void test_strings(void) {
    CTP_ASSERT_EQ_STR("hello", "hello");
}

int main(void) {
    ctestprobe_init();
    ctestprobe_register("addition", test_addition);
    ctestprobe_register("strings",  test_strings);
    int failed = ctestprobe_run_all();
    ctestprobe_console_report();
    return failed == 0 ? 0 : 1;
}
```

## API

### Registration and running

- `void ctestprobe_init(void)` — reset the internal registry.
- `void ctestprobe_register(const char *name, void (*fn)(void))` — register
  a test. `name` must outlive the run.
- `void ctestprobe_run_test(Test *t)` — run one test in place, updating its
  status. Rarely called directly; use `ctestprobe_run_all` instead.
- `int  ctestprobe_run_all(void)` — run every registered test and return the
  number that failed.
- `void ctestprobe_console_report(void)` — print a per-test summary to
  stdout.

### Assertions

Each assertion, on failure, marks the currently-running test `FAILED`,
prints `FAIL <file>:<line>: <detail>` to stderr, and returns from the test
function. Because they use `return`, they can only be called inside a
`void`-returning test.

- `CTP_ASSERT(cond)` — fail if `cond` is falsy.
- `CTP_ASSERT_EQ_INT(got, want)` — integer equality (widened to
  `long long`).
- `CTP_ASSERT_EQ_STR(got, want)` — C-string equality via `strcmp`.
- `CTP_ASSERT_EQ_BYTES(got, got_len, want, want_len)` — length + content
  equality on byte arrays.

## Limits

- Fixed registry of 256 tests per binary (compile-time constant).
- No setup / teardown callbacks — do that inline in the test.
- No parallelism, no fork isolation — a crashing test kills the runner.
- No timing — `Test.execution_time` is reserved but currently unused.

## License

See `LICENSE`.
