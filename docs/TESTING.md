# TradeBot Testing Contract

## Purpose And Authority

- Purpose: define how TradeBot behavior is verified.
- Authority level: testing authority below risk and architecture policy, above contributor convenience.
- Audience: Codex, contributors, maintainers, reviewers, and testers.

## Philosophy

Tests are evidence, not ceremony. TradeBot is financial-sensitive, so tests must protect deterministic replay behavior, risk gates, order execution boundaries, credential handling, and generated-output reproducibility.

## Verified Test Infrastructure

- Test framework: CTest with C++ test executables.
- Test registration: `CMakeLists.txt`.
- Registered tests:
  - `phase13_tests`
  - `phase15_tests`
  - `phase16_tests`
  - `phase17_tests`
  - `phase18_tests`
- All test executables link against `tradebot_core_lib`.
- Some tests create temporary files under `/tmp`.

## Commands

Configure:

```sh
cmake -S . -B build
```

Build:

```sh
cmake --build build
```

Full suite:

```sh
ctest --test-dir build --output-on-failure
```

Targeted test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
```

## Test Layers

- Unit-style tests: direct class behavior inside phase test executables.
- Integration tests: components wired together through portfolio, risk, execution, adapter, broker, metrics, and trigger-order paths.
- Replay tests: `phase18_tests` covers local CSV and binary replay roundtrip.
- Order-book tests: `phase18_tests` covers BBO application and best quote behavior.
- Financial-mode safety tests: phases 13, 15, 16, and 17 exercise paper/live-capable boundaries without live trading authorization.
- Performance tests: benchmark executables, governed by `BENCHMARKING.md`, not substitutes for correctness tests.

## Required Coverage By Change Type

- Order-book changes: targeted tests for valid/invalid BBO, level mutation, recentering if affected, and `apply_bbo_microbench` when performance is claimed.
- Replay changes: CSV, binary, malformed input, timestamp ordering, cursor, and generated binary compatibility.
- Risk changes: drawdown, position cap, VaR, close-only, halt, latency, error-rate, and live volatility scaling.
- Execution changes: buy/sell, close behavior, fees, slippage, blocked orders, partial fills, trigger orders, broker callback behavior.
- Credential/network changes: env loading, redaction, signing, malformed payloads, reconnect, gap-fill, TLS/local validation where relevant.
- Analytics/output changes: generated CSV path, schema, reproducibility, and no secret leakage.
- Documentation-only changes: `git diff --check`, doc grep audit, and index review.

## Determinism And Reproducibility

- Prefer deterministic inputs.
- Record random seeds if randomness is introduced.
- Use fixed temporary paths only when tests cleanly isolate them.
- Do not require external exchanges, network services, or credentials for normal tests.
- Tests must not depend on ignored generated outputs unless they create them inside the test.

## Fixtures And Test Data

- `data/samples/` exists but is currently empty.
- Tests may create temporary CSV/binary fixtures under `/tmp`.
- Historical data under `data/historical/` is referenced by benchmark code but not tracked in the verified tree.
- Fixture provenance must be documented before committing new data.

## Time And Timezone Tests

When changing time behavior, test:

- Epoch ordering.
- Nanosecond vs candle timestamp units.
- Resume behavior at checkpoint boundaries.
- Stale or out-of-order data handling.

## Concurrency Tests

Concurrency-sensitive areas include lock-free ring buffers, live-data queueing, broker callbacks, reconnect worker behavior, and metrics aggregation. Changes there require stress or integration tests that exercise queue overflow, dropped events, and shutdown.

## Performance And Benchmarks

Benchmark executables are separate from tests:

- `build/apply_bbo_microbench 10000`
- `build/throughput_bench <tick-count>`
- `build/phase18_burnin <tick-count> [replay-path]`

Benchmark output is not enough to accept behavior unless correctness tests also pass.

## Failure Reporting

Reports must include:

- Command run.
- Exit code or pass/fail summary.
- Relevant output lines.
- Warnings.
- Skipped checks and why.
- Whether generated outputs were produced.

Do not summarize a failed test as passed. Do not hide compiler warnings.

## Minimum Evidence For Completion

- Documentation-only: `git diff --check` and documentation audit grep reviewed.
- Narrow source change: build plus targeted tests.
- Shared behavior change: build plus full CTest suite.
- Performance claim: build, relevant tests, benchmark command, environment, input size, and comparative evidence.
- Financial-sensitive change: full tests plus risk-specific tests and operator approval.
