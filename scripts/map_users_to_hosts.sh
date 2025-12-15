#!/usr/bin/env bash
# map_users_to_hosts.sh
# Usage: ./map_users_to_hosts.sh discovered_orins.csv gin@engineer tif@scientific tori@historian orin@nvidia

CSV=${1:-discovered_orins.csv}
shift || true
USERS=($@)
OUT=${OUT:-hosts.txt}
PSSH_OUT=${PSSH_OUT:-pssh_hosts.txt}

if [[ ! -f "$CSV" ]]; then
  echo "CSV file not found: $CSV" >&2
  exit 1
fi

# Read CSV into an associative array mapping hostname/ip -> ip
declare -A ip_for_name
while IFS=',' read -r ip mac hostname open_ports rdns vendor; do
  if [[ "$ip" == "ip" || -z "$ip" ]]; then
    continue
  fi
  # prefer hostname or rdns over ip
  if [[ -n "$hostname" && "$hostname" != "" ]]; then
    key=$hostname
  elif [[ -n "$rdns" && "$rdns" != "" ]]; then
    key=$rdns
  else
    key=$ip
  fi
  ip_for_name["$key"]=$ip
  ip_for_name["$ip"]=$ip
done < <(tail -n +2 "$CSV")

# Create hosts file
> "$OUT"
> "$PSSH_OUT"
for u in "${USERS[@]}"; do
  if [[ "$u" == *"@"* ]]; then
    user=${u%@*}
    host=${u#*@}
  else
    user=$SSH_USER
    host=$u
  fi
  ip=${ip_for_name["$host"]}
  if [[ -z "$ip" ]]; then
    # try DNS resolution
    ip=$(getent hosts "$host" | awk '{print $1}' || true)
  fi
  if [[ -z "$ip" ]]; then
    ip=$(dig +short "$host" | head -n1 || true)
  fi
  if [[ -z "$ip" ]]; then
    ip=$host
  fi
  echo "${user}@${host} ${ip}" >> "$OUT"
  # pssh hosts file contains one host per-line in the form user@host (no IP)
  echo "${user}@${host}" >> "$PSSH_OUT"
done

cat "$OUT"

echo "Mapped users to hosts in $OUT"