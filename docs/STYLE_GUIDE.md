# TradeBot Style Guide

## Purpose And Authority

- Purpose: keep source and documentation style consistent until formal tooling is added.
- Authority level: style guidance below architecture, testing, and workflow policy.
- Audience: Codex, contributors, maintainers, and reviewers.

## Current Tooling State

No repository-configured formatter or static analyzer was verified. `clang-format` and `clang-tidy` were not available locally when this guide was created.

## C++ Style

- Use C++20.
- Match nearby file style.
- Keep headers in `include/` and implementation in `src/`.
- Prefer explicit ownership and clear subsystem boundaries.
- Keep comments useful and local to non-obvious behavior.
- Avoid broad formatting churn.
- Do not mix unrelated refactors with feature or bug fixes.
- Keep generated files out of source paths.

## Naming

- Preserve existing class names and subsystem vocabulary.
- Use mode names exactly as verified in code: `BACKTEST`, `PAPER`, `LIVE`.
- Use `TradeBot` for repository identity and `TradeBot-AIIO-Core` only when referring to the CMake project name.

## Documentation Style

- State purpose, authority level, audience, and relationships for governance docs.
- Prefer verified repository facts over generic policy.
- Keep current state current; do not turn `PROJECT_STATE.md` into a diary.
- Link to deeper authority docs instead of duplicating long policy.
- Do not document secret values.
- Avoid unsupported claims of profitability, live readiness, or production readiness.

## Review Style

Reviews should lead with findings, ordered by severity, grounded in files and lines where possible. Summaries are secondary.
