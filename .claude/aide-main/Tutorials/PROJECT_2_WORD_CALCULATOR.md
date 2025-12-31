# 📝 Project 2: Word Calculator

## 🎁 Reward: Pocket Protector for Dede!
Complete this project and Dede gets a nerdy pocket protector! Pure intellectual style. 🤓

---

## 🌟 What You'll Learn

This project teaches you to build a tool that analyzes text - counting words, characters, and sentences. But more importantly, you'll master:

- ✅ **Separation of Concerns** - keeping code organized in logical pieces
- ✅ **Prompting for Data Processing** - telling AI how to handle input → output
- ✅ **Recognizing Context Loss** - what to do when AI forgets your project
- ✅ **Testing Your Code** - making sure it actually works
- ✅ **Refactoring** - improving code without breaking it
- ✅ **Code Comments** - writing notes that help AI AND you

---

## 📓 NEW SKILL: Code Comments

Comments are notes in your code that explain what's happening. They help:
- **Future you** remember what you were thinking
- **AI** understand your code when it reads it
- **Other developers** understand your project

```javascript
// This is a single-line comment

/* 
   This is a multi-line comment
   spanning multiple lines
*/

/**
 * This is a docstring/JSDoc comment
 * @param {string} text - The text to count words in
 * @returns {number} The number of words
 */
function countWords(text) {
    // ...
}
```

**🗣️ ALWAYS ASK AI FOR COMMENTS:**
> "Add clear comments explaining what this code does"

---

## 🧠 New Concept: Separation of Concerns

Imagine a restaurant:
- **Chef** only cooks
- **Waiter** only serves
- **Cashier** only handles money

Each person has ONE job. If the chef also took orders AND handled money, things would get messy fast.

Code works the same way:
- **HTML** = the menu (structure)
- **CSS** = the decor (appearance)  
- **JavaScript** = the staff (behavior)

And WITHIN JavaScript:
- **One function** counts words
- **Another function** counts characters
- **Another function** updates the display

> 💡 **Why does this matter?** When something breaks, you know WHERE to look. When you want to change something, you only change ONE place.

---

## 🗺️ Phase 1: Planning Your Tool

### 📝 Your Turn: List the Features

Before touching AIDE, write down:

1. **What goes IN?** (Input)
2. **What comes OUT?** (Output)
3. **What calculations happen?** (Processing)

**Example:**
- **Input:** User types or pastes text
- **Output:** Word count, character count, sentence count, reading time
- **Processing:** Split text, count pieces, calculate time

---

## 🗣️ Phase 2: Project Setup

### Step 1: Create Your Files

**🗣️ SAY TO AIDE:**
> "Create a new folder called 'word-calculator' with three files inside: index.html, styles.css, and script.js"

**👀 WATCH FOR:**
- Three separate files, NOT everything in one file
- This is **separation of concerns** in action

> 💡 **Why separate files?** 
> - Easier to find things
> - Multiple people can work on different files
> - Browser can cache CSS/JS separately (faster loading)

---

### Step 2: Git Setup

**🗣️ SAY TO AIDE:**
> "Initialize git in word-calculator and create a .gitignore that ignores .DS_Store and any editor config files"

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Basic Layout

**🗣️ SAY TO AIDE:**
> "In index.html, create an HTML5 document that links to styles.css in the head and script.js at the end of the body. The title should be 'Word Calculator'"

**👀 CHECK THESE:**

✅ CSS link in `<head>`:
```html
<link rel="stylesheet" href="styles.css">
```

✅ Script at end of `<body>`:
```html
<script src="script.js"></script>
```

**🧠 WHY THIS ORDER?**
- CSS in head = page looks good immediately when loading
- Script at end of body = HTML exists before JavaScript tries to find it

---

### Step 2: Input Area

**🗣️ SAY TO AIDE:**
> "Add a main container div with class 'calculator'. Inside it, add a textarea with id 'text-input' and placeholder 'Type or paste your text here...'. Make the textarea large enough for a paragraph."

**👀 CHECK:**
- Textarea has BOTH `id` and `placeholder`
- The id is for JavaScript to find it
- Placeholder is what shows when empty

---

### Step 3: Stats Display

