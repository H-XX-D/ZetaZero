# 🎭 AI Course 9: Role-Based Prompting
## *Unlock Expert Perspectives*

---

## 🎮 COURSE OVERVIEW

**What is Role-Based Prompting?**

Role-based prompting assigns AI a specific persona, expertise, or perspective. Instead of talking to "general AI," you're talking to a specialist.

**Why It Works:**

```
WITHOUT ROLE:
"Review my code"
→ Generic feedback

WITH ROLE:
"You are a senior security engineer. Review my code."
→ Security-focused feedback

"You are a performance optimization expert. Review my code."
→ Performance-focused feedback
```

**The Power of Perspective:**

Different roles catch different issues:
- Security expert → Finds vulnerabilities
- UX designer → Finds usability issues
- Junior developer → Finds unclear code
- QA engineer → Finds test gaps

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - role patterns
- 🧠 Final Test - AI uncooperative, roles unlock better responses

---

# PART 1: ROLE-BASED FUNDAMENTALS

## 📚 Role Anatomy

### Basic Role Structure

```
"You are a [expertise level] [role] specializing in [specific area].

[Optional: Background context]

[Your request]"
```

### Examples

```
BASIC:
"You are a senior developer. Review this code."

DETAILED:
"You are a senior React developer with 10 years of experience.
You specialize in performance optimization and have worked on
large-scale applications with millions of users. You're known
for writing clean, maintainable code.

Review this component for performance issues."
```

---

## 📚 Common Developer Roles

### Technical Roles

```
SENIOR DEVELOPER:
"You are a senior developer with 15+ years of experience.
Focus on architecture, patterns, and maintainability."

SECURITY ENGINEER:
"You are a security engineer specializing in web application
security. Look for vulnerabilities, injection attacks, and
authentication issues."

PERFORMANCE ENGINEER:
"You are a performance engineer. Identify bottlenecks, memory
leaks, and optimization opportunities."

QA ENGINEER:
"You are a QA engineer. Find edge cases, identify missing tests,
and spot potential bugs."

DEVOPS ENGINEER:
"You are a DevOps engineer. Focus on deployment, monitoring,
scalability, and infrastructure concerns."

DATABASE EXPERT:
"You are a database expert. Optimize queries, suggest indexing,
and identify N+1 problems."
```

### Non-Technical Roles (Useful for UX/Docs)

```
TECHNICAL WRITER:
"You are a technical writer. Make this documentation clear,
concise, and beginner-friendly."

UX DESIGNER:
"You are a UX designer. Review this flow for usability issues,
confusing interactions, and accessibility."

JUNIOR DEVELOPER:
"You are a junior developer seeing this codebase for the first
time. Point out what's confusing or unclear."

END USER:
"You are a non-technical user. Would this interface make sense
to you? What would confuse you?"
```

---

## 📚 Role Stacking

### Multiple Perspectives

Ask the same question from different roles:

```
"Review this login system from three perspectives:

AS A SECURITY ENGINEER:
- What vulnerabilities exist?
- How could this be attacked?

AS A UX DESIGNER:
- What friction does the user experience?
- What would improve the flow?

AS A PERFORMANCE ENGINEER:
- What could slow this down at scale?
- Any unnecessary operations?"
```

### Expert Panel

```
"Imagine you're presenting to a panel of experts:
- Sarah, CTO: Cares about architecture and technical debt
- Mike, Security Lead: Cares about vulnerabilities
- Lisa, UX Director: Cares about user experience

What concerns would each raise about this design?"
```

---

## 📚 Adversarial Roles

### The Critic

```
"You are a harsh code reviewer known for finding every flaw.
Your job is to find problems, not praise. Be critical.

Review this code and list every issue you can find."
```

### The Attacker

```
"You are a hacker trying to exploit this system. 
How would you attack it? What vulnerabilities would you target?"
```

### The Devil's Advocate

```
"You are a skeptic. Argue AGAINST this approach.
What could go wrong? Why might this be a bad idea?"
```

---

## 📚 Expertise Levels

### Calibrating Role Expertise

```
JUNIOR LEVEL:
"Explain this like I'm a junior developer who just learned
the basics. Use simple terms and lots of examples."

MID LEVEL:
"Explain this for a developer with 2-3 years of experience.
You can use standard terminology but explain advanced concepts."

SENIOR LEVEL:
"Explain this for a senior developer. Be concise, use proper
terminology, and focus on nuances and edge cases."

EXPERT LEVEL:
"Explain this for someone deeply familiar with the internals.
Discuss implementation details and tradeoffs."
```

---

## 📚 Domain-Specific Roles

### Industry Experts

```
FINTECH:
"You are a fintech developer. Ensure this code handles money
correctly with proper decimal precision and audit logging."

HEALTHCARE:
"You are a healthcare software developer. Review for HIPAA
compliance, patient data protection, and audit trails."

E-COMMERCE:
"You are an e-commerce platform architect. Consider inventory
management, payment processing, and high-traffic events."

GAMING:
"You are a game developer. Focus on frame rate, memory management,
and responsive input handling."
```

