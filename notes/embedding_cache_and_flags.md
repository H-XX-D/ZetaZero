Embedding Cache + Runtime Flags (ZetaZero)
==========================================

What changed
------------
- Embedding cache now has hard on/off gating, live reconfiguration, and clear semantics.
- CLI flags and runtime endpoints let you tune cache size, TTL, and minimum text length.
- Startup logs print current cache config (enabled, max entries, TTL, min length).

Key file references
-------------------
- Server wiring and flags: llama.cpp/tools/zeta-zero/zeta-server.cpp
- Cache implementation: llama.cpp/tools/zeta-zero/zeta-embed-integration.h

CLI flags (parsed at startup)
-----------------------------
- --embed-cache-enable | --embed-cache-disable
- --embed-cache-max <N>        (default 500)
- --embed-cache-ttl <seconds>  (default 600)
- --embed-cache-minlen <chars> (default 10)
- Related toggles already present:
  - --dedup-enable | --dedup-disable | --dedup-threshold <F>
  - --embed-memory-enable | --embed-memory-disable
  - --tunnel-enable | --tunnel-disable
  - --git-autosave-enable | --git-autosave-disable
  - --causal-embeddings-enable | --causal-embeddings-disable
  - --research-enable | --deliberation-enable
  - --token-cache-enable | --token-cache-cap <tokens> | --token-cache-entries <N>
  - --ctx-14b <N> | --ctx-3b <N>
  - Model overrides: -m <14B>, --model-7b-coder <path>, --embed-model <path>
  - Infra: --port <N>, --gpu-layers <N>, --zeta-storage <path>

Runtime endpoints
-----------------
- GET  /debug/embed            → cache stats (enabled, entries, hits, misses, hit rate, TTL, minlen, dim)
- POST /debug/embed/clear      → clear cache
- POST /debug/embed/config     → live config; query params:
  - enabled=true|false
  - max=<int>   (entries)
  - ttl=<int>   (seconds)
  - minlen=<int>
- Other helpful endpoints:
  - GET /debug/dedup and /dedup/stats
  - GET /debug/memory (embed-memory stats)

Behavior notes
--------------
- Disabling the cache immediately bypasses get/put and clears stored entries.
- get/put honor the enabled gate and minimum text length.
- Stats include enabled state, thresholds, hits/misses, hit rate, TTL, minlen.

Operational quickstart
----------------------
- Disable cache at boot: add --embed-cache-disable
- Enable with custom sizing: --embed-cache-enable --embed-cache-max 800 --embed-cache-ttl 900 --embed-cache-minlen 12
- Adjust live without restart: POST /debug/embed/config?enabled=1&max=800&ttl=900&minlen=12
- Flush live: POST /debug/embed/clear

Context for unwired modules (from UNWIRED.md)
---------------------------------------------
- Not yet wired: deliberation, embed-integration (fully), proactive/self-modify/research-graph, swarm/text/story, semantic-attacks, aura-gkv, branching engine.
- Partially wired: graph-kv lacks branch-aware capture on node creation.
- Suggested next wiring (per UNWIRED.md):
  1) Causal embeddings (confidence-gated)
  2) Op-edges/auditor (sparse, decayed)
  3) Deliberation toggle/endpoint (navigate/verify/gate/veto/reflect pre-output)
  4) Embed-integration (cache/context manager)

Current defaults (if not overridden)
------------------------------------
- Embed cache: enabled, max=500, ttl=600s, minlen=10
- Dedup ingest: on, threshold=0.86
- Embed-memory hook: on
- Tunnel search: on
- Token cache: off (caps: 200000 tokens, 4096 entries)
- Causal embeddings: on (fallback to verb patterns if disabled)
- Research mode: off; Deliberation: off
- Context: 14B ctx=4096, 7B ctx=1024 (unless overridden)
- Models: 14B qwen2.5 instruct, 7B coder, embed model per config/defaults

Safety/remediation
------------------
- If cache misbehaves or skews latency, disable via flag or POST config; then clear.
- Changes are reversible without restart via /debug/embed/config and /debug/embed/clear.