**🗣️ SAY TO AIDE:**
> "Below the textarea, add a div with class 'stats-container'. Inside it, create four stat boxes, each with a number span and a label. The stats are: Words, Characters, Sentences, and Reading Time."

**👀 WATCH FOR:**

✅ Each stat box has:
```html
<div class="stat-box">
    <span class="stat-number" id="word-count">0</span>
    <span class="stat-label">Words</span>
</div>
```

✅ Each number span has a unique ID (we'll need these in JavaScript)

---

### 📝 Your Turn: Name the IDs

What should the ID be for each stat number? Fill in:

- Word count: `id="____________"`
- Character count: `id="____________"`
- Sentence count: `id="____________"`
- Reading time: `id="____________"`

<details>
<summary>Click for answer</summary>

- `id="word-count"`
- `id="character-count"`
- `id="sentence-count"`
- `id="reading-time"`

IDs should be descriptive and use hyphens (kebab-case) in HTML.
</details>

---

## ✏️ FILL IN THE BLANKS: String Methods

### 🧠 CONCEPT: Splitting Strings

JavaScript can break strings into arrays using `.split()`:

```javascript
"hello world".split(" ")  // ["hello", "world"] - split on space
"a,b,c".split(",")        // ["a", "b", "c"] - split on comma
"hello".split("")         // ["h", "e", "l", "l", "o"] - split each character
```

**The pattern:** `string.split(separator)` → returns an ARRAY

### Exercise 1: Count Words

```javascript
function countWords(text) {
    // Step 1: Remove extra spaces from start/end
    const cleaned = text._______();
    
    // Step 2: Handle empty text
    if (cleaned === '') return ____;
    
    // Step 3: Split into words (by spaces)
    const words = cleaned._______(" ");
    
    // Step 4: Return the count
    return words.________;
}

// Test:
countWords("Hello world")  // Should return 2
countWords("   ")           // Should return 0
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function countWords(text) {
    const cleaned = text.trim();        // removes whitespace from ends
    if (cleaned === '') return 0;       // empty = no words
    const words = cleaned.split(" ");   // break into array
    return words.length;                // count the array
}
```

**Why `.trim()` first?**
- `"  hello  ".split(" ")` → `["", "", "hello", "", ""]` (5 items, wrong!)
- `"  hello  ".trim().split(" ")` → `["hello"]` (1 item, correct!)
</details>

---

### 🧠 CONCEPT: Regular Expressions (Regex)

Regex is a pattern-matching language. Think of it as "search on steroids":

```javascript
/hello/        // matches the word "hello"
/[.!?]/        // matches any period, exclamation, or question mark
/\s+/          // matches one or more whitespace characters
```

**Using regex with split:**
```javascript
"hello   world".split(/\s+/)  // ["hello", "world"] - split on ANY whitespace
```

### Exercise 2: Count Sentences

```javascript
function countSentences(text) {
    // Sentences end with . ! or ?
    // Use regex to match these punctuation marks
    
    const sentenceEnders = text.______(/[.!?]/g);  // find all matches
    
    // .match() returns null if nothing found, or an array of matches
    if (sentenceEnders === ______) {
        return 0;
    }
    
    return sentenceEnders.________;
}

// Test:
countSentences("Hello! How are you? I'm good.")  // Should return 3
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function countSentences(text) {
    const sentenceEnders = text.match(/[.!?]/g);
    
    if (sentenceEnders === null) {
        return 0;
    }
    
    return sentenceEnders.length;
}
```

**Regex Breakdown:**
- `[.!?]` = "match any of these characters"
- `g` flag = "global" - find ALL matches, not just the first
- `.match()` returns an array of all matches, or `null` if none
</details>

---

### 🧠 CONCEPT: Updating the DOM

The DOM (Document Object Model) is how JavaScript sees your HTML.

```javascript
// Find an element
const element = document.getElementById('word-count');

// Change its content
element.textContent = "42";      // just text
element.innerHTML = "<b>42</b>"; // can include HTML
```

### Exercise 3: Display Results

```javascript
function updateDisplay(words, characters, sentences) {
    // Get the elements by their IDs
    const wordEl = document.____________('word-count');
    const charEl = document.____________('character-count');
    const sentEl = document.____________('sentence-count');
    
    // Update their text content
    wordEl.___________ = words;
    charEl.___________ = characters;
    sentEl.___________ = sentences;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function updateDisplay(words, characters, sentences) {
    const wordEl = document.getElementById('word-count');
    const charEl = document.getElementById('character-count');
    const sentEl = document.getElementById('sentence-count');
    
    wordEl.textContent = words;
    charEl.textContent = characters;
    sentEl.textContent = sentences;
}
```
</details>

---

### 🧠 CONCEPT: Event Listeners

Event listeners "watch" for user actions and run code when they happen:

```javascript
// When something happens to element, run this function
element.addEventListener('click', function() {
    console.log('You clicked!');
});
```

**Common events:**
| Event | When it fires |
|-------|---------------|
| `click` | User clicks |
| `input` | User types in input/textarea |
| `change` | Input value changes |
| `keydown` | User presses a key |

### Exercise 4: React to Typing

```javascript
// Get the textarea
const textInput = document.getElementById('text-input');

// Listen for typing
textInput._______________('_______', function() {
    // Get the current text
    const text = textInput.________;
    
    // Count everything
    const words = countWords(text);
    const chars = text.________;  // strings have a length!
    const sentences = countSentences(text);
    
    // Update the display
    updateDisplay(words, chars, sentences);
});
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const textInput = document.getElementById('text-input');

textInput.addEventListener('input', function() {
    const text = textInput.value;
    
    const words = countWords(text);
    const chars = text.length;
    const sentences = countSentences(text);
    
    updateDisplay(words, chars, sentences);
});
```

**Key points:**
- Use `'input'` event for textareas - fires on every keystroke
- Use `.value` to get textarea content (not `.textContent`)
- Strings have `.length` property (no parentheses - it's not a function)
</details>

---

### First Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add HTML structure with input and stats display'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Base Styles

**🗣️ SAY TO AIDE:**
> "In styles.css, add base styles: reset margin and padding on body, use a clean sans-serif font, and set a light gray background. Center the calculator container on the page with a max-width of 800 pixels."

**👀 CHECK:**

✅ Body reset: `margin: 0; padding: 0;`
✅ Font family is set (not using default Times New Roman)
✅ `.calculator` has `max-width` and `margin: auto` for centering

---

### Step 2: Textarea Styling

**🗣️ SAY TO AIDE:**
> "Style the textarea to be full width, at least 200 pixels tall, with padding, a subtle border, and rounded corners. When focused, give it a blue border to show it's active."

**👀 CHECK:**

✅ Uses `width: 100%` or similar
✅ Has both regular and `:focus` styles
✅ `resize` property controls if user can drag to resize

---

### Step 3: Stats Grid

**🗣️ SAY TO AIDE:**
> "Style the stats-container as a grid with 4 equal columns. On mobile screens under 600px, make it 2 columns instead. Style each stat-box with a white background, padding, centered text, and a subtle shadow."

**👀 CHECK FOR:**

✅ Grid layout: `display: grid; grid-template-columns: repeat(4, 1fr);`
✅ Media query for mobile: `@media (max-width: 600px) { ... }`

**🧠 UNDERSTAND THIS:**
```css
@media (max-width: 600px) {    ← "When screen is 600px or less..."
    .stats-container {
        grid-template-columns: repeat(2, 1fr);  ← "...use 2 columns"
    }
}
```

---

### Step 4: Number Styling

**🗣️ SAY TO AIDE:**
> "Make the stat numbers large and bold, maybe 2 or 3rem. Make the labels smaller and gray."

**Commit your CSS:**
> "Commit with message 'Add responsive styling for calculator'"

---

## 🗣️ Phase 5: JavaScript Logic

This is where **separation of concerns** really matters.

### Step 1: Get Element References

**🗣️ SAY TO AIDE:**
> "In script.js, get references to the textarea and all four stat display elements. Store them in clearly named constants."

**👀 CHECK:**

✅ Uses `const` not `let` (these won't change)
✅ Names match what they reference:
```javascript
const textInput = document.getElementById('text-input');
const wordCountDisplay = document.getElementById('word-count');
```

---

### ⚠️ Common AI Problem: Context Loss

Here's where AI sometimes forgets what project you're working on, especially if you've been chatting a while.

**🚨 SIGNS OF CONTEXT LOSS:**
- AI refers to files that don't exist
- Uses variable names you never created
- Suggests features you never mentioned

**🗣️ IF THIS HAPPENS, SAY:**
> "We're working on the word-calculator project. The files are index.html, styles.css, and script.js. The JavaScript needs to count words, characters, and sentences from a textarea."

Re-establishing context fixes most problems.

---

### Step 2: Create Counting Functions (Separation of Concerns!)

**🗣️ SAY TO AIDE:**
> "Create a function called countWords that takes a string and returns the number of words. It should handle empty strings and multiple spaces correctly. Add a JSDoc comment explaining the function."

**👀 CHECK THE LOGIC:**

✅ Handles empty string (should return 0, not 1)
✅ Trims whitespace first
✅ Splits on spaces or word boundaries
✅ **Has a comment explaining what it does**

**🧠 UNDERSTAND THIS:**
```javascript
/**
 * Counts the number of words in a text string
 * @param {string} text - The input text to analyze
 * @returns {number} The word count (0 for empty strings)
 */
function countWords(text) {
    // Handle empty or whitespace-only strings
    if (text.trim() === '') return 0;
    
    // Split on any whitespace and count resulting array
    return text.trim().split(/\s+/).length;
}
```

**🧠 WHY COMMENTS MATTER FOR AI:**
When AI reads your code later, comments help it understand:
- What the function is supposed to do
- What the inputs/outputs are
- Edge cases you've already thought about

Without comments, AI might "improve" your code by breaking the edge case handling!

---

**🗣️ SAY TO AIDE:**
> "Create a function called countCharacters that takes a string and returns the number of characters. Make two versions: with spaces and without spaces."

**👀 CHECK:**
```javascript
function countCharacters(text, includeSpaces = true) {
    if (includeSpaces) return text.length;
    return text.replace(/\s/g, '').length;
}
```

---

**🗣️ SAY TO AIDE:**
> "Create a function called countSentences that takes a string and returns the number of sentences. Count periods, exclamation points, and question marks as sentence endings."

---

**🗣️ SAY TO AIDE:**
> "Create a function called calculateReadingTime that takes a word count and returns reading time in minutes. Assume average reading speed of 200 words per minute. Return a nice string like '2 min' or 'Less than 1 min'."

---

### 📝 Your Turn: Why Separate Functions?

Answer this question before moving on:

**Why did we create FOUR separate functions instead of one big function that does everything?**

<details>
<summary>Click for answer</summary>

1. **Easier to test** - you can test word counting separately from sentence counting
2. **Easier to fix** - if word count is wrong, you know exactly which function to check
3. **Reusable** - you could use `countWords` in a different project
4. **Readable** - each function name tells you what it does
5. **Changeable** - if you want to change how reading time is calculated, you only change one function

This is **separation of concerns** in action!
</details>

---

### Step 3: The Main Update Function

**🗣️ SAY TO AIDE:**
> "Create a main function called updateStats that gets the text from the textarea, calls each counting function, and updates all the display elements with the results."

**👀 CHECK:**

✅ Gets text value: `textInput.value`
✅ Calls each counting function
✅ Updates each display's `textContent`

**🧠 THIS IS THE COORDINATOR:**
```javascript
function updateStats() {
    const text = textInput.value;  // Get input
    
    // Call the specialists
    const words = countWords(text);
    const characters = countCharacters(text);
    const sentences = countSentences(text);
    const readingTime = calculateReadingTime(words);
    
    // Update display
    wordCountDisplay.textContent = words;
    characterCountDisplay.textContent = characters;
    sentenceCountDisplay.textContent = sentences;
    readingTimeDisplay.textContent = readingTime;
}
```

Each function has ONE job. `updateStats` coordinates them.

---

### Step 4: Wire It Up

**🗣️ SAY TO AIDE:**
> "Add an event listener to the textarea that calls updateStats every time the user types. Also call updateStats once when the page loads in case there's already text."

**👀 CHECK:**

✅ Uses `'input'` event (fires on every keystroke)
✅ Initial call on load: `updateStats();` or `window.onload`

---

### Commit Your Logic

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add counting functions and live update'"

---

## 🧪 Phase 6: Testing

### Test Cases

Open your calculator and test these:

| Test | Input | Expected |
|------|-------|----------|
| Empty | (nothing) | All zeros |
| Single word | `Hello` | 1 word, 5 chars |
| Multiple spaces | `Hello    World` | 2 words (not more) |
| Sentences | `Hi. How are you?` | 2 sentences |
| Special chars | `Hello, world!` | 2 words, 13 chars |

### 📝 Your Turn: Test and Fix

Actually test each case. If something's wrong:

**DON'T SAY:** "The word count is broken"

**DO SAY:** "When I type 'Hello    World' with 4 spaces between, it should show 2 words but it shows 5. Check the countWords function's split logic."

---

## ⚠️ Common AI Problem: The Fix Loop

When debugging, AI might enter a loop:

```
AI: "I'll fix the word count by changing the regex"
*changes code*
You: "Now it shows 0 words for everything"
AI: "Let me fix that"
*changes code*
You: "Now it's counting letters as words"
AI: "Let me fix that"
...
```

**🚨 BREAK THE LOOP:**

**🗣️ SAY:**
> "Stop. Let's debug properly. Print the value of text.split(/\s+/) to the console so I can see what's actually happening."

**Then look at the console output and tell AI SPECIFICALLY what's wrong.**

Adding `console.log()` statements helps you SEE what the code is doing.

---

## 🗣️ Phase 7: Polish & Features

### Add Clear Button

**🗣️ SAY TO AIDE:**
> "Add a clear button below the textarea that empties the text and resets all stats"

---

### Add Character Toggle

**🗣️ SAY TO AIDE:**
> "Add a small toggle next to the character count that switches between 'with spaces' and 'without spaces'. Update the count when toggled."

**👀 CHECK:**
- Toggle has some way to track state (checkbox, data attribute, variable)
- Character count function is called with the right parameter

---

### Add Copy Stats

**🗣️ SAY TO AIDE:**
> "Add a button that copies all stats to clipboard in a readable format like 'Words: 150, Characters: 720, Sentences: 8, Reading Time: 1 min'"

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add clear button, character toggle, and copy feature'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Re-establish context when AI gets confused
- ✅ Use console.log to debug instead of letting AI guess
- ✅ Break fix loops by asking to SEE the data

### Code Concepts:
- ✅ **Separation of Concerns** - each function has ONE job
- ✅ Functions can call other functions
- ✅ Input → Process → Output pattern
- ✅ Event listeners for real-time updates

### Project Skills:
- ✅ Separate files for HTML/CSS/JS
- ✅ Test with specific inputs
- ✅ Mobile-responsive design

---

## 🎁 Unlock Your Reward!

Dede now has a pocket protector! Pure nerd energy! 🤓

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

Ask AIDE to add:

1. **Paragraph count** - "Count paragraphs by splitting on double newlines"
2. **Most common words** - "Show the top 5 most used words"
3. **Writing grade level** - "Calculate Flesch-Kincaid readability score"
4. **Word frequency chart** - "Add a simple bar chart showing word frequency"

---

## 📖 Quick Reference

### Separation of Concerns

| Layer | Job | Example |
|-------|-----|---------|
| HTML | Structure | `<textarea>` exists |
| CSS | Appearance | textarea is styled |
| JS | Behavior | typing updates stats |
| Function A | Count words | just counting |
| Function B | Update display | just displaying |

### Testing Tips

| ✅ Good Test | ❌ Bad Test |
|-------------|------------|
| Specific input: "Hello World" | Random typing |
| Check exact expected output | "Looks about right" |
| Test edge cases (empty, spaces) | Only test normal cases |
| Test one thing at a time | Change multiple things then test |

### Debugging with AI

| ✅ Do This | ❌ Not This |
|-----------|------------|
| "Add console.log before the split" | "Why is it broken?" |
| "The output is X but should be Y" | "It's not working" |
| "Let's trace through step by step" | Let AI guess repeatedly |

---

*Next up: Project 3 - Portfolio Website! 🎨 + 💼*
