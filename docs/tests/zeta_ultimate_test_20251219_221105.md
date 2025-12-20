# Z.E.T.A. Ultimate Test Report

**Date:** 2025-12-19 22:11:53
**Duration:** 48.0s
**Server:** http://192.168.0.165:8080

## Quick Summary

| Metric | Value |
|--------|-------|
| Auto-Pass | 2/4 (50.0%) |
| Needs Review | 4 tests |
| Errors | 0 |

## Summary by Category

| Category | Auto-Pass | Total | Notes |
|----------|-----------|-------|-------|
| Senior Coding | 2/4 | 4 | 4 need review |


---

# Full Test Results (For Manual Grading)

Each test below includes the full prompt and response for evaluation.
Mark each with your own grade: GOOD / OK / POOR / FAIL


## Senior Coding

### Test 1: System Design - Distributed Cache

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 15.9s

**Criteria:** Found 4/4 keywords, len=2473

<details>
<summary>📋 Prompt (click to expand)</summary>

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
</details>

**Response:**

### High-Level Architecture Diagram (ASCII)

```
+-------------------+
|   Client App       |
+--------+----------+
         |         
        /  \
 +------------+    +-------------+
 | Data Center1 |  | Data Center2 |
 |             |  |             |
 | Cache Node A|---> Cache Node B
 | Cache Node C<----- Cache Node D
 | (Master)     \   / (Slave)
 +--------+------|+-+-------+----+
                \|/          ^
                 ++----------++
            Consistent Hash Ring

```

### Key Data Structures

1. **Consistent Hashing Ring**: A circular data structure that maps keys to nodes in a way that minimizes rehashes when the set of nodes changes.
2. **Slab Allocator (Memory Pool)**: Efficiently manages memory allocation and deallocation for objects within Redis instances, ensuring low fragmentation even at high object counts.
3. **Replication Protocol**: Keeps track of master-slave relationships and ensures data consistency through periodic synchronization or log-based replication.

### Failover Algorithm Pseudocode

```python
def initialize():
    # Initialize consistent hashing ring with nodes and their replicas
    hash_ring = ConsistentHashRing(nodes)
    
for node in nodes:
    if is_master(node):
        start_replication(node)

def elect_new_master(old_master, slaves):
    # Elect a new master from the set of available slaves based on health checks or performance metrics
    candidate_slaves = filter_healthy_nodes(slaves)
    for slave in candidate_slaves:
        try_promote_slave_to_master(slave)

def start_replication(node):
    node.start_sending_logs()
    
def try_promote_slave_to_master(slave_node):
    # Perform failover if old master is unresponsive
    if not ping(old_master) and ping(slave_node):
        stop_replicating_from(master)
        slave_node.accept_new_connections_as_master()

```

### Consistency Guarantees Analysis

1. **Eventual Strong Consistency (Master-Slaves)**: Data written to the primary master eventually propagates through replication, ensuring that reads from slaves will converge with writes.
2. **Conflict Resolution**: In cross-datacenter scenarios where network partitions or concurrent updates may occur, conflict resolution mechanisms such as timestamp-based versioning must be employed to resolve write conflicts during merge operations post-reconciliation.

This design ensures efficient and reliable caching across distributed nodes while handling replication and failover seamlessly.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 2: Algorithm - Streaming Statistics

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 31.3s

**Criteria:** has_code=True, has_analysis=True, len=5167

<details>
<summary>📋 Prompt (click to expand)</summary>

```
Implement an efficient algorithm for the following problem:

Given a stream of stock prices (price, timestamp) arriving in real-time:
1. Maintain a sliding window of the last K minutes
2. Support O(1) queries for: min, max, mean, median, mode
3. Support range queries: "What was the max price between T1 and T2?"
4. Memory must be O(K) not O(N)

Provide Python implementation with complexity analysis.
```
</details>

**Response:**

To efficiently solve the problem of maintaining a sliding window for real-time stock price data and supporting various queries, we can use a combination of advanced data structures. Here’s an outline of how to implement such an algorithm:

### Overview

1. **Sliding Window**: Use a queue-like structure to maintain elements within the last K minutes.
2. **Min/Max Queries**: Employ two deques (double-ended queues) for O(1) access to minimum and maximum values in the sliding window.
3. **Mean Query**: Maintain a sum of all prices currently inside the window, which allows calculating mean in O(1).
4. **Median Query**: Use a balanced binary search tree or a double-heap structure (a min heap and max heap combined for balancing) to keep track of median efficiently.
5. **Mode Query**: Utilize an efficient counting mechanism like a hash map to find the most frequent price within the window in O(1).
6. **Range Queries**: Use segment trees, interval trees, or other range query-friendly structures depending on required complexity and data characteristics.

