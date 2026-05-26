# TradeBot Architecture

## Purpose

TradeBot is a trading-system project built around a baseline deterministic core for market-data replay, L2 order book logic, strategy execution, risk controls, simulation or dry-run behavior, tests, benchmarks, and reproducible outputs.

The architecture should also preserve clean extension boundaries for future Python or Julia research, optimization, and model-assisted workflows without making those layers part of the current production core.

## Primary Areas

- `src/` — Implementation source.
- `include/` — Headers.
- `tests/` — Tests and regression coverage.
- `src/benchmarks/` — Benchmarks.
- `data/historical/` — Historical input data.
- `data/results/` — Generated outputs and results.
- `docs/` — Active project documentation.

## Execution Modes

TradeBot defaults to non-live operation.

Expected modes:

- Offline research and analysis.
- Historical market-data replay.
- Dry-run or paper-trading simulation.
- Live trading only after explicit authorization, verified risk controls, credential hygiene, logging, and kill switch behavior.

## System Boundaries

The system should keep these concerns separate:

- Market-data ingestion and replay.
- L2 order book construction and validation.
- Strategy signal generation.
- Portfolio and position accounting.
- Risk checks and trading controls.
- Execution adapters.
- Analytics, reporting, and benchmarks.
- Future research, optimization, and model-assisted workflows.

## Future ML / Optimization Extension Boundary

TradeBot should preserve a future ML / optimization extension boundary without implying current production ML functionality.

The baseline deterministic core must remain testable, reproducible, and independent of ML framework dependencies.

Future research or optimization layers may use Python or Julia, but neither is mandatory. Those layers may propose signals, parameters, allocations, or strategy choices only after validation.

Future ML or optimization layers must not bypass:

- Risk controls.
- Execution-mode gates.
- Logging and audit trails.
- Kill switch behavior.
- Reproducibility requirements.

Future ML or optimization layers should connect through explicit interfaces:

- Configuration files.
- Structured replay or backtest outputs.
- Strategy parameter interfaces.
- Signal or intent interfaces.
- Offline research pipelines.

## Data Flow

Typical flow:

1. Historical or live-like market data enters the system.
2. Replay or ingestion normalizes events.
3. Order book, strategy logic, and future research or optimization consumers receive normalized data through explicit interfaces.
4. Portfolio and risk controls evaluate proposed actions.
5. Execution adapters operate only in the authorized execution mode.
6. Logs, metrics, and results are written for review.

## Constraints

- Prefer deterministic replay and reproducible tests.
- Keep live execution isolated from research and replay paths.
- Preserve kill switch behavior and fail-safe shutdown semantics for all live-capable execution paths.
- Do not hardcode credentials or machine-specific paths.
- Treat generated data as disposable unless intentionally versioned.
- Revalidate legacy roadmap claims before promoting them to current project state.
