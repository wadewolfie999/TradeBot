# TradeBot Data Policy

## Purpose And Authority

- Purpose: govern historical data, sample data, generated outputs, replay files, benchmark data, credentials, provenance, and retention.
- Authority level: data governance below risk policy and architecture, above workflow convenience.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, and research agents.

## Verified Data Paths

- `data/samples/`: exists and is currently empty.
- `data/results/`: generated outputs; ignored by Git.
- `data/archive/`: ignored by Git.
- `data/historical/`: referenced by `phase18_burnin`, not present as a tracked directory at verification.
- `build/phase19_revalidation/`: ignored generated Phase 19 logs/replay artifacts observed locally.
- `/tmp`: used by tests for temporary fixtures.

## Source-Control Policy

Tracked by default:

- Documentation.
- Source code.
- Tests.
- Small, intentional sample fixtures with clear provenance.

Not tracked by default:

- `build/`.
- `data/results/`.
- `data/archive/`.
- Large historical data.
- Generated replay files.
- Local logs.
- Credentials and `.env` files.

Large files require operator review. Git LFS was not configured or verified in this repository, so do not assume LFS is available.

## Historical Data

Historical data must include:

- Source or provider.
- Symbol/instrument.
- Time range.
- Timestamp units and timezone.
- Schema.
- Known gaps or corrections.
- Checksum for large or reused datasets.
- License or usage constraints when known.

Historical data must not include account identifiers, private balances, or secrets.

## Sample Data

Sample data should be small, deterministic, and safe to commit. It should support tests or documentation. Synthetic sample data must be labeled synthetic.

## Generated Results

Generated results include analytics CSVs, latency reports, snapshots, benchmark logs, generated binary replay files, and build artifacts.

Rules:

- Keep generated results out of Git unless intentionally versioned.
- Store default generated outputs under ignored paths such as `data/results/` or `build/`.
- Do not treat generated outputs as authoritative input data without provenance review.
- Do not commit result files containing secrets, account identifiers, live balances, or venue order IDs.

## Temporary Data

Temporary test files should use `/tmp` or another isolated temporary path. Tests should not depend on persistent temporary files across runs.

## Benchmark Data

Benchmark inputs must be identified as synthetic, historical, sampled, or generated. Benchmark reports must include command line, input size, build mode, compiler, and machine context.

## Sensitive Data And Credentials

Credentials are governed by `SECURITY.md` and `RISK_POLICY.md`.

- Never store API key values, secrets, tokens, private keys, account IDs, or `.env` files in tracked data.
- Never inspect secret values unless explicitly authorized.
- Redact logs before review or publication.

## Naming Conventions

- Use descriptive symbols and time ranges for historical datasets.
- Use generated-output names that identify benchmark/test and date or phase when needed.
- Avoid names that imply live authorization unless the data truly comes from an approved live run.

## Timestamp Conventions

- Prefer epoch timestamps for machine-readable data.
- State units: seconds, milliseconds, microseconds, or nanoseconds.
- `ReplayTick::timestampNs` uses nanoseconds.
- Avoid local timezone ambiguity.

## Schemas

Schema changes require:

- Documentation update.
- Tests for parser compatibility.
- Migration or rollback note if old generated outputs are affected.

## Provenance And Checksums

For nontrivial datasets, record:

- Source.
- Acquisition method.
- Transform steps.
- Checksum.
- Validation command.
- Date obtained or generated.

## Retention And Cleanup

- Generated artifacts may be cleaned when no longer needed, but do not delete unrelated user artifacts.
- Keep evidence needed for active plans until review is complete.
- Archive only with operator intent.

## Invalid Or Corrupted Data

Invalid data must be rejected or quarantined. Do not silently repair data in a way that changes research conclusions. Record repair logic if transformation is necessary.

## Replay Compatibility

Replay compatibility changes require:

- CSV and binary tests.
- Timestamp/unit review.
- Data policy update.
- Clear statement about whether old replay files remain valid.

## Privacy And Account Data

Do not commit private account data, balances, positions from real accounts, private order history, or account identifiers. If live or sandbox account data is needed for review, redact and get operator approval.
