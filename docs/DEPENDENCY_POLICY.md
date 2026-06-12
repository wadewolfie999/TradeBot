# TradeBot Dependency Policy

## Purpose And Authority

- Purpose: govern dependency addition, update, review, and offline-first handling.
- Authority level: dependency governance below security and risk policy, above contributor convenience.
- Audience: operator, maintainers, Codex, contributors, and reviewers.

## Current Dependency Surface

- Build system: CMake.
- Language standard: C++20.
- Required library found by CMake: Threads.
- Optional dynamic-link library lookup: `dl` when available.
- No tracked package manifests for Python, Julia, Node, or other ecosystems were found.
- No tracked third-party vendored dependencies were found.

## Addition Rules

New dependencies require a plan when they affect source, tests, runtime behavior, network access, security, or build reproducibility.

Review must cover:

- Purpose.
- License.
- Maintainer/source trust.
- Native code or install scripts.
- Network behavior.
- Credential access.
- Offline availability.
- Version pinning or reproducibility.
- Security implications.
- Removal or rollback path.

## Offline-First Handling

Development may operate under intermittent global connectivity. Dependency downloads should be grouped into planned connectivity windows. Do not assume package registry or GitHub access.

## Tooling Dependencies

Formatter, linter, static-analysis, or Markdown tools must be documented before they become required. At creation time, `clang-format`, `clang-tidy`, `markdown-link-check`, `lychee`, and `markdownlint` were not available locally.

## Prohibited Dependency Changes

- Do not add dependencies that require live exchange access for normal tests.
- Do not add dependencies that collect credentials or telemetry without review.
- Do not vendor large libraries without operator approval.
- Do not add package-manager lockfiles or generated dependency artifacts without a clear ecosystem decision.
