---
name: tradebot-handoff
description: Produce a complete TradeBot session or actor handoff. Use when work is interrupted, a plan spans sessions, verification is incomplete, another actor will resume work, or safety-sensitive context must be preserved.
---
# tradebot-handoff

## Purpose

Produce a complete TradeBot session or actor handoff.

## Activation Conditions

Use when work is interrupted, a plan spans sessions, verification is incomplete, another actor will resume work, or safety-sensitive context must be preserved.

## Must Not Be Used

Do not use to conceal incomplete work or skipped verification. Do not include secret values.

## Required Inputs

- Objective.
- Active plan, if any.
- Changed files.
- Commands run and results.
- Known risks and next action.

## Required Repository Inspection

Run:

```sh
git status --short
git branch --show-current
git log --oneline --decorate -n 5
git diff --stat
git diff --name-status
```

Read:

- `docs/HANDOFF.md`
- `docs/PROJECT_STATE.md`
- Active plan, if any.

## Procedure

1. Capture repository, branch, commit, and worktree status.
2. Identify active phase and plan.
3. Summarize completed work and changed files.
4. List exact commands and results.
5. Identify failed/skipped checks.
6. Record risks and unresolved questions.
7. State next exact action and prohibited next actions.
8. State required approvals.

## Related Skills

- Use as the final step after multi-skill workflows, interrupted work, incomplete verification, or safety-sensitive review.
- Include findings from `tradebot-authority-state-audit`, `tradebot-phase-gate-audit`, `tradebot-risk-review`, `tradebot-pr-readiness-review`, and any relevant implementation-review skills.
- Do not replace PR readiness, risk review, replay validation, L2 review, build/test, performance review, or benchmark review.

## Allowed Mutations

Create or update handoff Markdown only when requested. Otherwise report handoff in response.

## Prohibited Mutations

No source edits, commits, pushes, cleanup, credential access, or live actions.

## Verification Commands

Repository inspection commands above.

## Expected Outputs

A handoff matching `docs/HANDOFF.md`.

## Failure Behavior

If status or diff cannot be read, state that the handoff is incomplete and why.

## Reporting Format

Use the copy-pasteable template in `docs/HANDOFF.md`.

## Authority Documents

- `docs/HANDOFF.md`
- `docs/WORKFLOW.md`
- `docs/FAILURE_RECOVERY.md`
