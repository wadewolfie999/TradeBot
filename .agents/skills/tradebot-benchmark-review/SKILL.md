---
name: tradebot-benchmark-review
description: Review TradeBot benchmark design, reproducibility, workload realism, deterministic inputs, provenance, and performance-claim validity. Use when adding, changing, running, interpreting, or citing benchmark evidence or generated benchmark artifacts.
---
# tradebot-benchmark-review

## Purpose

Validate benchmark quality and prevent overclaims from weak or non-reproducible measurements.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use when adding, changing, running, interpreting, or citing benchmarks, generated benchmark outputs, Phase 19 performance evidence, throughput claims, or burn-in metrics.

## Must Not Be Used

Do not use to approve correctness, replay behavior, risk behavior, or source implementation by itself.

## Required Inputs

- Benchmark command.
- Build mode, compiler, and environment.
- Input size and workload source.
- Generated output paths.
- Claimed interpretation.
- Baseline or comparison target.

## Required Outputs

- Benchmark-quality finding.
- Reproducibility and provenance summary.
- Measurement limitations.
- Allowed or rejected benchmark claim.

## Required Inspection

Read:

- `docs/BENCHMARKING.md`
- `docs/TESTING.md`
- `docs/DATA_POLICY.md`
- Relevant benchmark source and generated-output policy when scoped

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Check workload realism and scope.
2. Verify deterministic inputs, provenance, sample size, warmup, timing method, compiler, build mode, and generated paths.
3. Confirm generated outputs remain ignored unless intentionally versioned and approved.
4. Distinguish microbenchmark improvement from system-level improvement.
5. Reject overclaiming, profitability claims, live-readiness claims, or cross-machine comparisons without environment detail.

## Validation Checklist

- Benchmark command is exact.
- Input size and workload provenance are recorded.
- Build/compiler environment is named.
- Timing method and warmup behavior are understood.
- Comparison baseline is valid.
- Generated output paths are reported and not treated as source truth.

## Failure Modes Caught

- Unrealistic benchmark workload.
- Non-reproducible timing evidence.
- Missing build mode or compiler.
- Generated artifact contamination.
- Microbenchmark result overstated as full-system improvement.

## Hard Prohibitions

- Do not make benchmark claims without command, input, environment, and comparison evidence.
- Do not treat benchmark success as correctness, profitability, live-readiness, or implementation authorization.
- Do not commit generated benchmark outputs unless explicitly approved.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Pair with `tradebot-performance-review` for performance-change risk.
- Pair with `tradebot-cpp-build-test` for build/test context.
- Pair with `tradebot-market-replay-validation` if benchmark inputs use replay data.
- Pair with `tradebot-l2-orderbook-review` for `applyBbo` or L2 benchmark evidence.

## Example Invocation Prompt

```text
Use $tradebot-benchmark-review to evaluate whether apply_bbo_microbench output supports the claimed Phase 19 performance result.
```

## Stop Conditions

Stop if benchmark provenance, input size, command, build mode, compiler, generated path, baseline, or timing method is missing for a performance claim.

## Reporting Format

```markdown
## Benchmark Review
- Benchmark:
- Command:
- Environment:
- Workload/provenance:
- Baseline:
- Result interpretation:
- Claim allowed:
```

## Authority Documents

- `docs/BENCHMARKING.md`
- `docs/TESTING.md`
- `docs/DATA_POLICY.md`
- `docs/ROADMAP.md`
