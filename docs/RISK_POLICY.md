# TradeBot Risk Policy

## Default Mode

TradeBot defaults to dry-run, replay, simulation, or paper-trading mode.

Live trading is not permitted by default.

## Live Trading Gate

Live trading requires explicit operator authorization and a verified configuration covering:

- Exchange or broker target.
- Account scope.
- Credential source.
- Maximum order size.
- Position limits.
- Loss limits.
- Kill switch behavior.
- Logging path.
- Successful dry-run or paper-trading test.

## Credentials

Credentials are security-sensitive.

Rules:

- Never commit API keys, secrets, tokens, passwords, private keys, account IDs, or credential files.
- Never paste secrets into chat, logs, docs, commits, or issue text.
- Use local environment variables or a local secret manager.
- Use placeholder labels only in documentation.

Placeholder format:

```sh
export TRADEBOT_VENUE_API_KEY="<set locally; do not paste into chat>"
export TRADEBOT_VENUE_API_SECRET="<set locally; do not paste into chat>"
```

## Kill Switch

Any live-capable execution path must support an operator-controlled kill switch.

The kill switch should stop new order submission and move the system into a safe state. Cancel behavior must be explicit and venue-aware.

## Backtest Caveats

Backtests do not prove strategy viability.

Every backtest review must consider:

* Fees.
* Slippage.
* Spread and liquidity constraints.
* Timestamp handling.
* Lookahead bias.
* Survivorship bias.
* Data leakage.
* Order book reconstruction correctness.
* Latency assumptions.
* Position sizing and risk limits.

## Sandbox and Live Progression

Progression toward live trading must be gated:

1. Offline replay or simulation.
2. Dry-run validation.
3. Paper-trading or sandbox validation.
4. Live sandbox only with explicit authorization.
5. Conditional live deployment only with explicit authorization and active risk controls.

## Network and Connectivity Risk

Intermittent or degraded global connectivity is an operational risk for market data, broker or exchange APIs, and control-plane access.

Any paper, sandbox, or live-capable workflow must define:

- Disconnect detection.
- Timeout thresholds.
- Stale-data handling.
- Retry policy with bounded behavior.
- Order and state reconciliation after reconnect.
- Safe behavior when market status, balances, positions, or recent orders cannot be verified.

No live-capable mode should assume stable network access during execution.

## Generated Outputs

Treat `data/results/` as generated output unless explicitly versioned.

Generated outputs should not be staged or committed by default.
