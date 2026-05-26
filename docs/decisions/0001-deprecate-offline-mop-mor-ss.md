# ADR 0001: Deprecate Offline MOP/MOR/SS Workflow

## Status

Accepted

## Decision

The old offline or intranet-era MOP/MOR/SS workflow is deprecated.

## Context

That workflow existed when GitHub or broader global access was unavailable or unreliable.

It remains historical reference only and is not an active operating model for TradeBot.

## Consequences

- Do not create new MOP, MOR, or SS artifacts.
- Keep active project docs lean and current.
- Use local Git and GitHub for traceability instead of document-based execution history.
- Revalidate legacy claims locally before promoting them into `docs/PROJECT_STATE.md`.
