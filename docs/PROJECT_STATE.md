# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state snapshot for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-06-19 in `/Users/vaheedgorgeen/TradeBot`.
- Last CMake/CTest verification evidence: 2026-06-06.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Current branch: `main`.
- Current local HEAD: `1bb888d Merge pull request #5 from wadewolfie999/docs/tradebot-git-safety`.
- Fetched `origin/main`: `1bb888d Merge pull request #5 from wadewolfie999/docs/tradebot-git-safety`.
- Recent mainline commits:
  - `1bb888d Merge pull request #5 from wadewolfie999/docs/tradebot-git-safety`
  - `2d91bfc Merge pull request #4 from wadewolfie999/docs/phase21-review-alignment`
  - `0980083 Merge pull request #3 from wadewolfie999/infra/git-enforcement-pipeline`
  - `56a29ee` merged Phase 19 revalidation history
  - `b7685ca Merge pull request #1 from wadewolfie999/docs/github-era-cleanup`
- Worktree status at documentation audit: dirty with documentation-only governance closeout edits for ADR 0003, Phase 21 approval, Phase 22 NO-GO status, and authority-doc reconciliation.
- Ignored artifacts observed: `build/`, `data/`, and `src/.DS_Store`.

## Active Phase

- Current active phase: Phase 21 Workstream I governance closeout.
- Evidence: `main` contains the merged Phase 19 revalidation history, infrastructure validation pipeline, Workstream I Phase 21 artifacts, ADR 0003, and `tradebot-git-safety`.
- Phase status: Phase 21 is Complete — Approved by operator decision. ADR 0003 is Accepted. Phase 22 is Blocked / NO-GO and has not been scoped or implemented.

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

- Documentation-only governance closeout for Phase 21 Workstream I approval state.
- Repository governance and Codex skill-system maintenance.

## Blocked Or Constrained Work

- Phase 22 source implementation is Blocked / NO-GO until the operator separately approves bounded scope, verification strategy, rollback path, and any broker-specific assumptions.
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
- Current governance closeout edits remain local until committed and pushed.

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
- GitHub is the long-term system of record after local work is committed and pushed; local `main` is fast-forwarded to fetched `origin/main` as of this audit.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Prepare Phase 22 scoping only. Do not implement Phase 22, add broker-specific assumptions, change credentials, enable live trading, or alter risk defaults without separate operator approval.

## Next Professional Halting Point

Stop after Phase 21 governance state is corrected, cross-document audit results are reported, and Phase 22 remains explicitly Blocked / NO-GO. Do not commit, push, alter source code, delete generated artifacts, or perform live-capable operations without explicit operator approval.
