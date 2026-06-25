---
name: tradebot-phase-gate-audit
description: Audit TradeBot phase and workstream transitions, entry and exit criteria, approval status, GO or NO-GO state, and unauthorized implementation risk. Use before Workstream II/Phase 23 scoping, phase transition claims, roadmap updates, implementation planning, or review handoffs.
---
# tradebot-phase-gate-audit

## Purpose

Protect TradeBot phase transitions from authority drift, implicit approvals, and premature implementation.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Workstream II/Phase 23 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Phase 22 is Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`. Phase 23/Workstream II remains Not Started, and broker-dependent implementation remains Blocked / NO-GO unless separately approved by the operator.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use before Workstream II/Phase 23 scoping, any phase status change, roadmap update, implementation plan, or review that depends on GO/NO-GO evidence.

## Must Not Be Used

Do not use to self-approve a phase transition or substitute for operator GO. Do not treat planning approval as implementation approval.

## Required Inputs

- Phase or workstream being reviewed.
- Claimed status and approval evidence.
- Related plan, ADR, roadmap, and project-state references.
- Proposed next action.

## Required Outputs

- Verified phase/workstream status.
- GO/NO-GO evidence summary.
- Missing approval list.
- Next safe action.

## Required Inspection

Read:

- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- Relevant ADRs
- Relevant Workstream I documents
- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md` when live-capable paths are involved

Run:

```sh
git status --short
git branch --show-current
git rev-parse --short HEAD
```

## Distinctions To Enforce

- Complete is not automatically approved.
- Proposed is not accepted.
- Planning is not implementation.
- Blocked is not authorized.
- Live-capable is not live-enabled.
- Accepted ADR does not authorize source implementation.
- Approved Phase 22 closure does not start Workstream II/Phase 23.

## Procedure

1. Identify the phase, workstream, and proposed transition.
2. Verify entry and exit criteria from roadmap, plans, ADRs, and operator decision.
3. Require explicit GO/NO-GO evidence before any transition.
4. Classify the proposed next action as documentation, scoping, implementation, live-capable, or live.
5. Protect Workstream II/Phase 23 from implicit activation.
6. Report missing approvals, unresolved gates, and required verification.

## Validation Checklist

- Phase 21 is Complete - Approved only as planning/governance.
- ADR 0003 is Accepted but not implementation authorization.
- Phase 22 is Complete — Accepted; Phase 23/Workstream II remains Not Started until explicit operator GO.
- Broker-specific implementation is absent unless separately approved.
- Live trading remains disabled unless exact operator approval exists.

## Failure Modes Caught

- Premature phase transition.
- Roadmap or plan status drift.
- ADR acceptance confused with implementation approval.
- Live-capable code treated as live authorization.
- Broker-specific assumptions added during scoping.

## Hard Prohibitions

- Do not implement Phase 23/Workstream II or broker-dependent integration.
- Do not alter risk limits, credentials, live mode, order routing, or broker code.
- Do not downgrade blocked status without operator GO.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-authority-state-audit` for phase-sensitive work.
- Run before `tradebot-plan-authoring` for Workstream II/Phase 23 planning.
- Pair with `tradebot-integration-architecture-review` and `tradebot-risk-review` for Workstream I scoping.
- Feed final gate state into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-phase-gate-audit to verify whether Workstream II/Phase 23 scoping is allowed and whether broker-dependent implementation remains NO-GO.
```

## Stop Conditions

Stop if GO/NO-GO evidence is missing, contradictory, or implies source, broker, credential, risk, or live behavior outside explicit operator approval.

## Reporting Format

```markdown
## Phase-Gate Audit
- Phase/workstream:
- Claimed status:
- Verified status:
- GO/NO-GO evidence:
- Missing approvals:
- Blockers:
- Next safe action:
```

## Authority Documents

- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- `docs/decisions/`
- `docs/RISK_POLICY.md`
