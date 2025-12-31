# 🎯 AI Course 8: Few-Shot Learning
## *Teach AI by Example*

---

## 🎮 COURSE OVERVIEW

**What is Few-Shot Learning?**

Few-shot learning means teaching AI what you want by showing examples. Instead of explaining, you demonstrate.

**Why It's Powerful:**

```
WITHOUT EXAMPLES:
"Format the output nicely"
→ AI guesses what "nicely" means
→ Usually wrong

WITH EXAMPLES:
"Format like this:
  Name: John
  Age: 25
  Role: Developer"
→ AI copies the exact format
→ Consistent, correct output
```

**Types of Few-Shot:**
- Zero-shot: No examples (just instructions)
- One-shot: One example
- Few-shot: 2-5 examples
- Many-shot: 5+ examples

**When to Use:**
- Custom output formats
- Specific coding styles
- Domain-specific patterns
- Complex transformations

**Learning Style:**
- ⌨️ Type it yourself - 1/3 starter code
- 📌 Code Helper - example patterns
- 🧠 Final Test - AI uncooperative, teach by example

---

# PART 1: FEW-SHOT FUNDAMENTALS

## 📚 The Power of Examples

### Example vs Description

**Describing what you want (verbose, ambiguous):**
```
"Create a function that takes a user object and returns a
formatted string with the user's name in uppercase, followed
by a pipe character, then their age, then another pipe, then
their email address in parentheses."
```

**Showing what you want (clear, unambiguous):**
```
"Format users like this:

Input: { name: "John", age: 25, email: "john@test.com" }
Output: "JOHN | 25 | (john@test.com)"

Input: { name: "Jane", age: 30, email: "jane@test.com" }
Output: "JANE | 30 | (jane@test.com)"

Now format this user:
{ name: "Bob", age: 22, email: "bob@test.com" }"
```

### Why Examples Work

```
┌─────────────────────────────────────────────────────────┐
│              WHY EXAMPLES WORK                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  DESCRIPTION:                                           │
│  "Make it like X" → AI interprets → Might be wrong      │
│                                                         │
│  EXAMPLE:                                               │
│  "[Example X]" → AI copies pattern → Usually right      │
│                                                         │
│  KEY INSIGHT:                                           │
│  AI is excellent at pattern matching.                   │
│  Feed it patterns, not descriptions.                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## 📚 Few-Shot Structure

### The Basic Pattern

```
[Optional: Brief instruction]

Example 1:
Input: [input 1]
Output: [output 1]

Example 2:
Input: [input 2]
Output: [output 2]

Example 3:
Input: [input 3]
Output: [output 3]

Now apply to:
Input: [your actual input]
```

### Number of Examples

```
1 Example:   Good for simple formats
2 Examples:  Better - shows pattern isn't coincidence
3 Examples:  Best for most cases
5+ Examples: Diminishing returns, but useful for
             complex patterns or edge cases
```

### Diverse Examples

```
✅ GOOD: Show variety
Example 1: Short name, young
Example 2: Long name, old
Example 3: Special characters, edge case

❌ BAD: Show only one type
Example 1: John, 25
Example 2: Jane, 26
Example 3: Jack, 27
(No variety = AI might not generalize well)
```

---

## 📚 Few-Shot for Code Style

### Teaching Naming Conventions

```
"Use these naming conventions:

// Variable naming:
const userAge = 25;          // camelCase
const isLoggedIn = true;     // boolean with 'is' prefix
const MAX_RETRIES = 3;       // constants in SCREAMING_SNAKE

// Function naming:
function fetchUserData() {}  // camelCase, verb first
function handleButtonClick() {} // 'handle' for event handlers
function useUserAuth() {}    // 'use' for React hooks

Now write code following these patterns for a shopping cart."
```

### Teaching Code Patterns

```
"Write functions following this pattern:

export const fetchUser = async (userId: string): Promise<User> => {
  try {
    const response = await api.get(`/users/${userId}`);
    return response.data;
  } catch (error) {
    logger.error('fetchUser failed', { userId, error });
    throw new AppError('USER_FETCH_FAILED', error);
  }
};

export const fetchProduct = async (productId: string): Promise<Product> => {
  try {
    const response = await api.get(`/products/${productId}`);
    return response.data;
  } catch (error) {
    logger.error('fetchProduct failed', { productId, error });
    throw new AppError('PRODUCT_FETCH_FAILED', error);
  }
};

Now write: fetchOrder(orderId: string)"
```

---

## 📚 Few-Shot for Data Transformation

### Parsing Patterns

```
"Parse addresses into components:

Input: "123 Main St, Apt 4B, New York, NY 10001"
Output: {
  street: "123 Main St",
  unit: "Apt 4B",
  city: "New York",
  state: "NY",
  zip: "10001"
}

