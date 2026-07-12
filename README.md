# ctestprobe

A minimal unit-testing framework for C — with hooks, filters, fork isolation, and TAP output.

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
- [Migration from v1](#migration-from-v1)
- [Limits](#limits)
- [Contributing](#contributing)
- [License](#license)

## Overview

C has no reflection, so tests must be registered manually — but the resulting API can still be light and legible. `ctestprobe` is a small static library that provides just enough scaffolding: register tests, run them, get a summary. Since v2 it also grew crash-safe fork isolation, TAP output for CI harnesses, and setup/teardown hooks — while keeping the header-plus-static-library shape.

## Features

- Explicit test registration — no reflection, no macros discovering tests behind your back
- Fatal `CTP_ASSERT_*` and non-fatal `CTP_EXPECT_*` variants, both routed through the same failure path
- Assertions work from any call depth via `setjmp`/`longjmp` bailout — not only the top-level test function
- Setup / teardown hooks, both global and per-test
- Substring filter to run one test (via API, CLI flag, or env var)
- Fork isolation: a crashing test reports as FAIL instead of aborting the runner (POSIX)
- Per-test timing populated via `clock_gettime(CLOCK_MONOTONIC)`
- TAP version 14 output and JUnit XML (Surefire dialect) for CI parsers
- Colored console output on a tty; opt out via `--no-color` or `NO_COLOR=1`
- Dynamic registry — no compile-time test cap

## Requirements

- A C11-capable compiler (`cc`, `clang`, or `gcc`)
- `make` and `ar` (POSIX toolchain)
- POSIX runtime (`fork`, `waitpid`, `clock_gettime`, `isatty`) — required for `--fork` and timing

## Installation

Three consumption paths — pick whichever fits your build system.

### Path 1: single-header drop-in (recommended for small projects)

Grab `ctestprobe.h` from the [latest release](https://github.com/ocrosby/ctestprobe/releases/latest) and drop it into your source tree. Then, in exactly one translation unit:

```c
#define CTP_IMPLEMENTATION
#include "ctestprobe.h"
```

In every other translation unit that uses assertions, just:

```c
#include "ctestprobe.h"
```

No library to build, no linker flags to add.

### Path 2: static library via Make

```bash
git clone https://github.com/ocrosby/ctestprobe.git
cd ctestprobe
make
sudo make install PREFIX=/usr/local   # or PREFIX=$HOME/.local, etc.
```

Installs `libctestprobe.a` to `$(PREFIX)/lib/`, `ctestprobe.h` to `$(PREFIX)/include/`, and `ctestprobe.pc` to `$(PREFIX)/lib/pkgconfig/`. Consume via pkg-config:

```bash
cc mytests.c $(pkg-config --cflags --libs ctestprobe) -o mytests
```

To uninstall: `sudo make uninstall PREFIX=/usr/local`.

### Path 3: CMake (in-tree or find_package)

In-tree (as a submodule or FetchContent):

```cmake
add_subdirectory(third_party/ctestprobe)
target_link_libraries(mytests PRIVATE ctestprobe::ctestprobe)
```

Installed:

```cmake
find_package(ctestprobe REQUIRED)
target_link_libraries(mytests PRIVATE ctestprobe::ctestprobe)
```

### Homebrew

A tap formula lives in [`packaging/homebrew/ctestprobe.rb`](packaging/homebrew/ctestprobe.rb) as a template. Once published to a `homebrew-ctestprobe` tap, install with `brew install ocrosby/ctestprobe/ctestprobe`.

### Build artifacts (repo layout)

- `build/libctestprobe.a` — the static library
- `build/ctestprobe.o` — the object file
- `build/ctestprobe.pc` — generated pkg-config metadata
- `build/test_ctestprobe` — self-test binary
- `build/example1` — example binary
- `single_include/ctestprobe.h` — generated single-header (via `make single-header`)

## Usage

Write a test file that registers each test and delegates `main` to the framework's CLI entry point:

```c
#include "ctestprobe.h"

static void test_addition(void) {
    CTP_ASSERT_EQ_INT(2 + 2, 4);
}

static void test_strings(void) {
    CTP_ASSERT_EQ_STR("hello", "hello");
}

int main(int argc, char **argv) {
    ctestprobe_init();
    ctestprobe_register("addition", test_addition);
    ctestprobe_register("strings",  test_strings);
    return ctestprobe_main(argc, argv);
}
```

The binary then accepts flags:

```bash
./mytests                          # run all, console report
./mytests --list                   # list registered test names, exit 0
./mytests --filter=addition        # run only tests whose name contains "addition"
./mytests --tap                    # emit TAP version 14 output instead
./mytests --junit=results.xml      # also write JUnit XML for a CI parser
./mytests --fork                   # run each test in a forked process (crash-safe)
./mytests --no-color               # disable ANSI color
./mytests --help                   # full usage
```

### Assertion macros

Fatal assertions (bail out of the current test via `longjmp`):

- `CTP_ASSERT(cond)` — fail if `cond` is falsy
- `CTP_ASSERT_TRUE(cond)` / `CTP_ASSERT_FALSE(cond)`
- `CTP_ASSERT_NULL(ptr)` / `CTP_ASSERT_NOT_NULL(ptr)`
- `CTP_ASSERT_EQ_INT(got, want)` — integer equality (widened to `long long`)
- `CTP_ASSERT_EQ_STR(got, want)` — C-string equality via `strcmp`
- `CTP_ASSERT_STR_CONTAINS(haystack, needle)` — `strstr` check
- `CTP_ASSERT_EQ_DOUBLE(got, want, epsilon)` — absolute-error comparison
- `CTP_ASSERT_EQ_BYTES(got, got_len, want, want_len)` — length + content equality on byte arrays

Non-fatal expectations (record the failure but keep running the test):

- `CTP_EXPECT(cond)`
- `CTP_EXPECT_EQ_INT(got, want)`
- `CTP_EXPECT_EQ_STR(got, want)`

Both families print `FAIL <file>:<line>: <detail>` (or `EXPECT ...`) to `stderr` and mark the current test as `CTP_FAILED`.

### Setup / teardown hooks

```c
static void global_setup(void *ud)    { /* once before any test */ }
static void global_teardown(void *ud) { /* once after all tests */ }
static void each_setup(void *ud)      { /* before every test */ }
static void each_teardown(void *ud)   { /* after every test (pass or fail) */ }

ctestprobe_set_global_setup(global_setup, my_user_data);
ctestprobe_set_each_setup(each_setup, my_user_data);
/* ... */
```

### Fork isolation

`--fork` (or `ctestprobe_set_fork_isolation(1)` / `CTP_FORK_TESTS=1`) runs each test in a `fork()`ed child. If the child dies from a signal (segfault, abort, timeout kill), the parent reports it as `FAIL` with the signal name and continues to the next test. Per-test timing is measured in the parent, so fork overhead shows up in the reported wall-clock.

## Configuration

Configuration is via CLI flags and environment variables — there is no config file.

| Setting          | Flag              | Env var               | Default          |
|------------------|-------------------|-----------------------|------------------|
| Substring filter | `--filter=SUBSTR` | `CTP_FILTER`          | (all tests)      |
| Fork isolation   | `--fork`          | `CTP_FORK_TESTS=1`    | off              |
| Colored console  | `--no-color` to disable | `NO_COLOR=1`    | auto (tty check) |
| TAP output       | `--tap`           | —                     | console report   |
| JUnit XML output | `--junit=PATH`    | `CTP_JUNIT=PATH`      | off              |
| List only        | `--list`          | —                     | run              |

`--junit=PATH` is additive: it writes the XML to `PATH` and still emits the console (or TAP) report to stdout, so GitHub Actions can render the test-summary widget from the XML while a human reads the console output.

Compile-time behavior is controlled via the Makefile:

- `CC` — C compiler (default: `cc`)
- `CFLAGS` — compile flags (default: `-std=c11 -Wall -Wextra -Wpedantic -O2 -g`)
- `AR` — archiver (default: `ar`)

## Development

Available make targets:

```bash
make               # build library, self-test, example, and pkg-config
make test          # run the self-test binary
make example       # build and run the example
make asan          # rebuild whole tree with -fsanitize=address,undefined and run self-test
make ubsan         # same, -fsanitize=undefined
make tsan          # same, -fsanitize=thread
make single-header # generate single_include/ctestprobe.h
make install       # install lib, header, and pkg-config to PREFIX (default /usr/local)
make uninstall
make clean
```

CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Releases

Releases are automated. A push to `main` runs the [`Release` workflow](.github/workflows/release.yml), which uses [`jedi-knights/go-semantic-release`](https://github.com/jedi-knights/go-semantic-release) to analyze [Conventional Commits](https://www.conventionalcommits.org/) since the last tag, cut the next semantic version, publish a GitHub Release, and attach a source tarball (`ctestprobe-<ver>.tar.gz`) as a release asset. The tarball is produced by `git archive` and expands into a versioned prefix directory `ctestprobe-<ver>/`.

Commit types map to release types:

- `feat:` → minor bump
- `fix:` → patch bump
- Any commit with a `!` or a `BREAKING CHANGE:` footer → major bump

## Migration from v1

v2.0.0 renamed the public types and enum members to prefixed identifiers so they don't collide with typical consumer code (`Test`, `FAILED`, etc. are extremely common names in test files).

| v1                       | v2                           |
|--------------------------|------------------------------|
| `Test`                   | `ctp_test_t`                 |
| `TestStatus`             | `ctp_test_status_t`          |
| `NOT_RUN`                | `CTP_NOT_RUN`                |
| `PASSED`                 | `CTP_PASSED`                 |
| `FAILED`                 | `CTP_FAILED`                 |
| (n/a)                    | `CTP_SKIPPED` (filtered out) |
| `Test.name` (`char *`)   | `ctp_test_t.name` (`const char *`) |

Bulk sed for consumer code:

```bash
sed -i.bak \
  -e 's/\bTest\b/ctp_test_t/g' \
  -e 's/\bTestStatus\b/ctp_test_status_t/g' \
  -e 's/\bNOT_RUN\b/CTP_NOT_RUN/g' \
  -e 's/\bPASSED\b/CTP_PASSED/g' \
  -e 's/\bFAILED\b/CTP_FAILED/g' \
  path/to/tests/*.c
```

Behavioral changes:

- `CTP_ASSERT_*` macros now use `setjmp`/`longjmp` for bailout — they work at any call depth, not only inside the top-level test function. `ctestprobe_fail` `abort()`s if called outside a running test frame (previously silently ignored).
- The 256-test compile-time cap is gone; the registry grows via `realloc`.
- `Test.execution_time` is now populated (via `CLOCK_MONOTONIC`); before it was a reserved field with a permanent zero value.
- `main` can now delegate to `ctestprobe_main(argc, argv)` for a full CLI (filter, list, TAP, fork, help). The old `ctestprobe_run_all` + `ctestprobe_console_report` idiom still works.

## Limits

- No parallelism — tests run sequentially. Fork isolation runs each test in its own process but they still run one at a time.
- No timeout per test — a hanging test hangs the runner unless you enable `--fork` and manage it with your own supervisor.
- `--fork` is POSIX-only; on Windows the flag has no effect (nothing to fork with).
- Registry ordering is FIFO. There is no dependency graph or explicit ordering annotation.

## Contributing

Issues and pull requests are welcome at [github.com/ocrosby/ctestprobe](https://github.com/ocrosby/ctestprobe). See [CONTRIBUTING.md](CONTRIBUTING.md) for the branch / commit / self-test discipline.

## License

Licensed under GPL-3.0. See [LICENSE](LICENSE).
