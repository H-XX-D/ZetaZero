# 🔗 AI Course 7: Chain of Thought Prompting
## *Make AI Show Its Work*

---

## 🎮 COURSE OVERVIEW

**What is Chain of Thought?**

Chain of Thought (CoT) prompting is a technique that forces AI to reason step-by-step before answering. Instead of jumping to conclusions, AI "shows its work."

**Why It Works:**

AI performs dramatically better on complex tasks when it reasons through problems explicitly rather than giving immediate answers.

```
WITHOUT CoT:                    WITH CoT:
Q: Complex problem             Q: Complex problem
A: Wrong answer (50%)          A: Step 1: ...
                                  Step 2: ...
                                  Step 3: ...
                                  Answer: Correct (80-90%)
```

**When to Use:**
- Complex logic problems
- Multi-step tasks
- Debugging
- Architecture decisions
- Anything that requires "thinking"

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - CoT patterns
- 🧠 Final Test - AI uncooperative, force reasoning anyway

---

# PART 1: CHAIN OF THOUGHT FUNDAMENTALS

## 📚 How Chain of Thought Works

### The Basic Principle

Instead of:
```
"What's the output of this code?"
→ AI guesses immediately
```

Use:
```
"Trace through this code step by step, 
then tell me the output."
→ AI reasons through each line
→ More accurate answer
```

### Why It Improves Results

```
┌─────────────────────────────────────────────────────────┐
│              WHY COT WORKS                              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  WITHOUT COT:                                           │
│  Input ──────────────────────────────────────→ Output   │
│         (One big jump, easy to miss steps)              │
│                                                         │
│  WITH COT:                                              │
│  Input → Step 1 → Step 2 → Step 3 → ... → Output        │
│         (Each step validated before next)               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## �� CoT Trigger Phrases

### Simple Triggers

```
"Think step by step."

"Let's work through this systematically."

"Reason through this before answering."

"Show your work."

"Walk me through your thinking."

"Break this down step by step."
```

### Structured Triggers

```
"Before answering:
1. Identify what we're trying to accomplish
2. List the steps needed
3. Consider edge cases
4. Then provide the solution"
```

### Domain-Specific Triggers

**For Debugging:**
```
"Trace the execution step by step:
1. What are the initial values?
2. What happens in each iteration?
3. What's the state at the end?"
```

**For Architecture:**
```
"Analyze this systematically:
1. What are the requirements?
2. What patterns could work?
3. What are the tradeoffs?
4. What's the recommendation?"
```

**For Code Review:**
```
"Review this code by checking:
1. Correctness - does it do what it should?
2. Edge cases - what could break?
3. Security - any vulnerabilities?
4. Performance - any concerns?"
```

---

## 📚 Types of CoT Prompts

### Type 1: Zero-Shot CoT

Just add "Let's think step by step" to your prompt.

```
"This function has a bug. Let's think step by step 
to identify it."

[Paste code]
```

**Best for:** Quick tasks where you just want more careful reasoning.

### Type 2: Few-Shot CoT

Show AI an example of the reasoning you want.

```
"Here's how to analyze a function:

Example:
Function: add(a, b) { return a - b }
Analysis:
- Purpose: Should add two numbers
- Issue: Uses subtraction instead of addition
- Fix: Change - to +

Now analyze this function the same way:
[Paste code]"
```

**Best for:** Consistent analysis format, complex tasks.

### Type 3: Structured CoT

Give AI a specific structure to follow.

```
"Analyze this using this structure:

GIVEN:
- [What we have]

GOAL:
- [What we want]

STEPS:
1. [First step]
2. [Second step]
...

RESULT:
- [Final answer]"
```

**Best for:** Complex problems, documentation.

---

## 📚 CoT for Debugging

### The Trace Method

```
"Trace this function execution with input x = [5, 3, 8, 1]:

Show me:
1. Initial state
2. Each iteration with variable values
3. What gets returned

