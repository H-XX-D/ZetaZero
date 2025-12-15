#!/usr/bin/env bash
# pre_commit_secret_check.sh
# Usage: ./scripts/pre_commit_secret_check.sh
# Grep for likely secret markers in config/ and other files. Returns 0 if no findings.

set -eo pipefail

PATTERNS=(
  "BEGIN PRIVATE KEY"
  "BEGIN RSA PRIVATE KEY"
  "BEGIN DSA PRIVATE KEY"
  "BEGIN ED25519 PRIVATE KEY"
  "BEGIN OPENSSH PRIVATE KEY"
  "HMAC_KEY_HEX"
  "hmac_key_hex:"
  "ed25519_private_hex"
  "ed25519_private_key_file"
  "api_key"
  "API_KEY"
  "secret_key"
  "secret:"
  "password:"
)

EXIT_CODE=0
for p in "${PATTERNS[@]}"; do
  echo "Searching for pattern: $p"
  if grep -RIn --exclude-dir=.git --exclude-dir=.venv --exclude-dir=.mypy_cache --exclude='*.log' "$p" .; then
    EXIT_CODE=2
  fi
done

if [[ "$EXIT_CODE" -ne 0 ]]; then
  echo "Potential secret(s) found. Please remove them before committing. See SECURITY_KEYS_HANDLING.md"
fi
exit $EXIT_CODE
