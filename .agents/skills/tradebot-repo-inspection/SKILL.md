---
name: tradebot-repo-inspection
description: Inspect TradeBot repository state before planning, editing, reviewing, or handing off work. Use when starting any nontrivial TradeBot task, resuming interrupted work, reviewing repository health, or preparing a handoff.
---
# tradebot-repo-inspection

## Purpose

Inspect TradeBot repository state before planning, editing, reviewing, or handing off work.

## Activation Conditions

Use when starting any nontrivial TradeBot task, resuming interrupted work, reviewing repository health, or preparing a handoff.

## Must Not Be Used

Do not use as a substitute for implementation, testing, or live-trading approval. Do not inspect secret values.

## Required Inputs

- Task objective.
- Repository path, normally `~/TradeBot`.
- Any active plan or handoff, if provided.

## Required Repository Inspection

Run or equivalent:

```sh
pwd
git rev-parse --show-toplevel
git status --short
git status --ignored --short
git branch --show-current
git log --oneline --decorate -n 15
find . -maxdepth 3 -type f | sort
find . -maxdepth 3 -type d | sort
find docs -maxdepth 3 -type f -print | sort
find .agents -maxdepth 4 -type f -print 2>/dev/null | sort
git ls-files
```

Read relevant docs: `AGENTS.md`, `PLANS.md`, `docs/PROJECT_STATE.md`, `docs/ROADMAP.md`, `docs/ARCHITECTURE.md`, `docs/RISK_POLICY.md`, and affected source/tests.

## Procedure

1. Identify branch, commit, status, untracked files, and ignored artifacts.
2. Identify active phase and current constraints.
3. Map affected subsystem paths.
4. Check whether the task is financial-sensitive, security-sensitive, data-sensitive, architecture-sensitive, or documentation-only.
5. Report contradictions, missing evidence, and safe next action.

## Allowed Mutations

None. This skill is inspection-only.

## Prohibited Mutations

No file edits, staging, commits, pushes, generated-data cleanup, credential access, or live service starts.

## Verification Commands

Inspection commands above. No build or test is required unless the task asks for readiness evidence.

## Expected Outputs

- Repository state.
- Relevant files inspected.
- Active phase.
- Risk classification.
- Constraints and uncertainties.
- Recommended next action.

## Failure Behavior

If inspection is blocked or state is ambiguous, stop and report what evidence is missing.

## Reporting Format

```markdown
## Repo Inspection
- Branch:
- Commit:
- Status:
- Active phase:
- Relevant paths:
- Risk class:
- Constraints:
- Missing evidence:
- Next safe action:
```

## Authority Documents

- `AGENTS.md`
- `docs/PROJECT_STATE.md`
- `docs/WORKFLOW.md`
- `docs/HANDOFF.md`
