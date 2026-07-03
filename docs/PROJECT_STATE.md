# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state summary for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap; the project workstream map is defined in `WORKSTREAM_ARCHITECTURE.md`, and phase definitions and statuses are delegated to `ROADMAP.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-07-03 in `/Users/vaheedgorgeen/TradeBot`.
- Last CMake/CTest verification evidence: 2026-06-25.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Branch, HEAD, and worktree values are runtime metadata. Inspect Git directly; historical snapshots are not current-state authority.
- The merged tree contains Phase 19 revalidation history, infrastructure validation, approved Workstream I artifacts, accepted ADR 0003, and the TradeBot skill-system expansion.
- Ignored artifacts observed: `build/`, `data/`, and `src/.DS_Store`.

## Current Roadmap State

`WORKSTREAM_ARCHITECTURE.md` defines the current Workstreams I-VII project map; `ROADMAP.md` is the deterministic phase authority. Current summary:

- Workstream I — Broker-Neutral Execution Foundation: Complete — Accepted through Phase 22.
- Workstream II — Broker Integration Program: next active strategic area for evidence/evaluation coordination only; not implementation-authorized.
- Workstreams III–VII: parallel/future domains unless separately activated.
- Phase 21: Complete — Approved; ADR 0003 is Accepted.
- Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness; Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`.
- Phase 23: next formal broker-selection/evaluation phase; Not Started, with bounded pre-phase strategic evidence preparation active and no broker selected.
- Phase 24: Blocked pending Phase 23 selection and operator-approved connection scope.
- Phase 25: Not Started; no documentation platform is selected.
- Phase 26: Blocked pending Phase 25 selection and operator-approved documentation architecture.
- Live trading: disabled and unauthorized.

## Completed Work Verified In Current Tree

- C++20 core library and `tradebot_core` executable are configured in `CMakeLists.txt`.
- Tests exist for phases 13, 15, 16, 17, 18, and 22.
- Benchmarks exist for `throughput_bench`, `phase18_burnin`, and `apply_bbo_microbench`.
- Existing accepted ADRs:
  - ADR 0001: deprecate offline MOP/MOR/SS workflow.
  - ADR 0002: GitHub as long-term system of record after local cleanup is committed and pushed.
  - ADR 0003: Workstream I broker-neutral integration architecture.
- Accepted Phase 22 broker-neutral implementation under `PLAN-20260624-workstream-i-broker-neutral-completion`; broker-dependent connectivity remains unauthorized.
- Runtime modes verified in `SystemConfig`: `BACKTEST`, `PAPER`, `LIVE`; default is `BACKTEST`.
- Credential loading verified through `AuthManager` and `SystemConfig` env names `AIIO_API_KEY` and `AIIO_API_SECRET`.

## In-Progress Work

- Repository governance and Codex skill-system maintenance.
- Workstream II strategic evidence preparation under Strategy 2 — Parallel Evidence Lanes With Wade Checkpoints. This is documentation/evaluation activity only.

## Blocked Or Constrained Work

- Phase 22 is closed at the accepted broker-neutral Workstream I boundary. Any external, broker-specific, platform-specific, credential, account, order-routing, or connectivity assumption remains Blocked / NO-GO.
- Workstream II's Demo/Sandbox environment setup path and Live Account readiness path are preparation-only. Broker connection, external broker calls, credentials, account or prop-account actions, real or sandbox order routing, live deployment, live trading, and formal Phase 23 activation remain unauthorized.
- Phase 24 is blocked until Phase 23 selects a broker and the operator approves connection scope.
- Phase 26 is blocked until Phase 25 selects a documentation platform and the operator approves documentation architecture.
- GitHub-dependent sync remains constrained by intermittent or costly global connectivity.
- Ubuntu compute-node commands are not verified in this workspace.
- Markdown link-checking is unavailable locally because no link-check tool was found.
- Static-analysis and formatter commands are unconfigured locally.

## Known Defects And Warnings

- Previous full compile output emitted existing warnings:
  - `src/AsyncNetworkClient.cpp`: unused `SSL_ERROR_NONE`.
  - `src/RiskEngine.cpp`: `totalPositioned` set but not used.
- No failing tests were observed during local verification.
- `data/.DS_Store` and `src/.DS_Store` exist as ignored local artifacts.

## Documentation System Status

- Root `AGENTS.md`, `PLANS.md`, and `CONTRIBUTING.md` are present locally.
- Dedicated testing, data, security, actor, workflow, handoff, benchmarking, dependency, configuration, style, failure-recovery, live-readiness, glossary, review, release, project workstream architecture, Workstream I, and Phase 22 offline-research documents are present locally.
- `.agents/skills/` TradeBot skill files are present locally, including `tradebot-git-safety`.
- `WORKSTREAM_ARCHITECTURE.md` is the current project-level Workstreams I-VII map; `ROADMAP.md` is the canonical phase authority; this document summarizes current state.

## Verification Evidence

Verified locally on 2026-06-25:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase22_tests --output-on-failure
```

Results:

- Configure succeeded.
- Build succeeded.
- Full CTest suite passed: 6 tests, 0 failed.
- Targeted `phase22_tests` passed.

## Operating Constraints

- Default operation remains offline-first and low-bandwidth-conscious.
- GitHub is the long-term system of record after local work is committed and pushed; inspect Git directly for current branch and commit state.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Assemble documentation-only pre-Phase-23 evidence lanes: technical evidence, adapter-fit audit, failure-mode checklist, and demo/live semantics analysis. Stop at a Wade checkpoint before broker selection or any new scope. Do not connect or log in to MT5, call a broker, access or create an account, use credentials, start implementation, route real or sandbox orders, enable live trading, or alter risk defaults.

## Next Professional Halting Point

Stop after the scoped evidence package and Wade checkpoint. Broker selection, Phase 24, broker-dependent implementation, and live trading remain explicitly Blocked / NO-GO without separate approval. Do not skip phases, delete generated artifacts, or perform connectivity, credential, account, real-order, sandbox-order, or live operations.
