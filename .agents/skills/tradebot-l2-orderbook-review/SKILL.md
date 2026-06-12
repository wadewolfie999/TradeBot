---
name: tradebot-l2-orderbook-review
description: Review or validate TradeBot L2 order-book behavior and Phase 19 applyBbo performance. Use when touching L2OrderBook, BBO updates, recentering, trigger-order BBO inputs, Phase 18 tests, or Phase 19 benchmarks.
---
# tradebot-l2-orderbook-review

## Purpose

Review or validate changes affecting TradeBot L2 order-book behavior and Phase 19 `applyBbo` performance.

## Activation Conditions

Use when touching or reviewing `L2OrderBook`, BBO updates, recentering, trigger-order BBO inputs, Phase 18 tests, or Phase 19 benchmarks.

## Must Not Be Used

Do not use for unrelated execution, broker, credential, or live-trading changes. Do not make performance claims without benchmark evidence.

## Required Inputs

- Changed files or diff.
- Claimed behavior or performance objective.
- Expected test/benchmark evidence.

## Required Repository Inspection

Read:

- `include/L2OrderBook.hpp`
- `src/L2OrderBook.cpp`
- `tests/phase18_tests.cpp`
- `src/benchmarks/apply_bbo_microbench.cpp`
- `docs/ARCHITECTURE.md`
- `docs/BENCHMARKING.md`

Run:

```sh
git status --short
```

## Procedure

1. Verify BBO validity rules are preserved.
2. Check bid/ask level mutation behavior.
3. Check recentering and index mapping if affected.
4. Confirm trigger-order paths still receive valid BBO.
5. Run targeted Phase 18 test.
6. Run `apply_bbo_microbench` when performance is relevant.
7. Report correctness before performance.

## Allowed Mutations

Only when assigned implementation scope includes L2 files or docs. Build and benchmark outputs may be generated under ignored paths.

## Prohibited Mutations

No live-trading changes, no risk-limit changes, no broker changes unless separately scoped, no generated-output commits.

## Verification Commands

```sh
cmake --build build
ctest --test-dir build -R phase18_tests --output-on-failure
build/apply_bbo_microbench 10000
```

## Expected Outputs

- Correctness findings.
- Benchmark command and metrics if run.
- Regression risks.
- Files requiring tests or docs.

## Failure Behavior

If BBO correctness is uncertain, stop performance claims and request or add tests before proceeding.

## Reporting Format

```markdown
## L2 Review
- Files inspected:
- Correctness:
- Performance evidence:
- Risks:
- Required fixes:
```

## Authority Documents

- `docs/ARCHITECTURE.md`
- `docs/TESTING.md`
- `docs/BENCHMARKING.md`
