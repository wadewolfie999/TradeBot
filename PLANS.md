# TradeBot Planning System

## Purpose And Authority

- Purpose: define when and how implementation plans are authored, approved, executed, resumed, closed, abandoned, or superseded.
- Authority level: active approved plans rank below `docs/ARCHITECTURE.md` and above `docs/PROJECT_STATE.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.
- Related documents: `AGENTS.md`, `docs/WORKFLOW.md`, `docs/HANDOFF.md`, `docs/ROADMAP.md`, and `docs/decisions/`.

TradeBot uses plans to prevent scope drift, preserve architectural continuity, and make safety-sensitive work resumable across sessions and actors.

## When A Plan Is Required

A plan is required before implementation when work involves any of the following:

- Source behavior changes.
- Architecture or subsystem-boundary changes.
- Financial, order-execution, risk-limit, credential, or live-capable behavior.
- Data schema, replay semantics, timestamp handling, or generated-output semantics.
- New dependencies or toolchain changes.
- Performance claims or benchmark-driven optimization.
- Multi-file changes with unclear sequencing.
- Work that may span sessions or require handoff.
- Any task where rollback or acceptance criteria are nontrivial.

## When A Direct Patch Is Acceptable

A direct patch is acceptable when all are true:

- Scope is small and clear.
- Risk classification is low.
- The change is documentation-only or a narrow correction.
- No public interface, runtime mode, financial behavior, credential handling, or architecture boundary changes.
- Verification is obvious and local.
- No active plan or ADR is contradicted.

Even direct patches must inspect state before mutation and report verification.

## Plan Identifiers

Use stable IDs:

```text
PLAN-YYYYMMDD-short-topic
```

Examples of valid shapes:

- `PLAN-20260606-phase19-revalidation-docs`
- `PLAN-20260606-l2-applybbo-benchmark`

Do not reuse IDs. If a plan is superseded, create a new ID and cross-reference the prior plan.

## Ownership And Review

- Owner: the operator or maintainer accountable for outcome and approvals.
- Implementer: the human or agent making changes.
- Review authority: the operator or designated reviewer who accepts evidence.
- Codex may draft plans and implement approved plans, but does not self-approve financial, architectural, live-trading, commit, push, or release decisions.

## Lifecycle States

- Draft: being written or refined.
- Proposed: ready for operator or review-authority approval.
- Approved: the plan is accepted for its stated purpose; implementation may begin only when the related roadmap phase is authorized and all required operator, risk, and phase gates are satisfied.
- In Progress: implementation has started.
- Blocked: progress requires unavailable evidence, approval, access, or dependency.
- Verifying: implementation is complete and checks are running.
- Complete: acceptance criteria and closure evidence are satisfied.
- Superseded: replaced by another plan.
- Abandoned: intentionally stopped without completion.

## Planning Rules

Every plan must state:

- Objective and success criteria.
- Scope and out-of-scope boundaries.
- Dependencies and preconditions.
- Assumptions and invariants.
- Expected files or subsystems to change.
- Implementation steps.
- Verification strategy.
- Rollback strategy.
- Risks and decision points.
- Progress log and deviations.
- Completion evidence.

Plans must not invent repository facts. If evidence is missing, record the uncertainty and define the inspection needed to resolve it.

Plan approval, phase approval, and ADR acceptance are distinct. None independently authorizes a blocked phase, source implementation, broker selection, documentation-platform selection, credentials, or live trading.

## Deviation Handling

If implementation discovers a material mismatch with the plan:

- Stop changing affected areas.
- Record the deviation.
- Inspect enough context to understand impact.
- If the deviation affects scope, architecture, financial safety, credentials, tests, or data semantics, request operator approval before proceeding.
- If the deviation is minor and still inside scope, document it in the progress log and final report.

## Session Resumption

Before resuming an active plan:

```sh
git status --short
git branch --show-current
git log --oneline --decorate -n 5
```

Then compare the plan with:

- Current `docs/PROJECT_STATE.md`.
- Relevant ADRs.
- Changed files and untracked files.
- Previous handoff in `docs/HANDOFF.md` format, if present.

Resume only from a verified state. If state is ambiguous, create a handoff-style assessment before further mutation.

## Closure Requirements

A plan may be closed only after:

- Scope is implemented or explicitly reduced by approval.
- Acceptance criteria are satisfied.
- Verification commands and outputs are recorded.
- Deviations and unresolved risks are recorded.
- Files changed are listed.
- Documentation updates are complete.
- Rollback path remains understandable.
- Final outcome states whether the plan is complete, superseded, or abandoned.

## Abandoned Or Superseded Plans

Do not delete abandoned or superseded plans if they contain decision-relevant history. Mark status clearly and link the replacement plan or reason for abandonment. Execution history belongs primarily in Git commits, pull requests, issues, and handoffs, not in a growing diary inside `docs/PROJECT_STATE.md`.

## Plan Schema

```markdown
# Plan: <title>

