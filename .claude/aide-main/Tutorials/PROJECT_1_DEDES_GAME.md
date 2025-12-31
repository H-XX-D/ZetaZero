# 🎮 Project 1: Dede's Floor is Lava!

## 🎁 Reward: Hula Skirt for Dede!
Complete this project and Dede gets a tropical hula skirt to show off those dance moves!

---

## 🌟 What You'll Learn

This isn't just about making a game. You're learning **how to work WITH an AI** to build things. By the end, you'll know:

- ✅ How to **talk to AIDE** - giving clear instructions that get results
- ✅ How to **read code** the AI generates - understanding what you're looking at
- ✅ How to **spot when AI goes wrong** - loops, confusion, tangents
- ✅ How to **plan a project** - thinking BEFORE coding
- ✅ How to **use Git** - saving your work like a pro
- ✅ How to **keep notes** - your project journal is your lifeline

---

## 📓 FIRST: Start Your Project Journal

Before ANY code, create a notes file. This is your **lifeline** when AI loses context.

**🗣️ SAY TO AIDE:**
> "Create a file called NOTES.md in my project folder"

Then write this yourself (not AI):

```markdown
# Floor is Lava - Project Notes

## What I'm Building
A game where platforms turn to lava and you jump to survive.

## Current Task
Setting up the project structure

## What's Working
- (nothing yet)

## What's Broken
- (nothing yet)

## My Decisions
- Player is 40x40 pixels
- 5 platforms on screen
- Score +1 per second survived

## AI Problems I Noticed
- (log them here as they happen)
```

**🧠 WHY THIS MATTERS:**
- AI forgets. You have notes.
- When AI loses context, paste your notes
- Track what YOU decided vs what AI decided
- Catch patterns in AI mistakes

---

## 📚 Before We Start: Understanding the Basics

### What is HTML, CSS, and JavaScript?

Think of building a website like building a person:

| Part | What It Does | Example |
|------|--------------|---------|
| **HTML** | The skeleton - structure | "There's a button here, text there" |
| **CSS** | The clothes - appearance | "The button is red, text is big" |
| **JavaScript** | The brain - behavior | "When you click the button, do this" |

### Reading Code: The Color System 🎨

When AIDE writes code, your editor will color it. Here's what to watch for:

| Color | What It Usually Means | Example |
|-------|----------------------|---------|
| 🔵 **Blue/Purple** | Keywords (special words) | `function`, `if`, `return` |
| 🟢 **Green** | Comments (notes to humans) | `// this is a comment` |
| 🟠 **Orange/Yellow** | Strings (text in quotes) | `"Hello World"` |
| ⚪ **White/Light** | Your variable names | `playerScore`, `gameOver` |
| 🔴 **Red** | Usually an ERROR | Missing bracket, typo |

### The Bracket Rule 🔒

Every opening bracket needs a closing bracket. They're like parentheses in math.

```
{  ← opens
   stuff inside
}  ← closes

(  ← opens
)  ← closes
```

**When AI writes code, COUNT THE BRACKETS.** If they don't match, something's wrong.

---

## 🗺️ Phase 1: Planning (You Don't Skip This!)

Before you tell AIDE anything, you need to THINK. Bad planning = messy code = frustrated you.

### 📝 Your Turn: Write Down Your Plan

Grab paper or open a note. Answer these:

1. **What is the game?** (One sentence)
2. **What does the player do?** (List actions)
3. **How do they win/lose?** (Rules)
4. **What do they see on screen?** (Visual elements)

**Example answers:**
1. A game where platforms turn to lava and you jump to survive
2. Click/tap platforms to jump, avoid lava
3. Win: survive longest. Lose: touch lava
4. Platforms, a character, lava, score counter

> 💡 **Why plan?** If YOU don't know what you want, the AI will guess. AI guessing = weird results. Be the director, not the audience.

---

## 🗣️ Phase 2: Setting Up (Your First AI Conversation)