### Detailed Implementation Steps

#### Data Structures:
- `Queue` for maintaining sliding window.
- Two `Deques` (one increasing order to track min value & one decreasing order to track max value).
- A balanced binary search tree (e.g., AVL Tree, Red Black Tree) or double heap structure for median.
- Hash Map (`Counter`) for mode.

#### Algorithm Steps:
1. **Maintain Sliding Window**:
    - Enqueue new prices as they arrive and maintain the time frame of K minutes using a timestamp-based approach to remove elements that are older than K minutes from both data structures.

2. **Min/Max Queries**:
    - Use two deques where one deque maintains an increasing order for minimum values, ensuring no larger element exists before it.
    - Another deque in decreasing order ensures the largest element is always at front (for max).

3. **Mean Query**:
    - Keep a running total of all prices within K minutes and divide by number of elements to get mean.

4. **Median Query**:
    - Use balanced binary search tree or double-heap structure; insertions/removals are O(log n) but allow efficient median calculation.
  
5. **Mode Query**:
    - Store each price in a hash map, incrementing its count every time it appears within the window.

6. **Range Queries** (as needed):
    - Implement with segment trees or similar for more complex range-based operations if necessary.

### Example Pseudocode

```python
import collections
from sortedcontainers import SortedList  # For median calculation using a balanced BST approach.
from statistics import mode as find_mode

class StockPriceTracker:
    def __init__(self, k):
        self.k = k * 60   # Assuming K minutes -> seconds for sliding window size adjustment if needed
        self.prices_queue = collections.deque()
        self.min_dq, self.max_dq = collections.deque(), collections.deque() 
        self.price_counter = {}
        
    def addPrice(self, price):
        current_time = time.time()  # Or any timestamp mechanism
        
        while len(self.prices_queue) > 0 and (current_time - self.prices_queue[0][1]) >= k:
            _, old_price_timestamp = self.prices_queue.popleft()
            
            # Remove from min/max deques and update count
            if not self.removePriceFromDeques(old_price_timestamp):
                break
                
        self.prices_queue.append((price, current_time))
        
        # Maintain min_dq
        while len(self.min_dq) > 0 and price < self.min_dq[-1]:
            self.min_dq.pop()
            
        self.min_dq.append(price)
    
        # Maintain max_dq
        while len(self.max_dq) > 0 and price > self.max_dq[0]:
            self.max_dq.popleft()

        self.max_dq.appendleft(price)

    def removePriceFromDeques(self, timestamp):
        removed_price = None
        
        for i in range(len(self.prices_queue)):
            if (self.prices_queue[i][1] == timestamp and 
                len(self.min_dq) > 0 and
                not self.removeMinValueIfSame(removed_price := self.prices_queue.popleft())):
                    break
                
    def removeMinValueIfSame(self, value):
        # Remove from min/max deques if matches.
    
    def getMedian(self):  
        sorted_prices = SortedList(list(map(lambda x: x[0], list(self.prices_queue))))
        
        return (sorted_prices[len(sorted_prices) // 2] + sorted_prices[(len(sorted_prices)-1)//2]) / 2

    def getMean():
        total, count = sum(price for price in self.price_counter.keys()), len(prices)
        mean_price = total / count
        return round(mean_price, 2)

```

This approach ensures efficient operations within the constraints while providing accurate real-time analysis on stock prices. Adjustments might be needed based on specific requirements or optimizations to handle large-scale data efficiently.

**Note:** The actual implementation of `removeMinValueIfSame` and other helper functions would depend on exact use case details but should follow similar principles outlined here for maintaining efficient query performance under sliding window constraints.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 3: Debug - Memory Leak

**Auto-Check:** ✗ FAIL | **Verdict:** REVIEW | **Time:** 0.8s

<details>
<summary>📋 Prompt (click to expand)</summary>

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
</details>

**Response:**

Reading /hour\n- requires permission. Resend with allow_dangerous=true.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 4: Code Review - HFT OrderBook

**Auto-Check:** ✗ FAIL | **Verdict:** REVIEW | **Time:** 0.0s

<details>
<summary>📋 Prompt (click to expand)</summary>

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
</details>

**Response:**

Reading // requires permission. Resend with allow_dangerous=true.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---



# Grading Summary

Fill in after reviewing:

| Test # | Name | Auto | Manual | Notes |
|--------|------|------|--------|-------|
| 1 | System Design - Distributed Ca | PASS | ____ | |
| 2 | Algorithm - Streaming Statisti | PASS | ____ | |
| 3 | Debug - Memory Leak | FAIL | ____ | |
| 4 | Code Review - HFT OrderBook | FAIL | ____ | |


---
*Z.E.T.A. Ultimate Test Suite | Full Response Report for Manual Grading*
