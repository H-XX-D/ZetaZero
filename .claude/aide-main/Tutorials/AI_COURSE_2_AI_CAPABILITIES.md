# 🧠 AI Course 2: Understanding AI Capabilities
## *Know What AI Can and Can't Do*

---

## 🎮 COURSE OVERVIEW

**Why This Course Matters**

The biggest mistakes people make with AI aren't about prompts—they're about **misunderstanding what AI can actually do**.

Some think AI is magic. They get frustrated when it fails.
Some think AI is useless. They miss massive productivity gains.

The truth: AI is **incredibly powerful within its capabilities** and **completely useless outside them**. This course teaches you to know the difference.

This knowledge will separate you from 90% of people trying to use AI. While they waste hours fighting against AI limitations, you'll know exactly when to use AI and when to use traditional methods.

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code, you complete the rest
- 📌 Code Helper - logic examples (3 guided exercises)
- 🧠 Final Test - AI is uncooperative, use your skills

---

# PART 1: WHAT AI DOES WELL

## 📚 AI's Superpowers

### 1. Pattern Recognition & Transformation

AI excels at recognizing patterns and transforming content between formats.

**AI is EXCELLENT at:**
```
✅ Convert JSON → TypeScript interfaces
✅ Transform SQL dialects (MySQL → PostgreSQL)
✅ Reformat code styles (tabs → spaces, naming conventions)
✅ Convert markdown → HTML → other formats
✅ Translate between programming languages
✅ Generate boilerplate from examples
```

**Example - JSON to TypeScript:**

You give:
```json
{
  "id": 123,
  "name": "John Doe",
  "orders": [{"id": 1, "total": 99.99}]
}
```

AI returns:
```typescript
interface Order {
  id: number;
  total: number;
}

interface User {
  id: number;
  name: string;
  orders: Order[];
}
```

**Why it works:** Clear pattern, well-defined transformation rules, millions of examples in training data.

---

### 2. Boilerplate & Scaffolding

AI generates repetitive, structured code extremely well.

**AI is EXCELLENT at:**
```
✅ CRUD operations (Create, Read, Update, Delete)
✅ REST API endpoints
✅ Form components with validation
✅ Unit test scaffolding
✅ Database migrations
✅ Configuration files
✅ Documentation templates
```

**Why it works:** These patterns are repeated millions of times across codebases. AI has seen every variation.

---

### 3. Explanation & Documentation

AI explains code and generates docs better than most developers.

**AI is EXCELLENT at:**
```
✅ Adding comments to complex code
✅ Generating JSDoc/docstrings/type hints
✅ Explaining what unfamiliar code does
✅ Creating README files
✅ Writing API documentation
✅ Translating technical → plain English
✅ Breaking down complex algorithms step-by-step
```

**Example:**
```
"Explain this regex: ^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d]{8,}$"

AI: "This validates passwords requiring:
     - At least 8 characters
     - At least one letter
     - At least one number
     - Only letters and numbers allowed"
```

---

### 4. Code Review & Bug Finding

AI spots common issues humans miss.

**AI is EXCELLENT at:**
```
✅ Finding common bugs (off-by-one, null checks)
✅ Spotting security vulnerabilities (SQL injection, XSS)
✅ Identifying performance anti-patterns
✅ Flagging code smell
✅ Suggesting improvements
✅ Checking for edge cases
```

**Example:**
```javascript
// You show AI this:
function getUser(id) {
  return db.query("SELECT * FROM users WHERE id = " + id);
}

// AI flags:
🚨 SQL INJECTION VULNERABILITY
Use parameterized queries:
db.query("SELECT * FROM users WHERE id = ?", [id]);
```

---

### 5. Learning & Research

AI is an exceptional learning companion.

**AI is EXCELLENT at:**
```
✅ Explaining concepts at YOUR level
✅ Providing multiple examples
✅ Comparing technologies/approaches
✅ Summarizing documentation
✅ Answering "how do I..." questions
✅ Creating personalized learning paths
✅ Debugging your understanding
```

---

