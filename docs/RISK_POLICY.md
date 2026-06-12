# TradeBot Risk Policy

## Purpose And Authority

- Purpose: authoritative risk policy for financial, execution, data, credential, reproducibility, and AI-agent risks.
- Authority level: below accepted ADRs and above architecture, plans, state, roadmap, workflow, skills, and contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, research agents, and any actor touching live-capable behavior.

## Default Mode

TradeBot defaults to `BACKTEST`, dry-run, simulation, or paper behavior. Live trading is not permitted by default.

## Live Trading Unlock Requirements

Live trading requires explicit operator authorization for the exact venue, account, scope, branch, commit, configuration, and time window. Authorization is not inferred from repository access, credentials being present, or the existence of `LIVE` mode in code.

Before live trading, all requirements in `LIVE_TRADING_READINESS.md` must be satisfied, including:

- Verified kill switch.
- Verified no-new-orders halt path.
- Verified credential source and least-privilege key permissions.
- Maximum order size, position limits, daily loss limits, and total drawdown limits.
- Paper or sandbox validation.
- Logging path and log-redaction review.
- State reconciliation plan.
- Network interruption and stale-data behavior.
- Operator monitoring and rollback procedure.

## Financial Risk

- Never change position sizing, drawdown gates, VaR thresholds, stop logic, order-routing behavior, or risk-limit defaults without explicit operator approval.
- Backtests and benchmarks do not prove profitability.
- Every strategy claim must account for fees, slippage, spread, liquidity, timestamp handling, lookahead bias, survivorship bias, and data leakage.
- Risk controls must fail closed when required data is unavailable.

## Order-Execution Risk

- Execution must pass through `ExecutionEngine` and risk gates before opening new positions.
- Live-capable broker interaction belongs at the `BrokerGateway` boundary.
- Paper behavior must remain simulated unless an approved sandbox integration changes it.
- Partial fills, cancellations, retries, and reconciliation must be explicit and venue-aware before live use.
- No actor may enable real orders by default.

## Market-Data Integrity

- Market data must be treated as invalid when malformed, stale, out of order, or inconsistent with expected schema.
- `LiveDataAdapter` integrity callbacks and `RiskEngine` halt behavior must not be bypassed.
- Replay data must preserve event order and timestamp semantics.
- Data gaps must be recorded, not silently filled without provenance.

## Timestamp And Timezone Handling

- Replay ticks use nanosecond timestamps where represented by `ReplayTick::timestampNs`.
- Candle timestamps use `MarketCandle::epochTimestamp`.
- Documentation and generated outputs must state timestamp units when relevant.
- Do not mix local timezone display with execution or replay ordering.
- Use UTC or explicit epoch values for reproducibility unless a file schema states otherwise.

## Replay Correctness

- Replay tests must cover CSV parsing, binary roundtrip, cursor behavior, and BBO shape when replay drives order-book logic.
- Synthetic replay data must be labeled as synthetic.
- Generated replay files must not be mistaken for historical market data.

## Lookahead Bias And Data Leakage

- Strategies may consume only information available at the simulated event time.
- Resume paths must avoid trading on already-processed candles.
- Analytics outputs must not feed future decisions unless an explicit online-feedback boundary is tested and documented.

## Transaction Costs And Slippage

- Performance and strategy reviews must include fees and slippage assumptions.
- `ExecutionEngine` fee and slippage parameters are part of financial-sensitive behavior.
- Actual fill feedback in live-capable paths must be distinguished from modeled slippage.

## Position Sizing And Drawdown Controls

- Position sizing and drawdown controls are financial-sensitive.
- `RiskEngine` drawdown, max position, VaR, close-only, and halted states must remain active unless a plan and approval change them.
- Clearing a halt is an operator-level action in live-capable contexts.

## Kill Switch

Any live-capable path must support operator-controlled halt behavior that stops new order submission. Cancel behavior must be explicit and venue-aware. A kill switch must be tested before any live unlock.

## Mode Confusion

- `BACKTEST`, `PAPER`, and `LIVE` are verified code modes.
- Sandbox is a governance concept, not a verified code mode.
- Documentation must distinguish dry-run, paper, sandbox, and live behavior.
- `LIVE` mode must not be used casually in tests or local runs without understanding whether network or order side effects can occur.

## Credential Exposure

- Never commit credentials, API keys, tokens, private keys, account IDs, or `.env` files.
- Never print secret values.
- Secret-looking files require operator review.
- Documentation may mention environment variable names such as `AIIO_API_KEY` and `AIIO_API_SECRET`, but never values.

## External API And Network Risk

- Intermittent or degraded connectivity is an operational risk.
- Paper, sandbox, and live-capable workflows must define disconnect detection, timeout thresholds, stale-data handling, bounded retry policy, and reconciliation after reconnect.
- Network-dependent work should be scheduled during planned connectivity windows.

## Partial Execution And Reconciliation

- Partial fills must be tracked and reconciled.
- Local portfolio state must not be assumed correct after disconnect or API failure.
- Reconciliation differences must be logged and reviewed before continuing live-capable operation.

## Malformed Inputs

- Malformed CSV, binary replay, external payload, or config input must fail safely.
- Tests for malformed inputs are required when parsers or external boundaries change.

## Generated-Data Contamination

- Generated data under `data/results/` and `build/` is not source truth.
- Generated benchmark/replay files must not be used as historical source data without provenance and operator approval.

## Dependency And Supply-Chain Risk

- New dependencies require review under `DEPENDENCY_POLICY.md`.
- Avoid adding network-dependent tooling unless it is necessary and approved.
- Dependency updates must not weaken reproducibility or security.

## Reproducibility Risk

- Record seeds, inputs, command lines, build mode, compiler, dataset provenance, and generated-output paths when making research or benchmark claims.
- Do not compare benchmarks across machines without naming the machines and build settings.

## Operator And AI-Agent Error

- Actors are not trusted merely because they have repository access.
- AI agents must inspect before mutation, avoid scope expansion, and report uncertainty.
- Financial-sensitive ambiguity requires halt and operator escalation.

## Rollback And Containment

- For source changes, rollback is normally a Git revert or branch reset approved by the operator.
- For generated artifacts, remove or archive only within approved scope.
- For credential exposure, follow `SECURITY.md`: revoke, rotate, audit logs, and notify the operator.
- For live-capable incidents, stop new orders, reconcile state, preserve evidence, and do not resume without operator approval.
