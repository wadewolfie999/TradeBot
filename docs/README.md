# TradeBot Documentation

This directory contains the active project documentation for TradeBot.

This cleanup is local only until committed and pushed. After that, GitHub will become the system of record for active project documentation.

Current connectivity condition: development is offline-first under intermittent global access. Local Git is the immediate working record until a planned VPN/global-access sync window is available for GitHub.

## Active Documents

- `PROJECT_STATE.md` — Current project state vector and latest verified time-slice.
- `ROADMAP.md` — Project worldline and all-time strategic trajectory.
- `ARCHITECTURE.md` — System structure, module boundaries, data flow, and execution modes.
- `RISK_POLICY.md` — Dry-run/live trading policy, credentials, kill switch, backtest caveats, and financial safety.
- `decisions/` — Permanent ADR-style project decisions.

## Documentation Rules

- Keep active docs lean, cohesive, and Codex-friendly.
- Treat legacy offline/intranet-era material as historical reference only.
- Do not recreate MOP/MOR/SS workflows.
- Do not treat unverified legacy claims as current project state.
- Keep generated outputs out of Git unless intentionally versioned.
- Never document secrets, credentials, account identifiers, or live trading keys.
