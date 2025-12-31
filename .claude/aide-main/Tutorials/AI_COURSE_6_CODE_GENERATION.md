# 💻 AI Course 6: Code Generation Mastery
## *Get Production-Ready Code from AI*

---

## 🎮 COURSE OVERVIEW

**Why Code Generation Mastery?**

Getting working code is easy. Getting GOOD code—code that's readable, maintainable, secure, and fits your project—requires skill.

This course teaches you to:
- Structure prompts for better code output
- Specify requirements that prevent common issues
- Guide AI toward your project's patterns
- Review AI code like a senior developer

**The Gap:**
```
What most people get:    Tutorial-quality code
What production needs:   Battle-tested code

This course bridges that gap.
```

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - prompt patterns
- 🧠 Final Test - AI uncooperative, get good code anyway

---

# PART 1: CODE GENERATION FUNDAMENTALS

## 📚 The Code Request Anatomy

### Perfect Code Request Structure

```
┌─────────────────────────────────────────────────────────┐
│              PERFECT CODE REQUEST                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. CONTEXT                                             │
│     What language/framework/project                     │
│                                                         │
│  2. WHAT                                                │
│     Exactly what to build                               │
│                                                         │
│  3. HOW                                                 │
│     Patterns, styles, conventions                       │
│                                                         │
│  4. CONSTRAINTS                                         │
│     What NOT to do, limits                              │
│                                                         │
│  5. EXAMPLE (Optional)                                  │
│     Show the style you want                             │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Example Comparison

**Basic request:**
```
"Create a function to fetch users"
```

**Professional request:**
```
"TypeScript/React project.

Create a function to fetch users from our API:
- Endpoint: GET /api/users
- Return type: Promise<User[]>
- Handle loading, error, and success states
- Use our existing apiClient wrapper
- Include TypeScript types
- Handle empty results gracefully
- Follow our pattern: async/await, try/catch

Our User type: { id: string; name: string; email: string }
```

**Result difference:**
- Basic: Generic, might not fit project
- Professional: Drops right into your codebase

---

## 📚 Specifying Language & Framework

### Be Explicit About Everything

```
✅ GOOD:
"TypeScript 5, React 18, Next.js 14 App Router"

❌ BAD:
"JavaScript" (which version? ES6? CommonJS?)
"React" (class or function? which version?)
"Node" (Express? Fastify? vanilla http?)
```

### Framework-Specific Details

**React:**
```
Specify:
- Functional vs Class components
- State management (useState, Redux, Zustand, etc.)
- Styling approach (CSS, Tailwind, styled-components)
- TypeScript or JavaScript
```

**Node.js:**
```
Specify:
- Framework (Express, Fastify, Koa)
- TypeScript or JavaScript
- Module system (ESM or CommonJS)
- Database (if relevant)
```

**Swift/iOS:**
```
Specify:
- SwiftUI or UIKit
- Architecture (MVVM, MVC)
- iOS version target
- Combine or async/await
```

---

## 📚 Defining Behavior

### Input/Output Specification

```
FUNCTION: validateEmail

INPUT:
- email: string (user-provided, potentially malicious)

OUTPUT:
- valid: boolean
- error: string | null (human-readable message if invalid)

EDGE CASES:
- Empty string → { valid: false, error: "Email required" }
- No @ symbol → { valid: false, error: "Invalid format" }
- Multiple @ → { valid: false, error: "Invalid format" }
- Valid email → { valid: true, error: null }
```

### State Machine for Complex Logic

```
COMPONENT: LoginForm

STATES:
- idle: Initial state, form empty
- typing: User entering data
- validating: Checking input
- submitting: API call in progress
- error: Login failed
- success: Login succeeded

TRANSITIONS:
- idle → typing: User focuses input
- typing → validating: User blurs input
- validating → typing: Validation fails
- typing → submitting: Form submitted
- submitting → error: API returns error
- submitting → success: API returns OK
- error → typing: User edits field
```

---

## 📚 Style & Convention Guides

### Naming Conventions

```
Specify your conventions:

FUNCTIONS:
- camelCase: fetchUserData
- Prefix: useUserData (hooks), handleClick (handlers)

