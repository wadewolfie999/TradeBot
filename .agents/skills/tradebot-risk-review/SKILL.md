---
name: tradebot-risk-review
description: Review TradeBot changes for financial, execution, data, credential, live-trading, reproducibility, and AI-agent risks. Use for changes involving SystemConfig, RiskEngine, ExecutionEngine, BrokerGateway, LiveDataAdapter, AuthManager, order sizing, risk limits, credentials, market data, replay semantics, or live-capable behavior.
---
# tradebot-risk-review

## Purpose

Review TradeBot changes for financial, execution, data, credential, live-trading, reproducibility, and AI-agent risks.

## Activation Conditions

Use for changes involving `SystemConfig`, `RiskEngine`, `ExecutionEngine`, `BrokerGateway`, `LiveDataAdapter`, `AuthManager`, order sizing, risk limits, credentials, market data, replay semantics, or live-capable behavior.

## Must Not Be Used

Do not use to authorize live trading. Only the operator can authorize live transitions.

## Required Inputs

- Diff or files to review.
- Claimed behavior.
- Verification evidence.
- Whether live-capable paths are affected.

## Required Repository Inspection

Read:

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/DATA_POLICY.md`
- Affected source/tests.

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Classify risk: financial, credential, market-data, replay, execution, dependency, reproducibility, or AI-agent.
2. Check default mode remains non-live.
3. Check risk gates and kill-switch behavior are not weakened.
4. Check secrets are not exposed.
5. Check data provenance and timestamp assumptions.
6. Check tests cover affected safety behavior.
7. Identify required operator approvals.

## Allowed Mutations

Risk review is inspection/reporting only unless separately assigned documentation updates.

## Prohibited Mutations

No source edits, credential edits, live operations, commits, pushes, or financial-limit changes.

## Verification Commands

Use relevant commands from `docs/TESTING.md`; for review-only work, repository inspection may be sufficient.

## Expected Outputs

- Findings ordered by severity.
- Required approvals.
- Missing tests.
- Live-readiness status if applicable.
- Residual risks.

## Failure Behavior

If live-capable safety cannot be established, state that live use is not authorized and identify missing evidence.

## Reporting Format

```markdown
## Risk Review
- Risk class:
- Findings:
- Required tests:
- Required approvals:
- Live authorization status:
- Residual risk:
```

## Authority Documents

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/DATA_POLICY.md`
