# ⚡ Algorithms
## *The Art of Problem Solving*

---

## 🎁 Reward: Speedster Badge for Dede!
Complete this book and Dede gets lightning bolt accessories!

---

## 🌟 Why Algorithms Matter?

```
┌─────────────────────────────────────────────────────────────────┐
│                 ALGORITHMS = RECIPES FOR SOLVING                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Same problem, different algorithm = HUGE time difference      │
│                                                                 │
│  Sorting 1 million items:                                       │
│    • Bubble Sort: ~1,000,000,000,000 operations (days)         │
│    • Quick Sort:  ~20,000,000 operations (seconds)             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Big O Notation

### 1.1 What is Big O?

Big O describes how an algorithm's time (or space) grows as input grows.

```
Common Complexities (best to worst):

O(1)       - Constant    - Same time regardless of input
O(log n)   - Logarithmic - Cuts problem in half each step
O(n)       - Linear      - Scales directly with input
O(n log n) - Linearithmic - Good sorting algorithms
O(n²)      - Quadratic   - Nested loops
O(2ⁿ)      - Exponential - Doubles each step (BAD!)
O(n!)      - Factorial   - Permutations (VERY BAD!)
```

### 1.2 Visualizing Big O

```
Time │
     │                                           O(n!)
     │                                         /
     │                                       /  O(2ⁿ)
     │                                     /  /
     │                                   /  /
     │                            -----/  /   O(n²)
     │                      -----     / /
     │               ------        / /        O(n log n)
     │        ------            / /
     │   ----               ---              O(n)
     │ --             -----                  O(log n)
     │-----------                            O(1)
     └───────────────────────────────────────────► Input Size
```

### 🧪 Fill-in-the-Blank: Big O

```python
# What is the Big O of each function?

# 1. Get first element
def get_first(arr):
    return arr[0]
# Big O: O(_____)

# 2. Find element in array
def find(arr, target):
    for item in arr:
        if item == target:
            return True
    return False
# Big O: O(_____)

# 3. Check all pairs
def has_duplicate_pair(arr):
    for i in range(len(arr)):
        for j in range(i + 1, len(arr)):
            if arr[i] == arr[j]:
                return True
    return False
# Big O: O(_____)

# 4. Binary search
def binary_search(arr, target):
    left, right = 0, len(arr) - 1
    while left <= right:
        mid = (left + right) // 2
        if arr[mid] == target:
            return mid
        elif arr[mid] < target:
            left = mid + 1
        else:
            right = mid - 1
    return -1
# Big O: O(_____)
```

<details>
<summary>💡 Solution</summary>

```python
# 1. O(1) - Constant, just one operation
# 2. O(n) - Linear, may check every element
# 3. O(n²) - Quadratic, nested loops
# 4. O(log n) - Logarithmic, halves search space each time
```
</details>

---

## 📚 CHAPTER 2: Searching Algorithms

### 2.1 Linear Search

```python
def linear_search(arr, target):
    """O(n) - Check each element"""
    for i, item in enumerate(arr):
        if item == target:
            return i
    return -1
```

### 2.2 Binary Search

```python
def binary_search(arr, target):
    """O(log n) - Requires sorted array!"""
    left, right = 0, len(arr) - 1
    
    while left <= right:
        mid = (left + right) // 2
        
        if arr[mid] == target:
            return mid
        elif arr[mid] < target:
            left = mid + 1
        else:
            right = mid - 1
    
    return -1

# Example:
# arr = [1, 3, 5, 7, 9, 11, 13]
# target = 7
# Step 1: mid = 3, arr[3] = 7 = target ✓ Found!
```

### 🧪 Fill-in-the-Blank: Binary Search

```python
def binary_search(arr, target):
    left, right = ____, len(arr) - ____
    
    while left ________ right:
        mid = (left + right) // ____
        
        if arr[mid] == target:
            return ____
        elif arr[mid] ____ target:
            left = mid + 1
        else:
            right = mid ____ 1
    
    return ____
```

<details>
<summary>💡 Solution</summary>

```python
def binary_search(arr, target):
    left, right = 0, len(arr) - 1
    
    while left <= right:
        mid = (left + right) // 2
        
        if arr[mid] == target:
            return mid
        elif arr[mid] < target:
            left = mid + 1
        else:
            right = mid - 1
    
    return -1
```
</details>

---

## 📚 CHAPTER 3: Sorting Algorithms

### 3.1 Bubble Sort (Simple but Slow)

```python
def bubble_sort(arr):
    """O(n²) - Compare adjacent pairs, bubble up largest"""
    n = len(arr)
    for i in range(n):
        for j in range(n - i - 1):
            if arr[j] > arr[j + 1]:
                arr[j], arr[j + 1] = arr[j + 1], arr[j]
    return arr

