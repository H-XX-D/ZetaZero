# 🎯 AI Course 1: Prompt Engineering Basics
## *The Art of Talking to AI*

---

## 🎮 COURSE OVERVIEW

**What is Prompt Engineering?**

Prompt engineering is the skill of crafting instructions that get AI to do exactly what you want. It's the difference between getting useless garbage and getting production-ready code in seconds.

**Why This Matters:**

```
Bad prompt:  "Write code"
AI output:   Random, useless code

Good prompt: "Write a Python function that validates email 
              addresses using regex. Include docstring, 
              type hints, and handle empty strings."
AI output:   Exactly what you needed
```

**Learning Style:**
- ⌨️ **Type it yourself** - 1/3 starter code, you complete the rest
- 📌 **Code Helper** - logic examples pop out (3 guided exercises)
- 🧠 **Final Test** - prove you can do it without AI

---

# PART 1: CONCEPTS

## 📚 The 4 Prompt Components

Every effective prompt has four parts:

| Component | Purpose | Example |
|-----------|---------|---------|
| **CONTEXT** | Background info | "I'm building a React dashboard" |
| **TASK** | What you want | "Create a bar chart component" |
| **FORMAT** | Output structure | "Use TypeScript, add JSDoc" |
| **CONSTRAINTS** | Rules/limits | "Must use D3.js, no jQuery" |

---

## 📚 The Specificity Rule

If AI could interpret your request two different ways, be more specific.

```
❌ "Make it good"
❌ "Add error handling"
✅ "Show red error text below each field when validation fails"
```

---

## 📚 The Iteration Loop

First output is rarely perfect. Refine with specific feedback:

```
❌ "Fix it" / "That's wrong"
✅ "The button is 32px. Make it 44px for touch targets."
✅ "Add try/catch and show user-friendly error message."
```

---

## 📚 Common AI Mistakes

| Mistake | Prevention |
|---------|------------|
| 🧠 Loses context | Re-establish project details |
| ✂️ Erases your code | Say "Keep existing code, only ADD..." |
| 🎭 Wrong assumptions | Be explicit about what exists |
| 📝 Ignores constraints | List requirements as checkboxes |

---

# PART 2: GUIDED EXERCISES

Complete 3 guided exercises with Code Helper windows.

---

## 🧪 EXERCISE 1: Complete the Prompt (GUIDED)

<!-- CODE HELPER WINDOW - OPENS AS FLOATING PANEL -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Prompt Structure                      ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  // Example (different scenario - a chat app):         ┃
┃                                                        ┃
┃  CONTEXT:                                              ┃
┃  I'm building a chat app using Vue.js.                 ┃
┃  The app already has user authentication.              ┃
┃                                                        ┃
┃  TASK:                                                 ┃
┃  Create a message input component that:                ┃
┃  1. Has a text field and send button                   ┃
┃  2. Shows character count                              ┃
┃  3. Disables send when empty                           ┃
┃                                                        ┃
┃  FORMAT:                                               ┃
┃  - Use Vue 3 Composition API                           ┃
┃  - Include TypeScript types                            ┃
┃                                                        ┃
┃  CONSTRAINTS:                                          ┃
┃  - Max 500 characters                                  ┃
┃  - Must handle emoji input                             ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Fill in details specific to YOUR scenario.   ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**YOUR SCENARIO:** Build a temperature display for a weather app.

**Starter code (1/3 done - complete the rest):**

```
CONTEXT: 
I'm building a weather app using React Native.
The app already has _______________________.

TASK:
Create a temperature display component that:
1. Shows current temperature in large text
2. _______________
3. _______________

FORMAT:
- Use functional components with hooks
- _______________

CONSTRAINTS:
- Support both Celsius and Fahrenheit
- _______________
```

---

## 🧪 EXERCISE 2: Fix the Broken Prompt (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Being Specific                        ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  // VAGUE vs SPECIFIC (different examples):            ┃
┃                                                        ┃
┃  ❌ "Add a header"                                     ┃
┃  ✅ "Add a sticky header with logo left,               ┃
┃      nav links center, user avatar right"              ┃
┃                                                        ┃
┃  ❌ "Make it look nice"                                ┃
┃  ✅ "Use 16px padding, 8px border-radius,              ┃
┃      shadow: 0 2px 4px rgba(0,0,0,0.1)"                ┃
┃                                                        ┃
┃  ❌ "Handle errors"                                    ┃
┃  ✅ "Catch network errors, show toast for 3s,          ┃
┃      include retry button, log to console"             ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: What could AI get wrong? Specify THAT.       ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**BROKEN PROMPT:**
```
Make a button that submits a form
```
**AI gave:** Plain HTML button, no styling, no loading state, wrong framework.

**Rewrite with all 4 components (1/3 done):**

```
CONTEXT:
I'm building a _______ using _______.
The form collects _______________________.

TASK:
Create a submit button that:
1. Submits the parent form
2. _______
3. _______

FORMAT:
- _______
- _______

CONSTRAINTS:
- Shows loading spinner while submitting
- Disabled when form is invalid
- _______
```

