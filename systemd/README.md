# Z.E.T.A. System Daemon

Run Z.E.T.A. as a native Linux system service with real-time priority and kernel-level integration.

## Quick Install

```bash
cd systemd
sudo ./install.sh
```

## What This Does

| Component | Location | Purpose |
|-----------|----------|---------|
| Binary | `/opt/zeta/zeta-server` | The Z.E.T.A. server |
| Config | `/etc/zeta/zeta.conf` | Main configuration |
| Environment | `/etc/zeta/environment` | System environment vars |
| Data | `/var/lib/zeta/` | GKV cache, dreams, storage |
| Logs | `journalctl -u zeta` | Systemd journal |
| Service | `/etc/systemd/system/zeta.service` | Systemd unit |

## Service Management

```bash
# Start/Stop/Restart
sudo systemctl start zeta
sudo systemctl stop zeta
sudo systemctl restart zeta

# Enable on boot
sudo systemctl enable zeta

# Check status
sudo systemctl status zeta

# View logs (live)
journalctl -u zeta -f

# View Speed Receipt timings
journalctl -u zeta | grep SPEED-RECEIPT
```

## Performance Features

The systemd unit includes several performance optimizations:

- **Real-time scheduling**: `CPUSchedulingPolicy=fifo` with priority 50
- **I/O priority**: `IOSchedulingClass=realtime`
- **Memory locking**: `LimitMEMLOCK=infinity` prevents swapping
- **Nice level**: `-10` for higher CPU priority
- **GPU access**: Supplementary groups `video` and `render`

## Security Hardening

- Runs as dedicated `zeta` user (not root)
- `NoNewPrivileges=true` - can't escalate
- `ProtectSystem=strict` - filesystem is read-only except allowed paths
- `PrivateTmp=true` - isolated /tmp

## Directory Structure

```
/opt/zeta/
├── zeta-server          # Binary
└── models/              # GGUF model files
    ├── qwen2.5-14b-instruct-q4_k_m.gguf
    ├── qwen2.5-coder-7b-instruct-q4_k_m.gguf
    └── nomic-embed-text-v1.5.f16.gguf

/etc/zeta/
├── zeta.conf            # Main config
└── environment          # Environment variables

/var/lib/zeta/
├── graph_kv/            # GKV cache (persistent attention states)
├── dreams/
│   ├── pending/         # Dreams awaiting consolidation
│   └── archive/         # Processed dreams
├── storage/             # General persistent storage
└── blocks/              # GitGraph blocks
```

## Future: eBPF Integration

Coming soon: kernel-level observability via eBPF hooks.

```
syscall → eBPF probe → Z.E.T.A. graph update → GKV cached
```

Ask "What caused that crash?" and get instant recall from kernel events.
