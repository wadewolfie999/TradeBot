---
name: tradebot-cpp-build-test
description: Build and test the TradeBot C++20 core using verified local commands. Use when C++ source, header, CMake, test, or benchmark changes need verification or when verification evidence is requested.
---
# tradebot-cpp-build-test

## Purpose

Build and test the TradeBot C++20 core using verified local commands.

## Activation Conditions

Use after C++ source/header changes, CMake changes, test changes, benchmark changes, or when verification evidence is requested.

## Must Not Be Used

Do not use to justify live trading or profitability claims. Do not run live services or external exchange operations.

## Required Inputs

- Changed files or subsystem.
- Whether full or targeted verification is needed.
- Any active plan acceptance criteria.

## Required Repository Inspection

Run:

```sh
git status --short
git branch --show-current
```

Read `docs/TESTING.md` and affected tests.

## Procedure

1. Configure if needed: `cmake -S . -B build`.
2. Build: `cmake --build build`.
3. Run targeted tests for touched subsystems.
4. Run full CTest for shared behavior changes.
5. Capture warnings and failures.
6. For performance work, run benchmark commands only after correctness checks.

## Allowed Mutations

Build/test commands may write generated build artifacts, temporary files, and generated results in ignored paths.

## Prohibited Mutations

No source edits by this skill alone, no formatter rewrites unless explicitly requested, no commits, no pushes, no credential changes, no live services.

## Verification Commands

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase18_tests --output-on-failure
```

Performance-specific:

```sh
build/apply_bbo_microbench 10000
```

## Expected Outputs

- Commands run.
- Pass/fail status.
- Compiler warnings.
- Test failures with relevant output.
- Generated output paths, if any.

## Failure Behavior

Stop on failed build. For failed tests, record command/output, inspect likely cause, and do not claim completion until resolved or explicitly accepted.

## Reporting Format

```markdown
## Build/Test Report
- Configure:
- Build:
- Tests:
- Benchmarks:
- Warnings:
- Failures:
- Generated outputs:
```

## Authority Documents

- `docs/TESTING.md`
- `docs/BENCHMARKING.md`
- `docs/FAILURE_RECOVERY.md`