# PART 2: WHAT AI DOES POORLY

## 📚 AI's Limitations

Understanding these saves HOURS of frustration.

### 1. 🚫 Real-Time / Current Information

AI's knowledge has a cutoff date. It doesn't know:

```
❌ Today's news or events
❌ Recent library updates/releases
❌ Current API documentation changes
❌ Whether a website is currently up
❌ Stock prices, weather, live data
❌ Your company's internal systems
```

**What happens:**
```
You: "What's the latest version of React?"
AI: "React 18.2" (might be months outdated)
Reality: React 19 just released last week
```

**Workaround:** Always verify version numbers in official docs. Tell AI what version YOU'RE using.

---

### 2. 🚫 Guaranteed Correctness

AI generates **plausible-sounding** text. Not necessarily **correct** text.

```
❌ Can invent functions that don't exist
❌ Can use deprecated/removed syntax
❌ Can mix up similar concepts
❌ Can hallucinate API endpoints
❌ Can create subtly broken logic
❌ Can be CONFIDENTLY wrong
```

**Hallucination Example:**
```
You: "How do I use Swift's NetworkManager class?"
AI: "Import NetworkManager and call .shared.fetch()..."

Reality: NetworkManager doesn't exist in Swift. 
         AI invented it because the name sounds real.
```

**Workaround:** ALWAYS test AI code. Verify function names in docs. Be skeptical of specific claims.

---

### 3. 🚫 Your Specific Codebase

AI doesn't know YOUR code unless you show it.

```
❌ Can't access your files
❌ Doesn't know your folder structure
❌ Doesn't understand your custom abstractions
❌ Doesn't know your naming conventions
❌ Can't see your environment variables
❌ Doesn't know your API endpoints
```

**What happens:**
```
You: "Add a route to my app"
AI: Creates generic Express route
Reality: You're using Next.js App Router

You: "Use our Button component"
AI: Creates new Button from scratch
Reality: You have custom Button at @/components/ui/Button
```

**Workaround:** Paste relevant code. Describe your structure. Be explicit about what exists.

---

### 4. 🚫 Complex Multi-Step Reasoning

AI struggles with problems requiring many logical steps.

```
❌ Multi-file debugging across call stacks
❌ Complex mathematical proofs
❌ Intricate business logic with many conditions
❌ Optimizing systems with many interacting parts
❌ Predicting emergent behavior
```

**Workaround:** Break complex problems into steps. Verify each step before proceeding.

---

### 5. 🚫 Truly Novel Solutions

AI recombines patterns it's seen. It doesn't truly innovate.

```
❌ Inventing new algorithms
❌ Solving unprecedented problems
❌ Creative breakthroughs
❌ Cutting-edge research applications
```

**Workaround:** Use AI for known patterns. For novel problems, use AI as a brainstorming partner, not the answer.

---

## 📚 The Capability Matrix

Use this to decide if AI will help:

| Task | AI Effectiveness |
|------|------------------|
| Boilerplate code | ⭐⭐⭐⭐⭐ Excellent |
| Code transformation | ⭐⭐⭐⭐⭐ Excellent |
| Explaining code | ⭐⭐⭐⭐⭐ Excellent |
| Finding common bugs | ⭐⭐⭐⭐ Very Good |
| Writing tests | ⭐⭐⭐⭐ Very Good |
| Documentation | ⭐⭐⭐⭐ Very Good |
| Refactoring | ⭐⭐⭐⭐ Very Good |
| Learning concepts | ⭐⭐⭐⭐ Very Good |
| API design | ⭐⭐⭐ Good |
| Architecture decisions | ⭐⭐⭐ Good |
| Complex debugging | ⭐⭐ Fair |
| Performance optimization | ⭐⭐ Fair |
| Novel algorithms | ⭐⭐ Fair |
| Real-time data | ⭐ Poor |
| Your specific codebase | ⭐ Poor |

---

## 📚 Decision Framework

Before asking AI, ask yourself:

```
1. Is this a common pattern?
   YES → AI will excel
   NO  → Be careful, verify output

2. Does this need current information?
   YES → Verify in official sources
   NO  → AI can help

3. Is this about MY specific codebase?
   YES → Provide relevant code as context
   NO  → AI can help directly

4. Does this require many reasoning steps?
   YES → Break it down, verify each step
   NO  → AI can handle it

5. Am I okay verifying the output?
   YES → Use AI freely
   NO  → Don't use AI for this
```

---

# PART 3: GUIDED EXERCISES

## 🧪 EXERCISE 1: Match Task to Capability (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Capability Quick Reference            ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  EXCELLENT FOR:                                        ┃
┃  • Pattern transformation (JSON→TS, SQL→SQL)          ┃
┃  • Boilerplate (CRUD, forms, tests)                   ┃
┃  • Explanation & docs                                  ┃
┃  • Finding common bugs                                 ┃
┃                                                        ┃
┃  POOR FOR:                                             ┃
┃  • Current/real-time information                       ┃
┃  • Your specific codebase (unless you show it)         ┃
┃  • Complex multi-step reasoning                        ┃
┃  • Guaranteed correctness                              ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Think about what data AI would need          ┃
┃     and whether it could have seen this pattern.       ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Rate each task (Excellent / Good / Poor) and explain why:**

1. "Convert this Python dictionary to a TypeScript interface"
   ```
   Rating: ___________
   Why: ____________________________________________
   ```

2. "What's the current AWS us-east-1 status?"
   ```
   Rating: ___________
   Why: ____________________________________________
   ```

3. "Generate unit tests for this sorting function"
   ```
   Rating: ___________
   Why: ____________________________________________
   ```

4. "Debug why my production server crashed at 3am"
   ```
   Rating: ___________
   Why: ____________________________________________
   ```

5. "Explain how JavaScript closures work"
   ```
   Rating: ___________
   Why: ____________________________________________
   ```

---

## 🧪 EXERCISE 2: Spot the Hallucination (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Hallucination Warning Signs           ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  RED FLAGS:                                            ┃
┃  ⚠️ Very specific claims you can't verify              ┃
┃  ⚠️ Confident tone about unusual information           ┃
┃  ⚠️ Functions/APIs you can't find in official docs     ┃
┃  ⚠️ URLs that lead nowhere                             ┃
┃  ⚠️ Statistics without sources                         ┃
┃  ⚠️ Version numbers that seem too recent               ┃
┃                                                        ┃
┃  VERIFICATION STEPS:                                   ┃
┃  1. Search official documentation                      ┃
┃  2. Check if the API/function exists                   ┃
┃  3. Verify version numbers on package sites            ┃
┃  4. Test code in isolated environment                  ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: The more specific the claim, the more        ┃
┃     likely it needs verification.                      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**AI gave this response. Identify potential hallucinations:**

```swift
To implement push notifications in SwiftUI, use the 
built-in PushKit.Manager API:

import SwiftUI
import PushKitManager

struct ContentView: View {
    @StateObject var pushService = PushKitManager.shared
    
    func setupNotifications() {
        pushService.requestPermission()
        pushService.registerDevice(appId: "com.yourapp")
    }
}

This API was introduced in iOS 16.4 and has a 97% 
delivery success rate according to Apple's 2024 
WWDC session on notification best practices.
```

**Mark what might be hallucinated:**
```
[ ] The PushKitManager import
[ ] The .shared singleton pattern
[ ] The specific iOS version (16.4)
[ ] The 97% statistic
[ ] The WWDC session reference
```

**How would you verify each claim?**
```
1. PushKitManager: _________________________________
2. iOS version: ____________________________________
3. Statistics: ____________________________________
```

---