CONSTANTS:
- SCREAMING_SNAKE: MAX_RETRIES, API_BASE_URL

COMPONENTS:
- PascalCase: UserProfile, LoginForm

FILES:
- kebab-case: user-service.ts
- Or PascalCase: UserService.ts
```

### Code Style

```
Specify preferences:

FUNCTIONS:
- Arrow functions preferred
- Named exports over default

CONDITIONALS:
- Early returns for guards
- No nested ternaries

ERROR HANDLING:
- Try/catch with specific errors
- Always log errors

TYPING:
- Explicit return types
- No 'any'
```

---

## 📚 Preventing Common AI Code Issues

### Issue 1: Missing Error Handling

```
❌ AI often gives:
const data = await fetch(url);
return data.json();

✅ You need:
try {
  const response = await fetch(url);
  if (!response.ok) throw new Error(response.status);
  return await response.json();
} catch (error) {
  console.error('Fetch failed:', error);
  throw error;
}
```

**Prevention prompt:**
```
"Include comprehensive error handling with try/catch.
Handle network errors, non-2xx responses, and parse errors."
```

### Issue 2: Missing Loading States

```
❌ AI often gives:
function UserList() {
  const [users, setUsers] = useState([]);
  // No loading indication
}

✅ You need:
function UserList() {
  const [users, setUsers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
}
```

**Prevention prompt:**
```
"Include loading, error, and empty states."
```

### Issue 3: Magic Numbers/Strings

```
❌ AI often gives:
if (retries < 3) { ... }
const url = 'https://api.example.com';

✅ You need:
const MAX_RETRIES = 3;
const API_BASE_URL = process.env.API_URL;
if (retries < MAX_RETRIES) { ... }
```

**Prevention prompt:**
```
"No magic numbers or hardcoded strings. 
Use constants and environment variables."
```

---

## 📚 Getting Clean, Readable Code

### Readability Prompts

```
"Write clean, readable code:
- Short functions (under 20 lines)
- Descriptive variable names
- Comments for complex logic only
- Early returns to reduce nesting"
```

### Documentation Prompts

```
"Include:
- JSDoc for public functions
- Type annotations (TypeScript)
- One-line comment explaining non-obvious code"
```

### Example-Driven Specification

```
"Here's an example of our code style:

export async function fetchUser(id: string): Promise<User> {
  const response = await apiClient.get(`/users/${id}`);
  return response.data;
}

Now create similar functions for:
- fetchPosts(userId: string)
- fetchComments(postId: string)"
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Complete the Request (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Request Anatomy                       ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  1. CONTEXT - language, framework, project type        ┃
┃  2. WHAT - exact functionality needed                  ┃
┃  3. HOW - patterns, style, conventions                 ┃
┃  4. CONSTRAINTS - limits, what NOT to do               ┃
┃  5. EXAMPLE - show the style (optional)                ┃
┃                                                        ┃
┃  DETAIL CHECKLIST:                                     ┃
┃  □ Language version                                    ┃
┃  □ Framework version                                   ┃
┃  □ Input types                                         ┃
┃  □ Output types                                        ┃
┃  □ Error handling approach                             ┃
┃  □ Edge cases                                          ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: More detail = less refinement needed.        ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Basic request to improve:**
```
"Make a shopping cart"
```

**Complete the professional request (1/3 started):**

```
CONTEXT:
Language: TypeScript
Framework: React 18 with hooks
State: _______________
Styling: _______________

WHAT:
Create a ShoppingCart component that:
- _______________________________________________
- _______________________________________________
- _______________________________________________

HOW:
- Use functional components with hooks
- _______________________________________________
- _______________________________________________

CONSTRAINTS:
- No class components
- _______________________________________________
- _______________________________________________

TYPES NEEDED:
interface CartItem {
  _______________________________________________
}
```

---

## 🧪 EXERCISE 2: Specify Edge Cases (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Common Edge Cases                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  FOR ARRAYS:                                           ┃
┃  • Empty array                                         ┃
┃  • Single item                                         ┃
┃  • Very large array                                    ┃
┃  • Array with duplicates                               ┃
┃                                                        ┃
┃  FOR STRINGS:                                          ┃
┃  • Empty string                                        ┃
┃  • Very long string                                    ┃
┃  • Special characters                                  ┃
┃  • Unicode/emoji                                       ┃
┃                                                        ┃
┃  FOR NUMBERS:                                          ┃
┃  • Zero                                                ┃
┃  • Negative                                            ┃
┃  • Decimal                                             ┃
┃  • Very large/small                                    ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Think "what could go wrong?"                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**You want a pagination function. List edge cases:**

```
FUNCTION: paginate(items, page, perPage)

EDGE CASES TO HANDLE:

Empty items: _____________________________________

Page number issues:
- Page 0: ________________________________________
- Negative page: _________________________________
- Page beyond data: ______________________________

Per page issues:
- Zero: __________________________________________
- Negative: ______________________________________

Other:
- _______________________________________________
- _______________________________________________
```

---

## 🧪 EXERCISE 3: Write Style Requirements (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Style Specification                   ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SPECIFY:                                              ┃
┃  • Naming (camelCase, PascalCase, etc.)                ┃
┃  • Functions (arrow vs function keyword)               ┃
┃  • Exports (named vs default)                          ┃
┃  • Conditionals (early return? ternaries?)             ┃
┃  • Error handling (try/catch style)                    ┃
┃  • Comments (when? format?)                            ┃
┃  • Types (explicit? strict?)                           ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Look at existing code in project.            ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Write a style guide for AI based on these code samples:**

**Sample 1:**
```typescript
export const fetchUser = async (userId: string): Promise<User> => {
  try {
    const response = await apiClient.get(`/users/${userId}`);
    return response.data;
  } catch (error) {
    logger.error('Failed to fetch user', { userId, error });
    throw new AppError('USER_FETCH_FAILED', error);
  }
};
```

**Sample 2:**
```typescript
export const validateEmail = (email: string): ValidationResult => {
  if (!email) return { valid: false, error: 'Required' };
  if (!email.includes('@')) return { valid: false, error: 'Invalid' };
  return { valid: true, error: null };
};
```

**Your extracted style guide (1/3 started):**

```
FUNCTIONS:
- Arrow function syntax
- Named exports
- _____________________________________

TYPES:
- Explicit return types
- _____________________________________

ERROR HANDLING:
- _____________________________________
- _____________________________________

RETURNS:
- _____________________________________

NAMING:
- _____________________________________
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI will give incomplete code, ignore your style requirements, and miss edge cases. You must craft requests that minimize these issues.

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
┃  • Context (lang/framework/project)                    ┃
┃  • What (functionality)                                ┃
┃  • How (patterns/style)                                ┃
┃  • Constraints (limits)                                ┃
┃  • Edge cases (what could go wrong)                    ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 MDN Web Docs - developer.mozilla.org               ┃
┃  🔗 TypeScript Docs - typescriptlang.org               ┃
┃  🔗 Google - google.com                                ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You need an authentication service for a Node.js/Express/TypeScript API.

Requirements:
- User registration with email/password
- Login returning JWT token
- Password hashing with bcrypt
- Input validation
- Error handling

---

### WRITE THE COMPLETE CODE REQUEST

**Part 1: Context (20 points)**
```
________________________________________________
________________________________________________
________________________________________________
```

**Part 2: Functionality (25 points)**
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 3: Style & Patterns (20 points)**
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 4: Edge Cases (20 points)**
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 5: Constraints (15 points)**
```
________________________________________________
________________________________________________
________________________________________________
```

---

### GRADING CRITERIA

| Section | Points |
|---------|--------|
| Clear context specification | /20 |
| Complete functionality requirements | /25 |
| Style and pattern guidance | /20 |
| Comprehensive edge cases | /20 |
| Explicit constraints | /15 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ Anatomy of perfect code requests
- ✅ Specifying language and framework details
- ✅ Defining behavior with input/output/states
- ✅ Communicating style conventions
- ✅ Preventing common AI code issues
- ✅ Getting readable, maintainable code

### Key Insight:
**Detailed requests get production code. Vague requests get tutorial code.** Invest time upfront to save debugging later.

### Next Course:
**AI Course 7: Chain of Thought Prompting**
