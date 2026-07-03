# TradeBot Documentation Index

## Purpose And Authority

- Purpose: index the active TradeBot documentation system and explain how documents relate.
- Authority level: general documentation index; it does not override `AGENTS.md`, ADRs, risk policy, architecture, or active plans.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, and research agents.

This directory contains the active project documentation for TradeBot. Historical offline/intranet-era material is not part of the active workflow unless reintroduced by an accepted ADR. ADR 0001 deprecates the old MOP/MOR/SS workflow.

## Authority Order

1. Explicit operator instruction for the current task.
2. Root `../AGENTS.md`.
3. Accepted ADRs in `decisions/`.
4. `RISK_POLICY.md`.
5. `ARCHITECTURE.md`.
6. Active approved plan using `../PLANS.md`.
7. `PROJECT_STATE.md`.
8. `ROADMAP.md`.
9. `WORKFLOW.md`.
10. Skill-specific instructions in `../.agents/skills/`.
11. `../CONTRIBUTING.md`.
12. General documentation.

Financial safety and operator authority cannot be silently overridden by any document.

## Active Documents

- `PROJECT_STATE.md`: current verified repository-state summary, constraints, and next safe action; it points to the workstream map and roadmap phase authority.
- `WORKSTREAM_ARCHITECTURE.md`: conceptually accepted TradeBot Workstream Architecture v1.0 project-level map, Workstream II amendment, Phase 23 evidence rhythm, ownership, and non-authorization gates.
- `ROADMAP.md`: deterministic phase authority within the Workstreams I-VII project map, including sequence, statuses, gates, and validation boundaries.
- `ARCHITECTURE.md`: system purpose, component boundaries, data/control flow, and architectural debt.
- `RISK_POLICY.md`: financial, order-execution, data, credential, reproducibility, and AI-agent risk controls.
- `TESTING.md`: test layers, commands, fixtures, deterministic checks, and minimum evidence.
- `DATA_POLICY.md`: historical/sample/generated data governance, provenance, checksums, schemas, and retention.
- `SECURITY.md`: credential storage, `.env`, redaction, dependency, shell, network, and incident-response policy.
- `ACTORS.md`: multi-actor roles, permissions, evidence, escalation, onboarding, and offboarding.
- `WORKFLOW.md`: end-to-end task intake, planning, implementation, verification, review, handoff, and professional halt.
- `HANDOFF.md`: copy-pasteable session and actor handoff template.
- `BENCHMARKING.md`: performance evidence, benchmark commands, generated outputs, and claim rules.
- `DEPENDENCY_POLICY.md`: dependency review and offline-first dependency handling.
- `CONFIGURATION.md`: verified modes, CLI flags, env vars, paths, and configuration boundaries.
- `STYLE_GUIDE.md`: C++ and Markdown style guidance for current unconfigured tooling.
- `FAILURE_RECOVERY.md`: recovery from interrupted work, failed checks, stale state, and containment events.
- `LIVE_TRADING_READINESS.md`: explicit live-trading unlock checklist.
- `GLOSSARY.md`: shared terms for modes, actors, phases, replay, risk, and benchmarks.
- `REVIEW_CHECKLIST.md`: practical review checklist for source, docs, risk, tests, data, and security.
- `RELEASE_POLICY.md`: commit, push, release, and live-transition gates.
- `WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`: approved Phase 21 broker-neutral integration architecture and subsystem boundaries.
- `WORKSTREAM_I_ADAPTER_CONTRACT.md`: approved Phase 21 adapter lifecycle, event, and ownership contract.
- `WORKSTREAM_I_RISK_MATRIX.md`: approved Workstream I subsystem risk classifications, controls, and gates.
- `WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`: approved deterministic replay and broker-event simulation compatibility gates.
- `PHASE22_OFFLINE_MT5_PROP_RESEARCH.md`: broker-neutral Phase 22 offline research artifact for MT5 integration surfaces, synthetic prop-account rules, deterministic fixtures, and non-activation gates; it does not authorize implementation or connectivity.
- `decisions/`: permanent architecture decision records.

Root-level documents:

- `../AGENTS.md`: primary Codex and agent operating contract.
- `../PLANS.md`: planning system and plan schema.
- `../CONTRIBUTING.md`: contributor workflow.

