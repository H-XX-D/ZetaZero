#!/usr/bin/env bash
set -euo pipefail

# Compatibility wrapper for VS Code task `deploy_and_build`.
# Actual deploy logic lives in scripts/deploy_to_z6.sh.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/scripts/deploy_to_z6.sh" "$@"
