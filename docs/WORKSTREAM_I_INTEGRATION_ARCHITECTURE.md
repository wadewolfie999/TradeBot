# Workstream I Integration Architecture

## Purpose And Authority

- Purpose: define the Phase 21 infrastructure-alignment architecture for the technical integration layer between the TradeBot core engine and future broker or exchange interfaces.
- Authority level: planning and architecture guidance below accepted ADRs, `RISK_POLICY.md`, and `ARCHITECTURE.md`; above Phase 22 implementation work once approved by the operator.
- Audience: operator, maintainers, Codex, implementers, reviewers, testers, and future integration agents.
- Status: Phase 21 planning artifact - Approved. This document does not authorize Phase 22 implementation, live trading, broker-specific integration, credential changes, or risk-limit changes.

Workstream I is a planning layer. It defines boundaries, constraints, contracts, and validation gates. It must not be treated as implemented behavior until Phase 22 is separately approved and completed.

## Repository Evidence

Verified integration-relevant boundaries in the current codebase:

- `SystemConfig`: runtime modes and shared configuration.
- `LiveDataAdapter`: live-like and live-capable market-data adapter boundary.
- `AsyncNetworkClient`: network bridge boundary.
- `AuthManager`: credential loading boundary.
- `BrokerGateway`: broker order, cancellation, fill, reconciliation, and live-capable boundary.
- `ExecutionEngine`: signal execution and trigger-order processing boundary.
- `RiskEngine`: risk gates, close-only state, halt state, latency and API-error circuit breakers, and broker-position synchronization.
- `L2OrderBook`: L2 level state and BBO updates.
- `LocalDataReplayAdapter`: deterministic local replay boundary.

Verified runtime modes are `BACKTEST`, `PAPER`, and `LIVE`. `BACKTEST` remains the deterministic default. `LIVE` remains prohibited unless explicitly authorized by the operator through existing risk and live-readiness gates.

## Workstream Scope

In scope:

- Broker-neutral integration architecture.
- Adapter ownership and subsystem boundaries.
- Execution pipeline constraints.
- Risk isolation and fail-closed requirements.
- Determinism and replay compatibility requirements.
- Validation gates for Phase 21 to Phase 22 transition.

Out of scope:

- Broker-specific REST, WebSocket, FIX, authentication, symbol, order-type, or error-code schemas.
- Source implementation.
- Live trading, real orders, account mutation, or external venue state mutation.
- Credential edits or credential inspection.
- Risk-limit, position-sizing, drawdown, halt, close-only, fee, or slippage default changes.
- Generated data cleanup or benchmark-result versioning.

## Phase Model

### Phase 21: Infrastructure Alignment

Purpose:

- Discover and define integration architecture, subsystem boundaries, adapter model, risk constraints, and system interfaces.

Entry criteria:

- Repository state, branch, commit, and worktree status are recorded.
- Phase 18 and Phase 19 L2 validation are treated as baseline continuity evidence.
- No active Phase 22 implementation has started.
- Operator has approved Phase 21 planning execution.

Exit criteria:

- Integration architecture document is complete.
- Adapter contract document is complete.
- Risk matrix is complete.
- Replay compatibility checklist is complete.
- Integration architecture ADR is accepted by operator decision.
- Phase 22 remains blocked until separate operator-approved scoping, verification strategy, and rollback path exist.

### Phase 22: Software Alignment

Purpose:

- Implement only the approved Phase 21 architecture and contracts.

Entry criteria:

- Phase 21 artifacts are approved.
- Phase 22 scope is frozen.
- Design changes are prohibited unless the work returns to Phase 21.
- Required verification strategy and rollback path are accepted.

Exit criteria:

- Approved contracts are implemented without design drift.
- Deterministic tests pass before live-capable paths are considered.
- Risk, replay, documentation, and build/test validation gates pass.
- Live trading remains disabled unless separately authorized.

## Canonical Integration Pipeline

The approved conceptual pipeline is:

```text
Market data or replay events
  -> strategy signal generation
  -> portfolio allocation
  -> ExecutionEngine order-intent handling
  -> RiskEngine gate and risk-state checks
  -> BrokerGateway broker-boundary handling
  -> future broker-neutral adapter implementation
  -> broker event feedback through BrokerGateway
  -> ExecutionEngine / PortfolioManager / RiskEngine state updates
```

Required constraints:

- Strategies must not directly submit broker orders.
- Execution code must not bypass `RiskEngine` before opening new exposure.
- Live-capable broker interaction must remain behind `BrokerGateway`.
- Credentials must remain outside strategies, analytics, replay, and allocation code.
- Replay and backtest paths must not depend on wall-clock time, network state, or live adapter availability.

## Subsystem Boundaries

### L2 Order Book

Responsibilities:

- Own L2 level storage, BBO validity, recentering, and BBO update behavior.
- Provide BBO inputs to trigger-order processing only through explicit interfaces.

