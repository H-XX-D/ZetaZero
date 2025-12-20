# Z.E.T.A. Ultimate Test Report

**Date:** 2025-12-19 22:24:34
**Duration:** 111.9s
**Server:** http://192.168.0.165:8080

## Quick Summary

| Metric | Value |
|--------|-------|
| Auto-Pass | 4/4 (100.0%) |
| Needs Review | 4 tests |
| Errors | 0 |

## Summary by Category

| Category | Auto-Pass | Total | Notes |
|----------|-----------|-------|-------|
| Senior Coding | 4/4 | 4 | 4 need review |


---

# Full Test Results (For Manual Grading)

Each test below includes the full prompt and response for evaluation.
Mark each with your own grade: GOOD / OK / POOR / FAIL


## Senior Coding

### Test 1: System Design - Distributed Cache

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 24.8s

**Criteria:** Found 4/4 keywords, len=3670

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
+-----------------+
|  Client Layer   |
|                 |
|    +--------+   |
|    |        |<--+
|    |        |   |
|    +--------+   |
|     /|\       \ /
|      V         X
|    +---+------+ |
|    | LoadBalancer|
|    +---+------+  |
|                   |
|                  / \
|                 /   \
|                v     v
|          +-----+   +----+
|          |DC1  |   | DC2|
|          +--+--++  ++---+
|             ||               |
|            / \              /\
|           V   V            VXV
|      +-------+------    +-------+
|      |Shard A1 Slave|->|Shard B1Slave|
|      +----^-----+     +-----------+
|          /|\                ||
|         /  \               / \
|       /        \           v   v
|    +--+---++------+   ++------+-++
|    |Master||SlaveA2|<>|MasterB2|S |
|    +-------++      +--+--------+--+
|              ||                  ||
|             \|/                 \|/
|         +----+-----+        +----+----+
|         |Shard A3  |<->     | Shard B3|
|         +----------+        +---------+

```

### Key Data Structures

1. **ConcurrentHashMap**: For local storage of cache keys and values.
2. **RingBuffer/Hash Ring**: Used for consistent hashing to distribute shards across nodes.
3. **Replica List**: Keeps track of slave replicas for each shard.
4. **Heartbeat Mechanism**: Monitors the health status of master/slave nodes.

### Failover Algorithm Pseudocode

```python
def failover(shard_id, current_master):
    # Step 1: Detect failure (heartbeat timeout)
    if isMasterDead(current_master) == True:
        print("Detected that Master Shard " + str(shard_id) + " went down")

    # Step 2: Promote a slave to master
    new_master = promoteSlaveToMaster(shard_id)

    # Step 3: Update routing table with the new master's address
    updateRoutingTable(new_master, shard_id)
    
def isMasterDead(current_master):
    return heartbeatTimeout(current_master) > threshold

def promoteSlaveToMaster(shard_id):
    slaves_list = getReplicaListForShard(shard_id)

    # Promote first available slave to master 
    for slave in slaves_list:
        if isConnected(slave):  # Check connectivity
            electAsNewMaster(slave)
            return slave
            
def updateRoutingTable(new_master, shard_id):
    routing_table[shard_id] = new_master

```

### Consistency Guarantees Analysis

1. **Eventual Consistency**: Due to the nature of replication and failover mechanisms in a distributed system, consistency is achieved eventually as slaves are updated with master data.
2. **Read-after-Write (CA)**: Clients will always see their own writes immediately if they write to the primary replica.
3. **Conflict Resolution**:
   - Cross-datacenter replication introduces potential conflicts due to network latency or partitions.
   - Conflict resolution strategies such as Last Write Wins, Vector Clocks/Version Numbers can be employed for resolving concurrent updates.

### Memory-Efficient Storage Techniques

1. **TTL (Time To Live)**: Automatically evict keys that haven’t been accessed in a specified time period.
2. **Eviction Policies**: Implement LRU or LFU policies to remove least-recently used/least frequently used items when memory pressure is high.
3. **Compression**: Use data compression algorithms like Snappy, Zstandard for storing serialized objects and reduce storage footprint.

This architecture ensures scalability through sharding, fault-tolerance via replication mechanisms, and cross-datacenter resilience by providing conflict resolution strategies suitable for large-scale deployments handling millions of keys efficiently.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 2: Algorithm - Streaming Statistics

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 28.5s

**Criteria:** has_code=True, has_analysis=True, len=4832

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

To implement an efficient algorithm that meets the requirements for maintaining a sliding window of stock prices and supporting O(1) time complexity queries (min, max), as well as range-based min/max/mean/mode calculations within this window, we need to break down each functionality:

### 1. Sliding Window Management

- Use a `Deque` or `LinkedHashMap` for maintaining the sliding window of stock prices based on timestamps.
    - This allows efficient insertion and removal of elements at both ends.

### 2. O(1) Queries
#### Min/Max:
- Maintain two priority queues (min heap, max heap) to track minimums and maximums efficiently within the current window.
  
#### Mean:
- Keep a running sum of prices in the sliding window and divide by `K` for mean calculation.

### 3. Median Calculation
- Use either a balanced binary search tree (such as an AVL Tree or Red-Black Tree) to maintain order statistics, or use two heaps: one max heap for lower half elements and another min heap for higher half.
    - This allows insertion/removal in logarithmic time while providing access to the median.

### 4. Mode Calculation
- Use a frequency map (hashmap) that tracks counts of each price within the window.
  
#### Range Queries:
For range queries, you need additional structures or mechanisms:

1. **Range Min/Max:**
   - Store ranges and their min/max values in a segment tree or sparse table for O(log N) query time.

2. **Range Mean/Median/Mode:**
   - For mean, maintain prefix sums to calculate the sum within any range in constant time.
   - Median can be more complex; consider using dynamic data structures that support deletions and insertions efficiently (like an augmented balanced binary search tree).
   - Mode calculation would involve tracking frequencies of elements dynamically as they enter or leave the window.

### Implementation Outline

```java
import java.util.*;
import java.awt.*;

