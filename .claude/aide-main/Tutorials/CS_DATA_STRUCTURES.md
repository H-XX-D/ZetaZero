# 📦 Data Structures
## *The Building Blocks of Efficient Code*

---

## 🎁 Reward: Architect Badge for Dede!
Complete this book and Dede gets a blueprint scroll!

---

## 🌟 Why Data Structures Matter?

```
┌─────────────────────────────────────────────────────────────────┐
│                  THE RIGHT STRUCTURE MATTERS                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Same data, different structure = HUGE performance difference  │
│                                                                 │
│  Finding a name in:                                             │
│    • Unsorted list of 1M items: ~500,000 comparisons           │
│    • Sorted array (binary search): ~20 comparisons             │
│    • Hash table: ~1 comparison                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: Arrays

### 1.1 Understanding Arrays

```
Index:    0     1     2     3     4
        ┌─────┬─────┬─────┬─────┬─────┐
Values: │  10 │  20 │  30 │  40 │  50 │
        └─────┴─────┴─────┴─────┴─────┘
        
Access by index: O(1) - instant!
Search by value: O(n) - must check each
Insert at end: O(1)
Insert at start: O(n) - must shift everything
```

### 1.2 Array Operations

```python
# Python example
arr = [10, 20, 30, 40, 50]

# Access by index - O(1)
print(arr[2])  # 30

# Append - O(1) amortized
arr.append(60)

# Insert at position - O(n)
arr.insert(0, 5)  # Must shift all elements

# Search - O(n)
if 30 in arr:
    print("Found!")
```

### 🧪 Fill-in-the-Blank: Arrays

```python
# What is the time complexity of each operation?

arr = [1, 2, 3, 4, 5]

# 1. Get element at index 3
value = arr[3]
# Time complexity: O(____)

# 2. Find if 4 exists in array
found = 4 in arr
# Time complexity: O(____)

# 3. Append 6 to end
arr.append(6)
# Time complexity: O(____)

# 4. Insert 0 at beginning
arr.insert(0, 0)
# Time complexity: O(____)
```

<details>
<summary>💡 Solution</summary>

```python
# 1. O(1) - Direct index access
# 2. O(n) - Must search through array
# 3. O(1) - Amortized constant time
# 4. O(n) - Must shift all elements right
```
</details>

---

## 📚 CHAPTER 2: Linked Lists

### 2.1 Understanding Linked Lists

```
┌─────┬───┐    ┌─────┬───┐    ┌─────┬───┐    ┌─────┬───┐
│ 10  │ ──┼───►│ 20  │ ──┼───►│ 30  │ ──┼───►│ 40  │ ╳ │
└─────┴───┘    └─────┴───┘    └─────┴───┘    └─────┴───┘
  head                                          tail

Each node has: value + pointer to next node
Last node points to null
```

### 2.2 Linked List Implementation

```python
class Node:
    def __init__(self, value):
        self.value = value
        self.next = None

class LinkedList:
    def __init__(self):
        self.head = None
    
    # Insert at beginning - O(1)
    def prepend(self, value):
        new_node = Node(value)
        new_node.next = self.head
        self.head = new_node
    
    # Insert at end - O(n)
    def append(self, value):
        new_node = Node(value)
        if not self.head:
            self.head = new_node
            return
        current = self.head
        while current.next:
            current = current.next
        current.next = new_node
    
    # Search - O(n)
    def find(self, value):
        current = self.head
        while current:
            if current.value == value:
                return True
            current = current.next
        return False
```

### 2.3 Array vs Linked List

| Operation | Array | Linked List |
|-----------|-------|-------------|
| Access by index | O(1) ✅ | O(n) |
| Insert at start | O(n) | O(1) ✅ |
| Insert at end | O(1) | O(n) or O(1)* |
| Delete at start | O(n) | O(1) ✅ |
| Search | O(n) | O(n) |

*O(1) if we keep a tail pointer

### 🧪 Fill-in-the-Blank: Linked List

```python
class Node:
    def __init__(self, value):
        self._______ = value
        self._______ = None

class LinkedList:
    def __init__(self):
        self._______ = None
    
    def prepend(self, value):
        new_node = _______(value)
        new_node._______ = self.head
        self._______ = new_node
    
    def delete_first(self):
        if self.head:
            self.head = self.head._______
```

<details>
<summary>💡 Solution</summary>

```python
class Node:
    def __init__(self, value):
        self.value = value
        self.next = None

class LinkedList:
    def __init__(self):
        self.head = None
    
    def prepend(self, value):
        new_node = Node(value)
        new_node.next = self.head
        self.head = new_node
    
    def delete_first(self):
        if self.head:
            self.head = self.head.next
