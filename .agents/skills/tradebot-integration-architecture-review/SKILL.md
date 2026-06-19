---
name: tradebot-integration-architecture-review
description: Review TradeBot Workstream I and broker-exchange integration architecture for broker-neutrality, subsystem boundary integrity, deterministic replay safety, and risk isolation. Use for Phase 22 scoping, adapter architecture, BrokerGateway boundaries, or integration design review.
---
# tradebot-integration-architecture-review

## Purpose

Review Workstream I integration architecture without allowing broker-specific implementation drift or boundary bypass.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use for Workstream I, Phase 22 scoping, adapter architecture, `BrokerGateway`, `ExecutionEngine`, `RiskEngine`, `SystemConfig`, `LiveDataAdapter`, `AuthManager`, `L2OrderBook`, or replay-boundary design.

## Must Not Be Used

Do not use to authorize implementation, broker-specific APIs, credentials, external connectivity, or live trading.

## Required Inputs

- Proposed architecture or diff.
- Related phase/plan/ADR.
- Affected subsystem boundaries.
- Claimed broker-neutral behavior.

## Required Outputs

- Boundary-integrity findings.
- Broker-neutrality finding.
- Determinism and risk-isolation finding.
- Required approvals or blockers.

## Required Inspection

Read:

- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`
- ADR 0003
- `docs/ARCHITECTURE.md`
- `docs/RISK_POLICY.md`
- Affected source if implementation is already authorized

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Verify Phase 22 is scoped only if explicit operator GO exists.
2. Check broker-neutrality and reject broker-specific assumptions without plan evidence.
3. Verify strategy, allocation, replay, L2, analytics, and portfolio code stay free of broker schemas.
4. Confirm future adapters attach below `BrokerGateway`.
5. Confirm new exposure passes through `ExecutionEngine` and `RiskEngine`.
6. Confirm deterministic replay and `BACKTEST` remain independent of network, broker, credential, and wall-clock state.

## Validation Checklist

- `ExecutionEngine`, `RiskEngine`, `BrokerGateway`, `LiveDataAdapter`, `AuthManager`, `SystemConfig`, `L2OrderBook`, and replay boundaries remain intact.
- Direct strategy-to-broker coupling is rejected.
- Risk bypass is rejected.
- Live/default behavior changes are rejected.
- Workstream I remains architecture/governance unless implementation is separately authorized.

## Failure Modes Caught

- Broker schemas leaking into core logic.
- Adapter attached beside or above `BrokerGateway`.
- Risk gates bypassed by convenience paths.
- Replay tests requiring live or wall-clock state.
- Phase 22 work starting from ADR acceptance alone.

## Hard Prohibitions

- Do not implement Phase 22 without explicit operator GO.
- Do not add broker-specific API assumptions without approved plan evidence.
- Do not weaken risk controls or deterministic defaults.
- Do not expose credentials or enable live trading.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-authority-state-audit` and `tradebot-phase-gate-audit`.
- Pair with `tradebot-risk-review` for financial-sensitive architecture.
- Pair with `tradebot-execution-pipeline-validation` for order lifecycle changes.
- Pair with `tradebot-network-live-boundary-review` for network, auth, PAPER, LIVE, or broker boundaries.

## Example Invocation Prompt

```text
Use $tradebot-integration-architecture-review to verify a Phase 22 adapter design stays broker-neutral and below BrokerGateway.
```

## Stop Conditions

Stop if architecture requires broker-specific details, risk-gate changes, live/default behavior changes, replay nondeterminism, credential work, or unapproved source implementation.

## Reporting Format

```markdown
## Integration Architecture Review
- Scope:
- Boundary findings:
- Broker-neutrality:
- Replay determinism:
- Risk isolation:
- Required approvals:
- Status:
```

## Authority Documents

- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/decisions/0003-workstream-i-integration-architecture.md`
