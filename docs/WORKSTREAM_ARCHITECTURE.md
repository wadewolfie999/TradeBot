# TradeBot Workstream Architecture v1.0

## Purpose And Authority

- Purpose: define the current project-level workstream map, ownership, and coordination model.
- Authority level: accepted project planning and governance map below repository architecture and risk policy; phase status and gates remain authoritative in `ROADMAP.md`.
- Status: Conceptually Accepted for documentation, planning, and evaluation coordination.
- Audience: operator, maintainers, Codex, technical evidence owners, reviewers, and research agents.

This architecture defines domains and coordination boundaries. It does not authorize source implementation, broker selection, broker connection, external broker calls, credential handling, sandbox orders, live-account setup or action, risk-limit changes, or live trading. Workstream activation never overrides phase gates, plans, ADRs, risk policy, or explicit operator approval requirements.

## Current Workstream Map

| Workstream | Domain | Current position |
| --- | --- | --- |
| Workstream I — Broker-Neutral Execution Foundation | Deterministic broker-neutral contracts, lifecycle, execution/risk alignment, replay, persistence, and local simulation foundation. | Complete — Accepted through Phase 22. |
| Workstream II — Broker Integration Program | Broker-selection evidence, adapter-fit analysis, failure semantics, and future broker-hosted validation/readiness paths. | Next active strategic area; evidence/evaluation coordination only. Phase 23 remains the next formal phase and no implementation is authorized. |
| Workstream III — Documentation & Knowledge Architecture | Documentation platform, information architecture, canonical knowledge, and maintenance workflows. | Parallel/future domain unless separately activated. |
| Workstream IV — ML Optimization & Strategy Research | Reproducible offline optimization, ML-assisted research, and strategy evidence. | Parallel/future domain unless separately activated. |
| Workstream V — Core Platform Enhancement | Broker-neutral core capability, reliability, maintainability, and performance improvements. | Parallel/future domain unless separately activated. |
| Workstream VI — Production Governance & Live Readiness | Operational controls, monitoring, release governance, and readiness evidence required before any live consideration. | Parallel/future domain unless separately activated; live trading remains unauthorized. |
| Workstream VII — Strategic Expansion Alternatives | Alternative venues, products, deployment models, and longer-horizon options. | Parallel/future domain unless separately activated. |

Workstreams III–VII may be researched or planned only within separately authorized scope. Their presence in this map is not activation, sequencing authority, or permission to implement.

## Workstream II Amendment

Workstream II contains two distinct future-facing paths:

1. Demo/Sandbox environment setup path
   - Purpose: prepare evidence and requirements for a future broker-hosted non-live validation environment.
   - Current authorization: documentation, evaluation criteria, adapter-fit evidence, and failure-mode analysis only.
   - Prohibited now: broker connection, external broker calls, credential use, account access, and sandbox order placement.

2. Live Account readiness path
   - Purpose: prepare governance, safety, monitoring, rollback, and approval requirements for possible future live-account consideration.
   - Current authorization: readiness analysis and checklist preparation only.
   - Prohibited now: live-account setup or action, credential handling, broker connection, live deployment, and live trading.

The paths are separate. Demo/sandbox evidence cannot be treated as live approval, and live-readiness preparation cannot create or mutate an account. Live deployment remains blocked until the operator grants exact live-readiness approval under `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`.

## Phase 23 Operating Rhythm

The primary rhythm is Strategy 2 — Parallel Evidence Lanes With Wade Checkpoints.

- Wade owns authority, scope, gates, the broker-selection decision, and acceptance.
- Bigi owns technical evidence, the adapter-fit audit, the failure-mode checklist, and demo/live semantics analysis.
- ChatGPT/review assistant supports prompt structure, output review, and governance-drift detection.

This rhythm governs strategic evidence preparation for the next Phase 23 decision. It does not start broker-dependent implementation, select a broker, authorize connection work, or open Phase 24.

Strategy 3 gate labels may be added later as a governance overlay. A full Kanban system is intentionally not part of v1.0.

## Decision And Gate Rules

- Phase 22 remains Complete — Accepted at the Workstream I broker-neutral boundary.
- Workstream II is strategic/evaluation-active only; Phase 23 remains the next formal broker-selection/evaluation phase.
- Only Wade may accept the evidence and make an explicit broker-selection decision.
- Phase 24 remains Blocked until Phase 23 records a selection and Wade separately approves connection scope.
- Workstream III–VII status changes require separate activation.
- No evidence lane may use credentials, call a broker, connect an account, place sandbox or real orders, or enable live behavior.
- No workstream label, checkpoint, artifact, or acceptance statement authorizes live trading.

## Professional Halting Point

Stop at a Wade checkpoint after the scoped evidence package is assembled. Continue only with an explicit operator decision that names the next documentation/evaluation scope. Implementation, connection, credential, account, sandbox-order, and live actions remain outside the current authorization.
