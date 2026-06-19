---
name: tradebot-git-safety
description: Provide safe Git-state governance for TradeBot before edits, reviews, commits, branch creation, or handoffs. Use when Codex needs to inspect branch and worktree state, classify changed files, detect unsafe Git conditions such as a dirty `main` branch, recommend or create a safe local docs/feature branch, run `git diff --check` after edits, or report the next safe Git action without discarding changes.
---

# tradebot-git-safety

## Purpose

Inspect TradeBot Git state before sensitive repository actions. Preserve all local work, detect unsafe states early, and report the next safe Git action without staging, committing, rewriting history, or discarding changes.

## Must Not Be Used

- Do not expose secret values or inspect credential contents.
- Do not discard changes.
- Do not stage or commit files unless the operator explicitly instructs that exact action.
- Do not force-push, hard reset, run destructive clean, rewrite history, delete branches, or push to a remote without explicit operator approval.

## Required Inputs

- Task scope.
- Whether the task is inspection-only or includes local branch creation.
- Any explicit operator approval for staging, commit, push, cleanup, or history changes.

## Required Inspection

Run:

```sh
git branch --show-current
git rev-parse HEAD
git status --short
git status --short --ignored
git diff --stat
git diff --name-status
git diff --cached --name-status
```

Run `git diff --check` after edits when applicable.

## Classification Rules

Classify each changed path into exactly one bucket:

- `source`: `include/`, `src/`, `CMakeLists.txt`, `cmake/`, and other build-definition files.
- `docs`: `docs/`, `AGENTS.md`, `PLANS.md`, `CONTRIBUTING.md`, Markdown governance files, and `.agents/skills/`.
- `tests`: `tests/` and test-only fixtures.
- `build artifacts`: `build/`, compiler outputs, CMake intermediates, `.dSYM`, object files, and local binaries.
- `generated files`: `data/results/`, `data/archive/`, replay outputs, benchmark outputs, rendered exports, and other reproducible machine-generated artifacts.
- `credentials`: `.env`, key files, certificate bundles, secret stores, or paths containing `secret`, `token`, `key`, or `credential`.
- `unknown`: anything not safely covered above.

Report mixed classifications explicitly, especially when docs and source changes are interleaved.

## Unsafe States

Treat these as unsafe and report them before any Git mutation:

- Dirty worktree on `main`.
- Detached `HEAD`.
- Unknown-file changes.
- Credential-like paths.
- Mixed staged and unstaged changes when the task expects a clean review boundary.
- Large ignored/generated artifacts that may confuse review or handoff.
- Requested remote push, branch deletion, cleanup, or history rewrite without operator approval.

## Branching Rules

1. Inspect first. Do not create a branch blindly.
2. If the worktree is dirty on `main`, recommend moving work to a scoped local branch before more edits.
3. Use descriptive branch names that match scope, for example `docs/<topic>`, `fix/<topic>`, `review/<topic>`, or `phase/<topic>`.
4. Create a local branch only when the operator asked for branch creation or the task clearly authorizes it.
5. When creating a branch that must preserve local changes, use:

```sh
git switch -c <branch-name>
```

6. If the target branch name already exists, stop and report the collision. Do not delete or overwrite any branch.
7. Never use stash juggling, reset, checkout of paths, or clean commands to "move" work.

## Procedure

1. Record branch, `HEAD`, status, ignored artifacts, and diff summary.
2. Classify every changed path.
3. Identify whether the task is docs-only, source-affecting, test-affecting, or mixed.
4. Check for unsafe states.
5. If branching is needed, recommend the safest branch name. Create it only when authorized.
6. After edits, run `git diff --check` when there are tracked changes.
7. Report branch, repository state, files changed, commands run, risks, and next safe action.

## Allowed Mutations

- Local branch creation with `git switch -c <branch-name>` when the task explicitly requests it or clearly authorizes it.
- No other Git mutation is allowed by this skill without explicit operator approval.

## Prohibited Mutations

- `git push`
- `git push --force` or any force-push variant
- `git reset --hard`
- `git clean -fd` or more destructive variants
- Rebase, amend, cherry-pick, revert, or other history-rewrite operations
- Branch deletion
- Staging or committing by default

## Usage Example

Current-case example:

- Situation: dirty worktree on `main` after Phase 21 docs completion.
- Inspection: run the required inspection commands and classify the changed files as `docs` unless evidence shows otherwise.
- Decision: flag `dirty worktree on main` as unsafe for continued work.
- Safe branch action: if the operator wants the branch created, preserve the current worktree with:

```sh
git switch -c docs/phase21-review-alignment
```

- Report: confirm that local changes were preserved, list files changed, state risks, and give the next safe action.

## Reporting Format

```markdown
## Git Safety Report
- Branch:
- HEAD:
- Repo state:
- Files changed:
- Classification:
- Commands run:
- Risks:
- Next safe action:
```

## Authority Documents

- `AGENTS.md`
- `docs/WORKFLOW.md`
- `docs/HANDOFF.md`
- `docs/RELEASE_POLICY.md`
