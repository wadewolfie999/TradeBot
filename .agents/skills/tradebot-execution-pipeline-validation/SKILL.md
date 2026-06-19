---
name: tradebot-execution-pipeline-validation
description: Validate the TradeBot order lifecycle through strategy, allocation, ExecutionEngine, RiskEngine, BrokerGateway, and adapter boundaries. Use for execution, order lifecycle, fills, cancellations, reconciliation, halt, close-only, broker feedback, or risk-bypass review.
---
# tradebot-execution-pipeline-validation

## Purpose

Validate execution and order lifecycle behavior across strategy, allocation, execution, risk, broker, and adapter boundaries.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use when touching or reviewing order submission, acknowledgements, rejects, fills, partial fills, cancellations, reconciliation, halt, close-only, broker feedback, portfolio sync, or risk-gate behavior.

## Must Not Be Used

Do not use to approve live trading, real orders, risk-limit changes, credential work, or broker-specific implementation.

## Required Inputs

- Changed files or proposed lifecycle design.
- Claimed order-state behavior.
- Relevant tests or missing test evidence.
- Whether source implementation is explicitly authorized.

## Required Outputs

- Canonical lifecycle trace.
- Risk-gate and broker-boundary findings.
- Missing lifecycle tests.
- Required fixes or stop condition.

## Required Inspection

Read:

- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/RISK_POLICY.md`
- `docs/ARCHITECTURE.md`
- Affected execution, risk, broker, portfolio, and trigger-order source/tests when scoped

Run:

```sh
git status --short
git diff --name-status
```

## Canonical Path

Validate this path:

```text
strategy signal -> allocation -> ExecutionEngine -> RiskEngine gate -> BrokerGateway boundary -> adapter
```

Broker feedback must return through gateway-owned events before portfolio or risk state changes.

## Procedure

1. Trace the order lifecycle from signal to broker boundary and back.
2. Verify `RiskEngine` gates new exposure before submission.
3. Check ack, reject, fill, partial-fill, cancel, reconcile, halt, and close-only flows.
4. Require confirmed fills or reconciliation before position sync.
5. Reject portfolio or risk mutation from unconfirmed broker events.
6. Identify missing tests for each touched lifecycle state.

## Validation Checklist

- Acknowledgement does not imply fill.
- Rejected order does not mutate portfolio as filled.
- Partial fill applies only filled quantity.
- Unknown cancel state requires reconciliation.
- Halt and close-only block risk-increasing exposure.
- Duplicate or stale broker events cannot double-apply state.

## Failure Modes Caught

- Direct strategy-to-broker path.
- `RiskEngine` bypass.
- Portfolio updates from local intent instead of confirmed fill.
- Reconciliation mismatch hidden from review.
- Paper/live mode confusion in execution path.

## Hard Prohibitions

- Do not enable real orders or live execution.
- Do not weaken risk limits, halt, close-only, drawdown, or position controls.
- Do not add broker-specific behavior without approved plan evidence.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Pair with `tradebot-risk-review` for financial-sensitive order behavior.
- Pair with `tradebot-integration-architecture-review` for Workstream I boundaries.
- Pair with `tradebot-network-live-boundary-review` for broker, PAPER, LIVE, auth, or network paths.
- Use `tradebot-cpp-build-test` for implementation verification when source changes are authorized.

## Example Invocation Prompt

```text
Use $tradebot-execution-pipeline-validation to review whether partial fills update positions only from confirmed broker events.
```

## Stop Conditions

Stop if order state, risk gate, fill confirmation, reconciliation, halt, close-only, or live authorization cannot be established from evidence.

## Reporting Format

```markdown
## Execution Pipeline Validation
- Files inspected:
- Lifecycle path:
- Risk gate:
- Broker feedback:
- Missing tests:
- Findings:
- Status:
```

## Authority Documents

- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/RISK_POLICY.md`
- `docs/ARCHITECTURE.md`
- `docs/TESTING.md`
