# 📊 AI Course 10: Structured Output
## *Get Machine-Readable Responses*

---

## 🎮 COURSE OVERVIEW

**What is Structured Output?**

Structured output means getting AI responses in predictable, parseable formats—JSON, YAML, tables, specific schemas—instead of freeform text.

**Why It Matters:**

```
FREEFORM OUTPUT:
"The user John is 25 years old and lives in NYC"
→ Hard to parse programmatically
→ Format varies each time

STRUCTURED OUTPUT:
{
  "name": "John",
  "age": 25,
  "city": "NYC"
}
→ Easy to parse
→ Consistent format
```

**Use Cases:**
- Building apps that consume AI output
- Data extraction and transformation
- API response generation
- Configuration file creation
- Test data generation

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - format patterns
- 🧠 Final Test - AI uncooperative, force structure anyway

---

# PART 1: STRUCTURED OUTPUT FUNDAMENTALS

## 📚 Format Specification

### Explicit Format Request

```
BASIC:
"Return the result as JSON."

BETTER:
"Return ONLY valid JSON, no additional text or markdown."

BEST:
"Return ONLY valid JSON matching this schema:
{
  "name": string,
  "age": number,
  "email": string
}
No markdown, no explanation, just the JSON object."
```

### Schema First

Always show the exact structure you want:

```
"Extract user info into this exact format:

{
  "firstName": string,
  "lastName": string,
  "email": string,
  "phone": string | null,
  "addresses": [
    {
      "type": "home" | "work",
      "street": string,
      "city": string,
      "state": string,
      "zip": string
    }
  ]
}"
```

---

## 📚 Common Formats

### JSON

```
"Return as JSON:
{
  "success": boolean,
  "data": { ... },
  "errors": string[] | null
}"
```

### YAML

```
"Return as YAML:
user:
  name: string
  settings:
    theme: light | dark
    notifications: boolean"
```

### Markdown Table

```
"Return as a markdown table:
| Column1 | Column2 | Column3 |
|---------|---------|---------|
| value   | value   | value   |"
```

### CSV

```
"Return as CSV with headers:
name,age,email
John,25,john@test.com"
```

### TypeScript Types

```
"Return as TypeScript interface:
interface User {
  id: string;
  name: string;
  ...
}"
```

---

## 📚 Preventing Format Errors

### Common Problems & Solutions

**Problem 1: AI adds markdown code blocks**
```
BAD AI OUTPUT:
\`\`\`json
{ "name": "John" }
\`\`\`

SOLUTION:
"Return ONLY the raw JSON, no markdown code blocks."
```

**Problem 2: AI adds explanatory text**
```
BAD AI OUTPUT:
Here is the JSON you requested:
{ "name": "John" }

SOLUTION:
"Return ONLY the JSON. No explanations, no other text."
```

**Problem 3: AI uses wrong types**
```
BAD AI OUTPUT:
{ "age": "25" }  // string instead of number

SOLUTION:
"Ensure age is a number, not a string."
Or: Show explicit example with correct types
```

**Problem 4: AI forgets fields**
```
SOLUTION:
"All fields are required. Include every field, even if null."
```

---

## 📚 Schema Enforcement

### Detailed Schema Specification

```
"Generate user data matching this EXACT schema:

{
  "id": string,           // UUID format
  "name": {
    "first": string,      // 2-50 characters
    "last": string        // 2-50 characters
  },
  "email": string,        // valid email format
  "age": number,          // integer, 0-150
  "role": "user" | "admin" | "moderator",
  "active": boolean,
  "createdAt": string,    // ISO 8601 datetime
  "metadata": object | null
}

Rules:
- All fields required unless marked with | null
- Strings must be properly escaped
- Numbers must be actual numbers, not strings
- Use lowercase true/false for booleans"
```

### Example-Based Schema

