# TradeBot Actor Registry v1.0

## Purpose and Authority

- Purpose: define recognized named actors, actor identity, actor status, roles, authority boundaries, allowed and forbidden scope, evidence obligations, stop conditions, approval boundaries, actor-class mappings, onboarding, and offboarding.
- Authority level: actor governance below root `AGENTS.md`, accepted ADRs, and risk policy.
- Audience: operator, maintainers, contributors, reviewers, testers, advisors, AI agents, and named TradeBot participants.

Actors are not automatically trusted merely because they have repository access.

This document is the canonical actor-governance document for TradeBot. It does not silently elevate itself above `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, or explicit operator instruction for the current task.

## Registry Model

### Named Actor

A named actor is an actual recognized participant in TradeBot governance.

Current named actors:

- Wade
- Bigi
- ChatGPT
- Codex

### Actor Class

An actor class is a reusable governance category that defines common permissions, prohibitions, evidence requirements, escalation paths, approval requirements, onboarding, and offboarding behavior.

A named actor may map to one or more actor classes.

Actor-class membership must not grant authority beyond the named actor's canonical role card.

When a role card and generic actor class differ, the stricter applicable boundary must be preserved unless explicit project authority says otherwise.

## Registry Rules

- Only recognized actors may appear in the current actor registry.
- Registration does not itself grant repository access.
- Repository access does not itself grant authority.
- Capability does not equal approval authority.
- Role cards define named-actor boundaries.
- Actor classes provide reusable governance behavior.
- Mappings must not silently expand authority.
- Suspended or offboarded actors should not be silently deleted from governance history.
- Authority changes require explicit governance action.
- Documentation approval is not implementation approval.
- Implementation approval is not live-trading approval.
- Broker research approval is not broker-connection approval.
- Demo/sandbox approval is not real-money trading approval.
- Live trading remains disabled and unauthorized unless explicitly approved under the applicable governance and risk controls.

## Current Registered Actors

### Wade

```text
Actor:
Wade

Role:
Founder, chief executive agent, project owner, and final approval authority for TradeBot.

Authority Level:
PROJECT_OWNER_AUTHORITY

Allowed Scope:
Full project governance authority across roadmap direction, phase/workstream prioritization, approval gates, contributor and agent authorization, repository-level decisions, documentation approval, implementation approval, release-readiness review, and final operator acceptance.

Forbidden Scope:
Wade must not bypass TradeBot governance, validation evidence, phase gates, risk controls, replay-determinism requirements, review requirements, credential safety, or live-trading approval boundaries. Full authority does not convert informal approval into sufficient approval for financially sensitive work.

Primary Deliverables:
Strategic direction, phase/workstream authorization, approval/rejection decisions, operator acceptance decisions, governance rulings, contributor/agent role approval, Codex task authorization, final review of high-risk work, and future live-trading readiness decisions when explicitly reached.

Validation Evidence:
Relevant diffs, affected-file summaries, test results, build results, documentation updates, risk-impact analysis, determinism-impact analysis, validation logs, review reports, and explicit confirmation that forbidden surfaces were not touched.

Stop Conditions:
Stop or withhold approval if scope is ambiguous, phase authority is unclear, validation evidence is missing, forbidden files were touched, broker/live/MT5/credential surfaces appear unexpectedly, risk controls are weakened, replay determinism is affected without review, source changes exceed authorization, tests are skipped without acceptable justification, or an actor infers approval from prior context.

Approval Boundary:
Wade can authorize project work, phase progression, actor roles, documentation changes, implementation tasks, and acceptance decisions. Each approval must state exactly what it authorizes and does not authorize. Documentation approval does not authorize implementation. Implementation approval does not authorize live trading. Broker research approval does not authorize broker connection. Demo/sandbox approval does not authorize real-money trading. Live trading requires explicit standalone approval.

Recommended Status:
Keep

Required Amendment:
High-risk approvals must be explicit, written, scoped, and tied to a specific task, branch, phase, or workstream.
```

### Bigi

```text
Actor:
Bigi

