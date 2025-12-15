#!/usr/bin/env bash
# generate_hosts_from_csv.sh
# Usage: ./generate_hosts_from_csv.sh discovered_orins.csv hosts.txt
CSV=${1:-discovered_orins.csv}
OUT=${2:-hosts.txt}

if [[ ! -f "$CSV" ]]; then
  echo "CSV file not found: $CSV" >&2
  exit 1
fi

awk -F',' 'NR>1{ name=$3; ip=$1; if(name=="" || name==" ") name=ip; gsub(/\./, "_", name); print name " " ip }' "$CSV" > "$OUT"

cat "$OUT"

echo "Saved hosts to $OUT"