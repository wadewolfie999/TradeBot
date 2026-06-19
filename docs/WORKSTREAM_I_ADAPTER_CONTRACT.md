# Workstream I Adapter Contract

## Purpose And Authority

- Purpose: define broker-neutral adapter contract expectations for Phase 21 infrastructure alignment.
- Authority level: planning contract below accepted ADRs, `RISK_POLICY.md`, and `ARCHITECTURE.md`; binding for Phase 22 only after operator approval.
- Audience: operator, implementers, reviewers, testers, and future adapter authors.
- Status: Phase 21 planning contract - Approved. This document does not authorize implementation, live trading, external API use, or credential changes.

This contract intentionally avoids broker-specific API shapes. Broker-specific schemas, order flags, authentication flows, and endpoint semantics require a separate approved plan.

## Contract Principles

- Preserve `BACKTEST` determinism.
- Keep all broker-facing behavior behind `BrokerGateway` or approved descendants of that boundary.
- Keep all network and credential handling outside strategy, allocation, replay, L2, analytics, and portfolio code.
- Represent adapter events as explicit lifecycle transitions.
- Fail closed when event validity, broker state, risk state, or data freshness is uncertain.
- Treat paper behavior as simulated unless an approved sandbox integration changes it.

## Conceptual Types

The following are contract concepts, not mandatory C++ type names.

### Order Request

Required meaning:

- Local order identifier.
- Symbol or instrument identifier as known to the core.
- Side.
- Quantity.
- Order intent type.
- Optional limit or stop price only when supported by approved core behavior.
- Strategy or source identifier where already available.
- Timestamp or sequence marker from deterministic input when replayed.

Constraints:

- Must not include broker-specific request fields in Phase 21.
- Must not include credentials.
- Must not bypass risk checks.

### Order Acknowledgement

Required meaning:

- Local order identifier.
- Broker order identifier when assigned.
- Accepted or rejected state.
- Rejection reason as a broker-neutral category.
- Adapter timestamp or deterministic fixture timestamp.

Constraints:

- Acknowledgement alone must not imply a fill.
- Missing broker order ID after accepted state is an adapter error unless an approved broker contract states otherwise.

### Fill Event

Required meaning:

- Local order identifier or broker order identifier.
- Filled quantity.
- Remaining quantity.
- Fill price.
- Fee and slippage values only when supported by approved execution policy.
- Full or partial fill state.

Constraints:

- Fill events must be idempotent or deduplicated by an approved key.
- Portfolio and risk state must update only from accepted fill evidence.
- Partial fills must remain visible for reconciliation.

### Cancel Request And Cancel Result

Required meaning:

- Local or broker order identifier.
- Requested cancellation action.
- Cancel accepted, rejected, already filled, already canceled, or unknown state.

Constraints:

- Unknown cancel state must not be treated as safely canceled.
- Cancel rejection must preserve reconciliation visibility.

### Reconciliation Snapshot

Required meaning:

- Open orders known to the broker boundary.
- Filled, partially filled, canceled, rejected, or unknown orders.
- Position quantities by symbol.
- Snapshot timestamp or deterministic replay marker.

Constraints:

- Reconciliation differences must be logged and reviewed before live-capable continuation.
- Risk position sync must use confirmed reconciliation evidence.

### Adapter Health Event

Required meaning:

- Connected, disconnected, degraded, stale, rate-limited, authentication failed, or unknown state.
- Latency or error signal where available.

Constraints:

- Degraded, stale, authentication-failed, or unknown states must fail closed for new exposure.
- Adapter health must not clear `RiskEngine` halt or close-only state automatically.

## Lifecycle Model

Canonical order lifecycle:

```text
created locally
  -> risk checked
  -> submitted through BrokerGateway
  -> acknowledged or rejected
  -> partially filled, filled, canceled, expired, rejected, or unknown
  -> reconciled
```

Required rules:

- Local creation does not imply broker acceptance.
- Broker acceptance does not imply execution.
- Partial fills remain open until fully filled, canceled, expired, or reconciled otherwise.
- Unknown state requires reconciliation before risk-increasing follow-up actions.
- Duplicate lifecycle events must not double-apply fills or cancellations.

## Ownership Boundaries

### ExecutionEngine

Owns:

- Signal and trigger-order execution intent.
- Local order lifecycle initiation.
- Interaction with `PortfolioManager` and `RiskEngine`.

Must not own:

- Broker-specific schemas.
- Credentials.
- Direct network calls.
- Live order side effects outside `BrokerGateway`.

### RiskEngine

Owns:

- Permission for new exposure.
- Close-only and halted states.
- Latency and API-error risk state.
- Broker-position synchronization inputs once confirmed.

Must not own:

- Broker transport details.
- Credential loading.
- Strategy signal generation.

### BrokerGateway

Owns:

- Broker-boundary submission, cancellation, fill confirmation, paper-mode simulation, and reconciliation.
- Mapping between local order IDs and broker order IDs.
- Adapter event normalization.

Must not own:

- Strategy logic.
- Risk-limit definitions.
- Credential values outside approved credential boundary.

### Future Adapter

Owns:

- Broker-specific transport and protocol conversion only after approval.
- Conversion from broker-native events into broker-neutral contract events.

Must not own:

- Trade permission.
- Portfolio accounting.
- Strategy decisions.
- Secret logging.
- Replay ordering.

## Failure Semantics

| Failure Mode | Required Behavior |
| --- | --- |
| Rejected order | Do not mutate portfolio as filled; preserve reason category. |
| Partial fill | Apply only filled quantity; keep remaining quantity visible. |
| Duplicate fill | Deduplicate or reject duplicate before state mutation. |
| Unknown cancel state | Require reconciliation; do not assume exposure is removed. |
| Network disconnect | Block new exposure unless approved paper simulation continues deterministically. |
| Authentication failure | Halt live-capable actions and avoid printing secret values. |
| Stale market data | Trigger risk/data integrity path and block new exposure. |
| Reconciliation mismatch | Fail closed and require review before continuation. |
| Latency breach | Enter or preserve close-only behavior according to existing risk policy. |
| API error-rate breach | Enter or preserve halted behavior according to existing risk policy. |

## Deterministic Simulation Requirements

Any Phase 22 simulation implementation must:

- Use deterministic fixtures or replay events.
- Avoid wall-clock-dependent order outcomes in backtest/replay.
- Preserve stable event order.
- Label synthetic or generated broker events.
- Keep generated outputs ignored unless intentionally versioned and approved.
- Include tests for rejected, partially filled, canceled, unknown, stale, duplicate, and reconciled states.

## Phase 22 Acceptance Constraints

Implementation may begin only after:

- Operator approves Phase 22 implementation scope after Phase 21 completion.
- The adapter contract is either accepted directly or incorporated into an accepted ADR.
- Tests and rollback path are defined.
- No broker-specific design choices remain open.

Implementation must halt if:

- Broker-specific fields become necessary.
- Any live-capable behavior would be enabled by default.
- Credentials or account identifiers are required.
- Risk behavior needs design changes.
- Replay compatibility cannot be proven.

Current status: Phase 21 contract approved; Phase 22 implementation remains Blocked / NO-GO.
