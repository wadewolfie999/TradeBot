# TradeBot Security Policy

## Purpose And Authority

- Purpose: govern credential handling, secret storage, dependency review, logging, shell safety, network services, and incident response.
- Authority level: security policy below risk policy where financial risk is involved, above workflow and contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, and AI agents.

## Credential Storage

- Do not commit credentials.
- Do not write credentials into docs, logs, tests, generated outputs, or chat.
- Prefer environment variables, OS keychain, or a local secret manager.
- `.env` files are ignored and must remain local.
- Any `.env.example` file must contain names and safe instructions only, never values.

Verified credential env names:

- `AIIO_API_KEY`
- `AIIO_API_SECRET`

## `.env` Handling

- Agents must not open or inspect `.env` values unless explicitly authorized.
- Agents must not modify `.env` files unless explicitly authorized.
- If a task requires checking whether an env file exists, report only existence and path sensitivity, not contents.

## Least Privilege

- Exchange or broker keys must not have withdrawal permissions for TradeBot testing.
- Use read-only or sandbox credentials when possible.
- Scope keys by venue, environment, and task.
- Rotate keys after exposure or suspected exposure.

## Log Redaction

- Logs must not contain secret values, raw auth headers, private keys, account IDs, or private balances.
- Use redacted representations such as first/last characters only when necessary.
- Generated logs under `build/` or `data/results/` must be reviewed before sharing.

## API Keys And Exchange Keys

- Documentation may mention variable names but not values.
- Do not paste keys into prompts.
- Do not use live exchange keys for tests.
- Do not infer authorization from credentials being configured.

## Source-Control Exclusions

`.gitignore` excludes:

- `.env`
- `.env.*`
- `*.pem`
- `*.key`
- `*.crt`
- `build/`
- `data/results/`
- `data/archive/`

Certificate files are ignored by default. If test certificates need to be versioned in the future, operator approval and clear test-only labeling are required.

## Dependency Review

New dependencies must be reviewed for:

- Source and maintainer trust.
- License.
- Network behavior.
- Native code or post-install scripts.
- Reproducibility.
- Credential access.

See `DEPENDENCY_POLICY.md`.

## Shell-Script Safety

- No tracked shell scripts were present when this policy was created.
- Do not add shell scripts without review.
- Avoid destructive commands.
- Avoid printing environment variables that may contain secrets.
- Prefer explicit paths and quoted variables when scripts are introduced.

## Network Services

- Network-capable code must be treated as sensitive.
- Do not start live services, exchange connections, or broker sessions unless explicitly authorized.
- Tests should use local simulation hooks rather than external services.
- Intermittent global connectivity is documented as an operational constraint.

## SSH Considerations

- Do not inspect private SSH keys.
- Do not add remote hosts or modify SSH config without operator approval.
- Do not push to remotes unless explicitly instructed.

## Incident Response

If a secret is exposed:

1. Stop work that might propagate it.
2. Preserve enough evidence to identify scope without copying the secret further.
3. Notify the operator.
4. Revoke or rotate affected credentials.
5. Audit Git status, logs, generated outputs, and remote publication risk.
6. Remove or redact exposed values with operator-approved remediation.
7. Document the incident without secret values.

## Compromised-Key Response

- Revoke the key immediately.
- Rotate dependent keys or tokens.
- Review account permissions and recent activity.
- Confirm no withdrawal permissions were present.
- Re-run relevant tests with safe credentials or no credentials.

## AI-Agent Restrictions

AI agents must:

- Avoid reading secrets.
- Avoid broad filesystem searches that expose sensitive files unless necessary.
- Report credential-related uncertainty.
- Refuse to enable live behavior without explicit authorization.
- Not commit, push, or publish security-sensitive material without approval.

## Security Review Requirements

Security review is required for changes to:

- `AuthManager`.
- `AsyncNetworkClient`.
- `LiveDataAdapter`.
- `BrokerGateway`.
- TLS/cert handling.
- `.gitignore` secret exclusions.
- Dependency manifests or build scripts.
- Logging or analytics that may include sensitive data.
