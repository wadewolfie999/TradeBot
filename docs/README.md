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

- `PROJECT_STATE.md`: current verified repository state, active phase, constraints, and next safe action.
- `ROADMAP.md`: phase model, gates, validation, and current phase marker.
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

## Documentation Rules

- Keep active docs current, cohesive, and evidence-based.
- Do not promote unverified legacy claims into current state.
- Do not recreate MOP/MOR/SS artifacts.
- Keep generated outputs out of Git unless intentionally versioned and approved.
- Never document secret values, credentials, account identifiers, or live trading keys.
- Update this index when adding or removing authority documents.