Input: "456 Oak Ave, Los Angeles, CA 90001"
Output: {
  street: "456 Oak Ave",
  unit: null,
  city: "Los Angeles",
  state: "CA",
  zip: "90001"
}

Input: "789 Pine Rd, Suite 100, Chicago, IL 60601"
Output: {
  street: "789 Pine Rd",
  unit: "Suite 100",
  city: "Chicago",
  state: "IL",
  zip: "60601"
}

Now parse: "321 Elm Blvd, Unit 2A, Seattle, WA 98101"
```

### Format Conversion

```
"Convert XML to JSON:

Input:
<user>
  <name>John</name>
  <age>25</age>
</user>

Output:
{
  "user": {
    "name": "John",
    "age": 25
  }
}

Input:
<product>
  <title>Widget</title>
  <price>9.99</price>
</product>

Output:
{
  "product": {
    "title": "Widget",
    "price": 9.99
  }
}

Now convert:
<order>
  <id>12345</id>
  <total>49.99</total>
</order>"
```

---

## 📚 Few-Shot for Documentation

### Comment Style

```
"Write JSDoc following this style:

/**
 * Fetches a user by their unique identifier.
 * @param userId - The unique identifier of the user
 * @returns Promise resolving to the User object
 * @throws {NotFoundError} If user doesn't exist
 * @example
 * const user = await fetchUser('abc123');
 */
async function fetchUser(userId: string): Promise<User>

/**
 * Validates an email address format.
 * @param email - The email address to validate
 * @returns True if email is valid, false otherwise
 * @example
 * validateEmail('test@example.com'); // true
 * validateEmail('invalid'); // false
 */
function validateEmail(email: string): boolean

Now document: deleteAccount(userId: string, reason: string): Promise<void>"
```

### README Style

```
"Write README sections like this:

## Installation

\`\`\`bash
npm install my-package
\`\`\`

## Usage

\`\`\`javascript
import { doThing } from 'my-package';

const result = doThing(options);
\`\`\`

## Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| timeout | number | 5000 | Request timeout in ms |
| retries | number | 3 | Number of retry attempts |

Now write a README for a user authentication library."
```

---

## 📚 Few-Shot for Error Messages

### Consistent Error Format

```
"Generate error messages like these:

Input: User tries to access without login
Output: {
  code: "AUTH_REQUIRED",
  message: "Please log in to continue",
  action: "redirect_to_login"
}

Input: User enters invalid email
Output: {
  code: "VALIDATION_ERROR",
  message: "Please enter a valid email address",
  action: "focus_field",
  field: "email"
}

Input: User tries to delete another user's post
Output: {
  code: "PERMISSION_DENIED",
  message: "You don't have permission to delete this post",
  action: "show_error"
}

Now generate: User's session has expired"
```

---

# PART 2: GUIDED EXERCISES

## 🧪 EXERCISE 1: Create Format Examples (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Example Creation                      ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  GOOD EXAMPLES:                                        ┃
┃  • Show the EXACT format you want                      ┃
┃  • Include 2-3 varied cases                            ┃
┃  • Cover edge cases (empty, special chars)             ┃
┃  • Show both input and output                          ┃
┃                                                        ┃
┃  STRUCTURE:                                            ┃
┃  Input: [what goes in]                                 ┃
┃  Output: [what comes out]                              ┃
┃                                                        ┃
┃  VARIETY:                                              ┃
┃  Example 1: Normal case                                ┃
┃  Example 2: Different data                             ┃
┃  Example 3: Edge case                                  ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: If you want specific format, show it.        ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a few-shot prompt for generating SQL queries from natural language:**

**Your few-shot prompt (1/3 started):**

```
Convert natural language to SQL:

Example 1:
Input: "Get all users"
Output: SELECT * FROM users;

Example 2:
Input: "Find users named John"
Output: _________________________________________

Example 3:
Input: "Get users older than 30, sorted by age"
Output: _________________________________________

Now convert:
Input: "Find the 5 most recent orders with total over $100"
```

---

## 🧪 EXERCISE 2: Teach Code Style (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Style Teaching                        ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  STYLE ELEMENTS TO SHOW:                               ┃
┃  • Function signature style                            ┃
┃  • Error handling pattern                              ┃
┃  • Return value structure                              ┃
┃  • Naming conventions                                  ┃
┃  • Import style                                        ┃
┃  • Type annotations                                    ┃
┃                                                        ┃
┃  TEACHING METHOD:                                      ┃
┃  Show 2-3 complete functions in YOUR style             ┃
┃  Then ask for a new function                           ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: Be complete - show all the patterns.         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a few-shot prompt to teach your React component style:**

**Your component style (1/3 started):**

