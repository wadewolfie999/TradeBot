---
name: tradebot-adr-review
description: Review TradeBot ADR lifecycle consistency, status, consequences, indexes, supersession, and implementation implications. Use before ADR status mutation, ADR index updates, architecture decision review, or when ADR acceptance may be confused with implementation authorization.
---
# tradebot-adr-review

## Purpose

Validate ADR status and implications without letting ADR acceptance become unauthorized implementation approval.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Workstream II/Phase 23 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Phase 22 is Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`. Phase 23/Workstream II remains Not Started, and broker-dependent implementation remains Blocked / NO-GO unless separately approved by the operator.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use before creating, accepting, superseding, deprecating, rejecting, or indexing an ADR, and before relying on an ADR for phase or implementation claims.

## Must Not Be Used

Do not use to approve source implementation, live trading, risk-limit changes, credential changes, or broker-specific work.

## Required Inputs

- ADR file or proposed ADR.
- Claimed ADR status.
- Related roadmap phase and plan.
- Proposed documentation or implementation implication.

## Required Outputs

- ADR file/index consistency result.
- Implementation-authorization finding.
- Required document updates.
- Required approval or stop condition.

## Required Inspection

Read:

- `docs/decisions/README.md`
- Relevant ADR file
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- Relevant architecture or Workstream I docs

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Verify ADR file status against the ADR index.
2. Check context, decision, consequences, validation, reversal conditions, and supersession.
3. Verify status consistency across ADR file, decisions index, roadmap, project state, and plans where applicable.
4. Identify implementation implications separately from implementation authorization.
5. Report required operator approvals and any documents that must be updated.

## Validation Checklist

- ADR file status and index status match.
- Accepted ADR is not treated as source implementation approval.
- Superseded or deprecated ADRs are clearly marked.
- Consequences include risk, testing, and rollback implications where relevant.
- Phase 22 is Complete — Accepted; Phase 23/Workstream II remains Not Started unless separate operator GO exists.

## Failure Modes Caught

- ADR file/index drift.
- Accepted ADR misread as implementation authorization.
- Missing supersession link.
- Roadmap or project-state mismatch.
- Durable decision hidden in ordinary docs.

## Hard Prohibitions

- Do not mutate ADR status without explicit operator or review-authority evidence.
- Do not authorize source, broker, credential, risk, or live changes.
- Do not implement Phase 23/Workstream II or broker-dependent integration.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-authority-state-audit` when ADR facts affect current state.
- Run before `tradebot-documentation-sync` when ADRs are touched.
- Run before `tradebot-phase-gate-audit` relies on ADR status for a transition.
- Feed unresolved ADR risks into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-adr-review to verify ADR 0003 status and confirm it does not authorize Workstream II/Phase 23 or broker-dependent implementation.
```

## Stop Conditions

Stop if ADR status, index status, supersession, operator approval, or implementation implication is contradictory or missing.

## Reporting Format

```markdown
## ADR Review
- ADR:
- File status:
- Index status:
- Related docs:
- Implementation implication:
- Required approvals:
- Findings:
```

## Authority Documents

- `docs/decisions/README.md`
- `docs/decisions/0000-template.md`
- Relevant ADRs
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