public class StockPriceTracker {
    private Deque<Integer> pricesDeque = new LinkedList<>();
    private TreeMap<Double, Integer> frequencyMap = new TreeMap<>(); // for mode calculation.
    
    int K; // Sliding Window Size
    
    public StockPriceTracker(int k) { 
        this.K = k;
    }

    // Function to add a price at the current timestamp
    public void add(double price) {
        
        pricesDeque.addLast(price);
                
        if(pricesDeque.size() > K){
            double removedValue = pricesDeque.removeFirst();
            
            frequencyMap.put(removedValue, frequencyMap.getOrDefault(removedValue, 0) - 1);
            if (frequencyMap.get(removedValue) == 0)
                frequencyMap.remove(removedValue);
        }
        
        // Update mode
        frequencyMap.put(price, frequencyMap.getOrDefault(price, 0) + 1);

    }

    public double min() { 
       return Collections.min(pricesDeque); 
    }

    public double max() {
      return Collections.max(pricesDeque);
    } 

    public double mean() {
         double sum = pricesDeque.stream().mapToDouble(val -> val).sum();
        return K > 0 ? sum / K : Double.NaN;
    }
    
    // Median calculation is more complex and typically uses heaps or balanced BSTs
    public double median(){
       List<Double> sortedPrices = new ArrayList<>(pricesDeque);
       Collections.sort(sortedPrices); 
       
       int index = (K - 1) >> 1; 

       if ((K & 1) == 0)
           return (sortedPrices.get(index) + sortedPrices.get(index+1)) / 2;
           
        else
            return sortedPrices.get((index));
    }

    public double mode(){
      // Get the entry with maximum value from frequency map
         Map.Entry<Double, Integer> maxEntry = 
          Collections.max(frequencyMap.entrySet(), Map.Entry.comparingByValue());
         
       if(maxEntry.getValue() == 1) return Double.NaN; // no clear mode.
       
        else {
            double[] modes = new double[maxEntry.getValue()];
           
           int index = 0;
            
            for(Map.Entry<Double, Integer> entry : frequencyMap.entrySet()){
                if(entry.getValue().equals(maxEntry.getValue())){
                    modes[index++] = (double)entry.getKey();
                }
            } 
          
          // Return the first mode found
           return modes[0];
        }

    }
    
}