Role:
Independent human operator-contributor supporting Wade across TradeBot strategy, research, architecture, implementation coordination, validation review, documentation, visual design, Codex/ChatGPT operation, and actor coordination.

Authority Level:
OPERATOR_AUTHORITY / WADE-SCOPED_IMPLEMENTATION_AUTHORIZED / LIVE_TRADING_PROHIBITED

Allowed Scope:
Bigi may operate broadly across approved TradeBot planning, research, architecture review, documentation, Codex prompting, ChatGPT prompting, implementation planning, implementation-diff creation, implementation-diff review, test-report review, validation-report review, visual presentations, flow diagrams/flow graphs, handoff summaries, and operator-acceptance recommendations.

Bigi may act as a broad Codex operator and broad ChatGPT operator. Bigi may prepare, send, refine, and evaluate prompts for Codex and ChatGPT across approved TradeBot tasks.

Bigi may make low-risk workflow decisions independently, including task ordering, prompt structure, documentation organization, research framing, review routing, handoff formatting, and coordination rhythm.

Bigi may create implementation diffs when the work is inside an approved project boundary, assigned workstream, or Wade-authorized task. Bigi may handle substantial module-level work when explicitly assigned by Wade.

Forbidden Scope:
Bigi may not authorize live trading, enable live trading, activate broker integrations, create or activate MT5/live broker connections, request or handle broker credentials, approve real-money deployment, bypass risk controls, weaken replay determinism, bypass validation, bypass phase gates, or reclassify Phase 22 as implementation-ready without explicit Wade/project-owner approval.

Bigi may not treat broad access, technical ability, operational trust, documentation approval, research approval, architecture discussion, implementation work, or implementation-diff creation as final approval authority.

Bigi may not independently approve merges, release readiness, production deployment, broker activation, credential use, or live execution/order-routing behavior.

Primary Deliverables:
Strategic-decision analysis for Wade; research memos; architecture reviews; Codex prompts; ChatGPT prompts; implementation plans; implementation diffs; implementation-diff reviews; test reports; validation reports; visual presentations; flow diagrams/flow graphs; handoff summaries; operator-acceptance recommendations; workstream coordination summaries.

Validation Evidence:
Bigi’s work is valid when traceable to explicit Wade direction, current TradeBot project state, approved workstream boundaries, reviewed documentation, visible diffs, test/build output, validation logs, replay-determinism checks, risk-control review, and clearly stated assumptions.

For implementation work, required evidence includes changed-file list, diff summary, test commands, test results, validation status, risk impact, replay-determinism impact, and unresolved concerns.

Insufficient evidence includes informal confidence, undocumented claims, missing diffs, missing test output, skipped validation without justification, unreviewed broker/live assumptions, undocumented phase changes, or inferred approval.

Stop Conditions:
Bigi must stop and escalate to Wade when scope is ambiguous, authority is ambiguous, phase-gate status is unclear, Phase 22 implementation is implied without authorization, broker/live/MT5 work appears, credentials are involved, execution/order-routing behavior may affect live trading, risk controls may be weakened, replay determinism may change, validation cannot be produced, source-code changes exceed the approved task boundary, or instructions conflict with current project state.

Approval Boundary:
Bigi’s output may advise, analyze, recommend, coordinate, prompt Codex, prompt ChatGPT, create implementation diffs, review implementation diffs, assess tests, assess validation, and recommend operator acceptance.

Bigi’s output does not authorize final project strategy, final merge approval, release readiness, broker activation, credential use, live trading, real-money deployment, or Phase 22 reclassification. Final approval remains with Wade unless explicitly delegated through a project-governance decision.

Recommended Status:
Keep / Expand, with task-scoped implementation authority.
```

### ChatGPT

```text
Actor:
ChatGPT

Role:
Project-context reasoning, advisory, planning, review, governance-analysis, documentation-support, handoff-preparation, and Codex-prompt-drafting actor for TradeBot.

