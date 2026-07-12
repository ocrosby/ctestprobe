# Contributing to ctestprobe

Thanks for your interest — here's what you need to know.

## Ground rules

- **Commits on `main` must use [Conventional Commits](https://www.conventionalcommits.org/).** The `Release` workflow uses `jedi-knights/go-semantic-release` to derive the next semantic version from commit messages, so a wrong prefix means a wrong version bump. Examples:
  - `feat(assertions): CTP_ASSERT_ARRAY_EQ` → minor bump
  - `fix(runtime): stderr flush after each FAIL` → patch bump
  - `refactor(api)!: rename ctp_run_all to ctp_execute_all` → major bump (breaking)
  - `docs: clarify fork isolation` → no version bump
- **One PR = one `type(scope)` pair.** If the change needs "and" to describe, split it.
- **Every change must build clean under `-Wall -Wextra -Wpedantic`** and pass the self-test (`make test`).
- **Every new runtime feature must have a case in `tests/test_ctestprobe.c`.** The self-test is the framework's regression net; if it's not covered there, it isn't real.

## Local development

```bash
make                # build library, self-test, example
make test           # run self-test
make example        # build and run example1
make asan           # run self-test under AddressSanitizer + UBSan
make ubsan          # run self-test under UBSan only
make single-header  # regenerate single_include/ctestprobe.h
make clean
```

CMake builds are also supported:

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## Filing issues

- **Bugs**: include the OS, compiler + version, and a minimal reproducer. If it's a crash, `make asan` output is worth 100 words of narrative.
- **Feature requests**: describe the use case first. `ctestprobe` is intentionally small; new surface has to earn its keep. Framework-shape decisions are guided by the "minimal, register manually, no reflection" premise.

## Opening a PR

1. Branch from `main`. Name the branch after the intended commit: `feat/floating-point-assertions`, `fix/fork-timing-race`, `docs/migration-guide`, etc.
2. Make the change, add a self-test case for it, ensure `make test` and `make asan` are clean.
3. Push and open a PR. The CI workflow runs a compiler matrix (gcc/clang on ubuntu, clang on macos), sanitizers, and cppcheck.
4. Once merged to `main`, the Release workflow will compute the next version, cut a GitHub Release, attach the source tarball and single-header asset.

## Coding conventions

- **C11** (`-std=c11`). No C99 fallbacks, no GNU-C extensions in public headers.
- **Public identifiers** — `ctestprobe_*` for functions, `ctp_*` for types (`ctp_test_t`, `ctp_test_status_t`), `CTP_*` for enum members and macros. No unprefixed identifiers escape the header.
- **Assertion macros** call `ctestprobe_fail` (fatal, `longjmp`s) or `ctestprobe_expect_fail` (nonfatal). Do not add a `return;` after them — it's dead code.
- **Static / file-scope globals** are prefixed `g_`. There's no other module-scope state.
- **POSIX-only features** (`fork`, `waitpid`, `isatty`) live inside guards or opt-in paths so a Windows port stays plausible.

## Release cadence

Releases are automatic on push to `main` (see `.github/workflows/release.yml`). No manual tag pushes.
