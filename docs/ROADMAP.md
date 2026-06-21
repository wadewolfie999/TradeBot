# TradeBot Roadmap

## Purpose And Authority

- Purpose: define the deterministic Workstreams I-III roadmap, phase sequence, status, and authorization gates.
- Authority level: primary roadmap authority for Workstreams I-III within the repository authority order; below current project state and above workflow details.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.

The roadmap is reconstructed from verified repository evidence, not from unverified legacy claims. Phase names below reflect observed source/test/commit structure. Completion status is recorded only when supported by current files, tests, commits, or accepted docs.

## Roadmap Principles

- Keep the deterministic core testable before adding research or live complexity.
- Treat every live-capable feature as financial-sensitive.
- Require replay, dry-run, paper, or sandbox validation before any live progression.
- Record durable architecture decisions in ADRs.
- Keep Git and GitHub as the execution record; do not recreate deprecated MOP/MOR/SS artifacts.
- Do not skip phases or infer authorization from sequence, completion, approval, or ADR acceptance.

## Roadmap Interpretation Rules

- "Next" identifies sequence; it does not authorize scoping or implementation.
- "Approved" records acceptance of the named artifact or phase outcome; it does not by itself authorize implementation.
- ADR acceptance records an architecture decision; it does not authorize source implementation.
- A blocked phase requires exact operator GO before its blocked activity may begin.
- No phase may be skipped.
- Phase 22 implementation remains Blocked / NO-GO until explicit operator GO. Documentation-only offline MT5/prop-account research is authorized; it does not activate implementation.
- Workstream II must not imply that a broker has been selected. Broker selection requires an explicit operator decision in Phase 23.
- Workstream III must not imply that a documentation platform has been selected. Platform selection requires an explicit operator decision in Phase 25.
- Nobitex is omitted from active Phase 22 scope. Phase 22 research must remain broker-neutral and must not select a broker, prop firm, program, account, or MT5 bridge topology.
- MT5/prop-account readiness is a downstream compatibility target for Phase 22 broker-neutral contracts. It does not select a broker or prop firm and does not authorize MT5 connectivity, account access, credentials, or implementation.
- Live trading remains disabled and unauthorized unless the operator grants exact approval under `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`.

## Deterministic Workstream Authority

This table is the canonical roadmap state for Workstreams I-III. `PROJECT_STATE.md` summarizes it and must not independently redefine these statuses.

| Workstream | Phase | Status | Authorized scope and gate |
| --- | --- | --- | --- |
| I - Technical Integration | Phase 21: Infrastructure Alignment | Complete — Approved | Internal TradeBot integration architecture and boundary alignment. ADR 0003 is Accepted; Phase 21 planning artifacts are approved. |
| I - Technical Integration | Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness | Blocked / NO-GO for implementation | Broker-neutral contract scoping and deterministic offline MT5/prop-account research are authorized. No broker or prop firm selection, connection method, connectivity, credentials, account access, real or sandbox orders, implementation, or live trading is authorized. |
| II - Brokerage & Operations | Phase 23: Broker Selection | Not Started | Identify and evaluate the Most Optimized Broker (M.O.B.). No broker is selected without an explicit operator decision. |
| II - Brokerage & Operations | Phase 24: Connection Protocol | Blocked | Define the TradeBot-to-selected-M.O.B. account connection protocol only after Phase 23 selection and operator approval of connection scope. |
| III - Documentation & Knowledge Management | Phase 25: Documentation Platform Evaluation & Selection | Not Started | Evaluate and select the documentation platform. No platform is selected. |
| III - Documentation & Knowledge Management | Phase 26: Core Documentation Architecture & Drafting | Blocked | Build the core documentation structure and draft canonical documentation only after Phase 25 selection and operator approval of the documentation architecture. |

## Historical And Current Phase Model

The Foundation through Phase 19 entries below are historical context reconstructed from repository evidence. Workstreams I-III and Phases 21-26 are current roadmap authority and use the statuses in the deterministic table above.

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
- Evidence: merged Phase 19 history on `main`; `src/benchmarks/apply_bbo_microbench.cpp`; generated Phase 19 logs under ignored build path.
- Scope: L2 BBO performance evidence and regression checks.
- Expected artifacts: benchmark outputs, CTest results, and source changes when approved by a task.
- Validation: `cmake --build build`, full CTest, targeted Phase 18 test, `build/apply_bbo_microbench <updates>`.
- Exit condition: operator-reviewed evidence shows functional tests pass and benchmark results are recorded with environment/input size.
- Dependencies: Phase 18 behavior and benchmark harness.
- Rollback: revert Phase 19 source changes or restore baseline commit if benchmark or tests regress, with operator approval.
- Status: implementation history is merged on `main`; future performance claims still require fresh benchmark evidence.

### Phase 21: Workstream I Infrastructure Alignment