Authority Level:
ADVISORY_CONTEXT_ACTOR / DOCS_DRAFTING_SUPPORT / CODEX_PROMPT_SUPPORT / LIVE_TRADING_PROHIBITED

Allowed Scope:
ChatGPT may analyze TradeBot context, reconstruct project state, identify risks, draft governance language, prepare handoffs, review plans and documentation, propose task sequencing, draft Codex prompts, assess scope boundaries, and support Wade/Bigi/Codex coordination.

ChatGPT may use project-only memory/context to improve continuity and accuracy, but only as contextual evidence, not as authority.

Forbidden Scope:
ChatGPT must not approve implementation, authorize live trading, bypass Wade/project-owner approval, infer authority from memory or access, reclassify Phase 22 as implementation-ready, handle broker credentials, activate broker/MT5/execution/live/prop-account workflows, expand scope without approval, or modify repository files unless explicitly instructed in a separate authorized implementation task.

Primary Deliverables:
Role cards; governance assessments; project-state summaries; risk and scope analysis; documentation drafts; task-intake prompts; Codex execution prompts; validation checklists; handoff memos; review comments; planning recommendations.

Validation Evidence:
Outputs must preserve TradeBot governance assumptions, identify phase gates and approval boundaries, distinguish research/scoping from implementation, maintain BACKTEST as default, treat live trading as disabled and unauthorized, avoid credential/live workflow handling, and provide clear validation/test/risk criteria where relevant.

Stop Conditions:
ChatGPT must stop or escalate when authority is ambiguous, phase status is unclear, allowed files are unspecified, implementation approval is missing, broker/live/credential/execution boundaries are implicated, risk impact is material, repository modification is requested without explicit authorization, or a task could bypass governance or validation.

Approval Boundary:
ChatGPT may recommend, analyze, draft, and review, but cannot approve implementation, merge changes, authorize Codex execution, authorize live trading, approve broker connectivity, or override Wade/project-owner decisions.

Memory/context improves continuity only and grants no decision authority.

Recommended Status:
Keep
```

### Codex

```text
Actor:
Codex

Role:
Bounded repository execution agent for inspection, explicitly scoped implementation, documentation, testing, validation, diffs, and Git-aware work.

Authority Level:
TASK_SCOPED_EXECUTION_AGENT / MUTATION_ONLY_WHEN_EXPLICITLY_AUTHORIZED / NO_SELF_APPROVAL / LIVE_TRADING_PROHIBITED

Allowed Scope:
Codex may perform read-only inspection and only those repository mutations explicitly authorized by a task that defines allowed files, forbidden files, deliverables, validation requirements, and stop conditions.

Codex may perform documentation, testing, validation, implementation, and Git-aware work only inside the approved task boundary.

Codex must follow the current PROJECT_STATE, accepted ADRs, approved plans, repository governance, risk policy, and phase gates.

Pre-Phase-23 evidence preparation remains documentation/evaluation-only unless explicitly authorized.

Forbidden Scope:
Codex may not self-approve, expand scope, infer authority from repository access, publish unauthorized Git changes, handle credentials, create or activate MT5/broker connections, access accounts, place sandbox or real orders, activate live trading, change risk limits, or make unauthorized execution, routing, replay-determinism, broker, live, or production-configuration changes.

Primary Deliverables:
Minimal repo-aligned changes; exact touched-file list; diff summary; commands executed; tests and results; skipped checks and reasons; scope-compliance confirmation; deviations; risks; unresolved questions; and required handoff.

Validation Evidence:
Codex must report commands executed and outcomes, scope-compliance confirmation, applicable build/test results, and tests not run with reasons.

For docs-only work, Codex must confirm no source, test, build, runtime, credential, broker, execution, live, or configuration changes.

For implementation work, Codex must report changed files, test coverage, validation results, replay-determinism impact, risk-control impact, and unresolved concerns.

Stop Conditions:
Codex must stop and escalate on ambiguous authority, ambiguous phase status, unclear file scope, unclear validation requirements, risk-control impact, replay-determinism impact, broker/live involvement, credential boundaries, unexpected repository changes, safety-relevant test failures, insufficient evidence, or required scope expansion.

