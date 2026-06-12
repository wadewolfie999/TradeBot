# TradeBot Agent Operating Contract

## Purpose And Authority

- Purpose: primary repository instruction contract for Codex and other repository-side agents.
- Authority level: repository-level operating authority, subordinate only to explicit operator instruction for the current task.
- Audience: Codex, human contributors, reviewers, testers, research agents, and future coding agents.
- Related documents: deeper policy lives in `docs/`; planning rules live in `PLANS.md`; contributor workflow lives in `CONTRIBUTING.md`.

## Repository Identity

- Project name: TradeBot.
- Repository root: `~/TradeBot`.
- Verified local root: `/Users/vaheedgorgeen/TradeBot`.
- CMake project name: `TradeBot-AIIO-Core`.
- Current implementation core: C++20.
- Current build system: CMake 3.14 minimum, verified locally with CMake 4.3.2, Unix Makefiles, Apple clang 21.0.0.
- Financial-sensitive classification: this repository contains live-capable trading-system code and must be treated as financial, credential, and operational-risk sensitive even when a task is documentation-only.

TradeBot is a trading-system research and engineering repository with a deterministic C++ core for market-data replay, strategy execution, portfolio and risk accounting, L2 order-book handling, trigger orders, analytics, metrics, broker/data adapters, tests, and benchmarks. It defaults to non-live operation.

Verified major subsystems:

- `include/SystemConfig.hpp`: runtime mode and shared configuration.
- `include/CsvReader.hpp`, `src/CsvReader.cpp`: candle CSV input.
- `include/LocalDataReplayAdapter.hpp`, `src/LocalDataReplayAdapter.cpp`: local CSV/binary replay ticks.
- `include/L2OrderBook.hpp`, `src/L2OrderBook.cpp`: L2 order book and BBO application.
- `include/IStrategy.hpp`, `SmaCrossStrategy`, `MeanReversionStrategy`: strategy boundary.
- `PortfolioAllocator`, `RegimeDetector`: allocation and regime-aware weighting.
- `PortfolioManager`, `RiskEngine`, `ExecutionEngine`, `TriggerOrderManager`: portfolio, risk, execution, and trigger-order boundaries.
- `LiveDataAdapter`, `BrokerGateway`, `AsyncNetworkClient`, `AuthManager`: live-capable data, broker, network, and credential boundaries.
- `AnalyticsEngine`, `MetricsAggregator`, `LocalMetricsExporter`, `StateSerializer`: result output, metrics, and resume state.

## Actor Hierarchy

1. Project owner/operator
   - Final authority for architecture, scope, risk changes, commits, pushes, releases, live-trading transitions, credentials, and external connectivity.
   - Must explicitly approve live-trading unlocks, financial-limit changes, history rewrites, destructive actions, and publication.

2. Strategic AI partner
   - Provides planning, architecture, review, governance, and safety analysis.
   - Does not replace operator approval.

3. Codex
   - Repository-side inspection and implementation agent.
   - May inspect, plan, edit, test, and report within assigned scope.
   - Must not silently expand scope, reinterpret project authority, or treat chat history as stronger than current operator instruction plus repository authority.

4. Other human or AI actors
   - Includes contributors, reviewers, testers, research agents, local/offline models, and future coding agents.
   - Repository access is not automatic trust. Actors must follow declared permissions, risk controls, documentation authority, and handoff requirements.

See `docs/ACTORS.md` for the full actor model.

## Authority Order

Use this order when instructions conflict:

1. Explicit operator instruction for the current task.
2. This `AGENTS.md`.
3. Accepted ADRs in `docs/decisions/`.
4. `docs/RISK_POLICY.md`.
5. `docs/ARCHITECTURE.md`.
6. Active approved plan under the `PLANS.md` schema.
7. `docs/PROJECT_STATE.md`.
8. `docs/ROADMAP.md`.
9. `docs/WORKFLOW.md`.
10. Skill-specific instructions in `.agents/skills/*/SKILL.md`.
11. `CONTRIBUTING.md`.
12. General documentation.

No document may silently override financial safety or operator authority. Conflicts involving live execution, credentials, order routing, risk limits, generated-data provenance, or architectural boundaries must be reported and halted until the operator resolves them.

## Codex Operating Rules

Codex must:

- Inspect before mutation.
- Identify scope, risk classification, current branch, and worktree status before editing.
- Preserve unrelated work, including user changes and generated outputs.
- Make focused changes aligned with the assigned scope.
- Prefer existing repository patterns over new abstractions.
- Update documentation when behavior, architecture, workflows, risk posture, or verification expectations change.
- Run appropriate verification and report exact commands and outcomes.
- Report exact files changed.
- Report failures, skipped checks, warnings, and limitations honestly.
- Avoid undocumented assumptions; record necessary assumptions in plans or handoffs.
- Stop safely when required evidence is unavailable or a safety-sensitive ambiguity cannot be resolved locally.

## Hard Prohibitions

Codex and other repository-side agents must not:

- Expose, print, copy, or modify secrets.
- Edit `.env` files, credentials, key material, account identifiers, or secret stores unless explicitly authorized for that exact action.
- Introduce live-trading behavior.
- Enable real orders.
- Weaken dry-run or backtest defaults.
- Alter financial risk limits, position sizing, drawdown gates, kill-switch behavior, order-routing behavior, or credential handling without explicit operator approval.
- Commit, push, tag, release, or publish unless explicitly instructed.
- Rewrite Git history unless explicitly approved.
- Delete unrelated work.
- Treat generated outputs as source material.
- Fabricate successful test results or hide failures.
- Claim profitability, production readiness, safety, or live readiness from backtests alone.
- Invent architectural facts.
- Silently bypass roadmap, plan, risk, or live-trading gates.
- Make broad refactors during narrowly scoped work.