### Step 1: Create Your Project Folder

**🗣️ SAY TO AIDE:**
> "Create a new folder called 'floor-is-lava-game' and inside it create an empty file called 'index.html'"

**👀 WATCH FOR:**
- AIDE should create a folder structure
- You should see `floor-is-lava-game/index.html` in your file explorer
- If AIDE asks clarifying questions, that's GOOD - answer them

**⚠️ IF AIDE DOES SOMETHING WEIRD:**
- Creates multiple folders? Say: "No, just one folder called floor-is-lava-game"
- Adds extra files you didn't ask for? Say: "Remove everything except index.html"

> 💡 **AI Lesson:** AIs try to be helpful by adding extra stuff. Sometimes too helpful. Be specific about what you want AND what you don't want.

---

### Step 2: Initialize Git (Saving Your Work)

**🗣️ SAY TO AIDE:**
> "Initialize a git repository in my floor-is-lava-game folder"

**👀 WHAT HAPPENED:**
Git is like a save system for your code. Every time you "commit," you create a save point you can return to.

**🗣️ THEN SAY:**
> "Create a .gitignore file that ignores .DS_Store files"

> 💡 **Why .gitignore?** Some files are junk (like .DS_Store on Mac). We tell Git to ignore them so our project stays clean.

---

## 🗣️ Phase 3: The HTML Skeleton

### Step 1: Basic Structure

**🗣️ SAY TO AIDE:**
> "In index.html, create a basic HTML5 structure with a title 'Floor is Lava' and a div with id 'game-container'"

**👀 WHEN AIDE WRITES THE CODE, CHECK:**

✅ **DOCTYPE** at the top - tells browser "this is HTML"
✅ **Opening and closing tags match:**
   - `<html>` has `</html>`
   - `<head>` has `</head>`  
   - `<body>` has `</body>`
   - `<div>` has `</div>`

✅ **The id is in quotes:** `id="game-container"` not `id=game-container`

**🧠 UNDERSTAND THIS:**
```
<div id="game-container">     ← This creates a box
</div>                        ← This closes the box

The "id" is like a name tag. Later, CSS and JavaScript will 
find this box by calling its name.
```

---

### Step 2: Add Game Elements

**🗣️ SAY TO AIDE:**
> "Inside the game-container div, add:
> - a div with class 'player dede' for our Dee Dee character - inside it add divs for dede's body parts: 'dede-body', 'dede-face', 'dede-eye-left', 'dede-eye-right', 'dede-smile', and 'dede-antenna'
> - a div with class 'platforms' that will hold our platform pieces  
> - a div with class 'score-display' showing 'Score: 0'
> - a div with class 'game-over-screen' that's hidden by default, with text 'Game Over' and a restart button"

**🎨 WE'RE BUILDING DEE DEE!**
Instead of a boring square, we're recreating the AIDE mascot using just HTML divs and CSS. This is how real game developers create characters - shapes, colors, and positioning!

**👀 CHECK THE CODE:**

✅ Are all the divs INSIDE game-container? (Between its opening and closing tags)
✅ Does each div have `class=` in quotes?
✅ Is the structure nested correctly?

**Correct nesting looks like:**
```
<div id="game-container">
    <div class="player">...</div>
    <div class="platforms">...</div>
    <div class="score-display">...</div>
    <div class="game-over-screen">...</div>
</div>
```

**Wrong nesting (WATCH FOR THIS):**
```
<div id="game-container">
    <div class="player">...</div>
</div>
<div class="platforms">...</div>   ← OUTSIDE! This is wrong!
```

> 💡 **AI Lesson:** Sometimes AI closes tags too early. Always check that child elements are INSIDE their parent.

---

### Step 3: First Git Commit

**🗣️ SAY TO AIDE:**
> "Commit my changes with the message 'Add HTML structure for game'"

