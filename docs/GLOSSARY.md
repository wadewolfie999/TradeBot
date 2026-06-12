# TradeBot Glossary

## Purpose And Authority

- Purpose: define shared terms used across TradeBot governance and implementation docs.
- Authority level: terminology reference below policy documents.
- Audience: all TradeBot actors.

- Actor: a human or AI participant with repository-related responsibilities.
- ADR: architecture decision record in `docs/decisions/`.
- Backtest: deterministic historical or local replay evaluation, usually `BACKTEST`.
- BBO: best bid and offer.
- Benchmark: performance measurement, not a correctness test by itself.
- Codex: repository-side inspection and implementation agent governed by `AGENTS.md`.
- Dry-run: validation that records or simulates actions without real orders.
- Generated output: file produced by build, test, benchmark, replay, or analytics, usually ignored.
- Handoff: structured state transfer between sessions or actors.
- Historical data: market data obtained from an external or archived source.
- Live trading: real venue/account operation capable of real orders or account mutation.
- Paper trading: simulated execution in a live-like environment; verified code mode `PAPER`.
- Phase: roadmap segment with scope and validation gates.
- Plan: approved implementation specification using `PLANS.md`.
- Replay: deterministic local processing of CSV or binary market events.
- Sandbox: external test venue or broker environment; not a verified `SystemMode`.
- Simulation: offline non-live execution.
- Skill: reusable Codex workflow under `.agents/skills/`.
- Source truth: tracked source, docs, tests, ADRs, and approved configuration, not generated outputs.
- Worktree: local Git working directory state.
