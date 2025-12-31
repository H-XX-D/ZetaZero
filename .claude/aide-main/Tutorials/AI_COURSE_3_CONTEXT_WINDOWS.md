# 📚 AI Course 3: Working with Context Windows
## *Master the Art of AI Memory*

---

## 🎮 COURSE OVERVIEW

**What is a Context Window?**

A context window is the AI's "working memory" - the amount of text it can consider at once. Think of it like a whiteboard: there's only so much space, and when it fills up, old stuff gets erased.

Understanding context windows is the difference between:
- ❌ Frustrating conversations where AI "forgets" everything
- ✅ Smooth sessions where AI maintains perfect understanding

**Why This Matters:**

Most people hit context limits without knowing it. The AI starts:
- Forgetting your framework/language
- Losing track of your requirements
- Contradicting earlier responses
- Giving increasingly generic answers

This course teaches you to work WITHIN context limits and recognize when you've hit them.

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - context management strategies
- 🧠 Final Test - AI uncooperative, use your skills

---

# PART 1: UNDERSTANDING CONTEXT

## 📚 How Context Windows Work

### The Whiteboard Analogy

```
┌─────────────────────────────────────────────────────────┐
│                    CONTEXT WINDOW                       │
│                  (The AI's Whiteboard)                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  [System Prompt] [Your Message 1] [AI Response 1]       │
│  [Your Message 2] [AI Response 2] [Your Message 3]      │
│  [AI Response 3] [Your Message 4] [AI Response 4]       │
│  [Your Message 5] [AI Response 5] [Current Message]     │
│                                                         │
│                    ← OLDER        NEWER →               │
│                                                         │
│  When full: Oldest content gets pushed out              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### What Gets Included

Everything counts toward context:
- Your messages
- AI responses
- Code you paste
- System prompts (instructions to AI)
- File contents you share

### Context Window Sizes (Approximate)

| Model | Context Size | Rough Equivalent |
|-------|-------------|------------------|
| GPT-3.5 | ~4K tokens | ~3,000 words |
| GPT-4 | ~8K-128K | ~6,000-100,000 words |
| Claude | ~100K-200K | ~75,000-150,000 words |
| Local models | ~2K-8K | ~1,500-6,000 words |

**Tokens ≠ Words:** 1 token ≈ 0.75 words or ~4 characters

---

## 📚 Signs You've Hit the Limit

### Red Flags

```
🚩 AI suddenly "forgets" your framework
   You: (20 messages about React)
   AI: "Here's the jQuery solution..."

🚩 AI contradicts earlier statements
   AI (before): "Use async/await"
   AI (after): "Use callbacks"

