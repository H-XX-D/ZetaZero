# Z.E.T.A. Test Report

**Date:** 2025-12-20 03:26:04
**Duration:** 180.2s
**Server:** http://192.168.0.165:8080
**Total Tests:** 4
**Critic Issues:** 0 tests flagged

## Summary by Category

| Category | Tests | Notes |
|----------|-------|-------|
| Senior Coding | 4 | For manual review |


---

# Full Test Results

Each test below includes the full prompt and response for manual evaluation.


## Senior Coding

### Test 1: System Design - Distributed Cache

**Time:** 180.0s

**Prompt:**
```
Design a distributed caching system like Redis Cluster that supports:
1. Consistent hashing for key distribution
2. Master-slave replication with automatic failover
3. Cross-datacenter replication with conflict resolution
4. Memory-efficient storage for 100M+ keys

Provide:
- High-level architecture diagram (ASCII)
- Key data structures
- Failover algorithm pseudocode
- Consistency guarantees analysis
```

**Response:**

[TIMEOUT]

**Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

---

### Test 2: Algorithm - Streaming Statistics

**Time:** 0.1s

**Prompt:**
```
Implement an efficient algorithm for the following problem:

Given a stream of stock prices (price, timestamp) arriving in real-time:
1. Maintain a sliding window of the last K minutes
2. Support O(1) queries for: min, max, mean, median, mode
3. Support range queries: "What was the max price between T1 and T2?"
4. Memory must be O(K) not O(N)

Provide Python implementation with complexity analysis.
```

**Response:**

[CONNECTION_ERROR]

**Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

---

### Test 3: Debug - Memory Leak

**Time:** 0.1s

**Prompt:**
```
Debug this production issue:

Our Kubernetes pods are crashing with OOMKilled after ~2 hours of running.
- Memory grows linearly at ~50MB/hour
- Java heap dump shows String objects dominating
- Most strings are UUID-like: "req-a1b2c3d4-..."
- The leak only happens under load (>100 RPS)
- No leak in staging (same code, 10 RPS)

Here's the suspect code:
```java
@Service
public class RequestTracker {
    private final Map<String, RequestContext> active = new ConcurrentHashMap<>();

    public void track(String reqId, RequestContext ctx) {
        active.put(reqId, ctx);
        ctx.onComplete(() -> active.remove(reqId));
    }
}
```

Find the bug and provide the fix with explanation.
```

**Response:**

[CONNECTION_ERROR]

**Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

---

### Test 4: Code Review - HFT OrderBook

**Time:** 0.0s

**Prompt:**
```
Review this Rust code for a high-frequency trading system.
Identify all issues (correctness, performance, safety):

```rust
use std::sync::Mutex;
use std::collections::HashMap;

struct OrderBook {
    bids: Mutex<HashMap<u64, Vec<Order>>>,
    asks: Mutex<HashMap<u64, Vec<Order>>>,
    last_trade: Mutex<Option<Trade>>,
}

impl OrderBook {
    fn match_order(&self, order: Order) -> Vec<Trade> {
        let mut trades = vec![];
        let mut bids = self.bids.lock().unwrap();
        let mut asks = self.asks.lock().unwrap();

        // Match logic...
        while let Some(best_ask) = asks.get(&order.price) {
            if best_ask.is_empty() { break; }
            let matched = best_ask.remove(0);
            trades.push(Trade::new(order.clone(), matched));
        }

        *self.last_trade.lock().unwrap() = trades.last().cloned();
        trades
    }
}
```

Provide refactored version with explanations.
```

**Response:**

[CONNECTION_ERROR]

**Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

---


---
*Z.E.T.A. Test Suite - Manual Review Report*
