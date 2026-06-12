# TradeBot Benchmarking Policy

## Purpose And Authority

- Purpose: govern benchmark execution, evidence, generated outputs, and performance claims.
- Authority level: benchmarking authority below testing, risk, and architecture policy.
- Audience: operator, maintainers, Codex, performance reviewers, and research agents.

## Verified Benchmark Targets

- `throughput_bench`: synthetic live-like throughput and latency benchmark; writes `data/results/latency_report.csv`.
- `phase18_burnin`: replay/order-book/trigger-order burn-in benchmark; writes `data/results/phase18_burnin_latency.csv`; can generate replay data.
- `apply_bbo_microbench`: Phase 19 BBO hot-path benchmark.

Build all benchmarks:

```sh
cmake --build build
```

Run Phase 19 microbenchmark:

```sh
build/apply_bbo_microbench 10000
```

## Evidence Requirements

Every performance claim must include:

- Branch and commit.
- Build command and build mode.
- Compiler and platform.
- Benchmark command.
- Input size.
- Output metrics.
- Comparison baseline.
- Whether generated outputs were created.
- Relevant tests run.

Do not compare benchmark results across machines without naming machine and build context.

## Correctness Before Performance

Correctness tests must pass before accepting performance work. For Phase 19 order-book work, minimum evidence is:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase18_tests --output-on-failure
build/apply_bbo_microbench 10000
```

## Generated Outputs

Benchmark CSVs, replay binaries, and logs are generated outputs. Keep them under ignored paths such as `data/results/` or `build/`. Do not commit benchmark outputs unless the operator intentionally versions evidence.

## Performance Claim Limits

- Do not claim profitability from latency or throughput benchmarks.
- Do not claim production readiness from benchmark success.
- Do not use synthetic benchmark data as evidence for live-market behavior without stating limitations.
- Report outliers, warnings, and failed runs.

## Regression Handling

If a benchmark regresses:

- Confirm correctness tests still pass.
- Re-run with the same input size.
- Compare against prior commit or archived evidence.
- Inspect build mode and compiler.
- Halt performance claims until the regression is understood.
