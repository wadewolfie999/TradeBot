# TradeBot Release Policy

## Purpose And Authority

- Purpose: govern commits, pushes, releases, publication, and live-transition gates.
- Authority level: release governance below operator instruction, `AGENTS.md`, ADRs, risk, and architecture.
- Audience: operator, maintainers, Codex, reviewers, and release preparers.

## Local Commit Gate

Before a commit:

- Worktree reviewed.
- Scope matches task or plan.
- Unrelated files excluded.
- `git diff --check` passes.
- Relevant tests pass or skipped checks are justified.
- Documentation updates are included.
- Operator explicitly authorizes Codex to commit.

## Push Gate

Before push:

- Local commit history reviewed.
- Remote state considered.
- Connectivity window available.
- No secrets or generated artifacts staged.
- Operator explicitly authorizes push.

## Release Gate

Before release:

- Release scope documented.
- Full tests pass.
- Known warnings and risks documented.
- ADRs and docs current.
- Data and generated outputs reviewed.
- Security review complete.
- Operator approves release.

## Live Transition Gate

Any live transition is governed by `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`. A software release is not live-trading authorization.

## Rollback

Rollback path must be identified for source, docs, configuration, dependencies, and live-capable behavior. Destructive Git operations require explicit operator approval.