# Visualization:
# [5, 3, 8, 1] → [3, 5, 8, 1] → [3, 5, 8, 1] → [3, 5, 1, 8]
#                                              8 bubbled up!
```

### 3.2 Selection Sort

```python
def selection_sort(arr):
    """O(n²) - Find minimum, put at start"""
    n = len(arr)
    for i in range(n):
        min_idx = i
        for j in range(i + 1, n):
            if arr[j] < arr[min_idx]:
                min_idx = j
        arr[i], arr[min_idx] = arr[min_idx], arr[i]
    return arr
```

### 3.3 Merge Sort (Efficient!)

```python
def merge_sort(arr):
    """O(n log n) - Divide and conquer"""
    if len(arr) <= 1:
        return arr
    
    mid = len(arr) // 2
    left = merge_sort(arr[:mid])
    right = merge_sort(arr[mid:])
    
    return merge(left, right)

def merge(left, right):
    result = []
    i = j = 0
    
    while i < len(left) and j < len(right):
        if left[i] <= right[j]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    
    result.extend(left[i:])
    result.extend(right[j:])
    return result

# Visualization:
#        [38, 27, 43, 3]
#        /            \
#    [38, 27]      [43, 3]
#     /    \        /    \
#   [38]  [27]   [43]   [3]
#     \    /        \    /
#    [27, 38]      [3, 43]
#        \            /
#      [3, 27, 38, 43]
```

### 3.4 Quick Sort (Most Common)

```python
def quick_sort(arr):
    """O(n log n) average - Pick pivot, partition"""
    if len(arr) <= 1:
        return arr
    
    pivot = arr[len(arr) // 2]
    left = [x for x in arr if x < pivot]
    middle = [x for x in arr if x == pivot]
    right = [x for x in arr if x > pivot]
    
    return quick_sort(left) + middle + quick_sort(right)
```

### Sorting Comparison

| Algorithm | Best | Average | Worst | Space | Stable |
|-----------|------|---------|-------|-------|--------|
| Bubble | O(n) | O(n²) | O(n²) | O(1) | Yes |
| Selection | O(n²) | O(n²) | O(n²) | O(1) | No |
| Merge | O(n log n) | O(n log n) | O(n log n) | O(n) | Yes |
| Quick | O(n log n) | O(n log n) | O(n²) | O(log n) | No |

---

## 📚 CHAPTER 4: Recursion

### 4.1 Understanding Recursion

```python
# Base case + Recursive case

def factorial(n):
    # Base case - when to stop
    if n <= 1:
        return 1
    
    # Recursive case - call yourself
    return n * factorial(n - 1)

# factorial(4)
# 4 * factorial(3)
# 4 * (3 * factorial(2))
# 4 * (3 * (2 * factorial(1)))
# 4 * (3 * (2 * 1))
# 4 * (3 * 2)
# 4 * 6
# 24
```

### 4.2 Classic Recursive Problems

```python
# Fibonacci
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

# Sum of array
def array_sum(arr):
    if len(arr) == 0:
        return 0
    return arr[0] + array_sum(arr[1:])

# Reverse string
def reverse(s):
    if len(s) <= 1:
        return s
    return reverse(s[1:]) + s[0]
```

### 🧪 Fill-in-the-Blank: Recursion

```python
# Power function: calculate base^exponent
def power(base, exponent):
    # Base case
    if exponent == ____:
        return ____
    
    # Recursive case
    return base ____ power(base, exponent - ____)

# Test
print(power(2, 4))  # Should print 16

# Count occurrences in list
def count(arr, target):
    # Base case
    if len(arr) == ____:
        return ____
    
    # Check first element
    found = ____ if arr[0] == target else ____
    
    # Recursive case
    return found ____ count(arr[____:], target)
```

<details>
<summary>💡 Solution</summary>

```python
def power(base, exponent):
    if exponent == 0:
        return 1
    return base * power(base, exponent - 1)

def count(arr, target):
    if len(arr) == 0:
        return 0
    found = 1 if arr[0] == target else 0
    return found + count(arr[1:], target)
```
</details>

---

## 📚 CHAPTER 5: Dynamic Programming

### 5.1 What is Dynamic Programming?

```
DP = Recursion + Memoization (Caching)

Fibonacci without DP: O(2ⁿ) - recalculates same values
Fibonacci with DP:    O(n)  - each value calculated once

                    fib(5)
                   /      \
               fib(4)     fib(3)    ← fib(3) calculated twice!
               /    \     /    \
           fib(3) fib(2) fib(2) fib(1)
           /    \
       fib(2) fib(1)
       
With memoization, we cache fib(3) and reuse it!
```

### 5.2 Top-Down (Memoization)

```python
def fibonacci_memo(n, memo={}):
    if n in memo:
        return memo[n]
    if n <= 1:
        return n
    
    memo[n] = fibonacci_memo(n - 1, memo) + fibonacci_memo(n - 2, memo)
    return memo[n]
```

### 5.3 Bottom-Up (Tabulation)

```python
def fibonacci_tab(n):
    if n <= 1:
        return n
    
    dp = [0] * (n + 1)
    dp[1] = 1
    
    for i in range(2, n + 1):
        dp[i] = dp[i - 1] + dp[i - 2]
    
    return dp[n]
```

### 5.4 Classic DP: Climbing Stairs

```python
# How many ways to climb n stairs (1 or 2 steps at a time)?
def climb_stairs(n):
    if n <= 2:
        return n
    
    dp = [0] * (n + 1)
    dp[1] = 1
    dp[2] = 2
    
    for i in range(3, n + 1):
        dp[i] = dp[i - 1] + dp[i - 2]
    
    return dp[n]
```

---

## 📚 CHAPTER 6: Graph Algorithms

### 6.1 BFS - Shortest Path (Unweighted)

```python
from collections import deque

def shortest_path(graph, start, end):
    if start == end:
        return [start]
    
    visited = {start}
    queue = deque([(start, [start])])
    
    while queue:
        node, path = queue.popleft()
        
        for neighbor in graph[node]:
            if neighbor == end:
                return path + [neighbor]
            if neighbor not in visited:
                visited.add(neighbor)
                queue.append((neighbor, path + [neighbor]))
    
    return None  # No path found
```

### 6.2 DFS - Cycle Detection

```python
def has_cycle(graph):
    visited = set()
    rec_stack = set()
    
    def dfs(node):
        visited.add(node)
        rec_stack.add(node)
        
        for neighbor in graph.get(node, []):
            if neighbor not in visited:
                if dfs(neighbor):
                    return True
            elif neighbor in rec_stack:
                return True
        
        rec_stack.remove(node)
        return False
    
    for node in graph:
        if node not in visited:
            if dfs(node):
                return True
    
    return False
```

### 6.3 Dijkstra's Algorithm (Weighted Shortest Path)

```python
import heapq

def dijkstra(graph, start):
    distances = {node: float('inf') for node in graph}
    distances[start] = 0
    pq = [(0, start)]
    
    while pq:
        current_dist, current_node = heapq.heappop(pq)
        
        if current_dist > distances[current_node]:
            continue
        
        for neighbor, weight in graph[current_node].items():
            distance = current_dist + weight
            
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                heapq.heappush(pq, (distance, neighbor))
    
    return distances
```

---

## 📚 CHAPTER 7: Common Patterns

### 7.1 Two Pointers

```python
# Find pair that sums to target (sorted array)
def two_sum_sorted(arr, target):
    left, right = 0, len(arr) - 1
    
    while left < right:
        current_sum = arr[left] + arr[right]
        
        if current_sum == target:
            return [left, right]
        elif current_sum < target:
            left += 1
        else:
            right -= 1
    
    return []
```

### 7.2 Sliding Window

```python
# Max sum of k consecutive elements
def max_sum_subarray(arr, k):
    window_sum = sum(arr[:k])
    max_sum = window_sum
    
    for i in range(k, len(arr)):
        window_sum += arr[i] - arr[i - k]
        max_sum = max(max_sum, window_sum)
    
    return max_sum
```

### 7.3 Fast & Slow Pointers

```python
# Detect cycle in linked list
def has_cycle(head):
    slow = fast = head
    
    while fast and fast.next:
        slow = slow.next
        fast = fast.next.next
        
        if slow == fast:
            return True
    
    return False
```

---

## 🏆 FINAL CHALLENGE: Solve These Problems

**🗣️ SAY TO AIDE:**
> "Help me solve these algorithm problems:
> 1. Find all pairs in array that sum to target
> 2. Find longest substring without repeating characters
> 3. Merge two sorted arrays in-place
> 4. Find minimum in rotated sorted array
> Include Big O analysis for each solution"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Big O describes growth, not exact time                 │
│  ✅ Binary search: O(log n), requires sorted data          │
│  ✅ Quick/Merge sort: O(n log n) - use for large data      │
│  ✅ Recursion: base case + recursive case                  │
│  ✅ DP: Cache repeated calculations                        │
│  ✅ BFS for shortest path, DFS for exploring               │
│  ✅ Two pointers: sorted arrays                            │
│  ✅ Sliding window: subarray problems                      │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've solved all 4 challenge problems!
