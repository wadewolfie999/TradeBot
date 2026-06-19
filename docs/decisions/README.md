# TradeBot Architecture Decision Records

## Purpose And Authority

- Purpose: record durable TradeBot architecture and governance decisions.
- Authority level: accepted ADRs rank below explicit operator instruction and `AGENTS.md`, and above risk, architecture, plans, state, roadmap, workflow, skills, and contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, and future agents.

This directory contains permanent ADR-style decisions. Execution history belongs in Git commits, pull requests, issues, and handoffs. The old MOP/MOR/SS workflow is deprecated by ADR 0001 and is not part of the active TradeBot workflow.

## When To Create An ADR

Create an ADR for decisions that should remain stable across sessions or actors, including:

- Architecture boundaries.
- Runtime mode semantics.
- Financial safety gates.
- Credential handling.
- Data schema or replay compatibility.
- Dependency/toolchain policy.
- Repository governance model.
- Release or live-readiness gates.

Do not create retrospective ADRs for decisions that cannot be verified.

## Status Values

- Proposed: under discussion.
- Accepted: active decision.
- Superseded: replaced by a later ADR.
- Deprecated: no longer recommended but not directly replaced.
- Rejected: considered and not adopted.

## ADR Requirements

Each ADR must include:

- Status.
- Context.
- Decision.
- Alternatives considered.
- Consequences.
- Validation.
- Reversal conditions.
- Supersession, when applicable.

Use `0000-template.md`.

## Index

- `0000-template.md`: ADR template.
- `0001-deprecate-offline-mop-mor-ss.md`: Deprecate the old offline/intranet-era MOP/MOR/SS workflow.
- `0002-github-as-system-of-record.md`: Establish GitHub as the system of record after local documentation cleanup is committed and pushed.
- `0003-workstream-i-integration-architecture.md`: Accepted broker-neutral Workstream I integration architecture.

## Conflict Handling

If an accepted ADR conflicts with another document below it in the authority order, the ADR governs until superseded. If an accepted ADR conflicts with current financial safety or operator instruction, halt and request operator resolution.
