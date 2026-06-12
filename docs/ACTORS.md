# TradeBot Actor Governance

## Purpose And Authority

- Purpose: define actor classes, permissions, evidence requirements, escalation, onboarding, and offboarding.
- Authority level: actor governance below root `AGENTS.md`, accepted ADRs, and risk policy.
- Audience: operator, maintainers, contributors, reviewers, testers, advisors, and AI agents.

Actors are not automatically trusted merely because they have repository access.

## Common Rules For All Actors

- Follow `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, and `SECURITY.md`.
- Do not expose secrets.
- Do not infer live-trading authorization.
- Preserve unrelated work.
- Provide evidence for claims.
- Escalate financial, credential, live-execution, architecture, or destructive actions to the operator.

## Operator

- Purpose: final owner and authority.
- Default permissions: approve scope, architecture, risk changes, commits, pushes, releases, credentials, and live transitions.
- Prohibited actions: none by repository policy, but should preserve auditability and safety.
- Required evidence: review reports, verification results, risk assessments.
- Documentation responsibilities: approve or request updates to authority docs and ADRs.
- Escalation path: final decision maker.
- Approval requirements: self-approval is allowed but should be explicit for live or destructive actions.
- Onboarding: owns repository and governance.
- Offboarding: transfer repository, credentials, and approval authority intentionally.

## Maintainer

- Purpose: steward implementation quality and documentation consistency.
- Default permissions: review changes, prepare branches, maintain docs, run tests.
- Prohibited actions: live unlocks, credential changes, pushes, releases, or financial limit changes without operator approval.
- Required evidence: diffs, tests, review notes.
- Documentation responsibilities: keep docs and ADRs current.
- Escalation path: operator.
- Approval requirements: operator approval for risk-sensitive actions.
- Onboarding: read `AGENTS.md`, this file, risk/security docs, and current state.
- Offboarding: hand off active branches, plans, and credentials access status.

## Implementer

- Purpose: make approved changes.
- Default permissions: inspect, edit in scope, run local verification.
- Prohibited actions: scope expansion, commits/pushes unless approved, live-capable changes without approval.
- Required evidence: commands run, files changed, test results.
- Documentation responsibilities: update docs affected by behavior or architecture.
- Escalation path: maintainer or operator.
- Approval requirements: active plan for nontrivial or sensitive work.
- Onboarding: read scope, active plan, state, risk docs.
- Offboarding: handoff using `HANDOFF.md`.

## Contributor

- Purpose: propose improvements or fixes.
- Default permissions: make focused branches and submit reviewable changes.
- Prohibited actions: credentials, live execution, destructive Git, broad refactors without plan.
- Required evidence: tests and documentation updates.
- Documentation responsibilities: update touched areas.
- Escalation path: maintainer.
- Approval requirements: maintainer/operator review.
- Onboarding: read `CONTRIBUTING.md`.
- Offboarding: close or hand off open work.

## Reviewer

- Purpose: find bugs, safety regressions, missing evidence, and documentation drift.
- Default permissions: inspect diffs, run tests, request changes.
- Prohibited actions: modifying scope or approving live transitions without authority.
- Required evidence: file/line findings and risk rationale.
- Documentation responsibilities: flag stale docs and missing ADRs.
- Escalation path: maintainer or operator.
- Approval requirements: authority assigned by operator or maintainer.
- Onboarding: read review checklist and relevant docs.
- Offboarding: summarize unresolved findings.

## Tester

- Purpose: verify behavior and evidence.
- Default permissions: run builds, tests, benchmarks, and doc audits.
- Prohibited actions: modifying code or generated source truth unless assigned.
- Required evidence: exact commands, outputs, failures, environment.
- Documentation responsibilities: report gaps in `TESTING.md`.
- Escalation path: implementer or maintainer.
- Approval requirements: none for safe local checks.
- Onboarding: read `TESTING.md` and `BENCHMARKING.md`.
- Offboarding: provide test report.

## Advisor

- Purpose: provide strategy, architecture, research, or safety guidance.
- Default permissions: inspect and advise.
- Prohibited actions: direct mutation unless also assigned as implementer.
- Required evidence: assumptions and rationale.
- Documentation responsibilities: suggest ADRs or doc updates when advice changes direction.
- Escalation path: operator.
- Approval requirements: operator decides adoption.
- Onboarding: read current state, roadmap, and architecture.
- Offboarding: summarize advice and open risks.

## Research Agent

- Purpose: analyze strategies, data, parameters, or experiments.
- Default permissions: inspect data and generated results within approved scope.
- Prohibited actions: live execution, credential access, profitability claims without evidence, source changes unless assigned.
- Required evidence: data provenance, seeds, configs, fees/slippage assumptions.
- Documentation responsibilities: record research assumptions and limitations.
- Escalation path: maintainer or operator.
- Approval requirements: operator for financial-sensitive conclusions.
- Onboarding: read data, risk, and benchmarking docs.
- Offboarding: provide reproducible experiment handoff.

## Coding Agent

- Purpose: implement scoped changes.
- Default permissions: inspect, edit, test, and report within scope.
- Prohibited actions: same hard prohibitions in `AGENTS.md`.
- Required evidence: files inspected/changed, commands, results, risks.
- Documentation responsibilities: update docs and plans.
- Escalation path: operator.
- Approval requirements: plan approval for nontrivial work.
- Onboarding: read `AGENTS.md`, active plan, state, workflow, relevant skill.
- Offboarding: `HANDOFF.md` template.

## Review Agent

- Purpose: perform code review or governance review.
- Default permissions: inspect and report.
- Prohibited actions: mutation unless explicitly assigned.
- Required evidence: findings first, severity order, file references.
- Documentation responsibilities: flag policy conflicts.
- Escalation path: maintainer or operator.
- Approval requirements: assigned review authority.
- Onboarding: read relevant docs and diff.
- Offboarding: list unresolved questions.

## Local Or Offline AI Agent

- Purpose: provide offline analysis, drafting, or review.
- Default permissions: only what the operator grants.
- Prohibited actions: assuming internet, remote, or live access; handling secrets without approval.
- Required evidence: local context and limitations.
- Documentation responsibilities: keep generated docs evidence-based.
- Escalation path: operator.
- Approval requirements: operator for mutation.
- Onboarding: read authority docs.
- Offboarding: provide handoff and limitations.

## Observer

- Purpose: read-only awareness.
- Default permissions: inspect public or granted repository material.
- Prohibited actions: mutation, credential access, live operations.
- Required evidence: none unless reporting.
- Documentation responsibilities: none.
- Escalation path: maintainer.
- Approval requirements: access controlled by operator.
- Onboarding: read security expectations.
- Offboarding: remove access when no longer needed.
