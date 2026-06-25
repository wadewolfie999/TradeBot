# Phase 22 Offline MT5/Prop-Account Research

## Purpose And Authority

- Purpose: record broker-neutral, offline research for Phase 22 Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness.
- Authority level: research artifact under the operator-approved research-only plan and below explicit operator instruction, accepted ADRs, `RISK_POLICY.md`, `ARCHITECTURE.md`, and approved Workstream I contracts.
- Plan: `PLAN-20260621-phase22-offline-mt5-prop-research` in `../PLANS.md`.
- Status: research artifact complete; bounded broker-neutral implementation is complete and accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`, without changing this document's research-only authority.
- Audience: operator, maintainers, reviewers, research agents, and future implementation planners.

This document is research evidence and a future-test specification. It is not an implementation contract, provider recommendation, broker selection, prop-firm selection, account-readiness claim, connectivity approval, credential approval, or live-trading approval.

## Scope

In scope:

- Offline research into MT5-facing integration surfaces and integration classes.
- Broker-neutral lifecycle, reconciliation, account, symbol, quantity-normalization, health, and failure concepts.
- Synthetic/configurable prop-account rule dimensions.
- Determinism constraints and a future fixture/replay validation matrix.
- Subsystem boundaries, non-activation controls, rollback, and blocked Phase 23+ decisions.

Out of scope:

- C++ source, headers, CMake, tests, scripts, configuration, data, runtime, or generated-output changes.
- MT5 terminal installation, startup, connection, automation, login, account access, or private data.
- Credentials, endpoints, certificates, account identifiers, or network sessions.
- Broker, provider, prop firm, program, rulebook, account type, instrument universe, or bridge-topology selection.
- Real orders, sandbox orders, live account mutation, live trading, Phase 22 implementation, or Phase 23 activation.
- Provider thresholds, provider-specific readiness claims, or production/live-readiness claims.

## Research Input And Evidence Handling

The operator supplied a completed deep-research memo as research input and enumerated its required findings in the task objective. No separate memo file was present in the supplied attachment directory during this audit. This artifact therefore records only the task-enumerated findings and repository-authoritative boundaries. It does not add uncited provider-specific, build-specific, license-specific, or version-sensitive claims.

External evidence does not authorize implementation. Any later provider example must be labeled as evidence for a rule dimension only, include a rulebook or API version and retrieval date, and remain non-normative until the operator approves the corresponding Phase 23+ decision.

### Evidence Register Structure

Every future evidence item must record:

| Field | Required content |
| --- | --- |
| Evidence ID | Stable local identifier, not a provider ticket or account identifier |
| Source class | Operator instruction, official platform documentation, broker/back-office documentation, provider rulebook, or repository authority |
| Title and locator | Public title and safe locator; no credentials, private URLs, account IDs, or tokens |
| Publisher or owner | Source owner; a named provider is evidence context, not a selection |
| Retrieved and effective dates | Retrieval date and, for rules, stated effective date or version |
| Jurisdiction/program context | Only when relevant; never generalized across programs |
| Integration class | Terminal-adjacent Python/IPC, terminal-adjacent MQL5 EA/service, or broker/back-office API class |
| Claim category | Lifecycle, event ordering, account, symbol, quantity, failure, prop-rule dimension, or operational constraint |
| Paraphrased finding | Concise finding without copying sensitive or excessive source text |
| Confidence and limitations | Verified, ambiguous, conflicting, version-sensitive, or requires review |
| Contradictions | Conflicting sources and the rule for resolving them |
| Authority effect | Always `research input only` unless the source is already repository authority |
| Redaction/provenance check | Confirms no secrets, account data, private balances, or proprietary payloads are stored |

Initial register:

| ID | Source | Finding used here | Limitation | Authority effect |
| --- | --- | --- | --- | --- |
| `E-OP-20260621` | Explicit operator objective | Phase title, docs-only scope, required research dimensions, implementation/live prohibitions | Applies only to this task's bounded scope | Authorizes documentation/research only |
| `E-MEMO-20260621` | Operator-provided deep-research memo, represented by findings enumerated in the objective | MT5 integration classes, lifecycle/event findings, metadata models, failure taxonomy, and prop-rule dimensions | Separate memo artifact and source citations were not supplied in the attachment directory | Research input only |
| `E-ADR-0003` | Accepted ADR 0003 | Future adapters remain below `BrokerGateway`; deterministic core and risk gates remain isolated | Does not authorize implementation | Repository architecture authority |
| `E-WI-CONTRACT` | Approved Workstream I adapter/replay/risk documents | Acknowledgement is not fill; unknown state reconciles; duplicate/stale events fail safely | Conceptual contract; current code is not claimed to implement all states | Approved planning evidence |

## Explicit Non-Activation Safeguards

- This document must not be used as a connection runbook, credential checklist, account-onboarding guide, or order-routing procedure.
- No executable code, configuration key, endpoint, certificate, credential, account identifier, provider threshold, or real symbol mapping is defined here.
- No MT5 terminal, Python integration, MQL5 program, back-office API, broker, prop firm, program, or topology is selected.
- `BACKTEST` must remain offline and deterministic. `PAPER` remains locally simulated unless separately approved work changes that boundary. `LIVE` remains unauthorized.
- Research GO does not change the Phase 22 implementation status, satisfy live-readiness gates, or start Phase 23.
- Any future implementation requires a new or approved implementation plan, explicit operator GO, bounded source scope, tests, rollback, security/risk review, and resolution of dependencies on blocked Phase 23+ decisions.
- Unknown, stale, malformed, contradictory, or unreconciled external state must block risk-increasing action in any future design.

## MT5 Surface Matrix

The rows below are research classes, not verified entitlements or selected interfaces. Availability and semantics can depend on product, deployment, terminal build, server configuration, license, broker, and operating environment.

| Integration class | Candidate surface to research offline | Broker-neutral outputs to model | Primary questions | Current status |
| --- | --- | --- | --- | --- |
| Terminal-adjacent Python/IPC | Local terminal-mediated requests, account/position/order/deal reads, symbol metadata, market/session observations, and process/session health | Normalized request result, execution events, account snapshot, symbol metadata, adapter health, reconciliation snapshot | Thread/process boundary, event delivery versus polling, history lag, failure detection, local IPC framing, timestamp source, and restart recovery | Research only; no Python component, terminal connection, or topology selected |
| Terminal-adjacent MQL5 EA/service | Terminal event callbacks, trade requests, trade-transaction events, local persistence/IPC, account/symbol queries, and permission state | Same broker-neutral contracts as every adapter; MT5-native objects remain below `BrokerGateway` | Event ordering, reentrancy, acknowledgement versus execution, terminal permissions, IPC durability, ticket mutation, and recovery after terminal restart | Research only; no EA/service, terminal session, or topology selected |
| Broker/back-office APIs such as Gateway, Manager, Server, Report, and Web API | Administrative, reporting, server, order, deal, position, symbol, account, session, and health surfaces where independently authorized and licensed | Same broker-neutral contracts; administrative capabilities are not assumed necessary or permitted | Product/license availability, least privilege, read/write separation, event streaming versus reports, data freshness, operational ownership, and auditability | Research only; no API entitlement, endpoint, account, provider, or topology selected |

Cross-class research surfaces:

| Surface | Offline research questions | Broker-neutral requirement |
| --- | --- | --- |
| Request submission | What constitutes local validation, dispatch, server receipt, acceptance, rejection, timeout, and unknown state? | Submitted, accepted, rejected, and filled remain distinct |
| Trade events | Which events carry execution quantity, price, costs, remaining quantity, and identifiers? | Versioned, idempotent `ExecutionEvent` with stable internal identity |
| Orders, deals, and positions | How are working requests, executions, and resulting exposure represented in netting and hedging contexts? | Normalize facts without importing MT5-native schemas into core contracts |
| History and reconciliation | When can order/deal/history views lag live events, and what snapshot is authoritative for recovery? | Timestamped reconciliation evidence with provenance and mismatch status |
| Account state | Which balance, equity, P&L, margin, currency, mode, and freshness values are available? | Complete `AccountSnapshot`; missing required fields fail closed |
| Symbol metadata | Which precision, tick, contract, volume, fill-policy, and session properties are available and mutable? | Versioned `InstrumentSpec` with effective time and freshness |
| Permissions and health | How are terminal trading permission, session/auth state, connectivity, rate limits, and degradation exposed? | Normalized adapter-health/failure events; no automatic risk-state clearing |
| Time | Which server, terminal, event, history, and local timestamps exist? | Explicit UTC/epoch units plus deterministic sequence/causality metadata |

## Order Lifecycle Findings

Request acceptance is not execution. The canonical local lifecycle must keep submitted, accepted, and filled as separate states even when an external surface compresses or reorders them.

| State | Broker-neutral meaning | Portfolio/risk mutation rule |
| --- | --- | --- |
| Created | Local intent exists | No fill mutation |
| Risk checked | Preliminary permission was evaluated | No fill mutation; permission may be revoked before send if state changes |
| Submitted | Request crossed the future adapter boundary, or dispatch is durably recorded | No fill mutation |
| Accepted | External boundary acknowledged the request | No fill mutation |
| Partially filled | Valid execution evidence covers part of the requested quantity | Apply only validated, deduplicated filled quantity and attributable costs |
| Filled | Valid execution evidence covers the final cumulative quantity | Apply only not-yet-applied validated quantity and costs |
| Cancel requested | Cancellation was requested | Do not assume the remainder is canceled |
| Canceled | Valid evidence says the remaining quantity is canceled | Preserve prior fills; remove only confirmed remainder |
| Expired | Valid evidence says the remaining quantity expired | Preserve prior fills; remove only confirmed remainder |
| Rejected | Request or action was rejected | Do not mutate portfolio as filled |
| Timeout | Expected response was not observed within the modeled deadline | Transition to unknown/reconciliation-required, not rejected or canceled by assumption |
| Unknown | Execution or cancellation outcome cannot be established | Block risk-increasing follow-up and reconcile |
| Reconciled | Approved snapshot resolves or identifies a mismatch | Mutate only through the approved reconciliation policy and preserve an audit trail |

Required future lifecycle contracts:

- Core-generated stable internal order and logical-position identifiers.
- Separate submission result, execution event, cancel result, adapter-health event, and reconciliation snapshot.
- Explicit cumulative and incremental quantities with units.
- Idempotency/deduplication key, source sequence when available, event timestamp with units, ingestion sequence, and causality link.
- Broker-native tickets and statuses retained only as opaque boundary evidence, not TradeBot core identity or enums.

## Asynchronous Events And Reconciliation

- Out-of-order events must be buffered, rejected, or deterministically reconciled according to a documented ordering policy; arrival order alone cannot define economic order.
- Duplicate events must be detected before portfolio, risk, fee, or P&L mutation. Deduplication must survive restart through deterministic fixture/state design.
- Delayed history or deal arrival must not overwrite newer validated state or be treated as a new fill. Reconciliation compares cumulative facts and provenance.
- A missing live event followed by history/reconciliation evidence must apply only the previously unapplied economic delta.
- Stable TradeBot internal IDs must remain authoritative when broker tickets change, are replaced, or represent different external objects over an order/position lifecycle.
- Cancel/fill races resolve from execution and reconciliation evidence. Unknown cancel status is not safely canceled.
- Reconciliation mismatches must be explicit: local-only, external-only, quantity, price/cost, identity, timestamp/freshness, or unsupported-state mismatch.
- No adapter health event, reconnect, or successful snapshot may automatically clear `RiskEngine` halt or close-only state.

## Account Snapshot Model

Real account snapshots are prohibited in Phase 22 research. Future tests use synthetic fixtures with explicit units and timestamps.

| Field | Contract requirement |
| --- | --- |
| Balance | Decimal amount, currency, source timestamp, and definition used by the adapter |
| Equity | Decimal amount, currency, and freshness; required components stated |
| Realized P&L | Period/cumulative scope, currency, included costs, and reset anchor |
| Unrealized P&L | Valuation timestamp, currency, price source class, and included costs |
| Margin | Used/reserved amount with currency and source semantics |
| Free margin | Available amount with currency and source semantics |
| Margin level | Dimension and invalid/undefined behavior; no division-by-zero inference |
| Currency | Explicit account currency code; conversion sources remain a blocked decision |
| Account mode | Broker-neutral capability set plus netting/hedging evidence; no MT5 enum in core |
| Freshness timestamp | UTC/epoch value with units, source clock, ingestion time, and stale threshold from config |
| Rule-clock anchors | Synthetic policy timezone, account-day start, effective rule version, and next reset instant |

Required validity fields include snapshot ID, schema version, source class, completeness flag, missing-field list, and reconciliation status. A missing or stale field required for permissioning fails closed.

## Symbol Metadata Model

| Field | Contract requirement |
| --- | --- |
| Canonical symbol | Stable TradeBot instrument identity independent of provider spelling |
| Execution alias | External alias kept at `BrokerGateway`/adapter boundary; mapping is versioned and blocked until provider selection |
| Precision/digits | Display/validation precision; must not substitute for tick size |
| Point size | Explicit decimal increment and units |
| Tick size/value | Price increment plus monetary value, currency, direction/asymmetry if applicable, and effective time |
| Contract size | Underlying units per lot/contract with unit name |
| Minimum/maximum/step/limit volume | Separate constraints with lot/contract units and effective time |
| Execution mode | Normalized capability description; external enum remains boundary evidence |
| Supported fill policies | Normalized capability set; external policy names remain research annotations |
| Trading/session availability | Timestamped session calendar, availability state, timezone, holidays/closures provenance, and freshness |

The model must include schema version, source/effective timestamps, freshness, and a completeness flag. Metadata changes invalidate any cached quantity normalization until the new specification is validated.

## Deterministic Lot Sizing And Rounding

The future model must define the core quantity unit before conversion. Where core quantity represents base units and contract size represents base units per lot, the research formula is:

```text
raw_lots = abs(core_quantity_base_units) / contract_size_base_units_per_lot
raw_steps = raw_lots / volume_step_lots
normalized_lots = declared_rounding_policy(raw_steps) * volume_step_lots
```

This formula is invalid if the quantity and contract-size units do not match. Future contracts must record input unit, output unit, metadata version, rounding policy, and arithmetic precision.

Permitted fixture policies are explicit values such as `round_toward_zero_to_step`, `reject_unaligned`, or `nearest_step_with_declared_tie_rule`. Phase 22 research does not choose one global production policy. Silent language/runtime rounding is prohibited.

Deterministic validation sequence:

1. Reject non-finite, non-positive, unit-ambiguous, or stale-metadata inputs.
2. Compute with a specified decimal/fixed-point model; binary floating-point behavior must not be an implicit contract.
3. Apply the fixture-declared rounding policy once.
4. Validate minimum, maximum, step, and aggregate volume-limit constraints.
5. Reject zero-after-rounding and invalid volume; do not silently raise to the minimum.
6. Convert back to normalized economic quantity and compute notional, margin, exposure, and applicable synthetic prop-rule effects.
7. Require a final `RiskEngine` permission check after normalization and immediately before any future submission.
8. Persist the metadata version, policy, raw value, normalized value, and decision for deterministic replay.

`BrokerGateway` owns external quantity normalization and request shaping, but it may not bypass the final `RiskEngine` recheck or increase exposure from an earlier approval by silent rounding.

## Failure Taxonomy

| Failure class | Broker-neutral meaning | Required future disposition |
| --- | --- | --- |
| Transport failure | IPC/network/channel unavailable, interrupted, or corrupt | No new exposure; preserve request identity; reconcile if dispatch may have occurred |
| Session/auth failure | Session invalid, authorization failed, or entitlement unavailable | Fail closed; no secret logging; do not infer permanent rejection of an in-flight request |
| Terminal-permission failure | Terminal-side trading/automation permission prevents the action | Reject before dispatch when known; otherwise mark unknown and reconcile |
| Request validation failure | Local or external validation rejects fields, symbol, volume, price, or policy | Record normalized reason; no fill mutation |
| Server-side rejection | External server rejects a received request | Record rejection separately from transport/validation; no fill mutation |
| Timeout | Required response/event absent by modeled deadline | Unknown/reconciliation-required; bounded retry only with an idempotency design |
| Unknown execution state | Dispatch or execution outcome cannot be established | Block risk increase, preserve identifiers, reconcile |
| Stale event | Event is older than accepted state/freshness policy | Do not apply blindly; retain as evidence and reconcile if material |
| Malformed event | Schema, units, identifiers, quantities, prices, or timestamps are invalid | Quarantine/reject; fail closed; no state mutation |
| Reconciliation mismatch | Local and approved external snapshot disagree | Classify mismatch, halt or close-only according to approved policy, require review |

## Synthetic Prop-Account Rule Taxonomy

All Phase 22 prop-account modeling is synthetic, configurable, versioned, and provider-neutral. No threshold below is hardcoded. A future `RuleProfile` must state units, formulas, timezone, effective version, precedence, required inputs, missing-data behavior, and breach actions.

| Rule dimension | Required configurable semantics |
| --- | --- |
| Daily-loss formula | Baseline source, measurement source, comparison operator, threshold unit, intraday high/low treatment, reset behavior, and breach persistence |
| Maximum-loss formula | Initial/static baseline or dynamic baseline, amount/percentage unit, measurement source, and persistence |
| Static/trailing/high-water behavior | Static, balance-trailing, equity-trailing, end-of-day lock, intraday high-water, floor/lock-in behavior, and reset rules |
| Open-vs-closed P&L inclusion | Whether unrealized/open and realized/closed P&L are included, valuation timing, and missing-price behavior |
| Commissions/swaps/fees inclusion | Included cost categories, posting time, currency conversion, late adjustments, and reconciliation policy |
| Reset clock/timezone | IANA timezone or fixed offset policy, account-day boundary, daylight-saving treatment, holidays, and authoritative clock source |
| Exposure caps | Gross/net quantity, notional, symbol, asset group, direction, concentration, and open-plus-pending treatment |
| Margin caps | Used margin, free margin, margin level, projected margin, and missing/stale snapshot behavior |
| Session windows | Allowed entry, exit-only, forced-flat, rollover, overnight, weekend, and holiday windows |
| News windows | Event source class, affected instruments, pre/post window, revision policy, stale calendar behavior, and action |
| Restricted instruments | Versioned canonical-instrument set, alias mapping boundary, and unknown-symbol behavior |
| Restricted strategy flags | Synthetic flags for EA/automation, copying, latency/arbitrage patterns, prohibited-order behavior, or account-sharing evidence; flags are not accusations or provider claims |
| Evaluation progression | Synthetic profit target, minimum trading days, inactivity, stages, reset, scaling, consistency, payout/reward adjustment, and rule-version transition |

Synthetic breach actions are composable policy outputs:

- `warn`: record a non-permission-changing event.
- `pause`: temporarily block configured actions pending a deterministic condition or operator review.
- `flatten`: request risk-reducing closure through normal `ExecutionEngine` -> `RiskEngine` -> `BrokerGateway` flow; never assume fills.
- `cancel_pending`: request cancellations and reconcile unknown outcomes.
- `close_only`: block risk-increasing actions while permitting approved risk reduction.
- `read_only`: prohibit order/cancel mutation and permit approved observation/reconciliation only.
- `halt`: block new actions and require operator-sensitive recovery.
- `termination`: mark the synthetic program state terminated; it is not an external account action.
- `reward_adjustment`: change only the synthetic evaluation ledger under an explicit formula.

Precedence and combinations must be deterministic. More permissive actions must never override an active `halt`, `read_only`, or `close_only` state without an approved recovery transition.

## Subsystem Boundaries

Canonical future flow:

```text
strategy/allocation
  -> ExecutionEngine broker-neutral intent and canonical lifecycle
  -> RiskEngine preliminary permission
  -> BrokerGateway external translation and deterministic normalization proposal
  -> RiskEngine final permission on normalized exposure
  -> BrokerGateway request dispatch through a future adapter
  -> BrokerGateway-normalized events/reconciliation
  -> ExecutionEngine lifecycle transition
  -> validated deduplicated portfolio/risk mutation
