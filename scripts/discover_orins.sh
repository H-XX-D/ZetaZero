#!/usr/bin/env bash
# discover_orins.sh
# Usage: ./discover_orins.sh 192.168.1.0/24 [OUTPUT_FILE]
# Runs nmap/arp scans, prints and optionally writes the discovered hosts to an output file.

set -eo pipefail
SUBNET=${1:-192.168.1.0/24}
OUTPUT_FILE=${2:-discovered_orins.csv}

if ! command -v nmap >/dev/null 2>&1; then
  echo "nmap is required. Install via: sudo apt-get install nmap" >&2
  exit 1
fi

if ! whoami >/dev/null 2>&1; then
  true
fi

# Header
printf "ip,mac,hostname,open_ports,reverse_dns,oui_vendor\n" > "$OUTPUT_FILE"

# Quick ping sweep (nmap does a ping by default but sometimes not)
# We use nmap -sn for hosts, then probe for port 22 to check SSH listening and use ARP cache to get MACs.

NMAP_HOSTS_TMP=$(mktemp)
trap 'rm -f "$NMAP_HOSTS_TMP"' EXIT

# Discover live hosts
nmap -sn "$SUBNET" -oG - | awk '/Up$/{print $2}' > "$NMAP_HOSTS_TMP"

# Ensure ARP table is populated by pinging the hosts (optional, less noisy)
# This helps when nmap did a simple ping and didn't populate ARP cache on the scanning host.
while IFS= read -r ip; do
  ping -c 1 -W 1 "$ip" >/dev/null 2>&1 || true
done < "$NMAP_HOSTS_TMP"

# Use arp -a if available to map IP -> MAC
# Note some systems may require arp-scan install: sudo apt-get install arp-scan

get_mac_from_arp() {
  local ip=$1
  # Linux's arp -n output or arp -a output
  if arp -n "$ip" 2>/dev/null | awk 'NR>1{print $3}' >/dev/null 2>&1; then
    arp -n "$ip" | awk 'NR==1{print $3}'
  else
    arp -a | grep "($ip)" | awk '{print $4}' | tr -d '()'
  fi
}

# For Linux, vendor lookup via maclookup (optional) or use the OUI from the MAC
get_oui_vendor() {
  local mac=$1
  if [[ -z "$mac" || "$mac" == "(incomplete)" ]]; then
    echo "unknown"
    return
  fi
  # take first 3 octets
  echo "${mac^^}" | awk -F ':' '{print toupper($1)":"toupper($2)":"toupper($3)}'
}

while IFS= read -r ip; do
  # Reverse DNS
  rdns=$(dig +short -x "$ip" | tr -d '\n') || rdns=""
  # Check open ports: probe port 22 to detect SSH
  open_ports=$(nmap -p 22 --open -Pn "$ip" -oG - | awk -F"/" '/22\/open/{print "22"}' | paste -s -d',')
  # Get mac
  mac=$(get_mac_from_arp "$ip" || echo "")
  # get OUI vendor
  vendor=$(get_oui_vendor "$mac")
  # Hostname via nmap -sL or -R
  hostname=""
  if [[ -n "$rdns" ]]; then
    hostname="$rdns"
  else
    # try nmap name resolution
    hostname=$(nmap -sL -n "$ip" | awk -F'(' '/Nmap scan report for/{print $2}' | tr -d ')') || hostname=""
  fi
  # Print & append
  printf "%s,%s,%s,%s,%s,%s\n" "$ip" "$mac" "$hostname" "$open_ports" "$rdns" "$vendor"
  printf "%s,%s,%s,%s,%s,%s\n" "$ip" "$mac" "$hostname" "$open_ports" "$rdns" "$vendor" >> "$OUTPUT_FILE"

done < "$NMAP_HOSTS_TMP"

echo "Discovery complete. Results saved to: $OUTPUT_FILE"

echo "Tip: To SSH test, run: ssh -i ~/.ssh/id_rsa user@IP"