### Language/Framework Experts

```
REACT EXPERT:
"You are a React core team member. Review this component for
best practices, performance, and React-specific patterns."

TYPESCRIPT EXPERT:
"You are a TypeScript expert. Improve the type safety and
suggest better type patterns."

PYTHON EXPERT:
"You are a Python core contributor. Review for Pythonic style,
performance, and standard library usage."
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Create Role Prompts (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Role Components                       ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  ROLE STRUCTURE:                                       ┃
┃  1. Expertise level (junior/senior/expert)             ┃
┃  2. Role title (developer/architect/engineer)          ┃
┃  3. Specialization (security/performance/UX)           ┃
┃  4. Experience context (optional)                      ┃
┃                                                        ┃
┃  FORMULA:                                              ┃
┃  "You are a [level] [role] specializing in [area].     ┃
┃   You have [experience context].                       ┃
┃   [What you want them to do]."                         ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Match role expertise to the problem.         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create role prompts for these scenarios:**

**Scenario 1:** Review authentication code for security issues

```
Role prompt: _____________________________________
_________________________________________________
_________________________________________________
```

**Scenario 2:** Make API documentation beginner-friendly

```
Role prompt: _____________________________________
_________________________________________________
_________________________________________________
```

**Scenario 3:** Optimize a slow database query

```
Role prompt: _____________________________________
_________________________________________________
_________________________________________________
```

---

## 🧪 EXERCISE 2: Multi-Perspective Review (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Multi-Perspective                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  STRUCTURE:                                            ┃
┃  "Review [code/design] from multiple perspectives:     ┃
┃                                                        ┃
┃   AS A [ROLE 1]:                                       ┃
┃   - What concerns you?                                 ┃
┃   - What would you improve?                            ┃
┃                                                        ┃
┃   AS A [ROLE 2]:                                       ┃
┃   - What concerns you?                                 ┃
┃   - What would you improve?                            ┃
┃                                                        ┃
┃   AS A [ROLE 3]:                                       ┃
┃   ..."                                                 ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Choose roles that see different issues.      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a multi-perspective review for a payment processing system:**

**Your prompt (1/3 started):**

```
Review this payment processing code from multiple perspectives:

AS A SECURITY ENGINEER:
- What vulnerabilities exist?
- How could payment data be compromised?

AS A _______________________:
- _________________________________________
- _________________________________________

AS A _______________________:
- _________________________________________
- _________________________________________

[Code to review]
```

---

## 🧪 EXERCISE 3: Adversarial Review (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Adversarial Roles                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  ADVERSARIAL PERSONAS:                                 ┃
┃  • The Harsh Critic: Finds every flaw                  ┃
┃  • The Hacker: Looks for exploits                      ┃
┃  • The Skeptic: Argues against the approach            ┃
┃  • The Pessimist: Assumes worst-case scenarios         ┃
┃                                                        ┃
┃  PURPOSE:                                              ┃
┃  Adversarial roles force AI to find problems           ┃
┃  instead of being agreeable                            ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Be explicit about wanting criticism.         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create an adversarial review prompt for a new feature design:**

**Your prompt (1/3 started):**

```
I'm proposing a new caching system for our API.

First, as a HARSH CRITIC:
- Find every flaw in this approach
- What will break?
- _________________________________________

Then, as a HACKER:
- _________________________________________
- _________________________________________

Finally, as a DEVIL'S ADVOCATE:
- _________________________________________
- _________________________________________

[System design to review]
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI will give generic, agreeable feedback. You must use roles to force specific, critical, and expert perspectives.

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
┃  • Roles focus AI perspective                          ┃
┃  • Expertise level calibrates depth                    ┃
┃  • Multi-perspective catches more issues               ┃
┃  • Adversarial roles force criticism                   ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  �� Google - google.com                                ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You're about to launch a new e-commerce checkout flow. You need comprehensive review before going live.

The checkout:
- Takes payment info (card number, CVV)
- Stores addresses
- Processes orders
- Sends confirmation emails

---

### CREATE A COMPREHENSIVE ROLE-BASED REVIEW

**Part 1: Security Role (25 points)**

Write a security expert role to review the checkout:
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 2: UX Role (25 points)**

Write a UX expert role to review the checkout:
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 3: Adversarial Role (25 points)**

Write an adversarial role to stress-test the design:
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 4: Combined Multi-Role Prompt (25 points)**

Write a single prompt that gets all three perspectives:
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
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
| Effective security role | /25 |
| Effective UX role | /25 |
| Effective adversarial role | /25 |
| Well-structured combined prompt | /25 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ Role anatomy and structure
- ✅ Common developer roles
- ✅ Non-technical roles for docs/UX
- ✅ Role stacking for multiple perspectives
- ✅ Adversarial roles for finding problems
- ✅ Domain-specific expertise roles

### Key Insight:
**AI is a chameleon.** It can be any expert you need—but only if you tell it which expert to be.

### Next Course:
**AI Course 10: Structured Output**
