#!/usr/bin/env bash
# list_memories.sh - List memories in AkashicRecord(s)
# Usage: ./scripts/list_memories.sh [AkashicRecord_path]

RECORD_PATH=${1:-$HOME/Documents/AkashicRecord_1}
INDEX_FILE="$RECORD_PATH/memory_index.json"

if [[ ! -d "$RECORD_PATH" ]]; then
  echo "AkashicRecord path not found: $RECORD_PATH" >&2
  exit 1
fi

if [[ ! -f "$INDEX_FILE" ]]; then
  echo "No memory_index.json found in: $RECORD_PATH" >&2
  exit 1
fi

if command -v jq >/dev/null 2>&1; then
  jq -r '.[] | [.id, .timestamp, (.isImmutable|tostring), (.tokenRange|length), .contentHash, .diskPath] | @tsv' "$INDEX_FILE" | awk -F"\t" '{printf "%s  | %s | immutable=%s | tokens=%s | hash=%s | path=%s\n", $1, $2, $3, $4, $5, $6}'
else
  python3 - <<'PY'
import json,sys,os
p = sys.argv[1]
with open(p,'r') as f:
  arr = json.load(f)
for s in arr:
  id = s.get('id')
  ts = s.get('timestamp')
  imm = s.get('isImmutable')
  tr = s.get('tokenRange', [])
  hash = s.get('contentHash')
  path = s.get('diskPath')
  print(f"{id} | {ts} | immutable={imm} | tokens={len(tr)} | hash={hash} | path={path}")
PY
fi

echo "\nZMEM files in ${RECORD_PATH}:"
ls -lh "$RECORD_PATH"/*.zmem 2>/dev/null || echo "(No .zmem files found)"

# List sizes of zmem files
if [[ -d "$RECORD_PATH" ]]; then
  echo "\nDetails of first zmem file (hex snippet):"
  first=$(ls -1 "$RECORD_PATH"/*.zmem 2>/dev/null | head -n1)
  if [[ -n "$first" ]]; then
    echo "File: $first"
    hexdump -C -n 64 "$first" || true
  else
    echo "No .zmem files to inspect"
  fi
fi