Approval Boundary:
Codex may execute only inside explicit operator-approved task boundaries.

Codex cannot approve implementation scope, sensitive changes, phase transitions, broker selection, broker connection, credentials, Git publication, release readiness, sandbox trading, or live trading.

Documentation/research approval is not implementation approval. Implementation approval is not live-trading approval. Repository access is not authority.

Recommended Status:
Keep
```

## Actor-to-Class Mapping

These mappings are conservative governance references only. They must not grant authority beyond the canonical role cards above.

Wade

- Operator

Bigi

- Advisor
- Research Agent
- Implementer only within Wade-authorized or otherwise approved task boundaries

Boundary note: Bigi's implementer mapping does not grant final approval authority, live-trading authority, broker activation authority, credential authority, release-readiness authority, or merge approval.

ChatGPT

- Advisor
- Review Agent

Boundary note: ChatGPT's mappings do not grant repository mutation, implementation approval, Codex execution authorization, broker connectivity approval, or live-trading authority.

Codex

- Coding Agent
- Implementer within explicitly authorized task boundaries
- Tester when explicitly assigned

Boundary note: Codex's mappings do not grant self-approval, scope expansion, Git publication authority, broker connection authority, credential authority, sandbox trading authority, or live-trading authority.

## Common Rules for All Actors

- Follow `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, and `SECURITY.md`.
- Do not expose secrets.
- Do not infer live-trading authorization.
- Preserve unrelated work.
- Provide evidence for claims.
- Escalate financial, credential, live-execution, architecture, or destructive actions to the operator.

## Reusable Actor Classes

### Operator

- Purpose: final owner and authority.
- Default permissions: approve scope, architecture, risk changes, commits, pushes, releases, credentials, and live transitions.
- Prohibited actions: none by repository policy, but should preserve auditability and safety.
- Required evidence: review reports, verification results, risk assessments.
- Documentation responsibilities: approve or request updates to authority docs and ADRs.
- Escalation path: final decision maker.
- Approval requirements: self-approval is allowed but should be explicit for live or destructive actions.
- Onboarding: owns repository and governance.
- Offboarding: transfer repository, credentials, and approval authority intentionally.

### Maintainer

- Purpose: steward implementation quality and documentation consistency.
- Default permissions: review changes, prepare branches, maintain docs, run tests.
- Prohibited actions: live unlocks, credential changes, pushes, releases, or financial limit changes without operator approval.
- Required evidence: diffs, tests, review notes.
- Documentation responsibilities: keep docs and ADRs current.
- Escalation path: operator.
- Approval requirements: operator approval for risk-sensitive actions.
- Onboarding: read `AGENTS.md`, this file, risk/security docs, and current state.
- Offboarding: hand off active branches, plans, and credentials access status.

### Implementer

- Purpose: make approved changes.
- Default permissions: inspect, edit in scope, run local verification.
- Prohibited actions: scope expansion, commits/pushes unless approved, live-capable changes without approval.
- Required evidence: commands run, files changed, test results.
- Documentation responsibilities: update docs affected by behavior or architecture.
- Escalation path: maintainer or operator.
- Approval requirements: active plan for nontrivial or sensitive work.
- Onboarding: read scope, active plan, state, risk docs.
- Offboarding: handoff using `HANDOFF.md`.

### Contributor

- Purpose: propose improvements or fixes.
- Default permissions: make focused branches and submit reviewable changes.
- Prohibited actions: credentials, live execution, destructive Git, broad refactors without plan.
- Required evidence: tests and documentation updates.
- Documentation responsibilities: update touched areas.
- Escalation path: maintainer.
- Approval requirements: maintainer/operator review.
- Onboarding: read `CONTRIBUTING.md`.
- Offboarding: close or hand off open work.

### Reviewer

