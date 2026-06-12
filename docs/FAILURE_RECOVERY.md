# TradeBot Failure Recovery

## Purpose And Authority

- Purpose: define recovery from interrupted work, failed checks, stale state, generated artifacts, and safety incidents.
- Authority level: recovery workflow below risk/security policy and above routine contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, and testers.

## Interrupted Work

1. Stop mutation.
2. Capture branch, commit, and worktree status.
3. Identify changed and untracked files.
4. Preserve generated evidence needed for review.
5. Write a handoff using `HANDOFF.md`.
6. Do not revert or delete unrelated work.

## Failed Build Or Test

1. Record the exact command and failure.
2. Identify whether failure is pre-existing or introduced by current work.
3. Avoid broad refactors while diagnosing.
4. Run targeted checks after fixes.
5. Re-run full CTest for shared behavior changes.
6. Report residual warnings and skipped checks.

## Documentation Audit Failure

- Fix broken references, stale paths, conflicting authority, or unsafe wording.
- If a link checker is unavailable, state that it was unavailable.
- Review grep hits for financial, credential, and live-trading terms rather than deleting necessary safety language.

## Stale Repository State

If branch, status, or commits differ from handoff:

- Inspect before proceeding.
- Do not assume previous scope remains valid.
- Reconcile with `PROJECT_STATE.md`, active plan, and ADRs.
- Escalate if changes affect scope or safety.

## Generated Artifact Containment

- Generated files under `build/` and `data/results/` may be ignored.
- Do not stage generated outputs unless approved.
- Do not delete generated outputs that may be needed as evidence for an active plan without approval.

## Credential Or Secret Incident

Follow `SECURITY.md`: stop propagation, notify operator, revoke/rotate, audit, and document without secret values.

## Live-Capable Incident

If live-capable behavior is unexpectedly implicated:

- Stop new order submission.
- Preserve logs without exposing secrets.
- Reconcile local and external state if authorized.
- Do not resume until the operator approves.

## Git Recovery

- Do not run destructive Git commands without operator approval.
- Prefer non-destructive inspection: `git status --short`, `git diff`, `git log`.
- Use commits, branches, and approved reverts for recovery.