---

## 🧪 EXERCISE 3: Catch the AI Mistake (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Spotting AI Overwrites                ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  // When AI "helps", it often erases your work:        ┃
┃                                                        ┃
┃  COMMON PATTERN:                                       ┃
┃  You had:  useState(yourExistingData)                  ┃
┃  AI gives: useState([]) // Your data is GONE!          ┃
┃                                                        ┃
┃  ANOTHER PATTERN:                                      ┃
┃  You had:  Complex custom validation logic             ┃
┃  AI gives: Simple generic validation // Yours GONE!    ┃
┃                                                        ┃
┃  PREVENTION PROMPT:                                    ┃
┃  "Keep ALL existing code intact. Only ADD the          ┃
┃   new [feature]. Do not modify [specific things]."     ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Compare the BEFORE and AFTER carefully.      ┃
┃     What data existed before? What's there now?        ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**YOUR ORIGINAL CODE:**
```javascript
const [todos, setTodos] = useState([
  { id: 1, text: "Learn AI", done: false },
  { id: 2, text: "Build app", done: true },
]);
```

**YOU ASKED:** "Add a delete button to each todo"

**AI RETURNED:**
```javascript
function TodoList() {
  const [todos, setTodos] = useState([]);  // ⚠️ LOOK CLOSELY
  
  const deleteTodo = (id) => {
    setTodos(todos.filter(t => t.id !== id));
  };
  
  return (
    <ul>
      {todos.map(todo => (
        <li key={todo.id}>
          {todo.text}
          <button onClick={() => deleteTodo(todo.id)}>Delete</button>
        </li>
      ))}
    </ul>
  );
}
```

**QUESTIONS:**

1. What mistake did the AI make?
```
_______________________________________________________
```

2. Write a better prompt that prevents this:
```
_______________________________________________________
_______________________________________________________
_______________________________________________________
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI ASSISTANCE DISABLED FOR THIS TEST**

You've made it through 3 exercises. You've got this.

The AI will not help you on this test. If you've been paying attention, you won't need it.

---

<!-- REFERENCE PANEL - Shows previous helpers + external resources -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📚 REFERENCE PANEL (No hints - just resources)        ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  PREVIOUS HELPERS:                                     ┃
┃  ├─ Exercise 1: Prompt Structure                       ┃
┃  ├─ Exercise 2: Being Specific                         ┃
┃  └─ Exercise 3: Spotting AI Overwrites                 ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  ├─ 🔗 stackoverflow.com/questions/tagged/javascript   ┃
┃  ├─ 🔗 developer.mozilla.org (MDN Web Docs)            ┃
┃  ├─ 🔗 reactjs.org/docs                                ┃
┃  └─ 🔗 google.com (yes, really)                        ┃
┃                                                        ┃
┃  SAMPLE PROMPTS TO TRY:                                ┃
┃  (Note: AI may not cooperate - that's the point)       ┃
┃  ├─ "Create a search component for..."                 ┃
┃  ├─ "Add filtering functionality that..."              ┃
┃  └─ "Implement debounced search with..."               ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? Submit a ticket:                      ┃
┃     aide.wiki/help  |  forum.aide.dev                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You're building a **note-taking app** and need a **search feature**.

**Requirements:**
- Search through note titles and content
- Highlight matching text in results
- Show "No results" when nothing matches
- Filter updates as user types (debounced)

---

### WRITE YOUR PROMPT

Complete this prompt using all 4 components.
**No starter code. No hints. Apply what you learned.**

```
CONTEXT:
____________________________________________
____________________________________________
____________________________________________

TASK:
____________________________________________
____________________________________________
____________________________________________
____________________________________________

FORMAT:
____________________________________________
____________________________________________
____________________________________________

CONSTRAINTS:
____________________________________________
____________________________________________
____________________________________________
____________________________________________
```

---

### GRADING CRITERIA

Your prompt passes if it includes:

| Requirement | ✓ |
|-------------|---|
| Specifies framework/language | ☐ |
| Mentions existing app structure | ☐ |
| Lists all 4 required features | ☐ |
| Specifies code format/style | ☐ |
| Includes at least 3 constraints | ☐ |
| Clear enough that AI can't misunderstand | ☐ |

**Pass: 5 of 6 requirements**

---

## ✅ COURSE COMPLETE!

**Congratulations!** You've proven you can craft effective prompts AND work without AI when needed.

### What You Learned:
- ✅ The 4 prompt components
- ✅ How to be specific
- ✅ How to catch AI mistakes
- ✅ How to work when AI doesn't help

### Stuck on the test?
- 🆘 Wiki: **aide.wiki/help**
- 💬 Forum: **forum.aide.dev**
- 📝 Ticket: Submit in the app

### Next Course:
**AI Course 2: Understanding AI Capabilities**
