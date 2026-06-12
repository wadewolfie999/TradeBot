# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state snapshot for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-06-08 in `/Users/vaheedgorgeen/TradeBot`.
- Last CMake/CTest verification evidence: 2026-06-06.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Current branch: `phase/phase19-revalidation`.
- Current commit: `ffbd352 phase19: optimize applyBbo hot path`.
- Recent Phase 19 commits:
  - `ffbd352 phase19: optimize applyBbo hot path`
  - `58fd436 phase19: add applyBbo microbenchmark`
  - `abcb3f8 phase19: restore implementation baseline for revalidation`
- Worktree status at documentation audit: dirty with tracked documentation edits, untracked governance/documentation files, and untracked `.agents/skills/`.
- Ignored artifacts observed: `build/`, `data/`, and `src/.DS_Store`.

## Active Phase

- Current active phase: Phase 19 revalidation.
- Evidence: branch name `phase/phase19-revalidation`; recent commits referencing Phase 19 and `applyBbo`; benchmark target `apply_bbo_microbench`; generated logs under ignored `build/phase19_revalidation/`.
- Phase status: implementation commits exist locally; validation is in progress and must remain evidence-driven.

## Completed Work Verified In Current Tree

- C++20 core library and `tradebot_core` executable are configured in `CMakeLists.txt`.
- Tests exist for phases 13, 15, 16, 17, and 18.
- Benchmarks exist for `throughput_bench`, `phase18_burnin`, and `apply_bbo_microbench`.
- Existing accepted ADRs:
  - ADR 0001: deprecate offline MOP/MOR/SS workflow.
  - ADR 0002: GitHub as long-term system of record after local cleanup is committed and pushed.
- Runtime modes verified in `SystemConfig`: `BACKTEST`, `PAPER`, `LIVE`; default is `BACKTEST`.
- Credential loading verified through `AuthManager` and `SystemConfig` env names `AIIO_API_KEY` and `AIIO_API_SECRET`.

## In-Progress Work

- Phase 19 revalidation of `L2OrderBook::applyBbo` performance and benchmark evidence.
- Repository governance and Codex skill-system creation.

## Blocked Or Constrained Work

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

- Root `AGENTS.md`, `PLANS.md`, and `CONTRIBUTING.md` are present locally and pending review/commit/sync.
- Dedicated testing, data, security, actor, workflow, handoff, benchmarking, dependency, configuration, style, failure-recovery, live-readiness, glossary, review, and release documents are present locally and pending review/commit/sync.
- `.agents/skills/` TradeBot skill files are present locally and validate with the Codex skill validator.
- Documentation and governance changes remain local until committed and pushed.

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
- GitHub is the long-term system of record after local work is committed and pushed, but local Git is the immediate working record until sync.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Finish the documentation audit, validate path and skill consistency, then review and commit/sync only with operator approval.

## Next Professional Halting Point

Stop after documentation state is corrected, cross-document audit results are reported, and remaining verification limits are stated. Do not commit, push, alter source code, delete generated artifacts, or perform live-capable operations without explicit operator approval.