**🧠 WHY COMMIT NOW?**
You have working HTML structure. If you mess up later, you can come back here. Commit after each working piece, not at the end.

**📓 UPDATE YOUR NOTES:**
```markdown
## What's Working
- HTML structure complete
- Game container with player, platforms, score, game-over divs
```

---

## 🚨 AI Mistake Pattern #1: Context Loss

Sometimes AI forgets what project you're working on. Signs:

- Suggests code for a different project
- Asks "what file are we working on?"
- References functions that don't exist in YOUR code

**🛠️ HOW TO FIX:**

**🗣️ SAY:**
> "Let me re-establish context. I'm building a 'Floor is Lava' game. Here's my current NOTES.md: [paste notes]. Here's my current index.html structure: [paste or describe]. Now, let's continue with [next task]."

Your notes file becomes a **context feeder** - giving AI the info it forgot.

---

## 🗣️ Phase 4: CSS Styling (Making It Pretty)

### Step 1: Add Style Section

**🗣️ SAY TO AIDE:**
> "Add a style section in the head of my HTML. Set the body to have no margin, hide overflow, and use a dark background color. Make game-container take up the full screen."

**👀 CHECK THE CODE:**

✅ Style section is in `<head>`, not `<body>`
✅ CSS rules have curly braces that open AND close: `body { ... }`
✅ Each property ends with semicolon: `margin: 0;`

**🧠 CSS STRUCTURE:**
```css
selector {
    property: value;
    property: value;
}

body {           ← "body" is the selector (what to style)
    margin: 0;   ← property: value; (what to change)
}
```

---

### Step 2: Style Dee Dee (The Fun Part!) 🎨

We're going to build Dee Dee piece by piece using CSS. This teaches you how game characters are made from simple shapes!

