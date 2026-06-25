---
name: tradebot-performance-review
description: Review TradeBot performance changes without weakening correctness, determinism, replay integrity, risk controls, or safety gates. Use for optimization, latency, throughput, hot-path changes, performance claims, benchmark-driven work, or Phase 19 evidence review.
---
# tradebot-performance-review

## Purpose

Review performance-sensitive changes while preserving correctness, determinism, replay integrity, and risk controls.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Workstream II/Phase 23 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Phase 22 is Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`. Phase 23/Workstream II remains Not Started, and broker-dependent implementation remains Blocked / NO-GO unless separately approved by the operator.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use before or after optimization, latency/throughput work, hot-path changes, benchmark-driven changes, Phase 19 evidence review, or any performance claim.

## Must Not Be Used

Do not use to replace correctness testing, benchmark methodology review, replay validation, risk review, or build/test verification.

## Required Inputs

- Changed files or proposed optimization.
- Claimed performance objective.
- Baseline measurement, if available.
- Correctness and replay validation evidence.
- Benchmark command and environment.

## Required Outputs

- Correctness-before-performance finding.
- Baseline and evidence summary.
- Determinism and risk-control finding.
- Allowed or rejected performance claim.

## Required Inspection

Read:

- `docs/BENCHMARKING.md`
- `docs/TESTING.md`
- `docs/RISK_POLICY.md`
- Relevant source/tests/benchmarks when scoped

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Require a baseline before optimization claims.
2. Identify correctness, determinism, replay, and risk invariants.
3. Verify performance changes do not alter order semantics, replay outputs, or risk behavior.
4. Require benchmark evidence before performance claims.
5. Defer measurement-quality questions to `tradebot-benchmark-review`.
6. Require replay and relevant tests after performance-sensitive source changes.

## Validation Checklist

- Correctness evidence comes before performance evidence.
- Baseline and after-measurement are comparable.
- Replay determinism is preserved.
- Risk controls are not weakened.
- Benchmark output is not overgeneralized into system-level claims.

## Failure Modes Caught

- Speedup hiding behavioral drift.
- Replay output changes treated as acceptable optimization.
- Risk checks skipped for latency.
- Microbenchmark result overstated as production readiness.
- Missing baseline.

## Hard Prohibitions

- Do not make performance claims without evidence.
- Do not weaken risk controls, determinism, or replay correctness for speed.
- Do not enable live trading or real orders.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Use `tradebot-benchmark-review` for benchmark design and measurement quality.
- Use `tradebot-cpp-build-test` for build/test evidence.
- Use `tradebot-market-replay-validation` for replay-affecting performance work.
- Use `tradebot-l2-orderbook-review` for L2 or `applyBbo` performance work.
- Use `tradebot-risk-review` when execution, risk, live-capable, or financial paths are affected.

## Example Invocation Prompt

```text
Use $tradebot-performance-review to review whether an applyBbo optimization preserves correctness before any benchmark claim.
```

## Stop Conditions

Stop if correctness, replay determinism, risk controls, baseline evidence, or benchmark evidence is missing for a performance claim.

## Reporting Format

```markdown
## Performance Review
- Changed area:
- Baseline:
- Correctness evidence:
- Benchmark evidence:
- Determinism/risk findings:
- Claim allowed:
```

## Authority Documents

- `docs/BENCHMARKING.md`
- `docs/TESTING.md`
- `docs/RISK_POLICY.md`
- `docs/ROADMAP.md`
