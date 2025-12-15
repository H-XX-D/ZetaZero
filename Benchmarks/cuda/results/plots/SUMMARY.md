# Z.E.T.A. Benchmark Results (5 runs, mean ± std)

## Retrieval Accuracy
| Config | Recall@1 | Recall@5 | MRR |
|--------|----------|----------|-----|
| Baseline | 0.0±0.0% | 0.0±0.0% | 0.000±0.000 |
| Subzero | 100.0±0.0% | 100.0±0.0% | 1.000±0.000 |
| Zero | 100.0±0.0% | 100.0±0.0% | 1.000±0.000 |

## Latency
| Pipeline | Total (ms) |
|----------|------------|
| SubZero | 729.8±2.2 |
| Zero | 60.6±9.2 |

## Ablation (Recall@5)
| Config | Score |
|--------|-------|
| raw_cosine | 80.0±0.0% |
| plus_cubic | 80.0±0.0% |
| plus_tunnel | 80.0±0.0% |
| plus_decay | 80.0±0.0% |
| plus_graph | 100.0±0.0% |
| full_subzero | 100.0±0.0% |