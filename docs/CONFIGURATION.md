# TradeBot Configuration

## Purpose And Authority

- Purpose: document verified runtime configuration, modes, flags, env vars, and output paths.
- Authority level: configuration reference below architecture and risk policy.
- Audience: operator, Codex, maintainers, testers, and contributors.

## Verified Runtime Modes

`SystemConfig` defines:

- `BACKTEST`: deterministic CSV replay; default.
- `PAPER`: live-data-like path with simulated broker execution.
- `LIVE`: live-capable data and broker execution path.

`parseModeFlag` accepts `backtest`, `paper`, and `live` case variants shown in code and returns `BACKTEST` for unrecognized strings.

## Verified CLI Flags

`src/main.cpp` handles:

```sh
build/tradebot_core --mode backtest <csv-files>
build/tradebot_core --mode paper <csv-files>
build/tradebot_core --mode live <csv-files>
build/tradebot_core --resume <snapshot-file> <csv-files>
```

If no file arguments are supplied, the program falls back to `data/BTCUSDT-15.csv`. That file was not present in the verified tracked tree.

## Credential Configuration

`SystemConfig` defaults:

- API key env name: `AIIO_API_KEY`
- API secret env name: `AIIO_API_SECRET`

Credentials may also be held in `SystemConfig` fields. Do not hardcode or document values.

## Network Defaults

`SystemConfig` contains default endpoint strings:

- WSS endpoint: `wss://stream.example.com/ws`
- REST endpoint: `https://api.example.com`

These defaults are not live authorization.

## Risk Configuration

Verified defaults include:

- `latencyMaxMs`: `500`
- `errorRateThresh`: `5`
- `atrScaleUpThreshold`: `1.5`
- `varScaleLowVolFactor`: `1.0`
- `varScaleHighVolFactor`: `0.5`

Financial limit changes require operator approval.

## Output Paths

- Analytics default: `data/results`.
- Snapshot default: `data/results/snapshot.json`.
- Throughput report: `data/results/latency_report.csv`.
- Phase 18 burn-in report: `data/results/phase18_burnin_latency.csv`.
- Phase 18 default replay path: `data/historical/BTCUSDT-L2-1M.bin`.

Generated outputs are ignored by Git unless intentionally versioned.

## Configuration Change Rules

Configuration changes require documentation updates when they affect:

- Runtime modes.
- CLI flags.
- Env var names.
- Output paths.
- Risk thresholds.
- Credential behavior.
- Live-capable network behavior.
