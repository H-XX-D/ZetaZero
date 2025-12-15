# Z.E.T.A. Benchmark Results

Generated: 2025-12-12T05:17:30.736140

---

### Section 6.3: Retrieval Accuracy

| Metric | Baseline | SubZero | Zero |
|--------|----------|---------|------|
| Recall@1 | 0.0% | 100.0% | 100.0% |
| Recall@5 | 0.0% | 100.0% | 100.0% |
| MRR | 0.000 | 1.000 | 1.000 |
| FPR | 0.0% | 80.0% | 80.0% |


### Section 6.5: Latency Breakdown

**SubZero Pipeline:**

| Operation | Mean (ms) | Std (ms) | P95 (ms) |
|-----------|-----------|----------|----------|
| Entity extraction (3B) | 74.2 | 96.4 | 344.9 |
| Graph walk (depth=3) | 0.1 | 0.0 | 0.2 |
| Fact retrieval | 0.1 | 0.1 | 0.4 |
| 3B generation | 671.8 | 4.5 | 680.4 |
| **Total SubZero** | 746.3 | 95.7 | 1015.8 |

**Zero Pipeline:**

| Operation | Mean (ms) | Std (ms) | P95 (ms) |
|-----------|-----------|----------|----------|
| Entity extraction (3B) | 37.7 | 11.3 | 64.2 |
| Graph walk (depth=3) | 0.1 | 0.0 | 0.1 |
| Fact retrieval | 0.1 | 0.1 | 0.2 |
| 14B generation | 0.7 | 0.1 | 0.8 |
| **Total Zero** | 38.6 | 11.2 | 65.1 |

**Latency Comparison:**

| Config | Total (ms) | Use Case |
|--------|------------|----------|
| Baseline | 1 | Fast, no memory |
| SubZero | 746 | Edge, real-time |
| Zero | 39 | Quality, async |


### Section 6.6: Memory Persistence

| Hours Since Learning | Recall Rate | Avg Score | Expected D(t) |
|----------------------|-------------|-----------|---------------|
| 0 (immediate) | 100% | 1.000 | 1.000 |
| 6 | 100% | 0.735 | 0.735 |
| 12 | 100% | 0.540 | 0.540 |
| 24 | 100% | 0.292 | 0.292 |
| 48 | 100% | 0.085 | 0.085 |


### Section 7.1: Component Contribution

| Configuration | Recall@5 | MRR | Notes |
|---------------|----------|-----|-------|
| cosine_only | 100.0% | 1.0000 | Pure similarity |
| plus_decay | 100.0% | 1.0000 | Recency bias |
| plus_entity | 100.0% | 1.0000 | Graph walk |
| full_subzero | 100.0% | 1.0000 | Full retrieval |


### Section 6.4: Context Extension

| Configuration | Effective Context | Peak RAM | Tokens/sec |
|---------------|-------------------|----------|------------|
| Baseline | 4K | 12032 MB | 0.0 |
| SubZero (5 blocks) | 6.7K | 12032 MB | 222.4 |
| SubZero (20 blocks) | 14.3K | 12032 MB | 200.7 |
| Zero (5 blocks) | 6.7K | 12032 MB | 0.0 |
| Zero (20 blocks) | 14.3K | 12032 MB | 0.0 |
