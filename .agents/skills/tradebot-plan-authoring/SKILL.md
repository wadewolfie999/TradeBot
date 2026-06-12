---
name: tradebot-plan-authoring
description: Draft, review, or update implementation plans using the TradeBot planning system. Use before nontrivial source changes, architecture changes, financial-sensitive work, data schema changes, dependency changes, performance work, or multi-session work.
---
# tradebot-plan-authoring

## Purpose

Draft, review, or update implementation plans using the TradeBot planning system.

## Activation Conditions

Use before nontrivial source changes, architecture changes, financial-sensitive work, data schema changes, dependency changes, performance work, or multi-session work.

## Must Not Be Used

Do not use to bypass operator approval. Do not create plans for trivial direct patches where `PLANS.md` says a direct patch is acceptable.

## Required Inputs

- Objective.
- Scope boundaries.
- Active phase or roadmap relation.
- Known constraints.
- Review authority.

## Required Repository Inspection

Read:

- `AGENTS.md`
- `PLANS.md`
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `docs/RISK_POLICY.md`
- Relevant ADRs and affected docs/source/tests.

Run:

```sh
git status --short
git branch --show-current
git log --oneline --decorate -n 5
```

## Procedure

1. Determine whether a plan is required.
2. Assign a plan ID shaped `PLAN-YYYYMMDD-short-topic`.
3. Fill every section of the `PLANS.md` schema.
4. Record assumptions and invariants.
5. Define acceptance criteria, verification, rollback, risks, and approval gates.
6. Identify documentation and ADR updates.
7. Mark state as Draft or Proposed until approved.

## Allowed Mutations

Create or update Markdown plan material when the task asks for planning or documentation. No source mutation.

## Prohibited Mutations

No source edits, live-capable configuration changes, commits, pushes, credential edits, or generated-data mutation.

## Verification Commands

```sh
git diff --check
```

For plan-only work, no CMake build is required unless plan content changes verified commands.

## Expected Outputs

- Complete plan with objective, scope, implementation steps, verification, rollback, and risks.
- Explicit approvals required.
- Clear resumption and closure rules.

## Failure Behavior

If scope or approval authority is unclear and cannot be discovered, stop and ask the operator.

## Reporting Format

```markdown
## Plan Summary
- Plan ID:
- Status:
- Objective:
- Scope:
- Out of scope:
- Verification:
- Required approvals:
- Risks:
```

## Authority Documents

- `PLANS.md`
- `docs/WORKFLOW.md`
- `docs/ROADMAP.md`
- `docs/decisions/README.md`