```

### Notes:
- The current implementation supports O(1) addition but does not fully satisfy all requirements due to complexities with range queries and exact median calculation.
- For full support, consider advanced data structures like segment trees for efficient range queries or specialized heaps/sorted collections.

This should give you a good starting point. Depending on the specific constraints of your application (e.g., whether real-time updates are required), further optimizations might be necessary!

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 3: Debug - Memory Leak

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 24.0s

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

To debug the OOM (Out of Memory) issue in your Kubernetes pods, you can follow these steps to identify and resolve the problem:

1. **Analyze Heap Dump:**
   - You've already noted that Java heap dumps show String objects dominating with most being UUID-like strings.
   
2. **Identify Leaking Objects:**
   - Use tools like `VisualVM`, `Eclipse Memory Analyzer (MAT)`, or `jmap` to analyze the heap dump and identify which specific code paths are allocating these string objects excessively.

3. **Check String Usage Patterns:**
   - UUIDs can be created frequently in applications due to their use as unique identifiers.
     - Ensure that you're not creating unnecessary UUID instances:
       ```java
       // Avoid this if it's being called too often and not stored properly afterwards
       UUID.randomUUID();
       
       // Use caching or reuse patterns when possible
       private static final AtomicLong uuidCounter = new AtomicLong(System.nanoTime());
       public synchronized String generateUUID() {
           return Long.toHexString(uuidCounter.getAndIncrement()).toUpperCase();
       }
       ```
     - Check if these strings are being stored in collections like `ConcurrentHashMap` without proper cleanup or removal.
   
4. **Review Application Code:**
   - Review the code where UUIDs and other long-lived string objects are created to ensure they're not being retained longer than necessary:
     ```java
     public void process(UUID id) {
         // Ensure id is used efficiently and doesn't cause memory leaks
         map.put(id, someValue);  // Check if `map` can be cleaned up properly after use.
         
         // If you don’t need the UUID anymore, consider removing it or using weak references:
         WeakHashMap<UUID, Object> weakMap = new WeakHashMap<>();
     }
     
     // Avoid storing in places like static variables
     public class MyClass {
         private static final Set<String> uuidSet = Collections.newSetFromMap(new ConcurrentHashMap<>());
         
         @PostConstruct 
         void init() {  
             for (int i=0; i<10000; ++i)  // This can fill up memory quickly without cleanup
                 uuidSet.add(UUID.randomUUID().toString());   
         }    
     }
     ```

5. **Tune JVM Settings:**
   - If the issue persists, you might need to adjust your JVM settings:
     ```properties
     # Increase maximum heap size if necessary but ensure it aligns with available resources on Kubernetes nodes.
     Xmx2048m
     
     # Enable G1GC which is better for pause time and large heaps compared to CMS or other GC algorithms.
     -XX:+UseG1GC 
     
     # Set the survivor ratio (adjust based on application behavior)
     -XX:SurvivorRatio=6
     ```

6. **Monitor Resource Usage with Prometheus/Grafana:**
   - Use Kubernetes monitoring tools such as `Prometheus` and `Grafana` to monitor resource consumption over time.
   
7. **Optimize Data Structures for Memory Efficiency:**
   - If you’re storing UUIDs in collections, consider using more memory-efficient data structures like primitive arrays or compact maps if possible.

8. **Garbage Collection Logs Analysis:**
   - Enable GC logging and analyze it to understand the garbage collection pattern.
     ```sh
     JAVA_OPTS="-XX:+PrintGCDetails -Xloggc:/path/to/gc.log"
     ```
   
9. **Kubernetes Resource Limits:**
   - Make sure your Kubernetes pods have appropriate memory limits set in their deployment configuration:
     ```yaml
     resources:
       requests:
         memory: "2Gi" 
       limits:
         memory: "4Gi"
     ```

By following these steps, you should be able to pinpoint the cause of string object accumulation and address it appropriately. This will help prevent your Kubernetes pods from being OOM-killed due to excessive memory usage over time.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---

### Test 4: Code Review - HFT OrderBook

**Auto-Check:** ✓ PASS | **Verdict:** REVIEW | **Time:** 34.6s

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

### Review and Refactoring of the Rust Code

The provided code snippet is designed to match orders within an order book for a high-frequency trading system. However, there are several potential issues related to correctness, performance, safety, and best practices in concurrent programming with Rust.

#### Issues Identified:

1. **Concurrency Safety**: 
   - The use of `unwrap()` on mutex locks can lead to panics if the lock operation fails.
   
2. **Performance**:
   - Frequent locking/unlocking operations might impact performance.
   - Using vectors (`Vec<Order>`) inside a hash map for bids and asks could be inefficient when dealing with many orders at various price levels.

3. **Correctness**:
   - The `match_order` function does not handle the case where there are multiple best ask prices available or ensure that only relevant orders from different price buckets are matched.
   
4. **Safety**:
   - Using raw locks (`lock().unwrap()`) is generally discouraged in favor of more idiomatic and safer alternatives.

### Refactored Code

Here's a refactored version of the `OrderBook` struct along with its methods, aiming to address these issues:

```rust
use std::collections::{HashMap, VecDeque};
use std::sync::{Arc, Mutex};

#[derive(Debug)]
struct Order {
    price: u64,
    quantity: u32,
}

#[derive(Debug)]
struct Trade {
    buyer_order: Order,
    seller_order: Order,
    trade_price: u64,
    trade_quantity: u32
}

impl Trade {
    fn new(buyer_order: Order, seller_order: Order) -> Self {
        let min_quantity = std::cmp::min(buyer_order.quantity, seller_order.quantity);
        Trade { 
            buyer_order, 
            seller_order, 
            trade_price: buyer_order.price,
            trade_quantity: min_quantity
        }
    }
}

