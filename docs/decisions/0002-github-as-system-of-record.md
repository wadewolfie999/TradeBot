# ADR 0002: GitHub as System of Record

## Status

Accepted

## Decision

GitHub becomes the long-term system of record after local documentation cleanup is committed and pushed.

## Context

The current environment may have intermittent or expensive global access.

Local Git is the immediate working record until a planned VPN or global-access sync window is available.

## Consequences

- Prepare commits locally in `~/TradeBot`.
- Push during planned connectivity windows.
- Do not assume remote state is current before sync.
- Keep active docs in the repository concise and current.