```
Write React components in this style:

Example 1:
interface ButtonProps {
  label: string;
  onClick: () => void;
  variant?: 'primary' | 'secondary';
}

export const Button: React.FC<ButtonProps> = ({ 
  label, 
  onClick, 
  variant = 'primary' 
}) => {
  return (
    <button 
      className={`btn btn-${variant}`}
      onClick={onClick}
    >
      {label}
    </button>
  );
};

Example 2:
interface CardProps {
  _________________________________________
}

export const Card: React.FC<CardProps> = ({
  _________________________________________
}) => {
  _________________________________________
};

Now create a Modal component with:
- title: string
- children: ReactNode
- isOpen: boolean
- onClose: () => void
```

---

## 🧪 EXERCISE 3: Transform Data by Example (GUIDED)

<!-- CODE HELPER WINDOW -->
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  📌 CODE HELPER: Data Transformation                   ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                                                        ┃
┃  TRANSFORMATION EXAMPLES SHOULD:                       ┃
┃  • Show source format clearly                          ┃
┃  • Show target format clearly                          ┃
┃  • Demonstrate how fields map                          ┃
┃  • Handle missing/optional data                        ┃
┃                                                        ┃
┃  COVER CASES:                                          ┃
┃  • All fields present                                  ┃
┃  • Some fields missing                                 ┃
┃  • Edge values (empty, null, etc.)                     ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  💡 HINT: The more varied examples, the better.        ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

**Create a few-shot prompt to transform API responses into your app format:**

**Your transformation (1/3 started):**

```
Transform API responses to app format:

Example 1 (all fields):
API Response:
{
  "user_id": "123",
  "first_name": "John",
  "last_name": "Doe",
  "email_address": "john@test.com",
  "created_at": "2024-01-15T10:30:00Z"
}

App Format:
{
  id: "123",
  name: "John Doe",
  email: "john@test.com",
  joinDate: "January 15, 2024"
}

Example 2 (missing optional field):
API Response:
{
  "user_id": "456",
  "first_name": "Jane",
  "last_name": null,
  "email_address": "jane@test.com",
  "created_at": "2024-02-20T15:45:00Z"
}

App Format:
{
  _________________________________________
}

Example 3 (edge case):
_________________________________________
_________________________________________

Now transform:
{
  "user_id": "789",
  "first_name": "Bob",
  "last_name": "Smith",
  "email_address": "bob@test.com",
  "created_at": "2024-03-10T08:00:00Z"
}
```

---

# PART 3: FINAL TEST

## �� CERTIFICATION TEST

**⚠️ AI WILL BE UNCOOPERATIVE FOR THIS SECTION**

The AI will try to use its own formats instead of yours. You must use few-shot examples to force your exact requirements.

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
┃  • Show, don't tell                                    ┃
┃  • 2-3 examples usually sufficient                     ┃
┃  • Include variety in examples                         ┃
┃  • Cover edge cases                                    ┃
┃                                                        ┃
┃  EXTERNAL RESOURCES:                                   ┃
┃  �� Stack Overflow - stackoverflow.com                 ┃
┃  🔗 Google - google.com                                ┃
┃                                                        ┃
┃  ─────────────────────────────────────────────────     ┃
┃  🆘 TRULY STUCK? → aide.wiki/help or Forum Ticket      ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

---

### TEST SCENARIO

You need AI to generate API endpoint documentation in a SPECIFIC format for your team. The format is:

```
## ENDPOINT_NAME

**Method:** GET/POST/etc
**Path:** /api/path
**Auth:** Required/Optional/None

### Request
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| id    | string | Yes    | Resource ID |

### Response
\`\`\`json
{
  "success": true,
  "data": { ... }
}
\`\`\`

### Errors
- `400` - Bad request
- `404` - Not found
```

---

### CREATE A FEW-SHOT PROMPT

**Write a complete few-shot prompt that will generate documentation in this exact format for any endpoint you describe.**

**Your few-shot prompt:**

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
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________
________________________________________________

Now document: POST /api/orders - Create a new order
with items array, shipping address, and payment method.
Returns the created order with ID.
```

---

### GRADING CRITERIA

| Criteria | Points |
|----------|--------|
| Complete first example with all sections | /25 |
| Second example showing variation | /25 |
| Covered edge cases (optional params, errors) | /25 |
| Clear instruction for new endpoint | /25 |

**Minimum to pass: 70/100**

---

## ✅ COURSE COMPLETE!

### What You Learned:
- ✅ Why examples beat descriptions
- ✅ Few-shot prompt structure
- ✅ Creating diverse, representative examples
- ✅ Teaching code style by example
- ✅ Data transformation with examples
- ✅ Documentation patterns

### Key Insight:
**Show, don't tell.** One good example is worth a thousand words of description.

### Next Course:
**AI Course 9: Role-Based Prompting**
