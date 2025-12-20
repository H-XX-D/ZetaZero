# Z.E.T.A. Test Results - Jetson Orin Nano
Date: 2025-12-17

## Stress Test (61 Prompts) - NEW
- **Tests Run:** 61
- **Success Rate:** 100%
- **Caching:** Verified (Identical prompts trigger 0 new nodes)
- **Adversarial:** 100% Robust (Injection attempts blocked/reset)
- **Memory Stability:** 262 MB range over 61 prompts
- **Final Graph:** 139 nodes, 279 edges

## Persistence Run (12 Tests)
- **Tests Run:** 12
- **Success Rate:** 100%
- **Total Tokens:** 5073
- **Overall TPS:** 7.4
- **Memory Baseline:** 5313 MB
- **Memory Final:** 5207 MB
- **Memory Range:** 589 MB delta

## Key Findings
1. **Caching:** Verified that Z.E.T.A. reuses embeddings for identical prompts, preventing redundant computation.
2. **Persistence:** The system successfully recalled the "ALPHA-7749" code in Test 12 after two server resets.
3. **Adversarial Robustness:** Prompt injection and memory probe attempts were successfully identified and neutralized.
4. **Memory Efficiency:** Observed "negative memory deltas" during long runs, indicating active graph pruning and buffer reclamation.
5. **Performance:** Throughput remained stable at ~7.6 t/s with 10 GPU layers.

Full data available in [zeta_graph_summary_20251217.json](zeta_graph_summary_20251217.json)
