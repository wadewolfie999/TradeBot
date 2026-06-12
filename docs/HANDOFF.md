# TradeBot Handoff Standard

## Purpose And Authority

- Purpose: define session and actor handoffs for interrupted or transferred work.
- Authority level: handoff workflow below `WORKFLOW.md` but required by `AGENTS.md` reporting for interrupted work.
- Audience: Codex, human implementers, maintainers, reviewers, testers, and the operator.

Use a handoff whenever work stops before completion, another actor will resume it, a plan spans sessions, verification is incomplete, or risk-sensitive context must be preserved.

## Required Fields

- Repository.
- Branch.
- Commit.
- Worktree status.
- Active plan.
- Active phase.
- Objective.
- Completed work.
- Changed files.
- Commands run.
- Test results.
- Failed checks.
- Unresolved questions.
- Risks.
- Next exact action.
- Safe stopping condition.
- Prohibited next actions.
- Required approvals.

## Handoff Rules

- Do not include secret values.
- Include exact command lines.
- Separate verified facts from assumptions.
- State whether generated outputs were created.
- State whether source, docs, data, or credentials were touched.
- Include the next exact action rather than a vague recommendation.

## Copy-Paste Handoff Template

```markdown
# TradeBot Handoff

- Repository:
- Branch:
- Commit:
- Worktree status:
- Active plan:
- Active phase:
- Date:
- Actor:

## Objective

## Completed Work

## Changed Files

## Commands Run

## Test Results

## Failed Or Skipped Checks

## Unresolved Questions

## Risks

## Next Exact Action

## Safe Stopping Condition

## Prohibited Next Actions

## Required Approvals

## Notes
```
