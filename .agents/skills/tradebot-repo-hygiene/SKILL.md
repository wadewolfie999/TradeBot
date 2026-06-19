---
name: tradebot-repo-hygiene
description: Inspect repository cleanliness, generated artifacts, staged files, ignored files, and documentation-safe diff health. Use before final reports, commits, handoffs, review preparation, or when unexpected files appear.
---
# tradebot-repo-hygiene

## Purpose

Inspect repository cleanliness, generated artifacts, staged files, ignored files, and documentation-safe diff health.

## Activation Conditions

Use before final reports, commits, handoffs, review preparation, or when unexpected files appear.

## Must Not Be Used

Do not use to delete files or rewrite history without explicit operator approval.

## Required Inputs

- Task scope.
- Whether cleanup is requested or inspection-only.

## Required Repository Inspection

Run:

```sh
git status --short
git status --short --ignored
git diff --check
git diff --stat
git diff --name-status
find . -maxdepth 4 -type f -size +1M -print
```

## Procedure

1. Identify modified, deleted, untracked, and ignored files.
2. Separate source/docs changes from generated artifacts.
3. Check whitespace errors.
4. Identify large files and secret-like paths.
5. Report unrelated pre-existing changes.
6. Recommend cleanup only when safe and in scope.

## Related Skills

- Remain the first-line generated-artifact and diff-health check with `tradebot-git-safety`.
- Use before `tradebot-pr-readiness-review` and `tradebot-handoff`.
- Do not replace domain reviews such as `tradebot-risk-review`, `tradebot-market-replay-validation`, or `tradebot-benchmark-review`.

## Allowed Mutations

None unless the task explicitly authorizes cleanup. Documentation-related `.gitignore` corrections may be made only when scoped.

## Prohibited Mutations

No deletion, staging, commits, pushes, history rewrites, or generated-data cleanup without approval.

## Verification Commands

The required repository inspection commands above.

## Expected Outputs

- Git status summary.
- Diff stats.
- Ignored/generated artifacts.
- Large files.
- Hygiene risks.

## Failure Behavior

If status changes unexpectedly during work, stop and inspect before continuing.

## Reporting Format

```markdown
## Hygiene Report
- Branch:
- Changed files:
- Untracked files:
- Ignored artifacts:
- Large files:
- Diff check:
- Risks:
```

## Authority Documents

- `AGENTS.md`
- `docs/DATA_POLICY.md`
- `docs/FAILURE_RECOVERY.md`