**🗣️ SAY TO AIDE:**
> "Style the player div to be 50x60 pixels, positioned absolutely, starting near the bottom center of the screen. Then style Dee Dee's parts:
> - dede-body: A rounded green rectangle (40x50px, background #22c55e, border-radius 50%)
> - dede-face: Centered in the body, slightly lighter green
> - dede-eye-left and dede-eye-right: Small white circles (8x8px) with black dot pupils, positioned in upper third of face
> - dede-smile: A curved line at the bottom of the face using border-bottom
> - dede-antenna: A small stem on top with a glowing tip, using a pseudo-element for the bulb"

**👀 WATCH FOR:**

✅ `position: absolute;` on player - lets us move Dee Dee anywhere
✅ `position: relative;` on dede-body - so child parts position correctly
✅ Numbers have units: `40px` not just `40`
✅ Green color matches AIDE theme: `#22c55e`
✅ Eyes use `border-radius: 50%` to become circles

**🧠 HOW WE'RE BUILDING A CHARACTER:**
```css
/* This is how game devs build characters from shapes! */
.dede-body {
    width: 40px;
    height: 50px;
    background: #22c55e;      /* AIDE green! */
    border-radius: 50%;       /* Makes it rounded/oval */
    position: relative;       /* So eyes/smile position inside */
}

.dede-eye-left, .dede-eye-right {
    width: 8px;
    height: 8px;
    background: white;
    border-radius: 50%;       /* Circle! */
    position: absolute;       /* Place precisely */
}
```

**🎁 WHAT YOU'RE LEARNING:**
You just built a character from scratch using only CSS! This is the same technique used in CSS art, game sprites, and animations. Dee Dee will bounce and move just like any game character!

---

### Step 3: Style the Platforms

**🗣️ SAY TO AIDE:**
> "Style the platform pieces to be rectangles, 100 pixels wide, 20 pixels tall, with a green color. Position them absolutely so we can place them anywhere."

---

### Step 4: Style the Lava Effect

**🗣️ SAY TO AIDE:**
> "When a platform has the class 'lava', change its color to orange-red and add a subtle glow animation"

**🧠 UNDERSTAND THIS:**
```css
.platform.lava {     ← "." means class, this targets platforms WITH lava class
    background: orangered;
}
```

The dot (.) targets classes. When you see `.player` it means "anything with class='player'"

---

### Step 5: Hide the Game Over Screen

**🗣️ SAY TO AIDE:**
> "Make the game-over-screen hidden by default using display none. Style it to be centered on screen with a semi-transparent dark background when visible."

**Commit your CSS work:**
> "Commit with message 'Add CSS styling for game elements'"

---

## 🗣️ Phase 5: JavaScript (The Brain)

Here's where the magic happens. JavaScript makes things MOVE and REACT.

### Step 1: Add Script Section

**🗣️ SAY TO AIDE:**
> "Add a script section at the bottom of the body. First, get references to our game elements using document.getElementById and document.querySelector"

**👀 WHEN AI WRITES THIS, CHECK:**

✅ Script tag is right before `</body>`, not in head
✅ Variable names match your element IDs/classes
✅ Uses `const` or `let`, not `var` (modern JavaScript)

**🧠 UNDERSTAND THIS:**
```javascript
const player = document.querySelector('.player');
      ↑                            ↑
   variable name              CSS selector (. = class)
   
// This says: "Find the thing with class 'player' and let me call it 'player'"
```

---

## ✏️ FILL IN THE BLANKS: Your First JavaScript

### 🧠 CONCEPT: Finding Elements in the Page

JavaScript can find and control HTML elements using selectors:

```javascript
// Find by ID (use #)
document.querySelector('#game-container')  // finds id="game-container"

// Find by class (use .)
document.querySelector('.player')          // finds class="player"

// Find by tag name (no symbol)
document.querySelector('button')           // finds first <button>
```

### Exercise 1: Get Your Game Elements

```javascript
// Find the game container by its ID
const gameContainer = document._____________('#game-container');

// Find the player by its class
const player = document._____________('._______');

// Find the score display by its class  
const scoreDisplay = document.querySelector('.______-_______');

// Find ALL platforms (returns a list, not just one)
const platforms = document.______________All('.platform');
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const gameContainer = document.querySelector('#game-container');
const player = document.querySelector('.player');
const scoreDisplay = document.querySelector('.score-display');
const platforms = document.querySelectorAll('.platform');
```

**querySelector vs querySelectorAll:**
- `querySelector` → returns ONE element (the first match)
- `querySelectorAll` → returns ALL matching elements (a NodeList)
</details>

---

### 🧠 CONCEPT: Variables Hold Data

Variables are like labeled boxes that hold values:

```javascript
let score = 0;           // A number
let playerName = "Dede"; // A string (text)
let isGameOver = false;  // A boolean (true/false)
let platforms = [];      // An empty array (list)
```

**let vs const:**
- `let` = value CAN change later
- `const` = value CANNOT change (use for things that stay the same)

### Exercise 2: Set Up Game State

```javascript
// Score that will increase - use let because it changes
_____ score = 0;

// Is the game currently running? Starts as true
let isRunning = ______;

// Player's position on screen (will change as they move)
_____ playerX = 200;
let playerY = ______;  // Start near bottom

// List to hold all our platform objects (empty to start)
let platforms = ____;

// The element references don't change, use const
_______ player = document.querySelector('.player');
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
let score = 0;
let isRunning = true;
let playerX = 200;
let playerY = 500;  // or any number near bottom
let platforms = [];
const player = document.querySelector('.player');
```
</details>

---

### Step 2: Game State Variables  

**🗣️ SAY TO AIDE:**
> "Create variables to track: the game score starting at 0, whether the game is running, the player's position, and an empty array to hold our platforms"

**👀 CHECK:**

✅ Variables are initialized: `let score = 0;` not just `let score;`
✅ Boolean uses true/false, not strings: `let isRunning = true;` not `"true"`
✅ Array is empty brackets: `let platforms = [];`

---

### ⚠️ Common AI Problem: The Tangent

At this point, AI sometimes goes wild and writes the ENTIRE game at once. 

**🚨 IF AIDE WRITES TONS OF CODE YOU DIDN'T ASK FOR:**

**🗣️ SAY:**
> "Stop. Undo that. I only want the variables I asked for. We'll add features one at a time."

**🧠 WHY THIS HAPPENS:**
AI sees "game" and thinks "I know how to make games!" and dumps everything. But YOU need to understand each piece. Keep the AI focused on small steps.

---

### Step 3: Create a Platform Function

**🗣️ SAY TO AIDE:**
> "Create a function called createPlatform that takes x and y position as parameters. It should create a new div element, add the 'platform' class, set its position using the x and y values, and add it to the platforms container"

**👀 CHECK THE STRUCTURE:**

✅ Function has opening and closing braces: `function createPlatform(x, y) { ... }`
✅ Parameters are in parentheses: `(x, y)`
✅ Uses `document.createElement('div')` to make new elements
✅ Uses `appendChild` or similar to add it to the page

---

## ✏️ FILL IN THE BLANKS: Functions

### 🧠 CONCEPT: What is a Function?

A function is a reusable block of code. Like a recipe you can use over and over:

```javascript
// Define the function (write the recipe)
function sayHello(name) {
    console.log("Hello, " + name + "!");
}

// Call the function (use the recipe)
sayHello("Dede");   // prints: Hello, Dede!
sayHello("Player"); // prints: Hello, Player!
```

**Parts of a function:**
- `function` - keyword that starts it
- `sayHello` - the name (how you call it)
- `(name)` - parameters (inputs it needs)
- `{ ... }` - the code to run

### Exercise 3: Create a Platform

```javascript
function createPlatform(x, y) {
    // Create a new div element
    const platform = document._______________('div');
    
    // Add the 'platform' class to it
    platform.classList._____('platform');
    
    // Set its position using CSS (must add 'px' to numbers!)
    platform.style.left = x + '____';
    platform.style.top = ____ + 'px';
    
    // Add it to the platforms container
    const container = document.querySelector('.platforms');
    container.________________(platform);
    
    // Return it so we can track it
    _________ platform;
}

// Usage:
createPlatform(100, 400);  // Creates platform at x=100, y=400
createPlatform(250, 350);  // Creates another at different spot
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function createPlatform(x, y) {
    const platform = document.createElement('div');
    platform.classList.add('platform');
    platform.style.left = x + 'px';
    platform.style.top = y + 'px';
    const container = document.querySelector('.platforms');
    container.appendChild(platform);
    return platform;
}
```

**Key Concepts:**
- `createElement('div')` - makes a new div (not on page yet!)
- `classList.add()` - adds a CSS class
- `style.left` - sets position (needs 'px' units)
- `appendChild()` - adds element to the page
- `return` - sends the platform back to whoever called the function
</details>

---

### 🧠 CONCEPT: Changing CSS with JavaScript

JavaScript can change any CSS property:

```javascript
element.style.left = "100px";       // position
element.style.backgroundColor = "red";  // color (camelCase!)
element.style.display = "none";     // hide it
element.style.display = "block";    // show it
```

**Notice:** CSS uses hyphens (`background-color`) but JavaScript uses camelCase (`backgroundColor`)

### Exercise 4: Move the Player

```javascript
function movePlayer(newX, newY) {
    // Update our tracking variables
    playerX = ______;
    playerY = newY;
    
    // Update the actual element on screen
    player._______._____= playerX + 'px';
    player.style._____ = _______ + 'px';
}

// Usage:
movePlayer(150, 400);  // Move player to x=150, y=400
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function movePlayer(newX, newY) {
    playerX = newX;
    playerY = newY;
    
    player.style.left = playerX + 'px';
    player.style.top = playerY + 'px';
}
```

**The pattern:**
1. Update your JavaScript variables (so you know where player is)
2. Update the CSS (so user sees the change)
</details>

---

**🧠 UNDERSTAND THIS:**
```javascript
function createPlatform(x, y) {    ← Function name and inputs
    const platform = document.createElement('div');  ← Make a new div
    platform.classList.add('platform');              ← Give it the class
    platform.style.left = x + 'px';                  ← Position it
    platform.style.top = y + 'px';
    container.appendChild(platform);                 ← Add to page
}
```

---

### Step 4: Create Starting Platforms

**🗣️ SAY TO AIDE:**
> "Create a function called initGame that uses a loop to create 5 platforms spaced vertically, starting from the bottom of the screen going up. Call this function when the page loads."

**👀 CHECK:**

✅ Loop syntax is correct: `for (let i = 0; i < 5; i++) { ... }`
✅ The function is actually CALLED somewhere, not just defined
✅ Spacing math makes sense (platforms not overlapping)

---

### Step 5: Player Movement

**🗣️ SAY TO AIDE:**
> "Add keyboard event listeners. When the left arrow is pressed, move the player left. When right arrow is pressed, move right. Make sure the player can't go off screen."

**👀 CHECK:**

✅ Event listener attached: `document.addEventListener('keydown', ...)`
✅ Key codes checked: `event.key === 'ArrowLeft'` or `event.keyCode === 37`
✅ Boundary check: player position shouldn't go below 0 or above screen width

**🧠 UNDERSTAND THIS:**
```javascript
document.addEventListener('keydown', function(event) {
    if (event.key === 'ArrowLeft') {
        playerX -= 10;  // Move left by 10 pixels
    }
});
```
- `addEventListener` = "when this happens..."  
- `'keydown'` = "...a key is pressed..."
- `function(event)` = "...do this (event has details about which key)"

---

### 📝 Your Turn: Write the Boundary Check

AIDE wrote the movement, but YOU write the boundary check.

**Fill in the blanks:**
```javascript
if (playerX < ___) {
    playerX = ___;
}
if (playerX > window.innerWidth - ___) {
    playerX = window.innerWidth - ___;
}
```

<details>
<summary>Click for answer</summary>

```javascript
if (playerX < 0) {
    playerX = 0;
}
if (playerX > window.innerWidth - 40) {
    playerX = window.innerWidth - 40;
}
```
(40 is the player width - we don't want them going off the right edge)
</details>

---

### Step 6: Platforms Turn to Lava

**🗣️ SAY TO AIDE:**
> "Create a function called turnToLava that picks a random platform and adds the 'lava' class to it. After 3 seconds, remove the lava class. Run this function every 2 seconds during the game."

**👀 CHECK:**

✅ Uses `Math.random()` to pick random platform
✅ Uses `classList.add('lava')` and `classList.remove('lava')`
✅ Uses `setInterval` for repeating, `setTimeout` for delays
✅ Intervals are stored in variables so we can stop them later

---

### Step 7: Collision Detection

**🗣️ SAY TO AIDE:**
> "Create a function that checks if the player is touching a lava platform. If they are, end the game by showing the game over screen and stopping all intervals."

**🧠 UNDERSTAND COLLISION:**
Two rectangles overlap if their edges cross. The math checks if:
- Player's right edge is past platform's left edge AND
- Player's left edge is before platform's right edge AND
- Same for top/bottom

---

### ⚠️ Common AI Problem: The Loop

Sometimes AI gets stuck in a loop of "fixing" the same thing:

```
AI: "I'll fix the collision detection"
*changes code*
AI: "There's an issue with collision detection, let me fix it"
*changes same code differently*
AI: "The collision still has a problem..."
```

**🚨 IF THIS HAPPENS:**

**🗣️ SAY:**
> "Stop. Show me the current code without changing it. Let me look at it."

Then describe the SPECIFIC problem you see. Don't let AI guess.

---

### Step 8: Score System

**🗣️ SAY TO AIDE:**
> "Create a score system that increases the score by 1 every second the player survives. Update the score display to show the current score."

---

### Step 9: Restart Function

**🗣️ SAY TO AIDE:**
> "Create a restartGame function that resets the score to 0, removes all platforms, calls initGame to create new ones, hides the game over screen, and resumes the game."

**Connect the restart button:**
> "Make the restart button in the game over screen call the restartGame function when clicked."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit all changes with message 'Complete game logic - player movement, lava, collision, scoring'"

---

## 🧪 Phase 6: Testing & Debugging

### Test Your Game

**🗣️ SAY TO AIDE:**
> "Open index.html in a browser" or "Start a live server for this project"

**🔍 TEST THESE:**
1. Can you move left and right?
2. Do platforms turn to lava?
3. Do you die when touching lava?
4. Does the score count up?
5. Does restart work?

### If Something's Broken

**DON'T SAY:** "It's broken, fix it" (too vague)

**DO SAY:** "When I press the left arrow, the player doesn't move. Check the keydown event listener."

> 💡 **AI Lesson:** Specific bug reports get specific fixes. Vague complaints get random changes that might break more things.

---

## ✅ Phase 7: Final Polish

**🗣️ SAY TO AIDE:**
> "Add these finishing touches:
> - Make the lava platforms pulse/glow
> - Add a starting message that says 'Press any key to start'
> - Make the game get harder over time by increasing lava frequency"

**Final commit:**
> "Commit with message 'Add polish - animations, start screen, difficulty scaling'"

---

## 🎓 What You Learned

### AI Communication Skills:
- ✅ Give specific, step-by-step instructions
- ✅ Check the code AI generates (brackets, structure, logic)
- ✅ Stop AI when it goes off track
- ✅ Describe bugs specifically

### Code Understanding:
- ✅ HTML = structure (what's there)
- ✅ CSS = style (how it looks)
- ✅ JavaScript = behavior (what it does)
- ✅ Brackets must match
- ✅ Semicolons end statements
- ✅ Colors tell you what kind of code it is

### Project Skills:
- ✅ Plan before coding
- ✅ Build piece by piece
- ✅ Commit after each working piece
- ✅ Test as you go

---

## 🎁 Unlock Your Reward!

You did it! Dede is now rocking a hula skirt! 🌺

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

Want more? Try asking AIDE to add:

1. **Sound effects** - "Add a sizzle sound when platforms turn to lava"
2. **High score** - "Save the highest score and display it"
3. **Mobile controls** - "Add touch buttons for mobile players"
4. **Levels** - "After 30 seconds, increase difficulty and show 'Level 2'"

Remember: Tell AIDE what you want, check what it writes, guide it if it goes wrong!

---

## 📖 Quick Reference

### Talking to AIDE

| ✅ Good Prompt | ❌ Bad Prompt |
|----------------|---------------|
| "Create a function called X that does Y" | "Make it work" |
| "The player doesn't move when I press left arrow" | "It's broken" |
| "Add a red border to the player div" | "Make it look better" |
| "Stop, undo that" | *letting AI keep going wild* |

### Reading Code Colors

| Color | Meaning |
|-------|---------|
| Blue/Purple | Keywords (function, if, return) |
| Green | Comments (// notes) |
| Orange | Strings ("text") |
| White | Your variable names |
| Red | Errors - FIX THIS |

### Common AI Problems

| Problem | Sign | Solution |
|---------|------|----------|
| **Loop** | Same "fix" over and over | "Stop. Show me the code. Don't change it." |
| **Tangent** | Writes way more than you asked | "Undo. Only do what I asked." |
| **Context Loss** | Forgets what project you're on | "We're working on the floor-is-lava game. Here's what we have so far..." |
| **Hallucination** | Refers to code that doesn't exist | "That function doesn't exist. Look at the actual code." |

---

*Next up: Project 2 - Word Calculator! 📝 + 🔢*