```

### ExecutionEngine

Owns broker-neutral intent and canonical local lifecycle state, including causal association among submit, acknowledgement, execution, cancel, expiry, rejection, timeout, unknown, and reconciliation transitions. It must not own MT5-native schemas, ticket semantics, transport, credentials, or direct external side effects.

### RiskEngine

Owns permissioning, exposure governance, synthetic prop-rule evaluation, and halt/close-only/fail-closed state. It performs the final check on normalized quantity and current snapshots. It must not decode raw MT5 statuses, own symbol aliases, transport requests, or allow adapter health to clear risk state automatically.

### BrokerGateway

Owns external translation, symbol aliasing, quantity normalization, request shaping, raw-status decoding, adapter health, local-to-external ID correlation, deduplication inputs, and reconciliation import. It must expose normalized facts and proposed normalized exposure to core owners; it must not own strategy decisions, prop-rule definitions, portfolio accounting, or permission to increase risk.

### Portfolio State

Portfolio state mutates only from validated, deduplicated fill evidence or approved reconciliation evidence. Request creation, submission, acceptance, cancel request, timeout, or adapter reconnect is not fill evidence. Costs and P&L adjustments require their own deduplication and provenance.

### Replay And Simulation Adapters

Replay/simulation adapters generate deterministic, versioned event streams without external connectivity, credentials, terminal state, account state, or wall-clock-dependent outcomes. They implement the same broker-neutral event semantics without pretending to be a selected MT5 topology.

## Determinism Constraints

- Same source revision, configuration, synthetic rule profile, instrument metadata, account snapshot sequence, fixture events, and seed must produce identical lifecycle, portfolio, risk, breach, and reconciliation ledgers.
- Every event has explicit timestamp units, source time, ingestion sequence, and deterministic tie-break ordering. Rule-clock calculations use a versioned timezone/calendar policy rather than machine-local time.
- Out-of-order, duplicate, delayed, stale, and malformed behavior is part of the fixture, not dependent on thread scheduling or network timing.
- Random latency, slippage, reject, or fill generation is absent or uses a named algorithm and recorded seed.
- Decimal/quantization behavior and tie rules are explicit across platforms.
- Replays may not read endpoints, credentials, live accounts, terminal processes, ignored historical outputs, or mutable external state.
- Unknown state persists across replay/resume until a deterministic reconciliation event resolves it.
- Schema, metadata, rule-profile, and fixture versions are included in replay provenance.

## Deterministic Fixture Matrix For Future Implementation

These fixtures are specifications only; no test implementation is authorized by this document.

| Fixture | Required event shape | Required invariant |
| --- | --- | --- |
| Accepted but not filled | Submit -> accept -> no execution through fixture end | No position/P&L mutation; open lifecycle remains visible |
| Partial fills under IOC semantics | Accept -> partial execution -> canceled remainder evidence | Apply only partial quantity; cancel remainder separately; external `IOC` label stays at boundary |
| Partial fills under RETURN semantics | Accept -> partial execution -> remainder remains working -> later event | Apply only each new delta; external `RETURN` label normalizes to remainder behavior rather than a core MT5 enum |
| Canceled remainder | Partial execution -> cancel request -> cancel confirmation | Preserve fills; remove only confirmed remainder |
| Rejected request | Submit -> validation or server rejection | No fill mutation; reason class preserved |
| Out-of-order trade transactions | Later-sequence execution arrives before earlier acceptance/execution | Deterministic reorder/reconcile; economic deltas apply once |
| Duplicate transactions | Identical and semantically duplicate events arrive more than once | Portfolio, fees, risk, and P&L apply once |
| Stale history arrival | Old order/deal/history evidence arrives after newer state | No rollback/double-apply; evidence retained for reconciliation |
| Missing live event then reconciliation | Live execution omitted -> later snapshot/history shows fill | Apply only unapplied delta through approved reconciliation evidence |
| Position-ticket change with stable logical identity | External ticket changes while economic position remains logically continuous | Stable internal logical ID persists; no artificial close/open P&L |
| Fee/swap/commission P&L changes | Execution followed by one or more delayed cost postings | Each cost applies once with currency, time, and provenance; prop ledger recomputes deterministically |
| Daily-loss reset at rule-clock boundary | Events immediately before/at/after configured boundary | Exactly one reset under the profile timezone/calendar and tie rule |
| Trailing drawdown lock-in | Equity/balance path raises high-water then falls | Baseline moves only per configured rule and never relaxes unless an explicit reset allows it |
| Flatten breach action | Rule breach -> close intents -> partial/unknown fills -> reconciliation | Risk-reducing flow uses normal lifecycle; no assumed flatten success |
| Cancel-pending breach action | Rule breach -> cancel requests -> mixed cancel/fill/unknown results | Prior fills persist; unknown remainders reconcile |
| Close-only breach action | Rule breach -> new entry and reduction intents | Entry blocked; permitted reduction still passes final risk check |
| Halt breach action | Rule breach -> any new action | New actions blocked; no automatic clear |
| Read-only breach action | Rule breach -> order/cancel requests plus snapshot import | Mutating actions blocked; approved observation/reconciliation remains available |

## Required Future Tests And Replay Validation

Before any separately authorized implementation can claim the broker-neutral contracts are satisfied, tests must cover:

- Every fixture above, including restart/resume at each lifecycle state.
- Idempotency across duplicate delivery, replay restart, and reconciliation replay.
- Deterministic ordering for equal timestamps, mixed source sequences, and delayed history.
- Stable internal identity across external ticket changes and cancel/replace flows.
- Netting/hedging evidence normalization without choosing a provider mode in Phase 22 research.
- Account/symbol snapshot completeness, freshness, version changes, stale values, malformed units, and missing fields.
- Quantity conversion boundaries: below minimum, above maximum/limit, off-step, exact step, zero-after-rounding, precision/tie edges, stale metadata, and post-normalization risk rejection.
- Static, trailing, and high-water drawdown; daily reset boundaries; open/closed P&L; fees/swaps/commissions; exposure/margin/session/news/restricted-instrument/restricted-strategy dimensions.
- Fail-closed behavior for every failure-taxonomy class.
- Boundary tests proving no strategy-to-adapter path, no risk bypass, no portfolio mutation from acknowledgement, and no external I/O in deterministic replay.
- Golden replay comparison of lifecycle, position, cash, fee, P&L, risk, breach, and reconciliation ledgers.

Future source-affecting validation would include configure, build, targeted replay tests, full CTest, and new adapter/risk/replay tests defined by an approved implementation plan. Those commands are not required for this documentation-only task.

## Performance And Operational Impact

This documentation has no runtime, latency, memory, throughput, network, terminal, or account impact. It makes no performance claim.

A future implementation plan must define measurements for normalization/deduplication throughput, event-queue depth and drops, policy-evaluation latency, reconciliation duration, memory growth by open lifecycle count, and deterministic replay overhead. No threshold is selected here.

## Rollback And Non-Activation

- Rollback is a documentation-only Git revert approved by the operator: remove this file and its index/authority references or restore the prior wording.
- No terminal, account, external session, process, credential, configuration, order, position, or provider state exists to unwind from this task.
- Reverting this document must not alter runtime defaults, risk limits, source behavior, CMake, tests, data, or generated artifacts.
- If later evidence contradicts a finding, mark the evidence item superseded or conflicting and revise the research artifact; do not patch runtime behavior from research alone.

## Decisions Blocked Until Phase 23 Or Later

| Decision | Why blocked |
| --- | --- |
| Broker/provider | Phase 23 has not started and requires explicit operator selection |
| Prop firm/program/rulebook version | Provider rules are external, versioned, and not selected |
| Account type/currency/leverage/size | Requires selected program/account evidence and financial-risk approval |
| Netting versus hedging mode | Depends on selected account/server capabilities and affects identity/reconciliation design |
| Instrument universe | Requires provider/broker capability, symbol mapping, and risk approval |
| Execution/fill policy | Requires approved venue/account semantics and deterministic contract decision |
| Symbol mapping | Requires a selected provider/instrument universe; aliases remain boundary data |
| Bridge topology | Python/IPC, MQL5 EA/service, and broker/back-office classes remain research alternatives |
| MT5 terminal/build/server/deployment OS | Requires supported deployment and operational ownership decisions |
| Credentials/endpoints/certificates/account IDs | Security-sensitive and prohibited in current scope |
| Operational monitoring and incident procedure | Requires selected topology/provider, service levels, actor ownership, and operator approval |

Phase 23 may evaluate and select a broker only after explicit operator activation. It does not automatically authorize connectivity, credentials, account access, orders, or live trading; those remain subject to later phases and exact approvals.

## Research Gate Recommendation

- GO: continue broker-neutral, offline, documentation-only evidence collection using the evidence register and synthetic fixtures.
- NO-GO: C++/CMake/test implementation, MT5 connectivity or login, account access, credentials, broker/prop-firm/program selection, bridge-topology selection, real or sandbox orders, live trading, risk-default changes, or Phase 23 activation.
