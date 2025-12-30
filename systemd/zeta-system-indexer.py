#!/usr/bin/env python3
"""
Z.E.T.A. System Indexer
Monitors system events and feeds them to ZETA's knowledge graph.

Usage:
    ./zeta-system-indexer.py --watch        # Live monitoring
    ./zeta-system-indexer.py --snapshot     # One-time system snapshot
    ./zeta-system-indexer.py --dmesg        # Index recent kernel logs
"""

import subprocess
import json
import requests
import argparse
import time
import os
from datetime import datetime

ZETA_URL = os.environ.get("ZETA_URL", "http://localhost:8080")

def query_zeta(prompt: str, extract_facts: bool = True) -> dict:
    """Send a query to ZETA and optionally extract facts."""
    try:
        response = requests.post(
            f"{ZETA_URL}/generate",
            json={"prompt": prompt, "max_tokens": 100},
            timeout=30
        )
        return response.json()
    except Exception as e:
        print(f"[ERROR] Failed to query ZETA: {e}")
        return {}

def index_fact(category: str, key: str, value: str):
    """Index a system fact into ZETA's graph."""
    prompt = f"[SYSTEM-INDEX] Remember this system fact: {category}/{key} = {value}"
    result = query_zeta(prompt)
    print(f"  [{category}] {key}: {value}")
    return result

def get_system_info() -> dict:
    """Gather current system information."""
    info = {}

    # Hostname
    info["hostname"] = subprocess.getoutput("hostname")

    # Kernel
    info["kernel"] = subprocess.getoutput("uname -r")

    # Uptime
    info["uptime"] = subprocess.getoutput("uptime -p")

    # Memory
    mem = subprocess.getoutput("free -h | grep Mem")
    parts = mem.split()
    if len(parts) >= 3:
        info["memory_total"] = parts[1]
        info["memory_used"] = parts[2]

    # Disk
    disk = subprocess.getoutput("df -h / | tail -1")
    parts = disk.split()
    if len(parts) >= 4:
        info["disk_total"] = parts[1]
        info["disk_used"] = parts[2]
        info["disk_percent"] = parts[4]

    # CPU
    cpu = subprocess.getoutput("nproc")
    info["cpu_cores"] = cpu

    # Load
    load = subprocess.getoutput("cat /proc/loadavg")
    parts = load.split()
    if parts:
        info["load_1m"] = parts[0]

    # GPU (if nvidia-smi available)
    try:
        gpu = subprocess.getoutput("nvidia-smi --query-gpu=name,memory.total,memory.used --format=csv,noheader,nounits 2>/dev/null")
        if gpu and "failed" not in gpu.lower():
            parts = gpu.split(",")
            if len(parts) >= 3:
                info["gpu_name"] = parts[0].strip()
                info["gpu_memory_total"] = f"{parts[1].strip()} MiB"
                info["gpu_memory_used"] = f"{parts[2].strip()} MiB"
    except:
        pass

    return info

def get_recent_dmesg(lines: int = 50) -> list:
    """Get recent kernel log messages."""
    output = subprocess.getoutput(f"dmesg --time-format=reltime 2>/dev/null | tail -{lines}")
    return [line.strip() for line in output.split("\n") if line.strip()]

def get_top_processes(n: int = 10) -> list:
    """Get top processes by CPU/memory."""
    output = subprocess.getoutput(f"ps aux --sort=-%cpu | head -{n+1}")
    lines = output.split("\n")[1:]  # Skip header
    processes = []
    for line in lines:
        parts = line.split(None, 10)
        if len(parts) >= 11:
            processes.append({
                "user": parts[0],
                "pid": parts[1],
                "cpu": parts[2],
                "mem": parts[3],
                "command": parts[10][:50]
            })
    return processes

def snapshot():
    """Take a one-time system snapshot and index it."""
    print(f"[SNAPSHOT] Indexing system state at {datetime.now()}")
    print()

    # System info
    print("[SYSTEM INFO]")
    info = get_system_info()
    for key, value in info.items():
        index_fact("system", key, value)

    print()

    # Top processes
    print("[TOP PROCESSES]")
    processes = get_top_processes(5)
    for i, proc in enumerate(processes):
        index_fact("process", f"top_{i+1}", f"PID {proc['pid']} ({proc['command']}) CPU:{proc['cpu']}% MEM:{proc['mem']}%")

    print()
    print("[SNAPSHOT] Complete")

