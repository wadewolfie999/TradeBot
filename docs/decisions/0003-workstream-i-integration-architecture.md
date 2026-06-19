# ADR 0003: Workstream I Broker-Neutral Integration Architecture

## Status

Accepted

## Context

TradeBot has completed Phase 18/19 continuity work around local replay, L2 order-book behavior, trigger-order BBO inputs, and `applyBbo` validation. The repository now needs a Phase 21 planning layer before any Phase 22 implementation work connects the deterministic core to future broker or exchange interfaces.

Verified repository boundaries already exist:

- `SystemConfig` defines `BACKTEST`, `PAPER`, and `LIVE` runtime modes.
- `ExecutionEngine` owns signal execution and trigger-order execution behavior.
- `RiskEngine` owns can-trade checks, close-only state, halted state, latency and API-error circuit breakers, and broker-position synchronization.
- `BrokerGateway` is the broker boundary for submission, cancellation, fill confirmation, paper simulation, and reconciliation.
- `LiveDataAdapter`, `AsyncNetworkClient`, and `AuthManager` form live-capable market-data, network, and credential boundaries.
- `LocalDataReplayAdapter` and `L2OrderBook` support deterministic replay and BBO behavior.

The repository is financial-sensitive and live-capable. Live trading remains prohibited unless explicitly authorized by the operator through existing risk and live-readiness gates.

## Decision

Adopt a broker-neutral Workstream I integration architecture for Phase 21 and future Phase 22 work:

- Future broker or exchange adapters must attach below `BrokerGateway`.
- Strategy, allocation, replay, L2, analytics, and portfolio code must not depend on broker-specific schemas.
- New exposure must continue to pass through `ExecutionEngine` and `RiskEngine` before broker-boundary submission.
- `RiskEngine` remains the authority for can-trade, close-only, halted, latency, API-error, and broker-position synchronization behavior.
- `BACKTEST` remains deterministic and independent of external network, broker, credential, or wall-clock state.
- Broker-like tests must use deterministic fixtures or replay events with explicit provenance.
- Phase 22 implementation may not begin until Phase 21 artifacts are approved and scope is frozen.

## Alternatives Considered

- Direct broker integration in `ExecutionEngine`: rejected because it would couple execution logic to external API details and increase risk of bypassing broker-boundary controls.
- Strategy-level broker access: rejected because strategies must remain signal producers and must not directly create live-capable order side effects.
- Broker-specific contract first: rejected because no approved external broker API or venue-specific design exists.
- Keep current behavior without a Phase 21 plan: rejected because Phase 22 would require design decisions during implementation, increasing financial and determinism risk.

## Consequences

Benefits:

- Preserves deterministic replay and backtest behavior.
- Keeps live-capable side effects behind existing boundaries.
- Makes Phase 22 implementation reviewable against explicit contracts.
- Reduces risk of broker-specific API leakage into core logic.
- Clarifies GO / NO-GO criteria before implementation.

Costs:

- Requires additional documentation and operator review before code work.
- Defers implementation until architecture and contracts are accepted.
- May require later ADR updates if a specific broker integration introduces durable constraints.

Risks:

- Documentation can become stale if Phase 22 deviates from Phase 21.
- Broker-specific requirements may later conflict with neutral contracts.
- Overly broad adapter abstractions could hide important venue behavior if not reviewed during a future broker-specific plan.

## Validation

Phase 21 validation:

- Workstream I integration architecture document exists.
- Adapter contract document exists.
- Risk matrix exists.
- Replay compatibility checklist exists.
- Documentation indexes reference new artifacts.
- Operator accepted ADR 0003 and approved the Phase 21 Workstream I artifacts on 2026-06-19.
- `git diff --check` passes.

Future Phase 22 validation:

- `cmake -S . -B build`
- `cmake --build build`
- `ctest --test-dir build -R phase18_tests --output-on-failure`
- `ctest --test-dir build --output-on-failure`
- Performance benchmarks only after correctness checks when performance is claimed.

## Reversal Conditions

This decision should be reversed or superseded if:

- The operator selects a specific broker or exchange whose required architecture cannot fit below `BrokerGateway`.
- A future accepted ADR changes runtime mode semantics or broker-boundary ownership.
- Deterministic replay requirements require a different event-contract model.
- Risk review finds that the neutral adapter model hides safety-critical broker behavior.

## Supersession

This ADR does not supersede an existing ADR.
