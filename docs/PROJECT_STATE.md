# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state summary for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap; Workstreams I-III phase definitions and statuses are delegated to `ROADMAP.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-06-19 in `/Users/vaheedgorgeen/TradeBot`.
- Last CMake/CTest verification evidence: 2026-06-06.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Branch, HEAD, and worktree values are runtime metadata. Inspect Git directly; historical snapshots are not current-state authority.
- The merged tree contains Phase 19 revalidation history, infrastructure validation, approved Workstream I artifacts, accepted ADR 0003, and the TradeBot skill-system expansion.
- Ignored artifacts observed: `build/`, `data/`, and `src/.DS_Store`.

## Current Roadmap State

`ROADMAP.md` is the deterministic authority for Workstreams I-III. Current summary:

- Phase 21: Complete — Approved; ADR 0003 is Accepted.
- Phase 22: Blocked / NO-GO; scoping is the only permitted next activity and implementation is unauthorized.
- Phase 23: Not Started; no broker is selected.
- Phase 24: Blocked pending Phase 23 selection and operator-approved connection scope.
- Phase 25: Not Started; no documentation platform is selected.
- Phase 26: Blocked pending Phase 25 selection and operator-approved documentation architecture.
- Live trading: disabled and unauthorized.

## Completed Work Verified In Current Tree

- C++20 core library and `tradebot_core` executable are configured in `CMakeLists.txt`.
- Tests exist for phases 13, 15, 16, 17, and 18.
- Benchmarks exist for `throughput_bench`, `phase18_burnin`, and `apply_bbo_microbench`.
- Existing accepted ADRs:
  - ADR 0001: deprecate offline MOP/MOR/SS workflow.
  - ADR 0002: GitHub as long-term system of record after local cleanup is committed and pushed.
  - ADR 0003: Workstream I broker-neutral integration architecture.
- Runtime modes verified in `SystemConfig`: `BACKTEST`, `PAPER`, `LIVE`; default is `BACKTEST`.
- Credential loading verified through `AuthManager` and `SystemConfig` env names `AIIO_API_KEY` and `AIIO_API_SECRET`.

## In-Progress Work

- Repository governance and Codex skill-system maintenance.

## Blocked Or Constrained Work

- Phase 22 source implementation is Blocked / NO-GO until the operator separately approves bounded scope, verification strategy, rollback path, and any broker-specific assumptions.
- Phase 24 is blocked until Phase 23 selects a broker and the operator approves connection scope.
- Phase 26 is blocked until Phase 25 selects a documentation platform and the operator approves documentation architecture.
- GitHub-dependent sync remains constrained by intermittent or costly global connectivity.
- Ubuntu compute-node commands are not verified in this workspace.
- Markdown link-checking is unavailable locally because no link-check tool was found.
- Static-analysis and formatter commands are unconfigured locally.

## Known Defects And Warnings

- `cmake --build build` completed but emitted existing warnings:
  - `src/AsyncNetworkClient.cpp`: unused `SSL_ERROR_NONE`.
  - `src/RiskEngine.cpp`: `totalPositioned` set but not used.
- No failing tests were observed during local verification.
- `data/.DS_Store` and `src/.DS_Store` exist as ignored local artifacts.

## Documentation System Status

- Root `AGENTS.md`, `PLANS.md`, and `CONTRIBUTING.md` are present locally.
- Dedicated testing, data, security, actor, workflow, handoff, benchmarking, dependency, configuration, style, failure-recovery, live-readiness, glossary, review, release, and Workstream I documents are present locally.
- `.agents/skills/` TradeBot skill files are present locally, including `tradebot-git-safety`.
- `ROADMAP.md` is the canonical Workstreams I-III roadmap authority; this document summarizes its current state.

## Verification Evidence

Verified locally on 2026-06-06:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase18_tests --output-on-failure
build/apply_bbo_microbench 10000
```

Results:

- Configure succeeded.
- Build succeeded with the two warnings listed above.
- Full CTest suite passed: 5 tests, 0 failed.
- Targeted `phase18_tests` passed.
- `apply_bbo_microbench 10000` completed and emitted finite latency/throughput metrics.

## Operating Constraints

- Default operation remains offline-first and low-bandwidth-conscious.
- GitHub is the long-term system of record after local work is committed and pushed; inspect Git directly for current branch and commit state.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Prepare Phase 22 scoping only. Do not implement Phase 22, assume a Workstream II broker selection, assume a Workstream III documentation-platform selection, change credentials, enable live trading, or alter risk defaults without separate operator approval.

## Next Professional Halting Point

Stop after roadmap authority is synchronized and cross-document audit results are reported. Phase 22 remains explicitly Blocked / NO-GO. Do not skip phases, alter source code, delete generated artifacts, or perform live-capable operations without explicit operator approval.
