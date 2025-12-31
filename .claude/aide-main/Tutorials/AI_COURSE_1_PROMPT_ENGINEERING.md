# 🎯 AI Course 1: Prompt Engineering Basics
## *The Art of Talking to AI*

---

## 🎁 Reward: Prompt Master Badge for Dede!
Complete this course and Dede gets a magic wand accessory!

---

## 🌟 What is Prompt Engineering?

```
┌─────────────────────────────────────────────────────────────────┐
│             PROMPT ENGINEERING = SPEAKING AI                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Bad prompt:  "Write code"                                     │
│  AI thinks:   *writes random code that doesn't help*           │
│                                                                 │
│  Good prompt: "Write a Python function that calculates         │
│               the area of a circle given its radius.           │
│               Include type hints and a docstring."             │
│  AI thinks:   *writes exactly what you need*                   │
│                                                                 │
│  Your prompt quality = Your output quality                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 LESSON 1: THE 4 COMPONENTS

### The Anatomy of a Great Prompt

```
┌─────────────────────────────────────────────────────────────┐
│  1. CONTEXT     - Background information AI needs           │
│  2. TASK        - What you want AI to do                    │
│  3. FORMAT      - How you want the output structured        │
│  4. CONSTRAINTS - Rules, limitations, requirements          │
└─────────────────────────────────────────────────────────────┘
```

### Example Breakdown

**❌ Weak Prompt:**
```
Make a login form
```

**✅ Strong Prompt:**
```
CONTEXT: I'm building a SwiftUI iOS app for a fitness tracker.
My app uses @AppStorage for persistence.

TASK: Create a login form with email and password fields.

FORMAT: 
- SwiftUI View
- Include comments explaining each section
- Use proper naming conventions

CONSTRAINTS:
- Email must be validated with regex
- Password must be at least 8 characters
- Use @State for form fields
- Include a "Forgot Password" link
- No third-party libraries
```

### 🧪 Fill-in-the-Blank: Build a Prompt

```
You want AI to create a todo list feature.

CONTEXT: I'm building a ________ app using ________.
My current data model uses ________.

TASK: Create a ________ that allows users to ________.

FORMAT:
- Use ________
- Include ________
- Add ________

CONSTRAINTS:
- Must support ________
- Should not ________
- Performance requirement: ________
```

<details>
<summary>💡 Example Solution</summary>

```
CONTEXT: I'm building a productivity app using React.
My current data model uses local storage.

TASK: Create a todo list component that allows users to add, complete, and delete tasks.

FORMAT:
- Use functional components with hooks
- Include TypeScript types
- Add comments explaining the logic

CONSTRAINTS:
- Must support drag-and-drop reordering
- Should not use any external state management
- Performance requirement: Handle 1000+ items smoothly
```
</details>

---

## 📚 LESSON 2: SPECIFICITY SPECTRUM

### From Vague to Precise

```
VAGUE ←──────────────────────────────────────→ SPECIFIC
   │                                               │
   │  "Make it good"                              │
   │       ↓                                       │
   │  "Make it user-friendly"                     │
   │       ↓                                       │
   │  "Add proper error messages"                 │
   │       ↓                                       │
   │  "Show inline validation errors              │
   │   in red below each field when               │
   │   the user moves to the next field"          │
   │                                               │
```

### The 5 Questions Test

Before sending a prompt, ask:
1. **WHO** is this for? (audience, user type)
2. **WHAT** exactly do I want? (specific output)
3. **WHERE** does this fit? (context, environment)
4. **HOW** should it work? (implementation details)
5. **WHY** these requirements? (purpose, constraints)

### 🧪 Improve This Prompt

**Before:**
```
Write a function to sort stuff
```

**After (fill in):**
```
Write a ________ function called '________' that:
- Takes a ________ of ________
- Sorts by ________ (________ order)
- Returns a ________ without ________
- Handles ________ gracefully
- Include ________ and ________
```

<details>
<summary>💡 Solution</summary>

```
Write a Python function called 'sort_users' that:
- Takes a list of user dictionaries with 'name' and 'age' keys
- Sorts by age (ascending order), then by name (alphabetical)
- Returns a new sorted list without modifying the original
- Handles empty lists gracefully
- Include type hints and docstring
```
</details>

---

## 📚 LESSON 3: POWER VERBS

### The Verb Changes Everything

| **Task Type** | **Weak Verb** | **Strong Verbs** |
|--------------|---------------|------------------|
| Creation | Make | Generate, Create, Build, Construct, Design |
| Explanation | Tell | Explain, Describe, Clarify, Break down, Illustrate |
| Analysis | Look at | Analyze, Evaluate, Compare, Assess, Critique |
| Modification | Change | Refactor, Optimize, Simplify, Enhance, Transform |
| Debugging | Fix | Debug, Troubleshoot, Identify, Resolve, Diagnose |
| Review | Check | Review, Audit, Validate, Verify, Inspect |
| Learning | Show | Demonstrate, Walk through, Teach, Illustrate |

### 🧪 Choose the Right Verb

```
# Scenario 1: You have messy code that works but is hard to read
# Weak: "_____ my code to be better"
# Strong: "_____ my code to improve readability and reduce duplication"
# Answer: ________

# Scenario 2: You don't understand why your code isn't working
# Weak: "_____ what's wrong"
# Strong: "_____ this error and _____ the root cause step by step"
# Answer: ________, ________