- Plan ID:
- Status:
- Owner:
- Implementer:
- Review authority:
- Related roadmap phase:
- Related issue or decision:
- Created:
- Updated:

## Objective

## Context

## Scope

## Out of Scope

## Preconditions

## Assumptions

## Invariants

## Files Expected to Change

## Implementation Steps

## Verification

## Risks

## Rollback

## Progress Log

## Deviations

## Completion Evidence

## Final Outcome
```

# Completed Plan: Phase 22 Execution Adapter Re-Scope

- Plan ID: `PLAN-20260621-phase22-execution-adapter-rescope`
- Status: Complete — Approved for Documentation/Scoping Only
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness
- Related issue or decision: ADR 0003 and the operator directive dated 2026-06-21
- Created: 2026-06-21
- Updated: 2026-06-21

## Objective

Replace the obsolete Phase 22 venue target with broker-neutral execution-adapter alignment and deterministic MT5/prop-account readiness contracts. Preserve Phase 22 implementation as Blocked / NO-GO and preserve live trading as unauthorized.

## Context

Phase 21 is Complete — Approved and ADR 0003 is Accepted. Those approvals establish broker-neutral architecture but do not authorize Phase 22 implementation. The operator approved this documentation/scoping re-scope and explicitly withheld authorization for source changes, MT5 connectivity, account access, credentials, broker-specific implementation, and live trading.

## Scope

- Synchronize the Phase 22 title, objective, boundaries, and gate in `docs/ROADMAP.md` and `docs/PROJECT_STATE.md`.
- Define broker-neutral contracts for order lifecycle, account/equity snapshots, position reconciliation, symbol metadata, lot sizing, execution-result mapping, and failure handling.
- Preserve deterministic replay and simulation requirements for all future contract validation.
- Define configurable prop-account risk-rule modeling requirements without selecting a prop firm or inventing account-specific thresholds.
- Permit offline MT5/prop-account compatibility research only.

## Out of Scope

- C++ source, tests, CMake, ADR 0003, broker code, credentials, risk limits, live configuration, or implementation files.
- Broker selection, prop-firm selection, or Workstream II Phase 23 decisions.
- MT5 terminal bridges, platform APIs, broker APIs, network connectivity, account access, or real order routing.
- Live trading, account mutation, secret inspection, or risk-control bypass.

## Preconditions

- Work begins from a clean `main` branch on a dedicated documentation branch.
- Phase 21 remains Complete — Approved.
- ADR 0003 remains Accepted.
- Phase 22 implementation and live trading remain Blocked / NO-GO.

## Assumptions

- MT5/prop-account readiness is a downstream compatibility target, not a broker or prop-firm selection.
- Offline research may identify contract requirements but cannot authorize connectivity or implementation.
- No firm-specific risk threshold is authoritative without a later explicit operator decision and evidence.

## Invariants

- Strategy and allocation logic remain independent of broker and platform schemas.
- New exposure remains gated through `ExecutionEngine` and `RiskEngine` before `BrokerGateway` submission.
- Future adapters attach below `BrokerGateway`; they do not bypass it.
- Portfolio and risk state change only from confirmed fill or reconciliation evidence.
- `BACKTEST` remains deterministic and independent of network, terminal, account, credential, wall-clock, and live endpoint state.
- Prop-account constraints may tighten existing risk controls but must never weaken them.

## Files Expected to Change

- `PLANS.md`
- `docs/ROADMAP.md`
- `docs/PROJECT_STATE.md`
- `docs/README.md` only if an index or authority cross-reference requires synchronization

## Implementation Steps

1. Verify Git state and current Phase 21, ADR 0003, Phase 22, and live-trading authority.
2. Replace obsolete Phase 22 venue wording with the approved broker-neutral title and scope.
3. Record MT5/prop-account readiness as a downstream compatibility target without selecting a broker, prop firm, connection method, or account.
4. Record the required broker-neutral execution, reconciliation, symbol, sizing, result, failure, replay, and prop-risk contracts.
5. Preserve explicit implementation, connectivity, credential, account-access, and live-trading NO-GO gates.
6. Run documentation consistency, obsolete-reference, phase-state, ADR-status, and live-status validation.

## Verification

Documentation-only verification:

```sh
git status --short
git diff --name-status
git diff --stat
git diff --check
rg -n -i 'Nobi''tex' docs/ROADMAP.md docs/PROJECT_STATE.md PLANS.md docs/README.md
rg -n "Phase 21|Phase 22|ADR[- ]?0003|ADR 0003|Accepted|Blocked / NO-GO|live trading|MT5|prop-account" docs/ROADMAP.md docs/PROJECT_STATE.md PLANS.md docs/README.md docs/decisions/README.md docs/decisions/0003-workstream-i-integration-architecture.md docs/RISK_POLICY.md docs/LIVE_TRADING_READINESS.md
```

CMake and CTest are not required because this plan changes documentation only and does not alter source behavior or verified build/test commands.

## Risks

- MT5 readiness language could be misread as connectivity or implementation authorization.
- Prop-account readiness could be misread as selecting a prop firm or accepting unverified risk thresholds.
- A platform-specific contract could leak into core strategy, allocation, replay, analytics, portfolio, execution, or risk code.
- Phase 22 scoping could be mistaken for phase activation.

## Rollback

Revert only this documentation re-scope with operator approval. ADR 0003, Phase 21 artifacts, source, tests, CMake, credentials, broker code, risk limits, and live configuration remain unchanged.

## Progress Log

- 2026-06-21: Operator approved the Phase 22 target re-scope for documentation and scoping only.
- 2026-06-21: Created `docs/phase22-execution-adapter-rescope` from clean `main` at `c1df5ee`.
- 2026-06-21: Synchronized roadmap, project-state, and planning authority; `docs/README.md` required no index change.

## Deviations

None currently recorded.

## Completion Evidence

Completed on 2026-06-21 with documentation-only validation:

- `git diff --check` passed with no whitespace errors.
- The obsolete Phase 22 venue-reference scan returned no matches.
- Phase 21 remains Complete — Approved and ADR 0003 remains Accepted.
- Phase 22 implementation remains Blocked / NO-GO.
- MT5 connectivity, account access, credentials, real order routing, and live trading remain unauthorized.
- Placeholder audit returned no hits.
- Documentation and skill inventories completed.
- CMake and CTest were skipped because source, tests, CMake, runtime behavior, and verified commands were unchanged.

## Final Outcome

Phase 22 is re-scoped to broker-neutral execution-adapter alignment with MT5/prop-account readiness as the downstream compatibility target. Documentation/scoping and offline research are authorized; implementation, MT5 connectivity, account access, credentials, real order routing, and live trading remain Blocked / NO-GO.

# Historical Completed Plan: Workstream I Integration Alignment

- Plan ID: `PLAN-20260618-workstream-i-integration-alignment`
- Status: Complete — Approved
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Phase 21 Infrastructure Alignment
- Related issue or decision: `docs/decisions/0003-workstream-i-integration-architecture.md`
- Created: 2026-06-18
- Updated: 2026-06-19

## Objective

Construct the missing Phase 21 plan layer for Workstream I: broker-neutral integration architecture between the deterministic TradeBot core and future broker or exchange interfaces.

## Context

Phase 18 and Phase 19 continuity around local replay, L2 order-book behavior, trigger-order BBO inputs, and `applyBbo` validation is treated as baseline evidence. No Phase 22 implementation is authorized by this plan.

## Scope

- Create the Workstream I integration architecture document.
- Create the Workstream I adapter contract document.
- Create the Workstream I risk matrix.
- Create the Workstream I replay compatibility checklist.
- Create an ADR for broker-neutral integration architecture.
- Update documentation indexes.

## Out of Scope

- C++ source edits.
- Broker-specific APIs, endpoints, schemas, or credentials.
- Live trading or real order routing.
- Risk-limit, position-sizing, drawdown, halt, close-only, fee, or slippage changes.
- Generated-data cleanup or benchmark-result publication.
- Phase 22 implementation.

## Preconditions

- Operator approved Phase 21 plan execution.
- Repository state was inspected before mutation.
- Work is documentation-only.

## Assumptions

- Phase 18/19 L2 validation is accepted as continuity evidence by the operator.
- Phase 21 artifacts are allowed to describe future Phase 22 constraints without implementing them.
- `BACKTEST` remains the deterministic default.
- `LIVE` remains prohibited without explicit future operator authorization.

## Invariants

- Strategies must not directly submit broker orders.
- New exposure must pass through `ExecutionEngine` and `RiskEngine`.
- Broker-facing behavior remains behind `BrokerGateway`.
- Credentials remain behind `AuthManager` and `SystemConfig`.
- Replay and backtest behavior must not depend on external network, broker, credential, or wall-clock state.

## Files Expected to Change

- `PLANS.md`
- `docs/README.md`
- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`
- `docs/decisions/README.md`
- `docs/decisions/0003-workstream-i-integration-architecture.md`