- Purpose: find bugs, safety regressions, missing evidence, and documentation drift.
- Default permissions: inspect diffs, run tests, request changes.
- Prohibited actions: modifying scope or approving live transitions without authority.
- Required evidence: file/line findings and risk rationale.
- Documentation responsibilities: flag stale docs and missing ADRs.
- Escalation path: maintainer or operator.
- Approval requirements: authority assigned by operator or maintainer.
- Onboarding: read review checklist and relevant docs.
- Offboarding: summarize unresolved findings.

### Tester

- Purpose: verify behavior and evidence.
- Default permissions: run builds, tests, benchmarks, and doc audits.
- Prohibited actions: modifying code or generated source truth unless assigned.
- Required evidence: exact commands, outputs, failures, environment.
- Documentation responsibilities: report gaps in `TESTING.md`.
- Escalation path: implementer or maintainer.
- Approval requirements: none for safe local checks.
- Onboarding: read `TESTING.md` and `BENCHMARKING.md`.
- Offboarding: provide test report.

### Advisor

- Purpose: provide strategy, architecture, research, or safety guidance.
- Default permissions: inspect and advise.
- Prohibited actions: direct mutation unless also assigned as implementer.
- Required evidence: assumptions and rationale.
- Documentation responsibilities: suggest ADRs or doc updates when advice changes direction.
- Escalation path: operator.
- Approval requirements: operator decides adoption.
- Onboarding: read current state, roadmap, and architecture.
- Offboarding: summarize advice and open risks.

### Research Agent

- Purpose: analyze strategies, data, parameters, or experiments.
- Default permissions: inspect data and generated results within approved scope.
- Prohibited actions: live execution, credential access, profitability claims without evidence, source changes unless assigned.
- Required evidence: data provenance, seeds, configs, fees/slippage assumptions.
- Documentation responsibilities: record research assumptions and limitations.
- Escalation path: maintainer or operator.
- Approval requirements: operator for financial-sensitive conclusions.
- Onboarding: read data, risk, and benchmarking docs.
- Offboarding: provide reproducible experiment handoff.

### Coding Agent

- Purpose: implement scoped changes.
- Default permissions: inspect, edit, test, and report within scope.
- Prohibited actions: same hard prohibitions in `AGENTS.md`.
- Required evidence: files inspected/changed, commands, results, risks.
- Documentation responsibilities: update docs and plans.
- Escalation path: operator.
- Approval requirements: plan approval for nontrivial work.
- Onboarding: read `AGENTS.md`, active plan, state, workflow, relevant skill.
- Offboarding: `HANDOFF.md` template.

### Review Agent

- Purpose: perform code review or governance review.
- Default permissions: inspect and report.
- Prohibited actions: mutation unless explicitly assigned.
- Required evidence: findings first, severity order, file references.
- Documentation responsibilities: flag policy conflicts.
- Escalation path: maintainer or operator.
- Approval requirements: assigned review authority.
- Onboarding: read relevant docs and diff.
- Offboarding: list unresolved questions.

### Local or Offline AI Agent

- Purpose: provide offline analysis, drafting, or review.
- Default permissions: only what the operator grants.
- Prohibited actions: assuming internet, remote, or live access; handling secrets without approval.
- Required evidence: local context and limitations.
- Documentation responsibilities: keep generated docs evidence-based.
- Escalation path: operator.
- Approval requirements: operator for mutation.
- Onboarding: read authority docs.
- Offboarding: provide handoff and limitations.

### Observer

- Purpose: read-only awareness.
- Default permissions: inspect public or granted repository material.
- Prohibited actions: mutation, credential access, live operations.
- Required evidence: none unless reporting.
- Documentation responsibilities: none.
- Escalation path: maintainer.
- Approval requirements: access controlled by operator.
- Onboarding: read security expectations.
- Offboarding: remove access when no longer needed.

## Registry Change Control

- New actors must be formally registered.
- Actor status changes must be recorded.
- Role or authority changes require explicit governance action.
- Authority increases must not be inferred from capability or access.
- Suspension or offboarding should update status rather than silently erase governance history.
- Mappings may change only through explicit approved governance updates.
- High-risk authority changes require explicit written approval.
- Change control does not create a new approval body or governance role.