```
"Generate data following this example EXACTLY:

{
  "product": {
    "id": "prod_abc123",
    "name": "Widget",
    "price": 29.99,
    "inStock": true,
    "tags": ["electronics", "gadgets"],
    "dimensions": {
      "width": 10.5,
      "height": 5.2,
      "unit": "cm"
    }
  }
}

Generate 3 more products in identical format."
```

---

## 📚 Structured Extraction

### From Unstructured Text

```
"Extract information from this text into JSON:

TEXT:
'Hi, I'm Sarah Johnson. I work at TechCorp as a senior engineer.
You can reach me at sarah.j@techcorp.com or call 555-0123.
I've been in the industry for 12 years.'

EXTRACT INTO:
{
  "name": string,
  "company": string,
  "title": string,
  "email": string,
  "phone": string,
  "yearsExperience": number
}"
```

### From Documents

```
"Parse this invoice into structured data:

[Invoice text]

STRUCTURE:
{
  "invoiceNumber": string,
  "date": string,
  "vendor": {
    "name": string,
    "address": string
  },
  "lineItems": [
    {
      "description": string,
      "quantity": number,
      "unitPrice": number,
      "total": number
    }
  ],
  "subtotal": number,
  "tax": number,
  "total": number
}"
```

---

## 📚 Generating Structured Data

### Test Data Generation

```
"Generate 5 test users as a JSON array.

Each user must have:
{
  "id": string (UUID),
  "username": string (lowercase, no spaces),
  "email": string (valid format),
  "password": string (at least 8 chars),
  "profile": {
    "firstName": string,
    "lastName": string,
    "avatar": string (URL format)
  },
  "settings": {
    "theme": "light" | "dark",
    "notifications": boolean
  },
  "createdAt": string (ISO date)
}

Make data realistic, not just "test1", "test2"."
```

### Configuration Generation

```
"Generate a Docker Compose configuration for:
- Node.js API (port 3000)
- PostgreSQL database
- Redis cache
- Nginx reverse proxy

Return as valid YAML, ready to use."
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Define Output Schema (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Schema Definition                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SCHEMA ELEMENTS:                                      ┃
┃  • Field name                                          ┃
┃  • Type (string, number, boolean, array, object)       ┃
┃  • Constraints (length, range, format)                 ┃
┃  • Required vs optional                                ┃
┃  • Enum values (if limited options)                    ┃
┃                                                        ┃
┃  FORMAT:                                               ┃
┃  {                                                     ┃
┃    "field": type,        // constraint/description     ┃
┃    "field2": type | null // optional                   ┃
┃  }                                                     ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Be explicit about EVERY detail.              ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a schema for an e-commerce order:**

**Your schema (1/3 started):**

```
{
  "orderId": string,           // Format: ORD-XXXXX
  "status": "pending" | "paid" | "shipped" | "delivered",
  "customer": {
    "id": string,
    "email": string,
    "name": string
  },
  "items": [
    {
      "productId": ___________________,
      "name": ___________________,
      "quantity": ___________________,
      "unitPrice": ___________________,
      "total": ___________________
    }
  ],
  "shipping": {
    ___________________
    ___________________
    ___________________
  },
  "totals": {
    ___________________
    ___________________
    ___________________
  },
  "createdAt": ___________________,
  "updatedAt": ___________________
}
```

---

## 🧪 EXERCISE 2: Extract to Structure (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Extraction Prompt                     ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  EXTRACTION STRUCTURE:                                 ┃
┃  1. Show the source text                               ┃
┃  2. Define the target schema                           ┃
┃  3. Specify extraction rules                           ┃
┃  4. Handle missing data                                ┃
┃                                                        ┃
┃  RULES TO SPECIFY:                                     ┃
┃  • What to do if field not found → null or default     ┃
┃  • How to format dates/numbers                         ┃
┃  • How to handle multiple matches                      ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Cover all edge cases in your prompt.         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create an extraction prompt for job postings:**

**Sample job posting text:**
```
Senior Frontend Developer at TechStartup Inc.
Location: Remote (US Only)
Salary: $120,000 - $160,000/year
Experience: 5+ years required
Skills: React, TypeScript, GraphQL, Node.js
Apply by: March 15, 2024
```

**Your extraction prompt (1/3 started):**

```
Extract job posting info into this JSON schema:

{
  "title": string,
  "company": string,
  "location": {
    "type": "remote" | "hybrid" | "onsite",
    "restriction": string | null
  },
  "salary": {
    "min": number,
    "max": number,
    "currency": string,
    "period": "hourly" | "yearly"
  },
  "requirements": {
    "yearsExperience": ___________________,
    "skills": ___________________
  },
  "applicationDeadline": ___________________
}

RULES:
- If salary not specified, use null for min/max
- ___________________
- ___________________

TEXT:
[job posting text]
```

---

## 🧪 EXERCISE 3: Generate Test Data (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Test Data Generation                  ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  SPECIFY:                                              ┃
┃  • Exact schema                                        ┃
┃  • Number of items                                     ┃
┃  • Realistic vs dummy data                             ┃
┃  • Any specific scenarios to include                   ┃
┃  • Relationships between items (if any)                ┃
┃                                                        ┃
┃  SCENARIOS TO INCLUDE:                                 ┃
┃  • Normal case                                         ┃
┃  • Edge cases (empty, max values)                      ┃
┃  • Error cases (invalid states)                        ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Ask for variety to test edge cases.          ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a prompt to generate test data for products:**

**Your prompt (1/3 started):**

```
Generate 5 test products as a JSON array.

SCHEMA:
{
  "id": string,              // Format: "prod_" + 8 chars
  "name": string,            // Realistic product name
  "price": number,           // 0.01 to 9999.99
  "category": "electronics" | "clothing" | "home" | "sports",
  "inventory": {
    "stock": ___________________,
    "warehouse": ___________________
  },
  "attributes": {
    ___________________
  },
  "active": boolean,
  "createdAt": string        // ISO 8601
}

INCLUDE THESE SCENARIOS:
1. Normal product, good stock
2. ___________________
3. ___________________
4. ___________________
5. ___________________

Return ONLY the JSON array, no markdown.
```

---

# PART 3: FINAL TEST

## 🎓 CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI will try to add extra text, use wrong formats, and miss fields. You must enforce strict structure through precise prompting.

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
┃  • Show exact schema                                   ┃
┃  • Specify types with constraints                      ┃
┃  • Say "no markdown, no explanation"                   ┃
┃  • Give examples for complex formats                   ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  🔗 JSON Schema - json-schema.org                      ┃
┃  🔗 Stack Overflow - stackoverflow.com                 ┃
┃  🔗 Google - google.com                                ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You're building an API that needs to parse natural language event descriptions into structured data.

**Sample input:**
```
"Team meeting on Friday at 2pm in Conference Room A. 
Attendees: John, Sarah, Mike. Duration: 1 hour.
This is a recurring weekly meeting until end of March.
Priority: High. Bring laptop."
```

---

### CREATE A COMPLETE STRUCTURED EXTRACTION PROMPT

**Part 1: Define the Schema (30 points)**
```
{
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
  ________________________________________________
}
```

**Part 2: Extraction Rules (25 points)**
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 3: Handle Edge Cases (25 points)**
```
________________________________________________
________________________________________________
________________________________________________
________________________________________________
```

**Part 4: Format Enforcement (20 points)**
```
________________________________________________
________________________________________________
________________________________________________
```

---

### GRADING CRITERIA

| Section | Points |
|---------|--------|
| Complete schema with all fields | /30 |
| Clear extraction rules | /25 |
| Edge case handling | /25 |
| Format enforcement | /20 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ Format specification techniques
- ✅ Schema definition with constraints
- ✅ Preventing format errors
- ✅ Structured data extraction
- ✅ Test data generation
- ✅ Format enforcement

### Key Insight:
**Schema first, data second.** Always define the exact structure before asking AI to fill it.

### Next Course:
**AI Course 11: Multi-Agent Collaboration**
