# TradeBot Roadmap

## Purpose And Authority

- Purpose: define the active phase model, validation gates, and current phase marker.
- Authority level: roadmap and phase guidance below current project state and above workflow details.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.

The roadmap is reconstructed from verified repository evidence, not from unverified legacy claims. Phase names below reflect observed source/test/commit structure. Completion status is recorded only when supported by current files, tests, commits, or accepted docs.

## Roadmap Principles

- Keep the deterministic core testable before adding research or live complexity.
- Treat every live-capable feature as financial-sensitive.
- Require replay, dry-run, paper, or sandbox validation before any live progression.
- Record durable architecture decisions in ADRs.
- Keep Git and GitHub as the execution record; do not recreate deprecated MOP/MOR/SS artifacts.

## Current Phase Marker

- Current phase: Phase 19 revalidation.
- Evidence: branch `phase/phase19-revalidation`; commits for Phase 19 baseline restoration, `apply_bbo` microbenchmark, and hot-path optimization.
- Goal inferred from evidence: validate and benchmark `L2OrderBook::applyBbo` hot-path behavior without weakening Phase 18 replay/trigger-order behavior.
- Status: in progress pending operator review and final validation evidence.

## Phase Model

### Foundation And Deterministic Core

- Purpose: establish C++20 core, CMake build, CSV input, strategies, portfolio accounting, execution simulation, analytics, and repeatable local tests.
- Entry conditions: repository builds locally.
- Scope: deterministic source paths, no live-order side effects.
- Expected artifacts: core headers/source, tests, analytics outputs under generated paths.
- Validation: CMake build and CTest.
- Exit condition: deterministic baseline passes local tests.
- Status: partially verified by current CMake project and phase tests.

### Phase 9: Resume And Advanced Orders

- Purpose: support snapshot/resume and pending order behavior.
- Evidence: `--resume` handling in `src/main.cpp`; `StateSerializer`; pending order state in `PortfolioManager`.
- Validation: relevant CTest coverage and resume-specific checks when changed.
- Stop/go gate: do not change resume semantics without data/replay compatibility review.
- Status: implementation evidence exists; completion beyond current tests is not independently asserted here.

### Phase 10-11: Multi-Strategy Allocation And Regime Handling

- Purpose: support multiple strategies, portfolio allocation, and regime-aware weighting.
- Evidence: `PortfolioAllocator`, `RegimeDetector`, SMA and mean-reversion strategies wired in `src/main.cpp`.
- Validation: CTest plus strategy/allocation-specific regression when changed.
- Stop/go gate: no profitability claims from strategy behavior without research evidence that models fees, slippage, data leakage, and lookahead bias.
- Status: implementation evidence exists.

### Phase 12-17: Live-Capable Infrastructure And Resilience

- Purpose: runtime modes, live-data adapter, broker gateway, async network client, authentication, TLS/local validation, metrics, fault injection, and resilience checks.
- Evidence: `SystemConfig`, `LiveDataAdapter`, `BrokerGateway`, `AsyncNetworkClient`, `AuthManager`, `LocalMetricsExporter`, `phase15_tests`, `phase16_tests`, `phase17_tests`.
- Validation: CTest phases 15-17; security review for credential/network changes.
- Stop/go gate: live-capable code must remain default-off and must not infer live authorization.
- Status: implementation and tests exist; live trading remains unauthorized.

### Phase 18: Replay, L2 Order Book, Trigger Orders, And Burn-In

- Purpose: local replay adapter, L2 BBO handling, stop-loss/OCO trigger execution, and burn-in latency validation.
- Evidence: `LocalDataReplayAdapter`, `L2OrderBook`, `TriggerOrderManager`, `phase18_tests`, `phase18_burnin`.
- Validation: `ctest --test-dir build -R phase18_tests --output-on-failure`; benchmark and burn-in evidence when performance is changed.
- Stop/go gate: replay correctness and trigger-order safety must not regress.
- Status: test target exists and passed during 2026-06-06 verification.

### Phase 19: L2 BBO Performance Revalidation

- Purpose: validate `applyBbo` hot-path performance and preserve functional correctness.
- Evidence: branch and commits; `src/benchmarks/apply_bbo_microbench.cpp`; generated Phase 19 logs under ignored build path.
- Scope: L2 BBO performance evidence and regression checks.
- Expected artifacts: benchmark outputs, CTest results, and source changes when approved by a task.
- Validation: `cmake --build build`, full CTest, targeted Phase 18 test, `build/apply_bbo_microbench <updates>`.
- Exit condition: operator-reviewed evidence shows functional tests pass and benchmark results are recorded with environment/input size.
- Dependencies: Phase 18 behavior and benchmark harness.
- Rollback: revert Phase 19 source changes or restore baseline commit if benchmark or tests regress, with operator approval.
- Status: current active phase.

### Future Research And Optimization Boundary

- Purpose: future Python or Julia research, optimization, ML-assisted strategy work, or offline analysis.
- Entry conditions: deterministic outputs, data policy, reproducible configs, and explicit interfaces.
- Scope: offline research only until promoted through validation.
- Validation: documented datasets, seeds, fees, slippage, leakage checks, and no production-readiness claims from backtests alone.
- Stop/go gate: research cannot bypass risk, execution-mode, logging, or live-readiness gates.
- Status: no tracked Python or Julia components currently exist.

### Live Trading Readiness

- Purpose: controlled path from replay/dry-run/paper/sandbox validation to live authorization.
- Entry conditions: explicit operator request plus all `LIVE_TRADING_READINESS.md` requirements.
- Scope: real venue/account operation only after approval.
- Validation: paper or sandbox evidence, kill switch, reconciliation, credentials, risk limits, logging, stale-data handling, and rollback plan.
- Stop/go gate: absent explicit operator authorization, live trading is prohibited.
- Status: not authorized.

## Recovery Considerations

- If roadmap state conflicts with `PROJECT_STATE.md`, use `PROJECT_STATE.md` for current evidence and update this roadmap after review.
- If an ADR conflicts with roadmap direction, the accepted ADR governs until superseded.
- If tests fail during a phase gate, halt implementation and follow `FAILURE_RECOVERY.md`.