🚩 AI asks questions you already answered
   You: (already said you're building iOS app)
   AI: "What platform are you targeting?"

🚩 Responses become generic
   Before: Specific to YOUR project
   After: Generic, tutorial-like answers

🚩 AI loses track of requirements
   You: "Remember, max 100 characters"
   AI: Returns code with no character limit
```

---

## 📚 Context Management Strategies

### Strategy 1: Front-Load Critical Context

Put the most important info at the START of your message.

```
✅ GOOD:
"React TypeScript project with Redux. Need a..."

❌ BAD:
"So I've been working on this thing and it's 
using this framework I like and basically..."
(important info buried at the end)
```

### Strategy 2: Periodic Context Refresh

Every 5-10 messages, re-state key context:

```
"Quick context refresh:
- React Native app
- TypeScript
- We're building the cart feature
- Using Zustand for state

Now, let's continue with..."
```

### Strategy 3: Minimize Response Length

Ask AI to be concise when you don't need verbose answers:

```
"Give me ONLY the function code, no explanation."
"One-line answer please."
"List format, no paragraphs."
```

### Strategy 4: Start Fresh for New Features

Don't try to do everything in one conversation:

```
Feature 1 → New conversation
Feature 2 → New conversation
Feature 3 → New conversation
```

### Strategy 5: Summarize Before Continuing

```
"Before we continue, let me summarize where we are:
1. Created User model ✓
2. Built login screen ✓
3. Next: Password reset

Confirm this is correct, then let's build password reset."
```

---

## 📚 The Context Budget

Think of context as a budget you spend:

| Action | Context Cost |
|--------|-------------|
| Short message | $ (cheap) |
| Long message | $$$ |
| Pasting code | $$$$ |
| Pasting entire files | $$$$$ |
| Long AI response | $$$$ |

### Budgeting Tips

```
✅ DO:
- Paste only RELEVANT code snippets
- Ask for concise responses
- Start new chats for new features
- Summarize periodically

❌ DON'T:
- Paste entire files when you only need a function
- Ask for "detailed explanations" every time
- Run 50+ message conversations
- Include unrelated context
```

---

## 📚 Context Recovery Techniques

### When AI Loses Context

**The Full Recovery Prompt:**
```
Let me re-establish our project context:

PROJECT: [Name] - [One-line description]
TECH: [Framework], [Language], [Key libraries]
ARCHITECTURE: [Pattern - MVC, MVVM, etc.]
CURRENT FILE: [Filename you're working on]
WHAT EXISTS: [Key components already built]
WHAT WE'RE BUILDING: [Current feature]

[Paste the specific code section you're working on]

We were working on [specific task]. Please continue.
```

**The Quick Recovery Prompt:**
```
Context reminder: React/TypeScript app, building shopping cart.
Continue with [specific task].
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Identify Context Issues (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Context Loss Signs                    ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SIGNS OF CONTEXT LOSS:                                ┃
┃  • Wrong framework/language suddenly                   ┃
┃  • Contradicting previous responses                    ┃
┃  • Re-asking answered questions                        ┃
┃  • Generic instead of specific answers                 ┃
┃  • Forgetting constraints/requirements                 ┃
┃                                                        ┃
┃  WHAT TO LOOK FOR:                                     ┃
┃  Compare EARLY responses vs LATE responses             ┃
┃  Check if specificity decreased                        ┃
┃  See if requirements got dropped                       ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  �� HINT: The conversation shows a progression.        ┃
┃     At what point does quality degrade?                ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Analyze this conversation snippet:**

```
Message 1: "Building a SwiftUI app with MVVM. Need a login screen."
AI 1: "Here's a SwiftUI LoginView with ViewModel..." (correct)

Message 5: "Add password validation"
AI 5: "I'll add validation to the ViewModel..." (correct)

Message 15: "Add forgot password"
AI 15: "Here's a forgot password screen..." (still specific)

Message 25: "Add biometric login"
AI 25: "To add biometric login in iOS, you can use..."
       (generic iOS, not SwiftUI-specific)

Message 30: "Make the button bigger"
AI 30: "You can increase the button size using CSS..."
       (CSS?! This is SwiftUI!)
```

**Questions:**

1. At what message did context loss become noticeable?
   ```
   Message: ___________
   ```

2. What specific signs indicated context loss?
   ```
   Sign 1: ______________________________________
   Sign 2: ______________________________________
   ```

3. Write a context recovery prompt to fix this:
   ```
   _______________________________________________
   _______________________________________________
   _______________________________________________
   ```

---

## 🧪 EXERCISE 2: Optimize Context Usage (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Context Budgeting                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  COST COMPARISON:                                      ┃
┃                                                        ┃
┃  EXPENSIVE:                                            ┃
┃  "Here's my entire 500-line file, find the bug"        ┃
┃                                                        ┃
┃  CHEAP:                                                ┃
┃  "Bug in this function (lines 45-60):                  ┃
┃   [paste just those 15 lines]                          ┃
┃   Error: 'undefined is not a function'"                ┃
┃                                                        ┃
┃  PRINCIPLES:                                           ┃
┃  • Only paste what's relevant                          ┃
┃  • Ask for concise responses                           ┃
┃  • Front-load important context                        ┃
┃  • Summarize, don't repeat everything                  ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Imagine you're paying per word.              ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Rewrite these prompts to use less context:**

**Original 1 (wasteful):**
```
"So I've been working on this project for a while now and
it's a React application that we're building for our client
who wants an e-commerce store and we're using TypeScript
because that's what the team decided and anyway I need you
to help me create a product card component that shows the
product image and the title and the price."
```

**Your optimized version:**
```
___________________________________________________
___________________________________________________
___________________________________________________
```

**Original 2 (wasteful):**
```
"Can you explain in great detail with lots of examples
and thorough explanations exactly how to add a click
handler to a button?"
```

**Your optimized version:**
```
___________________________________________________
```

**Original 3 (wasteful):**
```
[Pastes 300-line file]
"There's a bug somewhere in here"
```

**Your optimized version:**
```
___________________________________________________
___________________________________________________
___________________________________________________
```

---

## 🧪 EXERCISE 3: Write Context Recovery (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Recovery Template                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  FULL RECOVERY STRUCTURE:                              ┃
┃                                                        ┃
┃  "Let me re-establish context:                         ┃
┃                                                        ┃
┃   PROJECT: [Name] - [Description]                      ┃
┃   TECH: [Framework, Language, Libraries]               ┃
┃   ARCHITECTURE: [Pattern]                              ┃
┃   CURRENT WORK: [What we're building]                  ┃
┃   EXISTING: [What's already done]                      ┃
┃                                                        ┃
┃   [Relevant code snippet if needed]                    ┃
┃                                                        ┃
┃   Please continue with [specific next step]."          ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Be specific but concise. Include only        ┃
┃     what AI needs to continue the work.                ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Scenario:** You've been building a fitness tracking iOS app. After 30 messages, the AI started giving generic web development advice instead of SwiftUI code.

**Your project details:**
- SwiftUI iOS app
- HealthKit integration
- MVVM architecture
- Already built: Dashboard, WorkoutList
- Currently building: WorkoutDetail screen
- Need: Heart rate chart using Swift Charts

**Write the recovery prompt (1/3 done):**

```
Let me re-establish our project context:

PROJECT: FitTrack - iOS fitness tracking app
TECH: SwiftUI, ___________________
ARCHITECTURE: ___________________
EXISTING SCREENS: ___________________
CURRENT WORK: ___________________

I need you to ___________________
___________________
___________________
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI may lose context rapidly, give generic answers, or forget your requirements. This simulates real-world frustration with long AI conversations.

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
┃  • Context window = AI's working memory                ┃
┃  • Everything counts: your msgs + AI responses + code  ┃
┃  • Signs: wrong framework, contradictions, generic     ┃
┃  • Fix: context refresh, recovery prompts, new chat    ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 Google - google.com                                ┃
┃                                                        ┃
┃  SAMPLE PROMPTS TO TRY:                                ┃
┃  • "What is a context window?"                         ┃
┃  • "How do I recover lost context?"                    ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You're 40 messages into a conversation about building a Node.js/Express API. The AI just gave you Python Flask code instead of JavaScript.

---

### COMPLETE THE CONTEXT MANAGEMENT PLAN

**Part 1: Diagnosis (20 points)**

What likely happened?
```
________________________________________________
________________________________________________
```

What signs would have warned you earlier?
```
1. ____________________________________________
2. ____________________________________________
3. ____________________________________________
```

**Part 2: Recovery (30 points)**

Write a context recovery prompt:
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 3: Prevention (30 points)**

Write a context management strategy for future long conversations:
```
Every ___ messages, I will: ____________________
________________________________________________

Before pasting code, I will: ___________________
________________________________________________

When I notice __________, I will ______________
________________________________________________

I'll start a new conversation when: ___________
________________________________________________
```

**Part 4: Optimization (20 points)**

Rewrite this wasteful message to save context:
```
Original: "So remember how we were building that Express
API earlier and we talked about authentication and you
gave me that JWT middleware code? Well now I need to add
refresh tokens to it. Can you explain in detail how refresh
tokens work and then give me the complete updated code with
lots of comments explaining every part?"
```

Your optimized version:
```
________________________________________________
________________________________________________
________________________________________________
```

---

### GRADING CRITERIA

| Section | Points |
|---------|--------|
| Correct diagnosis of context loss | /20 |
| Effective recovery prompt | /30 |
| Practical prevention strategy | /30 |
| Optimized message | /20 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ What context windows are and how they work
- ✅ Signs that you've hit context limits
- ✅ Strategies to manage context efficiently
- ✅ How to recover when AI loses context
- ✅ How to budget your context usage

### Key Insight:
**Long conversations are the enemy.** The best AI users keep conversations focused, refresh context regularly, and start fresh for new features.

### Next Course:
**AI Course 4: Iterative Refinement**