Constraints:

- L2 updates must preserve Phase 18 replay and trigger-order behavior.
- `applyBbo` performance work must not weaken BBO correctness.
- L2 code must not depend on broker adapters, credentials, or live execution state.

### Execution Engine

Responsibilities:

- Translate strategy and trigger-order decisions into execution actions.
- Preserve order lifecycle ownership at the TradeBot core boundary.
- Route live-capable broker interactions through `BrokerGateway` when bound.

Constraints:

- New exposure requires `RiskEngine` permission.
- Close-only and halted states must be respected.
- Partial fills, rejected orders, and reconciliation feedback must not silently mutate portfolio state without broker-boundary evidence.

### Risk Engine

Responsibilities:

- Own trade permission, halt state, close-only state, error-rate circuit breakers, latency circuit breakers, volatility scaling, VaR, drawdown controls, and broker-position synchronization.

Constraints:

- No Phase 21 artifact changes risk defaults.
- Halt clearing remains operator-sensitive in live-capable contexts.
- Broker or network failure signals must fail closed when risk state is uncertain.

### Broker Gateway

Responsibilities:

- Own broker-facing submission, cancellation, fill confirmation, paper-mode simulation, and reconciliation snapshot handling.

Constraints:

- Future adapters must attach below this boundary, not beside it.
- Broker IDs, fill state, cancellation state, and reconciliation state must flow through explicit gateway-owned events.
- Broker-specific schemas must not leak into strategy, allocation, L2, replay, or analytics code.

### Networking And Authentication

Responsibilities:

- Network transport remains behind `AsyncNetworkClient`, `LiveDataAdapter`, and future adapter internals.
- Credential loading remains behind `AuthManager` and `SystemConfig`.

Constraints:

- Secret values must never be committed, logged, copied into docs, or exposed in test fixtures.
- External connectivity must not be required for deterministic backtests or replay tests.
- Network or credential ambiguity blocks Phase 22 implementation.

### Replay And Data

Responsibilities:

- `LocalDataReplayAdapter` owns deterministic local replay input behavior.
- Generated replay artifacts remain generated data, not source truth.

Constraints:

- Replay timestamp units and event order must remain explicit.
- Future broker-event simulation must be deterministic and provenance-labeled.
- Any new replay schema requires data-policy review and migration tests.

## Determinism Guarantees

Phase 22 implementation must preserve these guarantees:

- `BACKTEST` mode performs no external broker or network actions.
- Given identical source, config, replay input, and seed or deterministic fixture, replay outcomes are repeatable.
- Wall-clock latency, reconnect timing, API rate limits, and live endpoint state cannot affect replay or backtest decisions.
- Generated outputs under ignored paths do not become hidden inputs.
- Event processing order is stable and documented.
- Broker-like fixtures used in tests are synthetic, captured, or generated with explicit provenance.

## Risk Isolation Requirements

Required isolation rules:

- `RiskEngine::canTrade()` or an approved equivalent must gate new exposure.
- `RiskEngine::isCloseOnly()` must prevent new risk-increasing exposure.
- `RiskEngine::isHalted()` must block new trading.
- `RiskEngine::syncPosition()` must update from confirmed fill or reconciliation evidence only.
- Latency and API-error signals must not clear risk state automatically.
- Missing broker state, ambiguous fill state, stale data, or malformed adapter events must fail closed.

## Validation Gates

### Phase 21 Gate

GO only if:

- All Phase 21 artifacts are present and internally consistent.
- No broker-specific API assumptions are required.
- Risk and replay constraints are explicit.
- Phase 22 scoping can begin without reopening Phase 21 infrastructure decisions.

NO-GO if:

- Any subsystem boundary is ambiguous.
- Any risk-limit or live-mode behavior would change without operator approval.
- Replay compatibility requirements are incomplete.
- Documentation conflicts remain unresolved in safety-sensitive areas.

### Phase 22 Gate

GO only if:

- Operator explicitly approves implementation.
- ADR status and plan status support implementation.
- Verification commands and rollback path are accepted.
- Source edit scope is bounded.

NO-GO if:

- Implementation would require new design.
- External broker details are required but not approved.
- Determinism, risk isolation, credential handling, or replay compatibility are uncertain.

Current status: Blocked / NO-GO for Phase 22 implementation.

## Backlog

Non-blocking hardening:

- Expand malformed BBO tests for crossed quotes, zero or negative size, non-mappable prices, and recenter edges.
- Add trigger-order tests for stale or invalid BBO states.
- Add replay provenance checks for synthetic and generated replay data.
- Add benchmark-environment capture for Phase 19 comparisons.

Performance backlog:

- Explore `applyBbo` cache locality and branch behavior only after correctness gates.
- Add ignored baseline benchmark artifacts for local comparisons.
- Evaluate BBO batching only if replay order remains unchanged.
- Avoid execution or network hot-path optimization until adapter contracts are frozen.