## Verified Repository Map

- C++ headers: `include/`.
- C++ source: `src/`.
- C++ tests: `tests/`.
- Benchmarks: `src/benchmarks/`.
- Documentation: `docs/`.
- ADRs: `docs/decisions/`.
- Sample data directory: `data/samples/` exists and is currently empty.
- Historical data path referenced by benchmark code: `data/historical/`.
- Generated result path: `data/results/`, ignored by Git.
- Generated archive path: `data/archive/`, ignored by Git.
- Build directory: `build/`, ignored by Git.
- Phase 19 generated logs/replay artifacts observed under `build/phase19_revalidation/`.
- Codex skills: `.agents/skills/`.
- Python components: none tracked at the time this contract was created.
- Julia components: none tracked at the time this contract was created.
- Scripts/tooling: no tracked shell scripts or external tool wrappers found; CMake is the active tooling entrypoint.

## Verified Commands

Run from `~/TradeBot`.

Configure:

```sh
cmake -S . -B build
```

Build:

```sh
cmake --build build
```

Full CTest suite:

```sh
ctest --test-dir build --output-on-failure
```

Targeted phase test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
```

Targeted build:

```sh
cmake --build build --target phase18_tests
cmake --build build --target apply_bbo_microbench
```

Phase 19 BBO microbenchmark:

```sh
build/apply_bbo_microbench 10000
```

Repository hygiene:

```sh
git status --short
git diff --check
git diff --stat
git diff --name-status
```

Documentation audit commands:

```sh
find .agents/skills -name SKILL.md -print | sort
find docs -maxdepth 3 -type f -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

Verified local warnings during `cmake --build build`:

- `src/AsyncNetworkClient.cpp`: unused `SSL_ERROR_NONE`.
- `src/RiskEngine.cpp`: `totalPositioned` set but not used.

No configured formatter, static analyzer, Markdown linter, or Markdown link checker was found locally when this contract was written. `clang-format`, `clang-tidy`, `markdown-link-check`, `lychee`, and `markdownlint` were not available.

Ubuntu compute-node commands are not verified in this workspace. Use the same CMake/CTest commands only after confirming compiler, CMake, and dependency availability on that host.

## Runtime Modes

Verified code modes in `SystemConfig`:

- `BACKTEST`: default deterministic CSV-driven path.
- `PAPER`: live-data-like adapter path with simulated local broker behavior.
- `LIVE`: live data plus live-capable broker execution path.

Governance vocabulary:

- Simulation: offline or deterministic run with no external trading side effects. Usually `BACKTEST`.
- Dry-run: non-live validation where actions are recorded or simulated, not sent as real orders.
- Paper trading: `PAPER` mode or equivalent simulated execution.
- Sandbox: an external test venue or broker sandbox; not a distinct verified code flag.
- Live trading: any workflow capable of real orders, external account impact, or live venue state mutation.

TradeBot defaults to `BACKTEST` or dry-run behavior. Live trading is prohibited unless explicitly authorized by the operator and all requirements in `docs/LIVE_TRADING_READINESS.md` and `docs/RISK_POLICY.md` are met.

## Documentation Authority Map

- Codex instructions: `AGENTS.md`.
- Planning system: `PLANS.md`.
- Current state: `docs/PROJECT_STATE.md`.
- Roadmap and phase gates: `docs/ROADMAP.md`.
- Architecture: `docs/ARCHITECTURE.md`.
- Financial and operational risk: `docs/RISK_POLICY.md`.
- Testing contract: `docs/TESTING.md`.
- Data governance: `docs/DATA_POLICY.md`.
- Security and credentials: `docs/SECURITY.md`.
- Actor permissions: `docs/ACTORS.md`.
- Work protocol: `docs/WORKFLOW.md`.
- Handoffs: `docs/HANDOFF.md`.
- ADRs: `docs/decisions/`.
- Contribution workflow: `CONTRIBUTING.md`.
- Reusable Codex workflows: `.agents/skills/*/SKILL.md`.

## Definition Of Done

Documentation-only changes:

- Accurate to verified repository state.
- No invented facts, stale paths, or unsafe live-trading implications.
- Relevant indexes updated.
- `git diff --check` passes.
- Documentation audit grep commands reviewed.

Source changes:

- Scope is recorded.
- Relevant tests are run.
- Documentation is updated when behavior, interfaces, risk posture, or architecture changes.
- Warnings, skipped tests, and residual risk are reported.

Behavior changes:

- Acceptance criteria are defined in a plan unless the change is small and low risk.
- Existing regression tests pass.
- New or adjusted tests cover the changed behavior.
- Rollback path is documented.

Architecture changes:

- `docs/ARCHITECTURE.md` is updated.
- An ADR is created or updated when the decision should remain stable across future work.
- Risk and testing impacts are reviewed.

Performance changes:

- Benchmark command, environment, input size, and output are reported.
- No performance claim is made without comparative evidence.
- Generated benchmark outputs remain out of Git unless intentionally versioned.

Financial-sensitive changes:

- Operator approval is required before implementation.
- `docs/RISK_POLICY.md` and `docs/LIVE_TRADING_READINESS.md` gates are checked.
- Tests must prove fail-safe behavior for affected paths.
- Handoff must state prohibited next actions and required approvals.

## Reporting Contract

Every Codex task report must include:

- Scope received.
- Repository state found: branch, commit, worktree status, relevant ignored artifacts.
- Files inspected.
- Files changed.
- Design decisions.
- Commands run.
- Test results.
- Failures, warnings, skipped checks, or limitations.
- Risks.
- Remaining work.
- Git diff summary.

For long-running or interrupted tasks, create or update a handoff using `docs/HANDOFF.md`.
