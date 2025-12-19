# Mac mini → OrKheStrA Workstation + Nano Nodes

This file contains the minimum connection details your Mac mini needs to reach:
- This workstation (where the repo + UI run)
- The nano nodes (Ollama API on port `11434`)

No passwords or private keys are stored here.

## 1) Workstation (this computer)

- **LAN IP**: `192.168.0.165`
- **UI (Flask)**: `http://192.168.0.165:5001/`
- **Repo path (on workstation)**: `/home/xx/AI-Safety-System-Design`

### SSH (recommended)

On the Mac mini:

```bash
# generate a key if you don’t have one
ssh-keygen -t ed25519 -C "mac-mini" 

# copy public key to this workstation (replace xx if your Linux username differs)
ssh-copy-id xx@192.168.0.165
```

Add to the Mac mini `~/.ssh/config`:

```sshconfig
Host orka-workstation
  HostName 192.168.0.165
  User xx
  Port 22
  IdentitiesOnly yes
  IdentityFile ~/.ssh/id_ed25519
```

Then connect:

```bash
ssh orka-workstation
```

## 2) Nano nodes (Ollama HTTP API)

All nodes expose Ollama at:
- `http://<NODE_IP>:11434/` (Ollama)
- Primary endpoint: `POST /api/generate`
- Model list: `GET /api/tags`

### Researcher nodes (7)

| Node | IP | Expected model |
|---|---:|---|
| TIF | `192.168.0.213` | `phi3:mini` |
| GIN | `192.168.0.242` | `deepseek-r1:1.5b` |
| ORIN | `192.168.0.160` | `qwen2.5:3b` |
| TORI | `192.168.0.235` | `llama3.2:3b` |
| ARCHI | `192.168.0.232` | `codegemma:2b` |
| THEO | `192.168.0.168` | `gemma2:2b` |
| NOVEL | `192.168.0.146` | `qwen2.5-coder:3b` |

### Support nodes (local to workstation)

These run on the workstation (`localhost`) and are reachable from the Mac via the workstation IP:

| Node | Reach via Mac | Expected model |
|---|---|---|
| GUARD | `http://192.168.0.165:11434/` | `llama-guard3:1b` |
| PAGEQA | (local integration) | `paperqa:latest` |
| ORKA | `http://192.168.0.165:11434/` | `qwen2.5:3b` |

Note: `PAGEQA` is an on-box integration; it is not an Ollama node.

## 3) Quick connectivity checks from Mac

```bash
# UI
curl -sS http://192.168.0.165:5001/api/heartbeat | head

# Ollama on a nano
curl -sS http://192.168.0.213:11434/api/tags | head

# (Optional) SSH reachability
ssh -o ConnectTimeout=3 orka-workstation 'hostname && uptime'
```

## 4) Optional: Mac hosts file aliases

If you want stable local names on the Mac mini, add to `/etc/hosts`:

```text
192.168.0.165  orka-workstation
192.168.0.213  tif
192.168.0.242  gin
192.168.0.160  orin
192.168.0.235  tori
192.168.0.232  archi
192.168.0.168  theo
192.168.0.146  novel
```

## 5) Repo references (source of truth)

- Cluster roster/models: `ui/app.py` (`CLUSTER_NODES`)
- Nano node status + expected models: `ui/live_data_provider.py` (`NANO_NODES`)