def index_dmesg():
    """Index recent kernel logs."""
    print("[DMESG] Indexing recent kernel messages...")

    messages = get_recent_dmesg(20)

    # Group similar messages
    for msg in messages:
        # Skip noisy messages
        if any(skip in msg.lower() for skip in ["audit", "usb", "input:"]):
            continue

        # Index important ones
        if any(important in msg.lower() for important in ["error", "fail", "warn", "oom", "panic", "killed"]):
            index_fact("kernel", "alert", msg[:200])

    print(f"[DMESG] Indexed {len(messages)} messages")

def drill_down():
    """Deep system analysis - detailed snapshot."""
    print(f"\n[DRILL-DOWN] Deep system analysis at {datetime.now()}")

    # Full system info
    print("\n[SYSTEM]")
    info = get_system_info()
    for key, value in info.items():
        index_fact("system", key, value)

    # Top 10 processes
    print("\n[PROCESSES]")
    processes = get_top_processes(10)
    for i, proc in enumerate(processes):
        index_fact("process", f"rank_{i+1}", f"PID:{proc['pid']} CMD:{proc['command']} CPU:{proc['cpu']}% MEM:{proc['mem']}%")

    # Kernel logs
    print("\n[KERNEL]")
    messages = get_recent_dmesg(30)
    alerts = [m for m in messages if any(k in m.lower() for k in ["error", "fail", "warn", "oom", "killed"])]
    for msg in alerts[:5]:
        index_fact("kernel", "event", msg[:150])

    # Disk usage per mount
    print("\n[STORAGE]")
    df_output = subprocess.getoutput("df -h | grep -E '^/dev'")
    for line in df_output.split("\n")[:5]:
        parts = line.split()
        if len(parts) >= 6:
            index_fact("disk", parts[5], f"{parts[4]} used ({parts[2]}/{parts[1]})")

    # Network connections summary
    print("\n[NETWORK]")
    netstat = subprocess.getoutput("ss -tuln | grep LISTEN | wc -l")
    index_fact("network", "listening_ports", netstat.strip())

    print("\n[DRILL-DOWN] Complete")

def watch():
    """Live monitoring mode - drill down every 2 hours."""
    print("[WATCH] Starting system monitoring daemon...")
    print("[WATCH] Drill-down every 2 hours. Press Ctrl+C to stop")
    print()

    DRILL_INTERVAL = 2 * 60 * 60  # 2 hours in seconds
    ALERT_INTERVAL = 60  # Check for alerts every minute

    last_drill = 0
    last_load = None
    last_gpu_mem = None

    try:
        while True:
            now = time.time()

            # Drill down every 2 hours
            if now - last_drill >= DRILL_INTERVAL:
                drill_down()
                last_drill = now

            # Quick alert check every minute
            info = get_system_info()

            # Alert on high load
            try:
                load = float(info.get("load_1m", 0))
                if load > 4.0 and load != last_load:
                    index_fact("alert", "high_load", f"System load is {load}")
                    last_load = load
            except:
                pass

            # Alert on GPU memory pressure
            try:
                gpu_used = info.get("gpu_memory_used", "0 MiB")
                if gpu_used != last_gpu_mem:
                    gpu_val = int(gpu_used.split()[0])
                    if gpu_val > 14000:  # >14GB on 16GB card
                        index_fact("alert", "gpu_memory", f"GPU memory high: {gpu_used}")
                    last_gpu_mem = gpu_used
            except:
                pass

            time.sleep(ALERT_INTERVAL)

    except KeyboardInterrupt:
        print("\n[WATCH] Stopped")

def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. System Indexer")
    parser.add_argument("--snapshot", action="store_true", help="Take system snapshot")
    parser.add_argument("--dmesg", action="store_true", help="Index kernel logs")
    parser.add_argument("--watch", action="store_true", help="Live monitoring")
    parser.add_argument("--url", default="http://localhost:8080", help="ZETA URL")

    args = parser.parse_args()

    global ZETA_URL
    ZETA_URL = args.url

    if args.snapshot:
        snapshot()
    elif args.dmesg:
        index_dmesg()
    elif args.watch:
        watch()
    else:
        # Default: snapshot
        snapshot()

if __name__ == "__main__":
    main()
