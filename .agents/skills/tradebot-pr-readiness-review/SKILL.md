---
name: tradebot-pr-readiness-review
description: Review whether a TradeBot branch is ready for PR or merge handoff by checking branch state, changed files, validation evidence, documentation consistency, residual risk, rollback notes, and Phase 22 or live-authorization safety.
---
# tradebot-pr-readiness-review

## Purpose

Determine whether a branch has enough evidence, scope control, and risk clarity for PR or merge handoff.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use before PR creation, merge handoff, commit-readiness claims, review transfer, or final multi-skill workflow report.

## Must Not Be Used

Do not use as a substitute for code review, risk review, replay validation, L2 review, or build/test verification. Do not approve readiness if Phase 22 is implied active without explicit operator GO.

## Required Inputs

- Branch name and commit.
- Changed files and intended scope.
- Validation commands and results.
- Documentation and risk-review evidence.
- Rollback notes.

## Required Outputs

- Ready/not-ready status.
- Changed-file scope summary.
- Validation evidence summary.
- Residual risk and rollback summary.

## Required Inspection

Run:

```sh
git status --short
git status --short --ignored
git branch --show-current
git rev-parse --short HEAD
git diff --stat
git diff --name-status
git diff --cached --name-status
git diff --check
```

Read relevant docs, active plan, ADRs, and review outputs for changed areas.

## Procedure

1. Confirm the branch is not `main`.
2. Check worktree and staging status.
3. Confirm changed files match the stated scope.
4. Verify required validation evidence exists and covers the changed behavior.
5. Confirm documentation and indexes are consistent.
6. Confirm residual risk, rollback path, and required approvals are stated.
7. Fail readiness if Phase 22, live trading, broker-specific behavior, or risk changes are implied without approval.

## Validation Checklist

- Branch is not `main`.
- No staged or unstaged surprises.
- Expected files only.
- `git diff --check` passes.
- Required tests or docs audits are reported.
- Risk summary and rollback notes are concise and specific.

## Failure Modes Caught

- Dirty or wrong branch.
- Missing validation evidence.
- Untracked generated artifacts confusing review.
- Docs/index drift.
- Phase 22 or live authorization implied by wording.
- PR readiness claimed before required specialist review.

## Hard Prohibitions

- Do not stage, commit, push, merge, reset, clean, or discard changes.
- Do not approve readiness for source changes without relevant build/test evidence.
- Do not approve readiness if secrets, credentials, live trading, or real orders are ambiguous.
- Do not treat this skill as implementation authorization.

## Interaction With Existing Skills

- Run after relevant code, risk, replay, L2, performance, benchmark, ADR, and documentation reviews.
- Use `tradebot-git-safety` and `tradebot-repo-hygiene` first for worktree safety.
- Use `tradebot-authority-state-audit` and `tradebot-phase-gate-audit` for phase-sensitive readiness.
- Finish with `tradebot-handoff` when another actor resumes.

## Example Invocation Prompt

```text
Use $tradebot-pr-readiness-review to decide whether this TradeBot docs branch is ready for PR handoff.
```

## Stop Conditions

Stop if branch state is unsafe, validation is missing, changed files exceed scope, Phase 22 is implied active, live authorization is ambiguous, or rollback is not documented.

## Reporting Format

```markdown
## PR Readiness Review
- Branch:
- Commit:
- Changed files:
- Validation evidence:
- Documentation consistency:
- Residual risk:
- Rollback:
- Status:
```

## Authority Documents

- `AGENTS.md`
- `docs/RELEASE_POLICY.md`
- `docs/WORKFLOW.md`
- `docs/REVIEW_CHECKLIST.md`
- `docs/HANDOFF.md`
