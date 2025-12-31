# 🚧 AI Course 5: Understanding AI Limitations
## *Know When AI Will Fail You*

---

## 🎮 COURSE OVERVIEW

**Why Learn AI Limitations?**

The best AI users aren't the ones who trust AI completely—they're the ones who know exactly when AI will fail and plan accordingly.

This course teaches you to recognize:
- What AI cannot do well
- When AI will confidently give wrong answers
- How to verify AI output
- When to skip AI entirely

**The Uncomfortable Truth:**

AI will give you wrong code that LOOKS right. It will explain things that are FACTUALLY incorrect with complete confidence. It will not warn you when it's making things up.

Your job is to know when this happens.

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - verification techniques
- 🧠 Final Test - AI uncooperative, trust issues amplified

---

# PART 1: TYPES OF AI LIMITATIONS

## 📚 Knowledge Cutoff

### What It Is

AI training has an end date. It doesn't know about anything after that.

```
┌─────────────────────────────────────────────────────────┐
│                 AI KNOWLEDGE TIMELINE                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ◀━━━━━━━━ AI KNOWS ━━━━━━━━━▶│◀━━━ AI DOESN'T KNOW ━━━▶│
│                                │                        │
│  Past ─────────────────────> Cutoff ──────────────> Now │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### What This Means

AI might:
- Suggest deprecated APIs
- Miss new language features
- Not know about security patches
- Give outdated best practices
- Not know recent library versions

### Example Problems

```
YOU: "How do I use the new React Server Components?"
AI: [Might give outdated or incomplete info if trained before stable release]

YOU: "What's the latest version of Node.js?"
AI: [Gives the version from its training, not actual latest]