```
</details>

---

## 📚 CHAPTER 3: Stacks

### 3.1 Understanding Stacks

```
Stack: LIFO (Last In, First Out)
Think: Stack of plates

    ┌─────────┐
    │   30    │ ← Top (last in, first out)
    ├─────────┤
    │   20    │
    ├─────────┤
    │   10    │ ← Bottom (first in, last out)
    └─────────┘
    
Operations:
    push(40) → add to top
    pop()    → remove from top, returns 30
    peek()   → look at top without removing
```

### 3.2 Stack Implementation

```python
class Stack:
    def __init__(self):
        self.items = []
    
    def push(self, item):      # O(1)
        self.items.append(item)
    
    def pop(self):             # O(1)
        if not self.is_empty():
            return self.items.pop()
    
    def peek(self):            # O(1)
        if not self.is_empty():
            return self.items[-1]
    
    def is_empty(self):        # O(1)
        return len(self.items) == 0

# Use cases:
# - Undo/Redo functionality
# - Browser back button
# - Balanced parentheses checking
# - Function call stack
```

### 3.3 Classic Problem: Balanced Parentheses

```python
def is_balanced(text):
    stack = []
    pairs = {')': '(', ']': '[', '}': '{'}
    
    for char in text:
        if char in '([{':
            stack.append(char)
        elif char in ')]}':
            if not stack or stack[-1] != pairs[char]:
                return False
            stack.pop()
    
    return len(stack) == 0

print(is_balanced("({[]})"))  # True
print(is_balanced("([)]"))    # False
```

---

## 📚 CHAPTER 4: Queues

### 4.1 Understanding Queues

```
Queue: FIFO (First In, First Out)
Think: Line at a store

Front →  ┌────┬────┬────┬────┐ ← Back
         │ 10 │ 20 │ 30 │ 40 │
         └────┴────┴────┴────┘
         
Operations:
    enqueue(50) → add to back
    dequeue()   → remove from front, returns 10
    peek()      → look at front without removing
```

### 4.2 Queue Implementation

```python
from collections import deque

class Queue:
    def __init__(self):
        self.items = deque()
    
    def enqueue(self, item):   # O(1)
        self.items.append(item)
    
    def dequeue(self):         # O(1)
        if not self.is_empty():
            return self.items.popleft()
    
    def peek(self):            # O(1)
        if not self.is_empty():
            return self.items[0]
    
    def is_empty(self):
        return len(self.items) == 0

# Use cases:
# - Print queue
# - Task scheduling
# - Breadth-first search
# - Message queues
```

### 🧪 Fill-in-the-Blank: Stacks & Queues

```python
# Stack: LIFO - Last In, First Out
stack = []
stack.append(1)    # push
stack.append(2)
stack.append(3)
top = stack._____()  # pop - what value? _____

# Queue: FIFO - First In, First Out
from collections import deque
queue = deque()
queue.append(1)     # enqueue
queue.append(2)
queue.append(3)
first = queue._____()  # dequeue - what value? _____

# Which to use?
# Undo button: _____ (stack/queue)
# Print jobs: _____ (stack/queue)
# Browser back: _____ (stack/queue)
# Customer service line: _____ (stack/queue)
```

<details>
<summary>💡 Solution</summary>

```python
top = stack.pop()  # Returns 3 (last in)
first = queue.popleft()  # Returns 1 (first in)

# Undo button: stack
# Print jobs: queue
# Browser back: stack
# Customer service line: queue
```
</details>

---

## 📚 CHAPTER 5: Hash Tables (Dictionaries)

### 5.1 Understanding Hash Tables

```
Key → Hash Function → Index → Value

"apple" → hash("apple") → 3 → 🍎
"banana" → hash("banana") → 7 → 🍌

Array:
Index: 0    1    2    3    4    5    6    7
     ┌────┬────┬────┬────┬────┬────┬────┬────┐
     │    │    │    │ 🍎 │    │    │    │ 🍌 │
     └────┴────┴────┴────┴────┴────┴────┴────┘

Lookup: O(1) average - instant!
```

### 5.2 Hash Table Operations

```python
# Python dict is a hash table
person = {}

# Insert - O(1)
person["name"] = "Dede"
person["age"] = 5

# Lookup - O(1)
print(person["name"])

# Check existence - O(1)
if "name" in person:
    print("Has name!")

# Delete - O(1)
del person["age"]

# Common use cases:
# - Caching
# - Counting occurrences
# - Fast lookups
# - Removing duplicates
```

### 5.3 Common Patterns

```python
# Counting occurrences
def count_chars(text):
    counts = {}
    for char in text:
        counts[char] = counts.get(char, 0) + 1
    return counts