function findMax(arr) {
  let max = arr[0];
  for (let i = 1; i < arr.length; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }
  return max;
}"
```

**AI Response:**
```
TRACE: findMax([5, 3, 8, 1])

Initial:
  arr = [5, 3, 8, 1]
  max = arr[0] = 5

Iteration 1 (i = 1):
  arr[1] = 3
  3 > 5? No
  max remains 5

Iteration 2 (i = 2):
  arr[2] = 8
  8 > 5? Yes
  max = 8

Iteration 3 (i = 3):
  arr[3] = 1
  1 > 8? No
  max remains 8

Return: 8
```

### The Compare Method

```
"Compare expected vs actual:

EXPECTED: Function should return [specific output]
ACTUAL: Function returns [different output]

Step through and identify WHERE they diverge."
```

---

## 📚 CoT for Code Design

### The Requirements Analysis Method

```
"Design a user authentication system.

Before writing code, analyze:

1. REQUIREMENTS
   - What must it do?
   - What's optional?

2. COMPONENTS
   - What parts are needed?
   - How do they interact?

3. DATA FLOW
   - What comes in?
   - What goes out?
   - What's stored?

4. EDGE CASES
   - What could go wrong?
   - How do we handle it?

Then provide the design."
```

### The Tradeoff Analysis Method

```
"Compare two approaches:

APPROACH A: [description]
APPROACH B: [description]

Analyze each for:
1. Pros
2. Cons
3. Best use case
4. Performance implications

Then recommend one with justification."
```

---

## 📚 CoT for Complex Logic

### The State Machine Method

```
"Model this as a state machine:

1. What are all possible states?
2. What triggers each transition?
3. What happens in each transition?
4. Are there invalid states?

Then implement it."
```

### The Invariant Method

```
"Identify the invariants:

1. What must ALWAYS be true?
2. What must NEVER happen?
3. Check the code preserves these invariants.
4. Find where violations could occur."
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Add CoT to Prompts (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: CoT Patterns                          ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SIMPLE COT:                                           ┃
┃  "Let's think step by step."                           ┃
┃  "Show your reasoning."                                ┃
┃                                                        ┃
┃  STRUCTURED COT:                                       ┃
┃  "Before answering:                                    ┃
┃   1. [First analysis step]                             ┃
┃   2. [Second analysis step]                            ┃
┃   3. [Then provide answer]"                            ┃
┃                                                        ┃
┃  DOMAIN-SPECIFIC:                                      ┃
┃  Debug → "Trace execution"                             ┃
┃  Design → "Analyze requirements first"                 ┃
┃  Review → "Check each category"                        ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Match complexity to problem complexity.      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Add CoT to these prompts:**

**Original 1:** "Why isn't my loop working?"

```
With CoT: ________________________________________
_________________________________________________
_________________________________________________
```

**Original 2:** "Which database should I use?"

```
With CoT: ________________________________________
_________________________________________________
_________________________________________________
```

**Original 3:** "Debug this function: [code]"

```
With CoT: ________________________________________
_________________________________________________
_________________________________________________
_________________________________________________
```

---

## 🧪 EXERCISE 2: Create Structured Analysis (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Structured Analysis                   ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  ANALYSIS STRUCTURE:                                   ┃
┃                                                        ┃
┃  GIVEN: [What we have]                                 ┃
┃  GOAL: [What we want]                                  ┃
┃  APPROACH: [How we'll get there]                       ┃
┃  STEPS: [Detailed breakdown]                           ┃
┃  RESULT: [Final answer]                                ┃
┃                                                        ┃
┃  FOR DEBUGGING:                                        ┃
┃  INPUT: [Starting values]                              ┃
┃  TRACE: [Step-by-step execution]                       ┃
┃  ISSUE: [What goes wrong]                              ┃
┃  FIX: [Solution]                                       ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Think about what information AI needs.       ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a structured CoT prompt for this scenario:**

You have a sorting function that works for most inputs but fails on arrays with duplicate values. You need AI to debug it.

```javascript
function sort(arr) {
  for (let i = 0; i < arr.length; i++) {
    for (let j = i + 1; j < arr.length; j++) {
      if (arr[i] > arr[j]) {
        let temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
  }
  return arr;
}
```

**Your structured CoT prompt (1/3 started):**

```
Debug this sorting function that fails on duplicates.

GIVEN:
- Sorting function (provided)
- Works on: _________________________________
- Fails on: _________________________________

TRACE REQUEST:
- Trace with input: [7, 2, 7, 1]
- Show each comparison
- ___________________________________________

IDENTIFY:
- ___________________________________________
- ___________________________________________

THEN: Provide the fix
```

---

## 🧪 EXERCISE 3: Write Few-Shot CoT (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Few-Shot Pattern                      ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  FEW-SHOT STRUCTURE:                                   ┃
┃                                                        ┃
┃  "Here's how to [task]:                                ┃
┃                                                        ┃
┃   Example 1:                                           ┃
┃   Input: [example input]                               ┃
┃   Analysis:                                            ┃
┃   - Step 1...                                          ┃
┃   - Step 2...                                          ┃
┃   Result: [example result]                             ┃
┃                                                        ┃
┃   Example 2:                                           ┃
┃   [Similar pattern]                                    ┃
┃                                                        ┃
┃   Now apply this to:                                   ┃
┃   [Your actual problem]"                               ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Examples teach the format you want.          ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a few-shot prompt for code complexity analysis:**

**Your few-shot prompt (1/3 started):**

```
Here's how to analyze code complexity:

Example 1:
Code: 
  for (let i = 0; i < n; i++) { console.log(i); }
Analysis:
- Single loop through n elements
- Constant work inside loop
Complexity: O(n)

Example 2:
Code:
  for (let i = 0; i < n; i++) {
    for (let j = 0; j < n; j++) { ... }
  }
Analysis:
- _________________________________________
- _________________________________________
Complexity: _______

Now analyze this code:
[code to analyze]
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI will try to give quick, shallow answers. You must force deep reasoning through proper CoT prompting.

---

<!-- REFERENCE PANEL -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📚 REFERENCE PANEL                                    ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  PREVIOUS HELPERS: [Ex1] [Ex2] [Ex3]                   ┃
┃                                                        ┃
┃  KEY CONCEPTS:                                         ┃
┃  • Zero-shot CoT: "Think step by step"                 ┃
┃  • Few-shot CoT: Provide examples                      ┃
┃  • Structured CoT: Give analysis framework             ┃
┃  • Trace: Step through execution                       ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 Google - google.com                                ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You need to debug a complex issue: a shopping cart that calculates wrong totals when items have discounts and quantity > 1.

```javascript
function calculateTotal(cart) {
  let total = 0;
  for (const item of cart) {
    let price = item.price;
    if (item.discount) {
      price = price * item.discount; // discount is like 0.1 for 10% off
    }
    total += price;
  }
  return total;
}

// Cart: [{ name: "Shirt", price: 50, quantity: 2, discount: 0.1 }]
// Expected: $90 (50 * 2 = 100, minus 10% = 90)
// Actual: $5 (???)
```

---

### CREATE A COT DEBUG PROMPT

**Part 1: Setup Context (15 points)**

```
________________________________________________
________________________________________________
________________________________________________
```

**Part 2: Request Trace (25 points)**

```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 3: Structured Analysis Request (30 points)**

```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 4: Request Solution with Reasoning (30 points)**

```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

---

### GRADING CRITERIA

| Section | Points |
|---------|--------|
| Clear context setup | /15 |
| Proper trace request | /25 |
| Structured analysis framework | /30 |
| Solution with reasoning | /30 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ What Chain of Thought is and why it works
- ✅ CoT trigger phrases
- ✅ Zero-shot, few-shot, and structured CoT
- ✅ CoT for debugging (trace method)
- ✅ CoT for design decisions
- ✅ CoT for complex logic

### Key Insight:
**"Think step by step" is magic.** Four simple words that dramatically improve AI reasoning on complex problems.

### Next Course:
**AI Course 8: Few-Shot Learning**
