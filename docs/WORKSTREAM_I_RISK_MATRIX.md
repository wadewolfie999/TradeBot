# Workstream I Risk Matrix

## Purpose And Authority

- Purpose: classify Workstream I integration risks by subsystem and define required controls for Phase 21 and future Phase 22 work.
- Authority level: planning evidence below `RISK_POLICY.md` and accepted ADRs; complements `ARCHITECTURE.md`.
- Audience: operator, implementers, reviewers, testers, and future risk-review agents.
- Status: planning-only. This matrix does not authorize source edits, risk-limit changes, live trading, credentials work, or external connectivity.

## Risk Scale

- Critical: could create real order, credential, account, unreconciled exposure, or risk-control failure.
- High: could corrupt deterministic behavior, replay semantics, execution state, or broker boundary assumptions.
- Medium: could produce misleading validation evidence, degraded observability, or local-only failures.
- Low: documentation, ergonomics, or non-blocking hardening risk.

## Subsystem Matrix

| Subsystem | Classification | Primary Failure Modes | Required Controls | Validation Gate |
| --- | --- | --- | --- | --- |
| `L2OrderBook` | High correctness and performance risk | Invalid BBO, crossed quotes, recenter regression, stale BBO into trigger orders | Phase 18 tests, BBO edge tests, benchmark only after correctness | `phase18_tests` passes before performance claims |
| `ExecutionEngine` | Critical financial and order-lifecycle risk | Risk bypass, duplicate fills, partial-fill mishandling, incorrect cancel handling, portfolio mutation without fill evidence | Explicit lifecycle events, risk precheck, idempotency, reconciliation visibility | Order lifecycle tests cover reject, fill, partial fill, cancel, halt |
| `RiskEngine` | Critical financial risk | New exposure during halt, close-only bypass, stale synced positions, auto-clearing halt, latency/error breaker drift | Fail-closed rules, no default risk changes, operator-controlled halt recovery | Risk review confirms no weakening of gates |
| `BrokerGateway` | Critical live-capable boundary risk | Direct real order path, broker ID mismatch, reconciliation drift, paper/live confusion | All broker actions behind gateway, paper remains simulated, explicit mapping and reconciliation | Gateway tests prove no source can bypass boundary |
| `LiveDataAdapter` | High data-integrity and live-capable risk | Stale data, malformed payload, disconnect gap, nondeterministic queue behavior | Integrity callback, fail-closed risk state, deterministic simulation harness | Data integrity tests and no backtest dependency on live adapter |
| `AsyncNetworkClient` | High network and operational risk | Timeout ambiguity, API error storms, TLS/config drift, external dependency in tests | Bounded error reporting, no required external network for deterministic tests | Network tests remain simulated or explicitly isolated |
| `AuthManager` | Critical credential risk | Secret logging, secret fixture, wrong source precedence, accidental `.env` edit | Env-name-only documentation, no secret printing, no credential access by strategies | Security review confirms no secret exposure |
| `LocalDataReplayAdapter` | High replay and data-provenance risk | Timestamp-unit drift, event-order drift, generated data treated as source, binary compatibility break | Replay compatibility checklist, provenance labels, schema migration tests | Replay tests prove CSV/binary behavior unchanged |
| `SystemConfig` | Critical mode-confusion risk | `LIVE` enabled casually, `PAPER` connected to real venue, unsafe defaults | `BACKTEST` default, explicit operator live approval, documented mode semantics | Config review confirms defaults unchanged |
| Documentation and plans | Medium governance risk | Stale phase state, conflicting authority, premature Phase 22 work | Authority order, ADR status, plan status, doc index updates | Doc audit and operator approval before implementation |
| AI collaboration tooling | Medium governance and determinism risk | Subagent scope drift, MCP external-state dependency, conflicting reviews | Read-only first, main-agent ownership, no mutating MCP without approval | Delegation plan states scope and source of truth |

## Cross-Cutting Risk Controls

### Financial Risk

- No position sizing, drawdown, VaR, fee, slippage, max-position, halt, or close-only defaults may change without explicit operator approval.
- Any uncertainty in trade permission, broker state, or data freshness blocks new exposure.
- Backtests and benchmarks do not prove profitability or live readiness.

### Execution Risk

- Execution must pass through `ExecutionEngine`, `RiskEngine`, and `BrokerGateway`.
- Partial fills, cancellations, and reconciliation must be explicit.
- Unknown broker state is a blocking state, not a success state.

### Data And Replay Risk

- Replay event order and timestamp semantics must remain stable.
- Synthetic, generated, sampled, or historical data must be labeled.
- Generated outputs cannot become hidden source inputs.

### Credential And Network Risk

- Secret values must not be printed, committed, copied, or documented.
- External network access must not be required for deterministic validation.
- Network errors and auth failures must be reported without leaking credentials.

### Governance Risk

- Phase 21 is planning-only.
- Phase 22 requires separate operator approval.
- Proposed skills are not active unless explicitly approved.
- ADR status must be respected before treating architecture decisions as canonical.

## Phase 21 Risk Gate

GO only if:

- Risk controls are documented for each critical and high-risk subsystem.
- Phase 22 implementation can be bounded by approved contracts.
- Documentation does not imply live readiness.
- Open uncertainties are documented as blockers.

NO-GO if:

- Risk behavior or live-capable behavior is ambiguous.
- Any implementation task requires a broker-specific assumption.
- Any credential or real-account handling is implied.
- Replay compatibility is not testable.

## Phase 22 Risk Gate

GO only if:

- Operator separately approves source implementation.
- Critical and high-risk subsystem tests are identified before edits.
- Rollback is a Git revert or branch reset approved by the operator.

NO-GO if:

- Source changes would weaken deterministic defaults.
- Test evidence cannot isolate live-capable paths.
- Required review skills or human review are unavailable for critical changes.

