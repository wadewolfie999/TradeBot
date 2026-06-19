---
name: tradebot-network-live-boundary-review
description: Review TradeBot network, auth, credential, live-capable, PAPER, LIVE, broker, and external I/O boundaries. Use for AsyncNetworkClient, AuthManager, LiveDataAdapter, BrokerGateway, SystemConfig, external connectivity, logs, env vars, or mode-semantics review.
---
# tradebot-network-live-boundary-review

## Purpose

Protect network, credential, mode, broker, and live-capable boundaries from unsafe defaults or ambiguous external side effects.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use when touching or reviewing `AsyncNetworkClient`, `AuthManager`, `LiveDataAdapter`, `BrokerGateway`, `SystemConfig`, environment variables, logs, external I/O, PAPER/LIVE semantics, broker integration, or live-readiness claims.

## Must Not Be Used

Do not use to authorize live trading, inspect secret values, or run external exchange operations.

## Required Inputs

- Changed files or proposed design.
- Runtime modes affected.
- Credential or env-var names involved.
- External I/O or log behavior.
- Operator approval evidence if any live-capable action is requested.

## Required Outputs

- Mode-boundary finding.
- Network and external-I/O finding.
- Credential and log-safety finding.
- Live-authorization status.

## Required Inspection

Read:

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/CONFIGURATION.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- Affected source/tests when scoped

Run:

```sh
git status --short
git diff --name-status
```

## Mode Rules

- `BACKTEST` performs no external network calls.
- `PAPER` is simulated unless explicitly approved otherwise.
- `LIVE` is prohibited without exact operator approval.
- Live-capable code is not live-enabled behavior.

## Procedure

1. Identify each network, auth, credential, log, broker, and external I/O boundary.
2. Confirm default mode remains non-live.
3. Confirm secrets are never printed, copied, committed, or documented as values.
4. Confirm PAPER behavior remains simulated unless separately approved.
5. Confirm tests do not require real venue, account, or endpoint state.
6. Reject default-live, real-order, or secret-logging ambiguity.

## Validation Checklist

- No secret values are exposed.
- Env-var names may be documented; values must not be.
- `BACKTEST` remains deterministic and offline.
- `PAPER` is not real venue execution by default.
- `LIVE` requires exact operator approval for venue, account, scope, branch, commit, config, and time window.
- Logs redact or avoid sensitive values.

## Failure Modes Caught

- Default-live or real-order ambiguity.
- Secret logging or credential fixture leakage.
- External network dependency in deterministic tests.
- PAPER treated as sandbox without approval.
- Broker connection added outside approved boundary.

## Hard Prohibitions

- Do not expose, print, copy, or modify secrets.
- Do not edit `.env`, keys, certificates, credentials, or account IDs.
- Do not run live services, real orders, or exchange operations.
- Do not implement broker-specific behavior without approval.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Use with `tradebot-risk-review` for credential, network, live-capable, or broker risk.
- Use with `tradebot-integration-architecture-review` for Workstream I adapter boundaries.
- Use with `tradebot-execution-pipeline-validation` for order lifecycle paths touching broker feedback.
- Feed findings into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-network-live-boundary-review to verify a LiveDataAdapter change does not make BACKTEST depend on network state.
```

## Stop Conditions

Stop if secret exposure, live authorization, external side effects, PAPER semantics, or mode defaults are ambiguous.

## Reporting Format

```markdown
## Network/Live Boundary Review
- Modes affected:
- External I/O:
- Credential handling:
- Log safety:
- Live authorization:
- Findings:
- Status:
```

## Authority Documents

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/CONFIGURATION.md`
