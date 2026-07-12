# ctestprobe

A minimal unit-testing framework for C.

![CI](https://github.com/ocrosby/ctestprobe/actions/workflows/ci.yml/badge.svg)
![Release](https://github.com/ocrosby/ctestprobe/actions/workflows/release.yml/badge.svg)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Development](#development)
- [Limits](#limits)
- [Contributing](#contributing)
- [License](#license)

## Overview

Writing C after time in modern languages, I miss having a simple unit-testing harness. C has no reflection, so tests must be registered manually — but the resulting API can still be light and legible. `ctestprobe` is a small static library that provides just enough scaffolding: register tests, run them, get a summary.

The project ships as a static library plus a header, and consumers link it into their own test binaries.

## Features

- Single static library (`libctestprobe.a`) and one header (`ctestprobe.h`)
- Explicit test registration — no reflection, no macros to discover tests behind your back
- Assertions for integers, C strings, and byte arrays, plus a raw boolean check
- Per-test pass/fail tracking with a console summary reporter
- Self-test binary shipped in the repo for verifying the library itself

## Requirements

- A C11-capable compiler (`cc`, `clang`, or `gcc`)
- `make`
- `ar` (from binutils or your platform toolchain)

## Installation

Clone and build the static library:

```bash
git clone https://github.com/ocrosby/ctestprobe.git
cd ctestprobe
make
```

Artifacts land in `build/`:

- `build/libctestprobe.a` — the static library
- `build/ctestprobe.o` — the object file
- `build/test_ctestprobe` — self-test binary
- `build/example1` — example binary

To consume the library from another project, add `-I<path-to-ctestprobe>/include` to your compile flags and link against `build/libctestprobe.a`.

## Usage

Write a test file that registers each test and calls the runner:

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

### Registration and running

- `void ctestprobe_init(void)` — reset the internal registry.
- `void ctestprobe_register(const char *name, void (*fn)(void))` — register a test. `name` must outlive the run.
- `void ctestprobe_run_test(Test *t)` — run one test in place, updating its status. Rarely called directly; use `ctestprobe_run_all` instead.
- `int  ctestprobe_run_all(void)` — run every registered test and return the number that failed.
- `void ctestprobe_console_report(void)` — print a per-test summary to stdout.

### Assertions

Each assertion, on failure, marks the currently-running test `FAILED`, prints `FAIL <file>:<line>: <detail>` to stderr, and returns from the test function. Because they use `return`, they can only be called inside a `void`-returning test.

- `CTP_ASSERT(cond)` — fail if `cond` is falsy.
- `CTP_ASSERT_EQ_INT(got, want)` — integer equality (widened to `long long`).
- `CTP_ASSERT_EQ_STR(got, want)` — C-string equality via `strcmp`.
- `CTP_ASSERT_EQ_BYTES(got, got_len, want, want_len)` — length + content equality on byte arrays.

## Configuration

`ctestprobe` has no runtime configuration. Compile-time behavior is controlled through the `Makefile`:

- `CC` — C compiler (default: `cc`)
- `CFLAGS` — compile flags (default: `-std=c11 -Wall -Wextra -Wpedantic -O2 -g`)
- `AR` — archiver (default: `ar`)

Override by exporting or passing on the make command line, e.g. `make CC=clang`.

## Development

Available make targets:

```bash
make          # build library, self-test, and example
make test     # run the self-test binary
make example  # build and run the example
make clean    # remove build artifacts
```

### Releases

Releases are automated. A push to `main` runs the [`Release` workflow](.github/workflows/release.yml), which uses [`jedi-knights/go-semantic-release`](https://github.com/jedi-knights/go-semantic-release) to analyze [Conventional Commits](https://www.conventionalcommits.org/) since the last tag, cut the next semantic version, and publish a GitHub Release. The workflow then attaches a source tarball (`ctestprobe-vX.Y.Z.tar.gz`) as a release asset.

Commit messages must follow Conventional Commits (`feat:`, `fix:`, `chore:`, …) — the release type is derived from them:

- `feat:` → minor bump
- `fix:` → patch bump
- Any commit with a `!` or a `BREAKING CHANGE:` footer → major bump

## Limits

- Fixed registry of 256 tests per binary (compile-time constant).
- No setup / teardown callbacks — do that inline in the test.
- No parallelism, no fork isolation — a crashing test kills the runner.
- No timing — `Test.execution_time` is reserved but currently unused.

## Contributing

Issues and pull requests are welcome at [github.com/ocrosby/ctestprobe](https://github.com/ocrosby/ctestprobe). Commits on `main` must follow the [Conventional Commits](https://www.conventionalcommits.org/) format so that semantic-release can compute the next version correctly.

## License

Licensed under GPL-3.0. See [LICENSE](LICENSE).