## Implementation Steps

1. Inspect repository state, documentation indexes, planning schema, and ADR template.
2. Draft Phase 21 integration architecture from verified repository boundaries only.
3. Draft broker-neutral adapter contract without external API assumptions.
4. Draft subsystem risk matrix and replay compatibility checklist.
5. Draft ADR 0003 for operator disposition.
6. Update documentation indexes and this active plan.
7. Run documentation validation checks.

## Verification

Documentation-only verification:

```sh
git diff --check
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

CMake and CTest are not required because this plan changes documentation only and does not alter verified commands or source behavior.

## Risks

- Documentation can drift if Phase 22 implementation starts before Phase 21 artifacts are reviewed.
- Broker-neutral contracts may need future refinement when a specific broker or exchange is approved.
- At plan execution time, the checked-out branch was `main`, while project-state docs still referenced Phase 19 context.

## Rollback

Revert the documentation-only changes in Git or delete the newly added Workstream I artifacts and remove their index entries, with operator approval.

## Progress Log

- 2026-06-18: Operator approved Phase 21 plan execution and prohibited Phase 22 implementation.
- 2026-06-18: Phase 21 documentation artifacts and ADR were drafted.
- 2026-06-19: Operator accepted ADR 0003, approved Phase 21 Workstream I artifacts, and kept Phase 22 Blocked / NO-GO.

## Deviations

None currently recorded.

## Completion Evidence

Completed on 2026-06-18 with documentation-only validation:

```sh
git diff --check
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

Results:

- `git diff --check` passed with no whitespace errors.
- Documentation and skill inventories completed.
- Placeholder audit returned no hits.
- Risk-term audit returned expected safety-policy and Workstream I references; no secret values were printed.
- CMake and CTest were skipped because this plan changed documentation only and did not alter source behavior or verified commands.

## Final Outcome

Phase 21 plan-layer artifacts are complete and approved. ADR 0003 is accepted. Phase 22 implementation remains Blocked / NO-GO and is ready for scoping only.