YOU: "How do I fix CVE-2024-XXXX?"
AI: [Has no idea what you're talking about]
```

### How to Handle

```
✅ ALWAYS verify versions against current docs
✅ Check release dates of suggested libraries
✅ Use AI for concepts, verify details yourself
✅ Ask: "When was your training data cut off?"
```

---

## 📚 Hallucination

### What It Is

AI makes things up. Confidently. Without warning you.

```
HALLUCINATION EXAMPLES:

• Inventing function names that don't exist
• Creating fake API endpoints
• Citing papers that were never written
• Describing features that don't exist
• Making up statistics
```

### Why It Happens

AI predicts "likely" text. Sometimes "likely" is wrong.

```
AI thinking: "This looks like a situation where 
             X library would have Y function..."
             
Reality: X library has no such function
```

### Real Examples

```
AI: "Use the .groupBy() method on arrays"
Reality: Not in JavaScript (yet/without polyfill)

AI: "Import from 'react-native-swipe-gestures'"
Reality: AI might be mixing up library names

AI: "Set the maxRetries property"
Reality: That property might not exist in the version you're using
```

### Red Flags for Hallucination

```
🚩 Very specific function/method names you've never seen
🚩 Import paths that look slightly off
🚩 Configuration options that seem made up
🚩 "Facts" that you can't verify easily
🚩 Syntax that looks plausible but you're not sure about
```

---

## 📚 Logic and Math Errors

### What AI Struggles With

```
❌ Complex multi-step math
❌ Edge case logic
❌ Off-by-one errors
❌ Boundary conditions
❌ Correct comparison operators (< vs <= vs > vs >=)
```

### Classic Errors

**Off-by-one:**
```javascript
// AI might give:
for (let i = 0; i <= array.length; i++) // ← Wrong
// Should be:
for (let i = 0; i < array.length; i++)  // ← Correct
```

**Wrong comparison:**
```javascript
// AI might give:
if (age > 18) // Adults only
// When you need:
if (age >= 18) // 18-year-olds ARE adults
```

**Array bounds:**
```javascript
// AI might give:
array[array.length] = newItem;  // ← Wrong (out of bounds)
// Should be:
array[array.length - 1] = newItem; // Or use .push()
```

### Why This Happens

AI pattern-matches, not calculates. It's seen thousands of loops and picks what looks right—not what IS right.

---

## 📚 Context-Specific Blindness

### AI Doesn't Know Your Project

AI has no idea about:
- Your file structure
- Your custom functions
- Your team's conventions
- Your database schema
- Your existing code patterns
- Your business requirements

### Problems This Causes

```
YOU: "Add a delete function"
AI: Creates standalone function

REALITY: Your project uses classes, 
         and you needed a method

YOU: "Handle the user data"
AI: Creates UserData type

REALITY: You already have a User type 
         that it should extend
```

### How to Compensate

Always provide context:
```
"In my project:
- We use camelCase for functions
- All API calls go through the apiService
- State is managed with Zustand
- We use TypeScript strict mode

Given this, create..."
```

---

## 📚 Security Blindness

### AI Often Ignores Security

AI prioritizes "working code" over "secure code"

```
COMMON SECURITY OVERSIGHTS:

❌ SQL queries without parameterization
❌ Missing input validation
❌ Exposed API keys in code
❌ No authentication checks
❌ Hardcoded passwords
❌ Missing HTTPS
❌ XSS vulnerabilities
```

### Example Dangerous Code

```javascript
// AI might give you:
app.get('/user/:id', (req, res) => {
  const query = `SELECT * FROM users WHERE id = ${req.params.id}`;
  // ← SQL INJECTION VULNERABILITY!
});

// Should be:
app.get('/user/:id', (req, res) => {
  const query = `SELECT * FROM users WHERE id = ?`;
  db.query(query, [req.params.id]);
});
```

### Security Checklist

After getting AI code, ALWAYS check:
```
□ SQL injection (use parameters)
□ XSS (sanitize output)
□ Input validation (validate all inputs)
□ Authentication (verify user identity)
□ Authorization (verify user permissions)
□ Secrets (no hardcoded keys/passwords)
□ HTTPS (encrypt connections)
```

---

## 📚 Outdated Patterns

### AI Learned from the Past

Training data includes old code. AI might suggest:
- Deprecated methods
- Old patterns
- Anti-patterns that were once common
- Framework patterns from 3 versions ago

### Examples

```javascript
// AI might give jQuery-era code:
$.ajax({...})

// When you wanted:
fetch('...')

// AI might give class components:
class MyComponent extends React.Component

// When you wanted:
function MyComponent() { ... }

// AI might give callbacks:
fs.readFile(path, (err, data) => {...})

// When you wanted:
const data = await fs.promises.readFile(path)
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Spot the Hallucination (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Hallucination Indicators              ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SUSPICIOUS SIGNS:                                     ┃
┃  • Very specific method names you've never seen        ┃
┃  • Import paths that look "almost right"               ┃
┃  • Configuration options that seem too convenient      ┃
┃  • Features that solve your exact problem perfectly    ┃
┃                                                        ┃
┃  VERIFICATION STEPS:                                   ┃
┃  1. Search official docs for the method/property       ┃
┃  2. Check npm/package registry for the package         ┃
┃  3. Test in isolation before trusting                  ┃
┃  4. Google the exact syntax                            ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: If it sounds too good to be true...          ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Analyze this AI response for possible hallucinations:**

```javascript
import { autoValidate, schemaCheck } from 'express-validator/advanced';

app.post('/user', autoValidate({
  email: { type: 'email', required: true },
  age: { type: 'number', min: 0, max: 150 }
}), (req, res) => {
  // All validation happens automatically
});
```

**What looks suspicious?**
```
1. Import: ________________________________________
2. Function: ______________________________________
3. Options: _______________________________________
```

**How would you verify this?**
```
Step 1: ___________________________________________
Step 2: ___________________________________________
Step 3: ___________________________________________
```

---

## 🧪 EXERCISE 2: Find the Security Issues (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Security Checklist                    ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  CHECK FOR:                                            ┃
┃  □ SQL Injection (string concatenation in queries)     ┃
┃  □ XSS (unsanitized user input in HTML)               ┃
┃  □ Missing auth (no user verification)                 ┃
┃  □ Hardcoded secrets (passwords, API keys)            ┃
┃  □ No input validation                                 ┃
┃  □ Sensitive data exposure                             ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Assume every input is malicious.             ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Find all security issues in this AI-generated code:**

```javascript
const API_KEY = "sk-abc123secretkey456";

app.post('/login', (req, res) => {
  const { username, password } = req.body;
  
  const query = `SELECT * FROM users 
    WHERE username = '${username}' 
    AND password = '${password}'`;
  
  db.query(query, (err, user) => {
    if (user) {
      res.json({ success: true, user: user });
    }
  });
});

app.get('/admin/:command', (req, res) => {
  exec(req.params.command, (err, output) => {
    res.send(output);
  });
});
```

**List the security issues (1/3 started):**

```
1. Hardcoded API key: _____________________________
2. SQL Injection: _________________________________
3. ______________________________________________
4. ______________________________________________
5. ______________________________________________
6. ______________________________________________
```

---

## 🧪 EXERCISE 3: Verify AI Claims (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Verification Strategy                 ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  FOR EACH AI CLAIM, ASK:                               ┃
┃  • Can I find this in official documentation?          ┃
┃  • Does this exist in my version?                      ┃
┃  • Can I test this in isolation?                       ┃
┃  • Does this match what I know?                        ┃
┃                                                        ┃
┃  VERIFICATION SOURCES:                                 ┃
┃  • Official documentation                              ┃
┃  • MDN (for JavaScript/Web)                            ┃
┃  • Package README on GitHub                            ┃
┃  • Stack Overflow (with caution)                       ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: When in doubt, test it.                      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**AI made these claims. How would you verify each?**

**Claim 1:** "JavaScript arrays have a built-in .unique() method"
```
Verification: ____________________________________
Likely truth: ____________________________________
```

**Claim 2:** "React 18 introduced the useAutoEffect hook"
```
Verification: ____________________________________
Likely truth: ____________________________________
```

**Claim 3:** "Node.js 20+ supports native TypeScript execution"
```
Verification: ____________________________________
Likely truth: ____________________________________
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE PARTICULARLY UNRELIABLE FOR THIS SECTION**

The AI will hallucinate, give security-vulnerable code, and make confident errors. Trust nothing. Verify everything.

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
┃  • Knowledge cutoff - AI doesn't know recent things    ┃
┃  • Hallucination - AI makes things up confidently      ┃
┃  • Logic errors - AI pattern-matches, not calculates   ┃
┃  • Context blind - AI doesn't know YOUR project        ┃
┃  • Security blind - AI ignores security often          ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 MDN Web Docs - developer.mozilla.org               ┃
┃  🔗 Google - google.com                                ┃
┃  🔗 Official documentation for any library             ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You asked AI for a user authentication system. It provided:

```javascript
import { secureHash, validateToken } from 'crypto-auth-utils';

const JWT_SECRET = "super_secret_key_123";

app.post('/register', (req, res) => {
  const { email, password } = req.body;
  
  const hashedPassword = secureHash(password);
  const query = `INSERT INTO users (email, password) 
                 VALUES ('${email}', '${hashedPassword}')`;
  
  db.run(query, () => {
    res.json({ success: true });
  });
});

app.post('/login', (req, res) => {
  const query = `SELECT * FROM users 
                 WHERE email = '${req.body.email}'`;
  
  db.get(query, (err, user) => {
    if (user && user.password === secureHash(req.body.password)) {
      const token = jwt.sign({ userId: user.id }, JWT_SECRET);
      res.json({ token });
    }
  });
});

app.get('/profile', (req, res) => {
  const token = req.headers.authorization;
  const user = validateToken(token, JWT_SECRET);
  res.json(user);
});
```

---

### COMPLETE THE ANALYSIS

**Part 1: Hallucination Check (25 points)**

Identify likely hallucinations:
```
Suspicious 1: ____________________________________
Why suspicious: __________________________________

Suspicious 2: ____________________________________
Why suspicious: __________________________________
```

**Part 2: Security Audit (30 points)**

List ALL security vulnerabilities:
```
1. _______________________________________________
2. _______________________________________________
3. _______________________________________________
4. _______________________________________________
5. _______________________________________________
```

**Part 3: Verification Plan (25 points)**

What would you verify before using this code?
```
Verify 1: _______________________________________
How: ___________________________________________

Verify 2: _______________________________________
How: ___________________________________________

Verify 3: _______________________________________
How: ___________________________________________
```

**Part 4: Fixed Code Outline (20 points)**

Write the key fixes (just describe, don't code):
```
Fix for SQL injection: __________________________
Fix for hardcoded secret: _______________________
Fix for missing validation: _____________________
Fix for suspicious imports: _____________________
```

---

### GRADING CRITERIA

| Section | Points |
|---------|--------|
| Correctly identified hallucinations | /25 |
| Found all security vulnerabilities | /30 |
| Solid verification plan | /25 |
| Appropriate fixes described | /20 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ AI has a knowledge cutoff
- ✅ AI hallucinates confidently
- ✅ AI makes logic errors
- ✅ AI doesn't know your project
- ✅ AI often ignores security
- ✅ How to verify AI claims

### Key Insight:
**AI is a junior developer who lies confidently.** Treat its output with appropriate skepticism—useful starting point, but always needs review.

### Next Course:
**AI Course 6: Code Generation Mastery**