Codex skills:

Local skill folders contain `SKILL.md` workflow instructions:

- `../.agents/skills/tradebot-repo-inspection/`
- `../.agents/skills/tradebot-plan-authoring/`
- `../.agents/skills/tradebot-cpp-build-test/`
- `../.agents/skills/tradebot-l2-orderbook-review/`
- `../.agents/skills/tradebot-market-replay-validation/`
- `../.agents/skills/tradebot-repo-hygiene/`
- `../.agents/skills/tradebot-documentation-sync/`
- `../.agents/skills/tradebot-git-safety/`
- `../.agents/skills/tradebot-handoff/`
- `../.agents/skills/tradebot-risk-review/`
- `../.agents/skills/tradebot-authority-state-audit/`
- `../.agents/skills/tradebot-phase-gate-audit/`
- `../.agents/skills/tradebot-agent-loop-control/`
- `../.agents/skills/tradebot-adr-review/`
- `../.agents/skills/tradebot-pr-readiness-review/`
- `../.agents/skills/tradebot-integration-architecture-review/`
- `../.agents/skills/tradebot-execution-pipeline-validation/`
- `../.agents/skills/tradebot-network-live-boundary-review/`
- `../.agents/skills/tradebot-performance-review/`
- `../.agents/skills/tradebot-benchmark-review/`

Standard skill workflows:

- Governance/documentation workflow: `tradebot-git-safety`, `tradebot-repo-hygiene`, `tradebot-authority-state-audit`, `tradebot-phase-gate-audit`, `tradebot-adr-review` when ADRs are touched, `tradebot-documentation-sync`, `tradebot-pr-readiness-review`, `tradebot-handoff`.
- Pre-Workstream-II authority workflow: `tradebot-git-safety`, `tradebot-repo-hygiene`, `tradebot-authority-state-audit`, `tradebot-phase-gate-audit`, `tradebot-documentation-sync`, `tradebot-pr-readiness-review`, `tradebot-handoff`.
- Implementation workflow: `tradebot-git-safety`, `tradebot-repo-hygiene`, `tradebot-plan-authoring`, relevant architecture/risk skill, implementation, `tradebot-cpp-build-test`, `tradebot-market-replay-validation`, `tradebot-l2-orderbook-review` when L2/order-book behavior is touched, `tradebot-execution-pipeline-validation` when order lifecycle is touched, `tradebot-network-live-boundary-review` when live/network/auth/broker boundaries are touched, `tradebot-performance-review` and `tradebot-benchmark-review` when performance is claimed, `tradebot-pr-readiness-review`, `tradebot-handoff`.
- Agent-loop recovery workflow: `tradebot-agent-loop-control`, `tradebot-git-safety`, `tradebot-authority-state-audit`, one bounded corrective task, then stop and report.

Skill workflow rules:

- `tradebot-documentation-sync` defers to `tradebot-authority-state-audit` when current state is uncertain.
- `tradebot-plan-authoring` defers to `tradebot-phase-gate-audit` before Workstream II/Phase 23 planning.
- `tradebot-risk-review` cross-references network/live boundary, execution pipeline, and integration architecture reviews for those risk classes.
- `tradebot-repo-hygiene` and `tradebot-git-safety` remain first-line checks for worktree and generated-artifact safety.
- `tradebot-handoff` is the final step after multi-skill workflows.
- `tradebot-performance-review` interprets performance-change risk; `tradebot-benchmark-review` validates benchmark measurement quality.
- `tradebot-pr-readiness-review` verifies merge readiness after required specialist reviews; it does not replace them.
- Accepted ADRs and approved Phase 21 artifacts did not independently authorize Phase 22 implementation. Bounded broker-neutral implementation is complete and accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`; broker-dependent and live work remain Blocked / NO-GO.

## Documentation Rules

- Keep active docs current, cohesive, and evidence-based.
- Do not promote unverified legacy claims into current state.
- Do not recreate MOP/MOR/SS artifacts.
- Keep generated outputs out of Git unless intentionally versioned and approved.
- Never document secret values, credentials, account identifiers, or live trading keys.
- Update this index when adding or removing authority documents.
