#!/usr/bin/env bash
# move_secrets_to_user_dir.sh
# Safely move keys from config/ into a user-only secrets directory (~/.secrets)
# Usage: ./scripts/move_secrets_to_user_dir.sh config/development.yaml

set -euo pipefail
CONFIG_FILE=${1:-config/development.yaml}
SECRETS_DIR=${SECRETS_DIR:-$HOME/.secrets}

if [[ ! -f "$CONFIG_FILE" ]]; then
  echo "Config file not found: $CONFIG_FILE" >&2
  exit 1
fi

mkdir -p "$SECRETS_DIR"
chmod 700 "$SECRETS_DIR"

# List of keys to extract
# This simple script extracts ‘hmac_key_hex’ if present or ed25519 private key file reference
# This is a conservative script: it will NOT print or expose key contents; it only moves if a file path is in the config.

# If the config contains an inline HMAC hex, we'll prompt to copy it and then clear from the file (manual step required)
INLINE_HMAC=$(grep -E "hmac_key_hex\s*:\s*\"[0-9a-fA-F]+\"" "$CONFIG_FILE" || true)
if [[ -n "$INLINE_HMAC" ]]; then
  echo "Inline HMAC key found in $CONFIG_FILE. It's recommended to rotate and move it to env var or a secrets file." >&2
  echo "Please rotate/change it and create the environment variable HMAC_KEY_HEX. This script will not continue automatically to avoid exposing secrets." >&2
fi

# If the config safely references a file path for ed25519_private_key_file, copy the file
ED_PRIV_FILE=$(grep -oE "ed25519_private_key_file:\s*.+" "$CONFIG_FILE" | sed 's/ed25519_private_key_file:\s*//' | tr -d '" ' || true)
if [[ -n "$ED_PRIV_FILE" && -f "$ED_PRIV_FILE" ]]; then
  echo "Found ed25519 private key file: $ED_PRIV_FILE (copying into $SECRETS_DIR)"
  cp "$ED_PRIV_FILE" "$SECRETS_DIR/ed25519_private_key"
  chmod 600 "$SECRETS_DIR/ed25519_private_key"
  echo "Wrote: $SECRETS_DIR/ed25519_private_key" 
else
  echo "No ed25519_private_key_file reference found in $CONFIG_FILE or file does not exist."
fi

echo "If you have inline secrets (HMAC, API keys), rotate them now and populate env vars like HMAC_KEY_HEX, SEMANTIC_SCHOLAR_API_KEY, etc."

echo "Done. Remember to remove the keys from the config and repo and rotate the keys. See SECURITY_KEYS_HANDLING.md for details."