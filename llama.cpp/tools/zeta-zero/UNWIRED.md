UNWIRED / PARTIALLY WIRED MODULES (ZETA ZERO)
============================================

Active (already wired in zeta-server.cpp)
- zeta-cyclic.h: 3B parallel worker (input fact extraction, output salience tweaks)
- zeta-graph-kv.{h,c} + integration: KV cache inject/warmup/lazy capture, stats endpoint
- zeta-gitgraph.{h,c} + traversal endpoints: branches/status/branch/checkout/surface/explore
- zeta-dedup.{h,c}: concept-key dedup + LSH index wired for ingest; CLI toggle + /dedup/stats; rebuilds on load
- zeta-tunnel-search.{h,c}: momentum tunneling retrieval wired into snippet retrieval (seeded by embed + LSH)
- zeta-ternary.h: conflict checks; /ternary/consensus endpoint
- zeta-output-control.h, zeta-trm.h, zeta-critic.h, zeta-scratch-buffer.h, zeta-memory.*, zeta-kv-extract.*, zeta-mode-controller
- zeta-embed-memory.h: semantic dedup hook and /debug/memory endpoint wired; consolidation stats live

Present but NOT wired
- zeta-deliberation.h: deliberation engine (navigate/verify/gate/veto/reflect callbacks) never initialized/called
- zeta-embed-integration.h: embedding ctx/cache not referenced in server
- zeta-proactive-memory.h, zeta-self-modify.h, zeta-research-graph-integration.h, zeta-swarm.h, zeta-text-memory.h, zeta-story-integration.h, zeta-semantic-attacks.h, aura-gkv.h: present, not wired
- zeta-branching-engine.* and BRANCHING_ENGINE_INTEGRATION.cpp: exploratory; not wired

Recently wired
- zeta-causal-embeddings.h: CAUSES/PREVENTS anchor/classify/extract hooked into ingest with CLI toggle (fallback to verb patterns)
- zeta-gitgraph-persist.h: co-retrieval graph now initialized, saved/loaded, and fed by snippet retrievals

Partially wired / limited use
- Graph-KV: injection/warmup/lazy capture enabled; KV capture relies on warmup/lazy; no branch-aware capture on node creation

Suggested wiring order
1) Causal embeddings: CAUSES/PREVENTS extraction gated by confidence.
2) Op-edges/auditor: sparse directional reason-weighted hops; decay + stats.
3) Deliberation (toggle/endpoint): run navigate/verify/gate/veto/reflect before final output.
4) Embed-integration: wire embedding cache/context manager.

Notes
- Keep ternary checks after any hop (including op hops).
- Scope op-edges and auditor to stay sparse; avoid dense auto-generated shortcuts.
- Branch-scope KV keys; capture high-salience nodes; flush on shutdown.