struct OrderBook {
    bids: Arc<Mutex<HashMap<u64, VecDeque<Order>>>>,  // Use a queue to maintain order insertion time
    asks: Arc<Mutex<HashMap<u64, VecDeque<Order>>>>,  
    last_trade: Mutex<Option<Trade>>,
}

impl OrderBook {

    fn new() -> Self {
        OrderBook { 
            bids: Default::default(),
            asks: Default::default(), 
            last_trade: Default::default()
        }
    }

    // Ensure thread-safe access and error handling
    fn match_order(&self, order: Order) -> Vec<Trade> {

        let mut trades = vec![];
        
        if !order.quantity > 0 {
            return trades;
        }

        let best_asks_mutex = self.asks.clone();
        let bids_guard = self.bids.lock().unwrap(); 
        let asks_guard = match best_asks_mutex.try_lock() { // Try to avoid blocking
            Ok(guard) => guard,
            Err(_)    => return vec![],  // Return early if we can't acquire the lock
        };

        let mut bids_ref: &HashMap<u64, VecDeque<Order>> = &bids_guard;
        
        while order.quantity > 0 {
            
            match asks_guard.get(&order.price) { 
                Some(asks_queue) => {

                    if !asks_queue.is_empty() && order.price <= *asks_queue.front().unwrap().price {
                        let mut best_ask_order = asks_queue.pop_front();
                        
                        if let Some(best_ask) = best_ask_order.as_mut() {
                            // Match orders
                            match bids_ref.get(&order.price).and_then(|bids| !bids.is_empty()) { 
                                true => {
                                    let matched_bid = &mut **bids_ref.get_mut(&order.price).unwrap().front_mut();
                                    
                                    if order.quantity >= best_ask.quantity {
                                        trades.push(Trade::new(matched_bid.clone(), best_ask_order.unwrap()));
                                        // Remove fully filled orders
                                        bids_ref.get_mut(&order.price)
                                            .map(|vec| vec.drain_filter(|bid| bid.quantity == 0))
                                            .unwrap();
                                    } else if order.quantity < best_ask.quantity {
                                        trades.push(Trade::new(matched_bid.clone(), Order { quantity: order.quantity, ..best_ask_order.unwrap() }));
                                        
                                        let remaining_quantity = best_ask.quantity - order.quantity;
                                        *asks_queue.front_mut().unwrap().quantity -= order.quantity;

                                        // Add partially filled orders back to queue
                                        asks_queue.push_back(Order{ price: best_ask.price, quantity: remaining_quantity });
                                    }
                                },
                                false => break,
                            }

                        } else {
                            break;  // No more valid asks left for this price level.
                        }
                    } 
                },  
                
                None => { break; }
            }
        }
        
        let _ = self.last_trade.lock().unwrap_or_else(|_| panic!("Failed to lock last trade"));
        *self.last_trade.get_mut() = Some(trades.into_iter()
                                                  .max_by_key(|t| t.trade_quantity) // Return the latest trade
                                                  .cloned());
    
        trades
    }
}
```

### Explanation of Changes:

1. **Concurrency Control**:
   - Used `Arc<Mutex<HashMap<...>>>` for shared ownership and locking.
   - Employed `try_lock()` on mutexes to avoid blocking calls, which can cause deadlocks or unnecessary wait times.

2. **Efficiency**:
   - Utilized `VecDeque<Order>` instead of `Vec<Order>`, as it provides efficient insertion/deletion at both ends (useful for FIFO orders).
   
3. **Correctness and Safety**:
   - Added checks to ensure only valid bids/asks are matched.
   - Removed unnecessary locks or used try_locks where appropriate.

4. **Performance**:
   - Reduced the number of lock/unlock cycles by combining operations within a single guard scope when possible (e.g., handling asks and bids together).

This refactored code should be more robust, efficient, and safer for high-frequency trading environments in Rust.

**Manual Grade:** [ ] GOOD  [ ] OK  [ ] POOR  [ ] FAIL

**Notes:** _______________________________________

---



# Grading Summary

Fill in after reviewing:

| Test # | Name | Auto | Manual | Notes |
|--------|------|------|--------|-------|
| 1 | System Design - Distributed Ca | PASS | ____ | |
| 2 | Algorithm - Streaming Statisti | PASS | ____ | |
| 3 | Debug - Memory Leak | PASS | ____ | |
| 4 | Code Review - HFT OrderBook | PASS | ____ | |


---
*Z.E.T.A. Ultimate Test Suite | Full Response Report for Manual Grading*
