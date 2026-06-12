---
name: tradebot-documentation-sync
description: Keep TradeBot documentation synchronized with source behavior, architecture, risk policy, tests, roadmap, and skills. Use after behavior, architecture, risk, test command, data schema, skill, or governance edits.
---
# tradebot-documentation-sync

## Purpose

Keep TradeBot documentation synchronized with source behavior, architecture, risk policy, tests, roadmap, and skills.

## Activation Conditions

Use after behavior changes, architecture changes, risk changes, test command changes, data/schema changes, new skills, or governance edits.

## Must Not Be Used

Do not use to invent facts, hide uncertainty, or replace ADRs with ordinary documentation.

## Required Inputs

- Changed behavior or docs.
- Affected authority documents.
- Verification evidence.

## Required Repository Inspection

Read:

- `AGENTS.md`
- `docs/README.md`
- `docs/PROJECT_STATE.md`
- `docs/ARCHITECTURE.md`
- `docs/RISK_POLICY.md`
- Relevant ADRs and affected skills.

Run:

```sh
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
```

## Procedure

1. Identify authoritative source for each fact.
2. Update only documents whose authority boundary is affected.
3. Update `docs/README.md` when documents are added or removed.
4. Update `PROJECT_STATE.md` only for current state.
5. Create or update ADRs for durable decisions.
6. Run documentation audit commands.

## Allowed Mutations

Markdown docs, ADR templates/indexes, and skill docs within assigned scope.

## Prohibited Mutations

No source edits, generated-output edits, credential edits, live behavior, commits, or pushes.

## Verification Commands

```sh
git diff --check
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

## Expected Outputs

- Docs updated.
- Authority conflicts resolved or reported.
- Audit command results.
- Remaining uncertainty.

## Failure Behavior

If documents conflict on a safety-sensitive fact, stop and escalate instead of choosing silently.

## Reporting Format

```markdown
## Documentation Sync
- Documents changed:
- Authority conflicts:
- Audit results:
- Remaining uncertainty:
```

## Authority Documents

- `docs/README.md`
- `docs/decisions/README.md`
- `docs/WORKFLOW.md`