## 🧪 EXERCISE 3: Provide Context for Your Codebase (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Context Compensation                  ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  AI doesn't know your code. Compensate by providing:   ┃
┃                                                        ┃
┃  1. TECH STACK                                         ┃
┃     "Using React 18, TypeScript, Zustand for state"    ┃
┃                                                        ┃
┃  2. EXISTING COMPONENTS                                ┃
┃     "I have Button at @/components/ui/Button"          ┃
┃     "UserContext provides { user, logout }"            ┃
┃                                                        ┃
┃  3. DATA SHAPES                                        ┃
┃     "Product: { id: string, name: string, price }"     ┃
┃                                                        ┃
┃  4. API STRUCTURE                                      ┃
┃     "API returns { data: T[], meta: { total, page }}"  ┃
┃                                                        ┃
┃  5. CONVENTIONS                                        ┃
┃     "We use camelCase, functional components only"     ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: What would a new developer need to know?     ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Scenario:** You want AI to add a shopping cart feature to your e-commerce app.

**Fill in the context you'd provide:**

```
TECH STACK:
- Framework: _______________________
- State management: ________________
- Styling: _________________________

EXISTING COMPONENTS I'LL USE:
1. ________________________________
2. ________________________________
3. ________________________________

DATA SHAPES:
Product: {
  _______________________________
  _______________________________
  _______________________________
}

Cart Item: {
  _______________________________
  _______________________________
}

API ENDPOINTS:
- GET ____________: Returns __________
- POST ___________: Expects __________

CONVENTIONS:
- _________________________________
- _________________________________
```

---

# PART 4: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI might:
- Give vague, unhelpful answers
- "Forget" what you just told it
- Provide incorrect information
- Refuse to help properly

This simulates real-world AI frustration. Use your skills.

---

<!-- REFERENCE PANEL - All previous helpers available -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📚 REFERENCE PANEL                                    ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  PREVIOUS HELPERS: [Ex1] [Ex2] [Ex3]                   ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 Google Search - google.com                         ┃
┃  🔗 MDN Web Docs - developer.mozilla.org               ┃
┃  🔗 Official Documentation (framework-specific)        ┃
┃                                                        ┃
┃  SAMPLE PROMPTS TO TRY (AI may not cooperate):         ┃
┃  • "List AI strengths and weaknesses"                  ┃
┃  • "When should I NOT use AI?"                         ┃
┃  • "How do I verify AI output?"                        ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

Your team lead asks: "We're starting a new project. Create a document explaining when we should use AI assistance and when we shouldn't."

---

### COMPLETE THE AI USAGE GUIDE

```
# AI Usage Guidelines for Our Team

## When to Use AI (High Confidence)

1. _________________________________________________
   Why: ____________________________________________

2. _________________________________________________
   Why: ____________________________________________

3. _________________________________________________
   Why: ____________________________________________

## When to Use AI with Caution

1. _________________________________________________
   Verification needed: _____________________________

2. _________________________________________________
   Verification needed: _____________________________

## When NOT to Use AI

1. _________________________________________________
   Why: ____________________________________________

2. _________________________________________________
   Why: ____________________________________________

## Verification Checklist

Before accepting AI output, always:
[ ] _________________________________________________
[ ] _________________________________________________
[ ] _________________________________________________
[ ] _________________________________________________

## Red Flags (Signs AI Output Might Be Wrong)

1. _________________________________________________
2. _________________________________________________
3. _________________________________________________
```

---

### GRADING CRITERIA

| Requirement | Points |
|-------------|--------|
| 3+ appropriate "high confidence" uses | /20 |
| 2+ "use with caution" scenarios | /15 |
| 2+ appropriate "don't use" cases | /15 |
| 4+ verification checklist items | /20 |
| 3+ red flags identified | /15 |
| Overall clarity and accuracy | /15 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

**You now understand AI capabilities and limitations.**

### What You Learned:
- ✅ AI's 5 superpowers (patterns, boilerplate, explanation, bugs, learning)
- ✅ AI's 5 limitations (current info, correctness, your code, complex reasoning, novelty)
- ✅ The capability matrix for decision-making
- ✅ How to verify AI output
- ✅ How to compensate for limitations with context

### Key Insight:
**AI is a powerful tool, not a replacement for understanding.**
Those who know when to use it AND when not to will outperform everyone else.

### Next Course:
**AI Course 3: Working with Context Windows**
