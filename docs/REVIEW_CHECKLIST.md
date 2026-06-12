# TradeBot Review Checklist

## Purpose And Authority

- Purpose: provide a practical checklist for code, docs, risk, data, security, and benchmark reviews.
- Authority level: review aid below risk, architecture, testing, and workflow policy.
- Audience: reviewers, maintainers, Codex review agents, and operator.

## Repository State

- Branch and commit identified.
- Worktree status reviewed.
- Untracked and ignored artifacts understood.
- Scope matches task or plan.

## Source Review

- Change is focused.
- No unrelated refactor.
- Existing subsystem boundaries preserved.
- No generated outputs used as source.
- Error handling fails safely.
- Tests cover changed behavior.

## Risk Review

- No live behavior enabled by default.
- No real orders introduced.
- Risk limits not weakened without approval.
- Kill-switch or halt behavior preserved.
- Position sizing and drawdown changes approved.

## Security Review

- No credentials or secret values committed.
- `.env` untouched unless explicitly authorized.
- Logs redacted.
- Dependency changes reviewed.
- Network behavior understood.

## Data Review

- Data provenance documented.
- Generated outputs kept ignored.
- Timestamp units clear.
- Replay compatibility reviewed.
- No account/private data committed.

## Benchmark Review

- Correctness tests pass.
- Command, input size, build mode, compiler, and platform reported.
- Baseline comparison provided.
- No unsupported production or profitability claim.

## Documentation Review

- Authority docs updated where behavior changed.
- ADR added or updated for durable decisions.
- Project state remains current-state only.
- Links and indexes reviewed.

## Final Evidence

- Commands run.
- Results and warnings.
- Skipped checks and reasons.
- Remaining risks.
- Rollback path.
