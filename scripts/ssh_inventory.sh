#!/usr/bin/env bash
# ssh_inventory.sh - Run quick SSH checks in parallel using pssh or serially
# Usage: ./ssh_inventory.sh hosts.txt (or use env SSH_USER and SSH_KEY)
HOSTFILE=${1:-hosts.txt}
SSH_USER=${SSH_USER:-ubuntu}
SSH_KEY=${SSH_KEY:-~/.ssh/id_rsa}
SSH_OPTS="-o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no"

if [[ ! -f "$HOSTFILE" ]]; then
  echo "Hostfile not present: $HOSTFILE" >&2
  exit 1
fi

# prefer pssh if installed
if command -v pssh >/dev/null 2>&1; then
  echo "Using pssh for parallel connection checks"
  # If hosts file contains user@host entries (lines with @) do not pass -l
  if grep -q '@' "$HOSTFILE"; then
    pssh -h "$HOSTFILE" -i -x "-i $SSH_KEY $SSH_OPTS" "uname -a; uptime -p; tegrastats --samples 1 || true"
  else
    pssh -h "$HOSTFILE" -l "$SSH_USER" -i -x "-i $SSH_KEY $SSH_OPTS" "uname -a; uptime -p; tegrastats --samples 1 || true"
  fi
else
  while IFS= read -r line; do
    # Accept formats:
    # 1) user@host ip
    # 2) host ip
    # 3) user@host
    # 4) host (DNS-resolvable)
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    # Split into tokens
    set -- $line
    if [[ $# -eq 2 ]]; then
      left=$1; right=$2
      # left may be user@host or host
      if [[ "$left" == *"@"* ]]; then
        ssh_user=${left%@*}
        ssh_host=${left#*@}
      else
        ssh_user=$SSH_USER
        ssh_host=$left
      fi
      ip=$right
    elif [[ $# -eq 1 ]]; then
      left=$1
      if [[ "$left" == *"@"* ]]; then
        ssh_user=${left%@*}
        ssh_host=${left#*@}
      else
        ssh_user=$SSH_USER
        ssh_host=$left
      fi
      # try to resolve via getent/dig
      ip=$(getent hosts "$ssh_host" | awk '{print $1}' || true)
      if [[ -z "$ip" ]]; then
        ip=$(dig +short "$ssh_host" | head -n1 || true)
      fi
      if [[ -z "$ip" ]]; then
        ip=$ssh_host
      fi
    else
      # More than 2 tokens - try to parse as name ip
      ssh_user=$SSH_USER
      ssh_host=$1
      ip=${2:-$ssh_host}
    fi

    echo "\n--- ${ssh_user}@${ssh_host} (${ip}) ---"
    ssh -i "$SSH_KEY" $SSH_OPTS "${ssh_user}@${ip}" "uname -a; uptime -p; echo 'SSH OK'" || echo "SSH failed for ${ssh_user}@${ssh_host} (${ip})"
  done < "$HOSTFILE"
fi
