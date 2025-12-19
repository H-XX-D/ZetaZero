# Secure Key Handling & Secrets Management

This project can include runtime configuration in the `config/` folder. DO NOT commit private keys, HMAC keys, or secret values to this repository. If secrets were checked in, please follow the steps immediately to rotate and purge them.

1) Detecting secrets
- Basic search for secrets:
  - Check for `BEGIN PRIVATE KEY` markers, `ed25519`, `RSA`, `HMAC`, `API_KEY`, `secret`, or `token` using grep:
    ```bash
    grep -RIN --exclude-dir=.git 'BEGIN .*PRIVATE KEY\|HMAC\|ed25519\|API_KEY\|secret\|token' config || true
    ```
- Check `config/*.yaml` for `hmac_key_hex`, `ed25519_private_hex`, `api_key`, `ed25519_private` fields.

2) If secrets are present in `config/` in the repo (accidentally committed):
- Remove them from your working tree and git history.

- To remove from HEAD (recommended):
  ```bash
  git rm --cached config/development.yaml
  git commit -m "Remove config/development.yaml from repo (contains secrets)"
  ```
- To remove from entire repo history (rewrite required, **be careful**):
  - Using BFG (recommended and faster):
    ```bash
    # Install bfg (https://rtyley.github.io/bfg-repo-cleaner/)
    # Replace 'dev-config.yaml' with actual filename or path you need to remove
    java -jar bfg.jar --delete-files config/development.yaml
    git reflog expire --expire=now --all
    git gc --prune=now --aggressive
    git push --force
    ```
  - Using git filter-branch (older, slower):
    ```bash
    git filter-branch --force --index-filter '\n  git rm --cached --ignore-unmatch config/development.yaml\n' --prune-empty --tag-name-filter cat -- --all
    git push origin --force --all
    ```
- Remember: Force-pushing rewritten history will affect collaborators. Communicate and coordinate.

3) Rotate keys
- If any HMAC, JWT, Ed25519 or API keys were included, rotate them with the provider or relevant key management.
- Example: generate a new Ed25519 key for signing with `ssh-keygen -t ed25519 -f ~/.secrets/ed25519_key` and update your runtime environment to use it.

4) Secure storage (recommended approaches)
- Use environment variables for the runtime secrets, and read them from code using `ENV['SOME_KEY']`. Do NOT store secrets in code.
- For local machines, store secrets at `~/.secrets/` (with 600 perms):
  ```bash
  mkdir -p ~/.secrets && chmod 700 ~/.secrets
  mv some_secret.key ~/.secrets/some_secret.key
  chmod 600 ~/.secrets/some_secret.key
  ```
- For production, use secret managers like AWS Secrets Manager, HashiCorp Vault, GCP Secret Manager, or macOS keychain.

5) Update configuration to reference secrets via env variables
- Edit your runtime config to set `ed25519_private_hex` and `hmac_key_hex` via environment variables or key paths, not embedded in YAML files.

Example (YAML):
```yaml
secure_transport:
  enabled: true
  hmac_key_hex_env: HMAC_KEY_HEX
  ed25519_private_key_file: /run/secrets/ed25519_key
```

6) Confirm `.gitignore` blocks secrets and key material
- This repository ignores common key formats (e.g. `*.pem`, `*.key`, `id_ed25519`) and `secrets/` folders. Check with:
  ```bash
  git status --ignored
  ```

- If you keep runtime config in `config/`, do not embed secrets in tracked files. Prefer env vars or files under `~/.secrets/`.

7) If you need help with secure key handling or rotating keys, I can add automation for storing keys into `~/.secrets` or integrate HashiCorp Vault or an Ansible Vault workflow.

---

If you want, I can:
- Add a small helper script to move secrets to `~/.secrets` and update the runtime to read from that path.
- Add a CI check to refuse commits that include `BEGIN PRIVATE KEY` or `hmac` entries in config files.
- Add an interactive helper to safely remove secrets and replace them with env var placeholders.
