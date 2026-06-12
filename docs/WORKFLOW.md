# TradeBot Work Protocol

## Purpose And Authority

- Purpose: define the end-to-end protocol for tasks, plans, implementation, verification, review, commit preparation, handoff, and halt.
- Authority level: workflow policy below roadmap and above skill-specific instructions.
- Audience: operator, maintainers, Codex, contributors, reviewers, and testers.

## Standard Workflow

1. Task intake: capture objective, scope, risk class, and expected output.
2. State inspection: run repository status, inspect relevant docs, inspect source/tests before mutation.
3. Scope definition: identify in-scope and out-of-scope files and behavior.
4. Risk classification: normal, architecture-sensitive, data-sensitive, security-sensitive, financial-sensitive, or live-capable.
5. Planning: use `PLANS.md` when required.
6. Approval: get operator or review-authority approval for nontrivial, sensitive, live-capable, or destructive work.
7. Implementation: edit only in scope and preserve unrelated changes.
8. Verification: run targeted and broad checks appropriate to risk.
9. Review: prioritize bugs, regressions, missing evidence, safety, docs, and architecture drift.
10. Correction: fix findings or record why unresolved issues remain.
11. Commit preparation: review status, diff, and staged files; do not stage unrelated files.
12. Operator approval: required before commits, pushes, releases, live unlocks, and risk changes.
13. Push or release: perform only when explicitly instructed.
14. Documentation update: update state, architecture, risk, testing, data, ADRs, or plans as needed.
15. Handoff: provide `HANDOFF.md` fields when work continues elsewhere.
16. Professional halt: stop at a clean evidence boundary when further action needs approval, access, or new scope.

## Small Fix Workflow

- Inspect state and affected files.
- Confirm no plan is required.
- Patch narrowly.
- Run targeted verification.
- Report files changed, commands, results, and residual risk.

## Feature Work Workflow

- Draft or locate an active plan.
- Inspect architecture, risk, tests, and affected boundaries.
- Implement in increments.
- Add or update tests.
- Run full CTest when shared behavior is touched.
- Update docs and handoff.

## Phase Work Workflow

- Confirm phase marker in `ROADMAP.md` and `PROJECT_STATE.md`.
- Use a plan with phase ID.
- Preserve phase gates and validation requirements.
- Record benchmark/test evidence.
- Do not declare phase completion without operator review.

## Architecture Change Workflow

- Inspect existing architecture and ADRs.
- Draft plan and ADR if the decision is durable.
- Identify prohibited coupling and affected tests.
- Implement only after approval.
- Update `ARCHITECTURE.md`, `ROADMAP.md` if phase scope changes, and relevant skills.

## Documentation Change Workflow

- Inspect existing docs and authority order.
- Avoid duplication and invented facts.
- Update indexes.
- Run `git diff --check` and doc audit grep.
- Report skipped link-checking if no local tool exists.

## Research Experiment Workflow

- State objective, dataset, provenance, config, seeds, fees, slippage, and success criteria.
- Keep generated outputs ignored unless approved.
- Avoid production or profitability claims from backtests alone.
- Handoff reproducibility evidence.

## Financial-Sensitive Workflow

- Stop and require explicit operator approval before mutation unless the task is inspection-only.
- Inspect `RISK_POLICY.md`, `LIVE_TRADING_READINESS.md`, and relevant code.
- Create a plan.
- Verify dry-run/paper/sandbox behavior before any live consideration.
- Include kill switch, stale-data, partial-fill, and reconciliation evidence.

## Emergency Containment Workflow

- Stop new risky actions.
- Preserve evidence without exposing secrets.
- Identify branch, commit, worktree, running processes if relevant, and generated outputs.
- Notify operator.
- Do not clean up, revert, or rotate secrets without approval unless the operator has delegated that exact authority.
- Produce a handoff with prohibited next actions.

## Professional Halt Conditions

Halt when:

- Required approval is absent.
- Credentials or live trading are implicated unexpectedly.
- Tests fail in a safety-relevant way.
- Repository state changes unexpectedly and cannot be attributed.
- Evidence is insufficient for a claim.
- Scope would need expansion.
- Network or external access is required but unavailable.
