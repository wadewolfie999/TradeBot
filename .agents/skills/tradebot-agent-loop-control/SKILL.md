---
name: tradebot-agent-loop-control
description: Prevent repeated TradeBot agentic loops by forcing loop-risk identification, prior-attempt review, bounded execution, invariants, and explicit stop conditions. Use when agents repeat broad rewrites, re-audit the same state, chase stale docs, or continue without new evidence.
---
# tradebot-agent-loop-control

## Purpose

Break unproductive agent loops and force one bounded, evidence-producing next action.

## Global TradeBot Rules

- Prefer correctness before speed, determinism before convenience, and risk controls before feature development.
- Resolve documentation authority before documentation sync; run `tradebot-authority-state-audit` before `tradebot-documentation-sync` when current state is uncertain.
- Run `tradebot-phase-gate-audit` before phase transitions or Phase 22 planning; run `tradebot-adr-review` before ADR status mutation; run `tradebot-pr-readiness-review` before PR or merge handoff.
- Accepted ADRs and approved Phase 21 artifacts do not authorize implementation. Phase 22 remains Blocked / NO-GO until explicit operator GO.
- Live trading remains disabled unless exact operator approval exists.
- Do not make broker-specific assumptions, destructive Git changes, or source/test changes unless a future task explicitly authorizes them.

## Activation Conditions

Use when a workflow repeats inspections or rewrites without new evidence, cycles between docs, broadens scope after failures, or cannot identify a concrete invariant.

## Must Not Be Used

Do not use to skip required verification, ignore failures, or rush through safety-sensitive ambiguity.

## Required Inputs

- Current objective.
- Prior attempts and their outcomes.
- Suspected loop pattern.
- Missing evidence or invariant.
- Proposed bounded next action.

## Required Outputs

- Named loop pattern.
- Missing invariant or evidence.
- Single bounded next action.
- Explicit stop condition and result.

## Required Inspection

Run:

```sh
git status --short
git branch --show-current
git diff --name-status
```

Read the most relevant authority doc, active plan, previous handoff, or failed validation output that explains the repeated loop.

## Procedure

1. Name the loop pattern.
2. List prior attempts and what evidence they added.
3. Identify the missing invariant or decision.
4. Choose one bounded next action that can change knowledge or state.
5. Define a concrete stop condition before taking the action.
6. Stop and report if the bounded action fails to add new evidence.

## Validation Checklist

- Broad rewrite is rejected unless new evidence justifies it.
- Documentation work starts with authority docs, not lower-order summaries.
- Only one bounded next action is active after loop detection.
- Stop condition is explicit before continuing.
- Phase 22, live, broker, credential, and risk gates remain unchanged.

## Failure Modes Caught

- Repeating audits without decision output.
- Rewriting docs to hide uncertainty.
- Chasing generated artifacts or stale indexes as source truth.
- Reattempting failed validation without a changed hypothesis.
- Expanding from governance work into implementation.

## Hard Prohibitions

- Do not repeat broad rewrites without new evidence.
- Do not modify source, tests, credentials, broker code, or generated artifacts unless explicitly authorized by a later task.
- Do not implement Phase 22 or enable live trading.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Use before re-running long multi-skill workflows after repeated failure.
- Run `tradebot-git-safety` after loop containment and before edits.
- Run `tradebot-authority-state-audit` before documentation corrections.
- End unresolved loops with `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-agent-loop-control to stop repeated TradeBot docs rewrites and choose one bounded corrective action.
```

## Stop Conditions

Stop if no bounded action can be named, if the same missing evidence remains after the action, or if progress would require unauthorized source, broker, credential, risk, or live changes.

## Reporting Format

```markdown
## Agent Loop Control
- Loop pattern:
- Prior attempts:
- Missing invariant:
- Bounded next action:
- Stop condition:
- Result:
- Next safe action:
```

## Authority Documents

- `AGENTS.md`
- `docs/WORKFLOW.md`
- `docs/HANDOFF.md`
- `PLANS.md`