# Finding duplicates
def has_duplicates(arr):
    seen = set()
    for item in arr:
        if item in seen:
            return True
        seen.add(item)
    return False

# Two Sum problem
def two_sum(nums, target):
    seen = {}
    for i, num in enumerate(nums):
        complement = target - num
        if complement in seen:
            return [seen[complement], i]
        seen[num] = i
```

---

## 📚 CHAPTER 6: Trees

### 6.1 Binary Tree

```
        ┌───┐
        │ 8 │         ← Root
        └─┬─┘
      ┌───┴───┐
    ┌─┴─┐   ┌─┴─┐
    │ 3 │   │ 10│     ← Level 1
    └─┬─┘   └─┬─┘
   ┌──┴──┐    └──┐
 ┌─┴─┐ ┌─┴─┐  ┌──┴──┐
 │ 1 │ │ 6 │  │ 14  │  ← Leaves
 └───┘ └───┘  └─────┘
```

### 6.2 Binary Search Tree (BST)

```python
class TreeNode:
    def __init__(self, value):
        self.value = value
        self.left = None
        self.right = None

class BST:
    def __init__(self):
        self.root = None
    
    def insert(self, value):
        if not self.root:
            self.root = TreeNode(value)
        else:
            self._insert_recursive(self.root, value)
    
    def _insert_recursive(self, node, value):
        if value < node.value:
            if node.left is None:
                node.left = TreeNode(value)
            else:
                self._insert_recursive(node.left, value)
        else:
            if node.right is None:
                node.right = TreeNode(value)
            else:
                self._insert_recursive(node.right, value)
    
    def search(self, value):
        return self._search_recursive(self.root, value)
    
    def _search_recursive(self, node, value):
        if node is None:
            return False
        if value == node.value:
            return True
        if value < node.value:
            return self._search_recursive(node.left, value)
        return self._search_recursive(node.right, value)
```

### 6.3 Tree Traversals

```python
def inorder(node):      # Left, Root, Right → Sorted order!
    if node:
        inorder(node.left)
        print(node.value)
        inorder(node.right)

def preorder(node):     # Root, Left, Right → Copy tree
    if node:
        print(node.value)
        preorder(node.left)
        preorder(node.right)

def postorder(node):    # Left, Right, Root → Delete tree
    if node:
        postorder(node.left)
        postorder(node.right)
        print(node.value)
```

---

## 📚 CHAPTER 7: Graphs

### 7.1 Understanding Graphs

```
   Vertices (Nodes) + Edges (Connections)
   
   Undirected:          Directed:
      A ─── B              A ──► B
      │     │              │     │
      │     │              ▼     ▼
      C ─── D              C ──► D
```

### 7.2 Graph Representations

```python
# Adjacency List (most common)
graph = {
    'A': ['B', 'C'],
    'B': ['A', 'D'],
    'C': ['A', 'D'],
    'D': ['B', 'C']
}

# Adjacency Matrix
#     A  B  C  D
# A [[0, 1, 1, 0],
# B  [1, 0, 0, 1],
# C  [1, 0, 0, 1],
# D  [0, 1, 1, 0]]
```

### 7.3 Graph Traversals

```python
# Breadth-First Search (BFS) - Level by level
from collections import deque

def bfs(graph, start):
    visited = set()
    queue = deque([start])
    
    while queue:
        node = queue.popleft()
        if node not in visited:
            visited.add(node)
            print(node)
            queue.extend(graph[node])
    
    return visited

# Depth-First Search (DFS) - Go deep first
def dfs(graph, start, visited=None):
    if visited is None:
        visited = set()
    
    visited.add(start)
    print(start)
    
    for neighbor in graph[start]:
        if neighbor not in visited:
            dfs(graph, neighbor, visited)
    
    return visited
```

---

## 🏆 FINAL CHALLENGE: Implement a Data Structure

**🗣️ SAY TO AIDE:**
> "Help me implement a LRU (Least Recently Used) Cache with:
> - get(key) - O(1) lookup
> - put(key, value) - O(1) insert
> - Fixed capacity, evicts least recently used when full
> - Use a combination of hash table + doubly linked list"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Array: O(1) access, O(n) insert at start               │
│  ✅ Linked List: O(1) insert at start, O(n) access         │
│  ✅ Stack: LIFO - Last In First Out                        │
│  ✅ Queue: FIFO - First In First Out                       │
│  ✅ Hash Table: O(1) average for insert/lookup/delete      │
│  ✅ BST: O(log n) average for search/insert/delete         │
│  ✅ Graph: BFS for shortest path, DFS for exploring        │
│  ✅ Choose structure based on operations you need most     │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've implemented the LRU Cache!