# Scenario 3: You want to understand a concept
# Weak: "_____ how promises work"
# Strong: "_____ JavaScript promises with _____ examples"
# Answer: ________, ________
```

<details>
<summary>💡 Answers</summary>

```
1. Refactor
2. Debug, identify
3. Explain, practical / Demonstrate, code
```
</details>

---

## 📚 LESSON 4: CONTEXT IS KING

### The Context Pyramid

```
                    ▲
                   /│\
                  / │ \
                 /  │  \    IMMEDIATE TASK
                /   │   \   "Add dark mode toggle"
               /────┼────\
              /     │     \
             /      │      \   PROJECT CONTEXT
            /       │       \  "SwiftUI iOS app, 
           /────────┼────────\  using @AppStorage"
          /         │         \
         /          │          \   ENVIRONMENT
        /           │           \  "Xcode 15, iOS 17,
       /────────────┼────────────\  targeting iPhone 14+"
      ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔
```

### Context Checklist

- [ ] Language/Framework?
- [ ] Existing code patterns?
- [ ] Target platform?
- [ ] Dependencies available?
- [ ] Performance requirements?
- [ ] Style/coding conventions?
- [ ] What's already built?
- [ ] What should NOT be changed?

### 🧪 Build Context

**Scenario:** You want AI to add a search feature

**Fill in the context:**
```
ENVIRONMENT:
- Language: ________
- Framework: ________
- Version: ________

PROJECT CONTEXT:
- App type: ________
- Existing features: ________
- Data source: ________

IMMEDIATE TASK:
- Feature: ________
- Must integrate with: ________
- User interaction: ________
```

---

## 📚 LESSON 5: OUTPUT FORMATTING

### Tell AI How You Want Results

```
"Format the output as..."

• a numbered list
• a markdown table
• a code block with comments
• bullet points
• a step-by-step guide
• JSON format
• a comparison table
• pseudocode first, then implementation
```

### Format Examples

```
❌ "Compare databases"

✅ "Compare PostgreSQL, MySQL, and SQLite.
   Format as a markdown table with columns:
   - Feature
   - PostgreSQL
   - MySQL
   - SQLite
   
   Include rows for: Performance, Learning Curve, 
   Best Use Case, Scalability, and Cost"
```

### 🧪 Add Format to This Prompt

**Weak prompt:**
```
Explain REST API methods
```

**Add formatting:**
```
Explain REST API methods.

Format as:
- ________
- ________
- ________

Include for each:
- ________
- ________
- ________
```

<details>
<summary>💡 Solution</summary>

```
Explain REST API methods.

Format as:
- A markdown table
- With code examples
- Followed by a practical scenario for each

Include for each:
- Method name and HTTP verb
- When to use it
- Request/Response example
```
</details>

---

## 📚 LESSON 6: ITERATIVE REFINEMENT

### The Conversation Loop

```
┌─────────────────┐
│  Initial Prompt │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     Not quite right?
│   AI Response   │────────────────────┐
└────────┬────────┘                    │
         │                             │
         ▼                             │
┌─────────────────┐                    │
│   Evaluate it   │                    │
└────────┬────────┘                    │
         │                             │
    ┌────┴────┐                        │
    │         │                        │
 Perfect?  Needs work?                 │
    │         │                        │
    ▼         └────────────────────────┘
  Done!              │
                     ▼
              ┌─────────────────┐
              │  Refine Prompt  │
              │  "Actually..."  │
              │  "But also..."  │
              │  "Change X to Y"│
              └─────────────────┘
```

### Magic Refinement Phrases

| **Situation** | **Say This** |
|--------------|--------------|
| Too complex | "Simplify this. I'm a beginner." |
| Too simple | "Add more advanced features like..." |
| Wrong style | "Rewrite in a more [casual/formal/technical] tone" |
| Missing feature | "Add [X] to this solution" |
| Wrong language | "Convert this to [Swift/Python/etc]" |
| Need explanation | "Explain what this code does line by line" |
| Too long | "Make this more concise" |
| Not working | "This gives error [X]. Debug it." |

---

## 🏆 COURSE CHALLENGE: PROMPT MAKEOVER

Transform these weak prompts into powerful ones:

### Challenge 1
**Before:** "Make a todo app"

**Your prompt:**
```
CONTEXT:
_________________________________________

TASK:
_________________________________________

FORMAT:
_________________________________________

CONSTRAINTS:
_________________________________________
```

### Challenge 2
**Before:** "Fix my code it's broken"

**Your prompt:**
```
_________________________________________
_________________________________________
_________________________________________
_________________________________________
```

### Challenge 3
**Before:** "Explain APIs"

**Your prompt:**
```
_________________________________________
_________________________________________
_________________________________________
_________________________________________
```

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Use the 4 components: Context, Task, Format, Constraints│
│  ✅ Be specific - vague prompts = vague results            │
│  ✅ Choose strong verbs (refactor vs fix)                  │
│  ✅ Provide enough context for AI to understand            │
│  ✅ Specify output format explicitly                       │
│  ✅ Iterate and refine based on responses                  │
│  ✅ Quality of input = Quality of output                   │
└────────────────────────────────────────────────────────────┘
```

---

## 🔓 NEXT COURSE

**Course 2: Understanding AI Capabilities** →
Learn what AI can and can't do so you know when to use it!

---

**[  ] Mark Complete** when you've transformed all 3 challenge prompts!
