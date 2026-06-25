# Workstream I Replay Compatibility Checklist

## Purpose And Authority

- Purpose: define replay and deterministic-validation requirements for Workstream I Phase 21 and future Phase 22 implementation.
- Authority level: planning checklist below `RISK_POLICY.md`, `DATA_POLICY.md`, accepted ADRs, and `ARCHITECTURE.md`.
- Audience: operator, implementers, reviewers, testers, and future replay-validation agents.
- Status: Phase 21 planning checklist - Approved. This checklist does not authorize source edits, schema changes, generated-data promotion, or live behavior.

## Compatibility Goals

- Preserve deterministic `BACKTEST` behavior.
- Preserve Phase 18 replay and L2 trigger-order continuity.
- Prevent external broker, network, wall-clock, or credential state from affecting replay.
- Make any future broker-event simulation reproducible and provenance-labeled.
- Require migration review before changing replay schemas or generated-output semantics.

## Entry Checklist For Phase 22

Before implementation starts:

- [x] Phase 21 artifacts are approved.
- [ ] Replay-affecting files and interfaces are identified.
- [ ] Existing replay timestamp units are recorded.
- [ ] Existing CSV and binary replay behavior is understood.
- [ ] Test fixtures or synthetic broker-event fixtures have stated provenance.
- [ ] Expected generated output paths are ignored or intentionally approved.
- [ ] Rollback path is documented.
- [ ] No live network or broker access is required for tests.

## Determinism Requirements

Required:

- [ ] Same replay input and configuration produce the same core decisions.
- [ ] Event ordering is determined by replay sequence or event timestamp, not wall-clock scheduling.
- [ ] Future simulated broker events are deterministic fixtures or generated from deterministic rules.
- [ ] No test requires external exchange state, real account state, or live endpoint availability.
- [ ] Randomness is absent or seeded and recorded.
- [ ] Generated files do not influence source behavior unless intentionally loaded by explicit test configuration.

Prohibited:

- [ ] Wall-clock latency changing backtest decisions.
- [ ] Network reconnect timing changing replay outcomes.
- [ ] Live adapter queue timing changing deterministic replay.
- [ ] Secret or account state appearing in replay fixtures.
- [ ] Generated replay data being treated as historical source data without provenance and approval.

## Timestamp And Ordering Checklist

- [ ] Replay tick timestamp units are explicit where used.
- [ ] Candle timestamp units are explicit where used.
- [ ] Broker-event fixture timestamp units are explicit before implementation.
- [ ] Mixed timestamp units fail tests or validation.
- [ ] Out-of-order input handling is defined before implementation.
- [ ] Duplicate event handling is defined before implementation.
- [ ] Partial-fill event ordering is deterministic.
- [ ] Cancel and fill race states require deterministic resolution or reconciliation.

## L2 And Trigger-Order Checklist

- [ ] BBO validity rules are unchanged.
- [ ] `applyBbo` correctness remains prioritized over performance.
- [ ] Trigger orders receive only valid BBO state.
- [ ] Stale or invalid BBO does not create unsafe fills.
- [ ] Recenter behavior remains covered by tests when changed.
- [ ] Phase 18 tests pass before Phase 19 or Workstream I performance claims.

## Broker-Event Simulation Checklist

Future Phase 22 broker-event simulation must prove:

- [ ] Accepted order does not imply filled order.
- [ ] Rejected order does not mutate portfolio as filled.
- [ ] Partial fill updates only filled quantity.
- [ ] Duplicate fill does not double-apply.
- [ ] Unknown cancel state blocks unsafe assumptions.
- [ ] Reconciliation mismatch is visible and fail-closed.
- [ ] Adapter health degradation blocks new exposure where required.
- [ ] Synthetic and generated fixtures are labeled.

## Data Provenance Checklist

For every replay or broker-event fixture:

- [ ] Path is recorded.
- [ ] Provenance is recorded: synthetic, generated, sampled, historical, or captured.
- [ ] Timestamp units are recorded.
- [ ] Symbol or instrument assumptions are recorded.
- [ ] Generation command or capture source is recorded when applicable.
- [ ] Generated outputs are excluded from Git unless intentionally approved.
- [ ] Historical-source claims are not made for synthetic or generated data.

## Validation Commands

Minimum expected validation for replay-affecting Phase 22 work:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -R phase18_tests --output-on-failure
ctest --test-dir build --output-on-failure
```

Additional validation for L2 performance claims:

```sh
build/apply_bbo_microbench 10000
```

Additional validation for replay burn-in when scoped:

```sh
build/phase18_burnin 10000
```

## GO / NO-GO Checklist

GO to Phase 22 implementation only if:

- [x] Phase 21 artifacts are approved.
- [x] Replay compatibility risks are documented.
- [x] Deterministic fixture strategy is defined.
- [x] Existing Phase 18 replay tests are expected to remain valid.
- [x] Any new schema has a migration and rollback plan.

NO-GO if:

- [ ] Replay event order depends on thread scheduling.
- [ ] Live network or broker state is required for deterministic tests.
- [ ] Timestamp units are ambiguous.
- [ ] Generated data provenance is missing.
- [ ] Broker-event simulation can mutate portfolio without confirmed fill evidence.
- [ ] Risk state cannot be reconstructed or validated after replay.

Current status: Complete — Accepted for the bounded broker-neutral implementation under `PLAN-20260624-workstream-i-broker-neutral-completion`. The unchecked NO-GO conditions remain active for broker-dependent, live, or future replay-affecting work; broker-dependent and live work remain unauthorized.