- Purpose: define broker-neutral integration architecture, subsystem boundaries, adapter lifecycle contracts, risk controls, and replay compatibility gates before implementation.
- Evidence: `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`, `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`, `docs/WORKSTREAM_I_RISK_MATRIX.md`, `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`, and ADR 0003.
- Scope: documentation-only planning and operator review.
- Expected artifacts: Phase 21 planning docs, ADR 0003 disposition, and an explicit Phase 22 NO-GO until separate scoping is approved.
- Validation: documentation audit, cross-document consistency check, and operator approval for ADR/plan status.
- Exit condition: operator accepts ADR 0003 and approves Phase 21 artifacts.
- Dependencies: Phase 18 replay/L2 behavior, Phase 19 BBO benchmark harness, existing risk and live-readiness gates.
- Rollback: revise documentation-only Phase 21 artifacts with operator approval.
- Status: Complete — Approved.

### Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness

- Purpose: align TradeBot execution boundaries around broker-neutral contracts and research offline how MT5-facing integration classes and synthetic prop-account rules could map to those boundaries without selecting a provider or activating implementation.
- Entry conditions for authorized scoping/research: Phase 21 artifacts approved, ADR 0003 accepted, explicit operator authorization for documentation/scoping only, and preserved implementation NO-GO.
- Authorized scope: broker-neutral documentation and offline research covering order lifecycle, account/equity snapshots, position reconciliation, account and symbol metadata, deterministic quantity normalization, lot sizing, execution-result mapping, failure handling, synthetic/configurable prop-rule dimensions, deterministic fixtures, replay validation, risks, and non-activation safeguards.
- Compatibility boundary: MT5-specific transport, terminal bridges, broker APIs, credentials, account access, network connectivity, real order routing, broker selection, and prop-firm selection remain outside the authorization.
- Research artifact: `PHASE22_OFFLINE_MT5_PROP_RESEARCH.md`.
- Documentation validation: Git diff hygiene, documentation audits, cross-document authority consistency, broker-neutrality review, deterministic contract review, and non-activation wording review.
- Future implementation entry conditions: a separately approved bounded implementation plan, frozen scope, explicit operator GO, accepted contract decisions, required adapter/risk/replay tests, rollback path, and resolution of all Phase 23+ decisions that the implementation actually depends on.
- Stop/go gate: no C++ or CMake edits, test implementation, runtime/config/data changes, broker-specific implementation, MT5 connection or login, credentials, account access, broker or prop-firm selection, real or sandbox orders, live trading, risk-limit changes, or Phase 23 activation under the scoping/research authorization.
- Status: broker-neutral documentation/scoping and offline research GO; software implementation Blocked / NO-GO.

### Phase 23: Workstream II Broker Selection

- Purpose: identify and evaluate the Most Optimized Broker (M.O.B.).
- Entry conditions: Phase 22 sequence is respected and the operator authorizes broker-selection evaluation.
- Scope: broker evaluation and an explicit operator selection decision; no connection implementation.
- Stop/go gate: evaluation evidence must not imply that a broker has already been selected.
- Status: Not Started; no broker is selected.

### Phase 24: Workstream II Connection Protocol

- Purpose: define the TradeBot-to-selected-M.O.B. account connection protocol.
- Entry conditions: Phase 23 records an explicit broker selection and the operator approves the connection scope.
- Scope: connection protocol only within the separately approved scope.
- Stop/go gate: no account connection, credential use, or live trading before exact operator approval and applicable risk gates.
- Status: Blocked on Phase 23 selection and operator approval.

### Phase 25: Workstream III Documentation Platform Evaluation And Selection

- Purpose: evaluate and select the documentation platform.
- Entry conditions: prior phases are not skipped and the operator authorizes platform evaluation.
- Scope: platform evaluation and an explicit operator selection decision.
- Stop/go gate: evaluation must not imply that a platform has already been selected.
- Status: Not Started; no documentation platform is selected.

### Phase 26: Workstream III Core Documentation Architecture And Drafting

- Purpose: build the core documentation structure and draft canonical documentation.
- Entry conditions: Phase 25 records an explicit platform selection and the operator approves the documentation architecture.
- Scope: documentation architecture and drafting only within the separately approved scope.
- Stop/go gate: no platform-specific architecture or drafting may begin from an assumed selection.
- Status: Blocked on Phase 25 selection and operator approval.

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

- If `PROJECT_STATE.md` conflicts with the Workstreams I-III table, stop and reconcile the summary against this roadmap and higher authority before acting.
- If an ADR conflicts with roadmap direction, the accepted ADR governs until superseded.
- If tests fail during a phase gate, halt implementation and follow `FAILURE_RECOVERY.md`.

## Next Roadmap Action

The next major step is operator review of the Phase 22 research artifact plus continued broker-neutral contract scoping and offline evidence collection. Phase 22 software implementation, MT5 connectivity, credentials, account access, broker or prop-firm selection, real or sandbox orders, live trading, risk changes, and Phase 23 activation remain unauthorized.
