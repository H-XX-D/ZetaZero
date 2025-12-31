# 🎮 Project 12: Platformer Game - The Graduation Project

## 🎁 Reward: Graduation Cap + MASTER CODER Badge!
Complete this final project and Dede graduates with honors! 🎓 You'll also unlock the ultimate "Master Coder" badge showing you've completed the entire curriculum!

---

## 🌟 What You'll Learn

Build a full platformer game with physics, levels, and enemies. This is your FINAL PROJECT - combining everything you've learned:

- ✅ **Game loop architecture** - the heartbeat of games
- ✅ **Physics simulation** - gravity, collision, momentum
- ✅ **State machines** - player states, game states
- ✅ **Level design** - data-driven level creation
- ✅ **Polish & juice** - what makes games FEEL good
- ✅ **Project architecture** - organizing complex code
- ✅ **GitHub & CI/CD** - professional project management

---

## 🎓 GRADUATION SKILL: Full Project Management

This is your capstone. You'll use ALL the AI management skills from previous projects:

### Your Project Management Files

Create these files FIRST, before any code:

**🗣️ SAY TO AIDE:**
> "Create these files in my platformer-game project: README.md, CONTEXT.md, TASKS.md, SCOPE.md"

**README.md** - Project overview (from Project 3)
```markdown
# Platformer Game
2D platformer with physics, enemies, and multiple levels.

## Tech Stack
HTML5 Canvas, Vanilla JavaScript

## How to Play
Open index.html, use arrow keys to move, space to jump

## Project Status
🟡 In Development
```

**CONTEXT.md** - AI context feeder (from Project 4)
```markdown
# Platformer - AI Context

## Current State Structure
- gameState: menu/playing/paused/gameover
- player: { x, y, vx, vy, state, isOnGround }
- level: current level data
- enemies: array of enemy objects

## File Purposes
- js/game.js - Main loop, state management
- js/player.js - Player movement, physics
- js/level.js - Level loading, rendering
- js/input.js - Keyboard handling
- js/physics.js - Collision detection

## Key Decisions
- 60 FPS game loop using requestAnimationFrame
- Collision detection happens BEFORE position update
- Player states: idle, running, jumping, falling
```

**TASKS.md** - Current work tracker (from Project 7)
```markdown
# Tasks

## 🎯 Current Focus
Phase: Player Movement

## ✅ Completed
- [x] Project setup
- [x] Canvas and HTML structure
- [x] Input system

## 🔄 In Progress
- [ ] Player physics
- [ ] Platform collision

## 📋 Backlog
- [ ] Enemies
- [ ] Multiple levels
- [ ] Polish effects
```

**SCOPE.md** - Feature boundaries (from Project 8)
```markdown
# Scope

## ✅ IN SCOPE
- Player movement (run, jump)
- Platform collision
- Basic enemies (patrol, stomp to kill)
- 2-3 levels
- Win/lose conditions
- Basic polish (sounds, particles)

## ❌ OUT OF SCOPE
- Online multiplayer
- Level editor
- Save/load progress
- Mobile controls
- Advanced AI enemies
```

---

## 📓 NEW SKILL: GitHub & CI/CD Basics

You're ready for professional tools!

### Pushing to GitHub

**🗣️ SAY TO AIDE:**
> "Help me create a GitHub repository for this project and push my code"

**Basic Git → GitHub workflow:**
```bash
# One-time setup
git remote add origin https://github.com/yourusername/platformer-game.git

# Push your code
git push -u origin main
```

**🧠 WHY GITHUB:**
- Backup your code online
- Share with others
- Track history across devices
- Looks professional on your portfolio

### GitHub Pages (Free Hosting!)

**🗣️ SAY TO AIDE:**
> "How do I deploy my game to GitHub Pages so anyone can play it?"

Your game gets a free URL like: `yourusername.github.io/platformer-game`

### CI/CD Concept (Continuous Integration / Continuous Deployment)

CI/CD = automatic checks and deployment when you push code.

**Simple GitHub Action example:**

**🗣️ SAY TO AIDE:**
> "Create a .github/workflows/deploy.yml file that automatically deploys to GitHub Pages when I push to main"

```yaml
# .github/workflows/deploy.yml
name: Deploy to GitHub Pages

on:
  push:
    branches: [ main ]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./
```

**🧠 WHAT THIS DOES:**
1. You push code to GitHub
2. GitHub automatically runs this workflow
3. Your game is deployed to GitHub Pages
4. No manual deployment needed!

---

## 📋 PROJECT BRIEF: Dede's Adventure

Before diving into code, here's everything you need to know about what you're building.

### 🎯 The Concept

**Game Title:** Dede's Adventure (or your own name!)

**Genre:** 2D Side-Scrolling Platformer

**Premise:** Help Dede (or your hero) navigate through dangerous platforms, avoid enemies, and reach the goal in each level.

**Core Mechanics:**
| Mechanic | Description |
|----------|-------------|
| **Movement** | Left/right arrow keys to run |
| **Jumping** | Spacebar to jump, hold for higher jump |
| **Gravity** | Constant downward pull when airborne |
| **Collision** | Land on platforms, blocked by walls |
| **Enemies** | Stomp from above to defeat, die if hit from side |
| **Goal** | Reach the flag/door to complete level |

**Win Condition:** Complete all levels
**Lose Condition:** Fall off screen or get hit by enemy

---

### 🎨 ASSETS NEEDED

You'll need these visual and audio assets. You can use placeholders first, then upgrade later.

**Visual Assets:**

| Asset | Size | Description | Placeholder Option |
|-------|------|-------------|-------------------|
| Player sprite | 32x32 or 64x64 | Your character | Colored rectangle |
| Platform tile | 32x32 | Ground/floors | Brown rectangle |
| Enemy sprite | 32x32 | Patrolling baddie | Red rectangle |
| Background | 800x600 | Sky/world backdrop | CSS gradient |
| Goal flag/door | 32x64 | Level end marker | Green rectangle |
| Collectible | 16x16 | Optional coins/gems | Yellow circle |

**Audio Assets (Optional but adds polish):**

| Sound | When It Plays |
|-------|---------------|
| Jump | Player jumps |
| Land | Player hits ground from height |
| Stomp | Enemy defeated |
| Hurt | Player takes damage |
| Coin | Collectible picked up |
| Win | Level complete |
| Music | Background loop |

**🗣️ FOR FREE ASSETS, SAY TO AIDE:**
> "Where can I find free game assets for a platformer? I need sprites, tiles, and sound effects."

**Popular free resources:**
- **OpenGameArt.org** - Sprites, tiles, sounds
- **Kenney.nl** - High-quality free game assets
- **Freesound.org** - Sound effects
- **Incompetech.com** - Royalty-free music

---

### 📐 GAME SPECIFICATIONS

**Canvas Size:** 800 x 600 pixels (standard, fits most screens)

**Physics Constants:**
```javascript
const GRAVITY = 0.5;        // Pixels per frame squared
const JUMP_FORCE = -12;     // Negative = upward (pixels per frame)
const MOVE_SPEED = 5;       // Horizontal pixels per frame
const MAX_FALL_SPEED = 10;  // Terminal velocity
const FRICTION = 0.8;       // Ground slowdown multiplier
```

**Frame Rate:** 60 FPS using `requestAnimationFrame`

**Level Data Format:**
```javascript
const level1 = {
    name: "Grasslands",
    playerStart: { x: 50, y: 500 },
    goal: { x: 750, y: 480, width: 40, height: 80 },
    platforms: [
        { x: 0, y: 550, width: 800, height: 50 },    // Ground
        { x: 200, y: 450, width: 100, height: 20 },  // Platform
        { x: 400, y: 350, width: 100, height: 20 },  // Higher platform
    ],
    enemies: [
        { x: 300, y: 530, patrolLeft: 250, patrolRight: 450 }
    ]
};
```

---

### 🗂️ FILE STRUCTURE

```
platformer-game/
├── index.html              # Game canvas and UI
├── css/
│   └── styles.css          # Game styling, menus
├── js/
│   ├── game.js             # Main loop, state machine
│   ├── player.js           # Player movement, physics
│   ├── input.js            # Keyboard handling
│   ├── physics.js          # Collision detection
│   ├── level.js            # Level loading, rendering
│   ├── enemy.js            # Enemy behavior
│   └── effects.js          # Particles, screen shake
├── assets/
│   ├── images/             # Sprites, tiles, backgrounds
│   └── sounds/             # Audio files
├── README.md               # Project documentation
├── CONTEXT.md              # AI context feeder
├── TASKS.md                # Task tracking
└── SCOPE.md                # Feature boundaries
```

---

### 🎮 GAME STATES

Your game will have these states:

```
┌─────────┐
│  MENU   │ ← Game starts here
└────┬────┘
     │ Press Enter
     ▼
┌─────────┐
│ PLAYING │ ← Main gameplay
└────┬────┘
     │         │
     │ ESC     │ Die / Fall
     ▼         ▼
┌─────────┐ ┌──────────┐
│ PAUSED  │ │ GAMEOVER │
└────┬────┘ └────┬─────┘
     │           │
     │ Resume    │ Retry
     ▼           ▼
┌─────────┐ ┌─────────┐
│ PLAYING │ │ PLAYING │
└─────────┘ └─────────┘
     │
     │ Reach Goal
     ▼
┌─────────────┐
│ LEVELCOMPLETE│
└──────┬──────┘
       │ Next Level / Win
       ▼
┌─────────┐ or ┌─────────┐
│ PLAYING │    │ VICTORY │
└─────────┘    └─────────┘
```

---

## 💡 HIDDEN HINTS

<details>
<summary>🔍 Hint 1: Player Falls Through Platforms</summary>

**Problem:** Player passes through platforms instead of landing.

**Likely Cause:** You're updating position BEFORE checking collision.

**Fix:** Check collision FIRST, then update position:
```javascript
// WRONG ORDER:
player.y += player.vy;  // Move first
checkCollision();        // Check after (too late!)

// RIGHT ORDER:
const nextY = player.y + player.vy;  // Calculate next position
if (!wouldCollide(player.x, nextY)) {
    player.y = nextY;  // Only move if safe
} else {
    player.vy = 0;     // Stop falling
    player.isOnGround = true;
}
```
</details>

<details>
<summary>🔍 Hint 2: Player Gets Stuck in Walls</summary>

**Problem:** Player moves into platform and can't escape.

**Likely Cause:** You're only checking vertical collision, not horizontal.

**Fix:** Check and resolve collisions on BOTH axes separately:
```javascript
// Check X movement first
player.x += player.vx;
if (collidesWithPlatform()) {
    player.x -= player.vx;  // Undo X movement
}

// Then check Y movement
player.y += player.vy;
if (collidesWithPlatform()) {
    player.y -= player.vy;  // Undo Y movement
    player.vy = 0;
}
```
</details>

<details>
<summary>🔍 Hint 3: Jump Feels Floaty or Too Fast</summary>

**Problem:** Jumping doesn't feel right.

**Fix:** Tune these values:
```javascript
// Floaty? Increase gravity:
const GRAVITY = 0.8;  // Was 0.5

// Too fast up? Decrease jump force:
const JUMP_FORCE = -10;  // Was -12

// Pro tip: Make falling faster than rising
if (player.vy > 0) {
    player.vy += GRAVITY * 1.5;  // Fall faster
} else {
    player.vy += GRAVITY;  // Rise normally
}
```
</details>

<details>
<summary>🔍 Hint 4: Enemy Collision Not Working</summary>

**Problem:** Player walks through enemies OR always dies.

**Likely Cause:** Not checking WHERE player hits enemy.

**Fix:** Check if player is above enemy:
```javascript
function checkEnemyCollision(player, enemy) {
    if (!rectIntersect(player, enemy)) return 'none';
    
    // Is player falling and above enemy's midpoint?
    if (player.vy > 0 && 
        player.y + player.height < enemy.y + enemy.height / 2) {
        return 'stomp';  // Player wins!
    }
    return 'hurt';  // Enemy wins
}
```
</details>

<details>
<summary>🔍 Hint 5: Game Loop Running Too Fast/Slow</summary>

**Problem:** Game speed changes on different computers.

**Likely Cause:** Not using delta time.

**Fix:** Calculate time between frames:
```javascript
let lastTime = 0;

function gameLoop(timestamp) {
    const deltaTime = (timestamp - lastTime) / 16.67;  // Normalize to 60fps
    lastTime = timestamp;
    
    update(deltaTime);  // Pass delta to update
    render();
    
    requestAnimationFrame(gameLoop);
}

// In update, multiply movements by deltaTime:
player.x += player.vx * deltaTime;
player.y += player.vy * deltaTime;
```
</details>

<details>
<summary>🔍 Hint 6: Keyboard Input Feels Laggy</summary>

**Problem:** Movement stutters or feels delayed.

**Likely Cause:** Using keydown events directly instead of tracking state.

**Fix:** Track key STATE, check in game loop:
```javascript
const keys = {};

document.addEventListener('keydown', e => keys[e.code] = true);
document.addEventListener('keyup', e => keys[e.code] = false);

// In update function:
if (keys['ArrowLeft']) player.vx = -MOVE_SPEED;
else if (keys['ArrowRight']) player.vx = MOVE_SPEED;
else player.vx = 0;
```
</details>

<details>
<summary>🔍 Hint 7: AI Keeps Breaking Working Code</summary>

**Problem:** You ask AI to add a feature and it breaks something else.

**Fixes:**
1. **Commit BEFORE asking AI for changes** - you can always revert
2. **Be specific:** "Add screen shake to the render function. Don't change player.js or physics.js."
3. **Use your CONTEXT.md** - paste it so AI knows what exists
4. **One feature at a time** - don't ask for multiple things at once
</details>

<details>
<summary>🔍 Hint 8: Level Design Tips</summary>

**Design principles for fun levels:**

1. **Teach, then test** - Introduce mechanic safely, then make it challenging
2. **Ramp difficulty** - Level 1 is easy, each level adds challenge
3. **Clear goals** - Player should see where to go
4. **Safe spawns** - Never spawn player next to danger
5. **Breathing room** - Not every jump should be hard

**Level 1 layout suggestion:**
```
                                    [FLAG]
                              ████████████
                        ████████
                  ████████
            ████████
      ████████
████████████████████████████████████████████
[PLAYER]               [ENEMY→←]
```
</details>

---

## 🧠 New Concept: The Game Loop

Games run in a loop that repeats 60 times per second:

```
Loop forever:
    1. Check inputs (what keys are pressed?)
    2. Update state (move things, check collisions)
    3. Render (draw everything)
    Repeat 60 times per second
```

This is fundamentally different from event-driven web apps.

```javascript
function gameLoop() {
    handleInput();       // Check keys
    update();            // Move things
    render();            // Draw things
    requestAnimationFrame(gameLoop);  // Do it again
}
```

---

## 🧠 New Concept: State Machines

Your player isn't just "on screen." They're in a STATE:

- **Idle** → standing still
- **Running** → moving left/right  
- **Jumping** → going up
- **Falling** → going down
- **Dead** → game over

Each state has different rules for what can happen next.

```
Idle + press right → Running
Running + press jump → Jumping
Jumping + hit ground → Running or Idle
Any state + hit enemy → Dead
```

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: Design Your Game

Before code, decide:

1. **Theme** - what's your world? (Forest, Space, Underwater, Castle?)
2. **Player** - what are you? (Character, animal, robot?)
3. **Goal** - what ends a level? (Reach flag? Collect items?)
4. **Enemies** - what tries to stop you? (Patrolling, jumping, shooting?)
5. **Power-ups** - what helps you? (Speed boost, invincibility, extra life?)

**Sketch a simple level on paper.** Where are platforms? Where do you start? Where's the goal?

---

## 🗣️ Phase 2: Project Setup

### Organized from the Start

This is a complex project. We'll organize from the beginning.

**🗣️ SAY TO AIDE:**
> "Create a platformer-game project with this structure:
> - index.html
> - css/styles.css
> - js/game.js (main game loop)
> - js/player.js (player logic)
> - js/level.js (level loading)
> - js/input.js (keyboard handling)
> - js/physics.js (movement and collision)
> - assets/ folder for images and sounds
> Initialize git."

**👀 WHY MULTIPLE FILES:**
- Each file has ONE responsibility
- Easier to find and fix bugs
- Easier for AI to focus on one part

---

## 🗣️ Phase 3: HTML & Canvas Setup

### Step 1: Game Canvas

**🗣️ SAY TO AIDE:**
> "In index.html, create a canvas element with id 'game' and a wrapper div for centering. Add a simple start screen overlay with title and 'Press Enter to Start' message. Load all JS files in the correct order."

**👀 SCRIPT ORDER MATTERS:**
```html
<script src="js/input.js"></script>
<script src="js/physics.js"></script>
<script src="js/player.js"></script>
<script src="js/level.js"></script>
<script src="js/game.js"></script>  <!-- Main last, uses others -->
```

---

### Step 2: Canvas Sizing

**🗣️ SAY TO AIDE:**
> "Style the canvas to be fixed at 800x600 pixels for consistent gameplay. Center it on screen with a dark background around it."

---

### Commit Setup

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add game structure with canvas and file organization'"

---

## 🗣️ Phase 4: Input System

### Step 1: Keyboard State

**🗣️ SAY TO AIDE:**
> "In input.js, create an input system that tracks which keys are currently pressed. Use keydown to set keys as pressed, keyup to set as released. Track arrow keys and spacebar."

**🧠 WHY TRACK STATE, NOT EVENTS:**

Event-driven:
```javascript
// Fires once when key pressed
document.onkeydown = () => moveRight();
```

State-tracked:
```javascript
// Game loop checks: "Is right arrow STILL down?"
if (keys.right) player.x += speed;
```

State tracking allows smooth movement.

---

### Step 2: Input Interface

**🗣️ SAY TO AIDE:**
> "Create functions isLeft(), isRight(), isJump() that return true/false based on key state. This gives a clean interface other code can use."

---

### Commit Input

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add keyboard input tracking'"

---

## 🗣️ Phase 5: Physics System

### Step 1: Constants

**🗣️ SAY TO AIDE:**
> "In physics.js, define physics constants: GRAVITY as pixels per frame squared, MAX_FALL_SPEED as terminal velocity, FRICTION as ground slowdown factor. Make these tweakable."

**👀 GOOD DEFAULTS:**
```javascript
const GRAVITY = 0.5;
const MAX_FALL_SPEED = 10;
const FRICTION = 0.8;
const JUMP_FORCE = -12;  // Negative = up
```

---

### Step 2: Apply Physics

**🗣️ SAY TO AIDE:**
> "Create applyGravity function that takes an entity, adds gravity to its vertical velocity, and caps at max fall speed. Create applyFriction that reduces horizontal velocity when on ground."

---

### Step 3: Collision Detection

**🗣️ SAY TO AIDE:**
> "Create collision functions: rectIntersect checks if two rectangles overlap, checkPlatformCollision checks if an entity is colliding with any platform and returns collision info including which side."

**🧠 RECTANGLE COLLISION:**
```javascript
function rectIntersect(a, b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}
```

---

### Step 4: Resolve Collision

**🗣️ SAY TO AIDE:**
> "Create resolveCollision that pushes an entity out of a platform based on collision side. If hitting from above, place entity on top of platform and set vertical velocity to 0. If hitting from side, stop horizontal movement."

---

### 📝 Your Turn: Collision Sides

When player hits platform from ABOVE, what should happen?

1. Player's bottom = Platform's top
2. Vertical velocity = ___
3. Player is on ground = ___

<details>
<summary>Click for answer</summary>

1. Player's bottom = Platform's top (snap to surface)
2. Vertical velocity = **0** (stop falling)
3. Player is on ground = **true** (can jump again)
</details>

---

### Commit Physics

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add physics system with gravity and collision'"

---

## 🗣️ Phase 6: Player

### Step 1: Player Object

**🗣️ SAY TO AIDE:**
> "In player.js, create a player object with position x and y, velocity vx and vy, size width and height, state like idle/running/jumping/falling, and flags for isOnGround and facingRight."

---

### Step 2: Player Update

**🗣️ SAY TO AIDE:**
> "Create player update function that runs each frame: check inputs to set horizontal velocity, apply physics, check collisions with all platforms, resolve collisions, update state based on velocity and ground contact."

**👀 UPDATE ORDER:**
1. Handle input → set velocity intentions
2. Apply gravity → modify velocity
3. Apply velocity → calculate new position
4. Check collisions → before moving?
5. Resolve collisions → adjust position
6. Update state → for animation

---

### Step 3: State Machine

**🗣️ SAY TO AIDE:**
> "Implement player state machine. States: idle when not moving on ground, running when moving on ground, jumping when moving up and not on ground, falling when moving down and not on ground. State affects which sprite to show."

---

### Step 4: Jumping

**🗣️ SAY TO AIDE:**
> "Implement jumping: player can only jump when isOnGround is true. When jump is pressed, set vertical velocity to negative jump force, set isOnGround to false, change state to jumping."

---

### Commit Player

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add player with movement and state machine'"

---

## ⚠️ Common AI Problem: Physics Chaos

Physics bugs are hard. You might see:
- Player falling through platforms
- Player stuck in platforms
- Jittering on ground

**🚨 DEBUGGING PHYSICS:**

**🗣️ SAY:**
> "Draw debug rectangles around player and platforms. Different colors for player state. Show velocity values as text on screen."

Visual debugging is ESSENTIAL for physics.

---

## 🗣️ Phase 7: Level System

### Step 1: Level Data Format

**🗣️ SAY TO AIDE:**
> "In level.js, create a level data format. Each level has: platforms array with x, y, width, height and type, player start position, goal position, and enemies array. Create Level 1 data."

```javascript
const level1 = {
    platforms: [
        { x: 0, y: 550, width: 800, height: 50, type: 'ground' },
        { x: 200, y: 450, width: 100, height: 20, type: 'platform' },
        // ...
    ],
    playerStart: { x: 50, y: 500 },
    goal: { x: 750, y: 520 }
};
```

---

### Step 2: Level Loading

**🗣️ SAY TO AIDE:**
> "Create loadLevel function that takes level data, resets player to start position, loads platforms into game state, spawns enemies, and resets score/timer."

---

### Step 3: Level Rendering

**🗣️ SAY TO AIDE:**
> "Create renderLevel that draws all platforms on the canvas. Different platform types could have different colors or patterns. Draw the goal as a distinct object like a flag or door."

---

### Step 4: Level Completion

**🗣️ SAY TO AIDE:**
> "Check for level completion - when player reaches the goal area. Show level complete message, maybe a score, and after delay load next level or show victory screen."

---

### Commit Levels

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add level system with loading and completion'"

---

## 🗣️ Phase 8: Enemies

### Step 1: Enemy Type

**🗣️ SAY TO AIDE:**
> "Create a basic enemy type that patrols back and forth between two points. It has position, velocity, patrol boundaries, and collision box."

---

### Step 2: Enemy Update

**🗣️ SAY TO AIDE:**
> "Create enemy update function: move in current direction, when hitting patrol boundary reverse direction. Check for collision with player."

---

### Step 3: Player-Enemy Collision

**🗣️ SAY TO AIDE:**
> "Handle player-enemy collision: if player hits enemy from above (jumping on them), destroy enemy and give bounce. If player hits enemy from side, player dies or loses health."

**🧠 CLASSIC MARIO LOGIC:**
```javascript
if (player.vy > 0 && player.y + player.height < enemy.y + enemy.height/2) {
    // Player is falling and hit top of enemy = stomp!
} else {
    // Player hit from side = damage!
}
```

---

### Commit Enemies

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add patrolling enemies with stomp mechanic'"

---

## 🧩 Phase 9: The Game Loop Challenge

**This is your final test.** You've learned everything you need. Now build the game loop YOURSELF.

### 🎯 Your Mission

Create a working game loop that:
1. Runs continuously at ~60 frames per second
2. Checks player input each frame
3. Updates all game objects (player, enemies)
4. Renders everything to the canvas
5. Handles different game states (menu, playing, paused, game over)

### 📋 Requirements

| Requirement | Description |
|-------------|-------------|
| **Continuous loop** | Uses browser's animation frame API |
| **Consistent timing** | Game runs same speed on all computers |
| **State-based logic** | Different behavior for menu vs playing vs paused |
| **Clean structure** | Separate functions for input, update, render |

### 🤔 Think About This

Before asking AI for help, consider:
- What browser API repeats a function smoothly?
- How do you measure time between frames?
- How do you organize code that runs differently based on game state?
- What order should input → update → render happen?

---

### 📝 Your Turn: Build It

Try to create the game loop yourself first. Use what you learned in this project and previous ones.

**When you're ready to test your knowledge:**

1. Create the main loop function in `game.js`
2. Implement the timing system
3. Add state machine logic
4. Wire up input, update, and render

**Stuck? Check the hints below. But try first!**

---

<details>
<summary>💡 Hint: What API to use?</summary>

The browser provides `requestAnimationFrame()` - it calls your function before the next screen repaint (~60 times per second).

```javascript
function loop() {
    // Your code here
    requestAnimationFrame(loop);  // Schedule next frame
}

// Start the loop
requestAnimationFrame(loop);
```
</details>

<details>
<summary>💡 Hint: How to track time?</summary>

`requestAnimationFrame` passes a timestamp to your function:

```javascript
let lastTime = 0;

function loop(currentTime) {
    const deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    
    // deltaTime is milliseconds since last frame
    // ~16.67ms at 60fps
    
    requestAnimationFrame(loop);
}
```
</details>

<details>
<summary>💡 Hint: How to structure game states?</summary>

Use a variable to track current state, then switch behavior:

```javascript
let gameState = 'menu';  // 'menu', 'playing', 'paused', 'gameover'

function update(dt) {
    switch (gameState) {
        case 'menu':
            // Wait for start input
            break;
        case 'playing':
            // Update player, enemies, check collisions
            break;
        case 'paused':
            // Do nothing, wait for unpause
            break;
        case 'gameover':
            // Wait for restart input
            break;
    }
}
```
</details>

<details>
<summary>💡 Hint: Complete game loop structure</summary>

Here's the full pattern - but make sure you understand each part!

```javascript
// Game state
let gameState = 'menu';
let lastTime = 0;

// Main loop
function gameLoop(timestamp) {
    // Calculate delta time
    const deltaTime = (timestamp - lastTime) / 1000;  // Convert to seconds
    lastTime = timestamp;
    
    // Cap delta time (prevents huge jumps if tab was hidden)
    const dt = Math.min(deltaTime, 0.1);
    
    // Process input (always, for pause/unpause)
    handleInput();
    
    // Update based on state
    if (gameState === 'playing') {
        update(dt);
    }
    
    // Always render (shows menus, pause screen, etc.)
    render();
    
    // Schedule next frame
    requestAnimationFrame(gameLoop);
}

// Start the game
function startGame() {
    lastTime = performance.now();
    requestAnimationFrame(gameLoop);
}

// Call when page loads
startGame();
```
</details>

<details>
<summary>💡 Hint: State transitions</summary>

```javascript
function handleInput() {
    switch (gameState) {
        case 'menu':
            if (keys['Enter']) {
                gameState = 'playing';
                initLevel(1);
            }
            break;
            
        case 'playing':
            if (keys['Escape']) {
                gameState = 'paused';
            }
            // Player movement handled in update()
            break;
            
        case 'paused':
            if (keys['Escape'] || keys['Enter']) {
                gameState = 'playing';
            }
            break;
            
        case 'gameover':
            if (keys['Enter']) {
                gameState = 'playing';
                initLevel(1);
            }
            break;
    }
}
```
</details>

---

### ✅ When It Works

You'll know your game loop is working when:
- [ ] The game runs smoothly without freezing
- [ ] Player moves at consistent speed
- [ ] Pressing ESC pauses the game
- [ ] Game over screen appears when you die
- [ ] You can restart after game over

**Commit your game loop:**
> "Commit with message 'Add main game loop with state machine'"

---

### Step 3: State Transitions

**🗣️ SAY TO AIDE:**
> "Implement state transitions: press Enter on menu to start playing, press Escape while playing to pause, reaching goal goes to levelComplete, dying goes to gameOver, states have resume/restart options."

---

### Commit Game Loop

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add main game loop with state management'"

---

## 🗣️ Phase 10: Polish (The "Juice")

Games need FEEL. This is what separates good from great.

### Step 1: Screen Shake

**🗣️ SAY TO AIDE:**
> "Add subtle screen shake on impacts - landing from high fall, stomping enemy, taking damage. Shake the canvas offset briefly then return to normal."

---

### Step 2: Particles

**🗣️ SAY TO AIDE:**
> "Add simple particle effects: dust clouds when landing or running, sparkles when collecting items, explosion particles when enemy dies."

---

### Step 3: Sound Effects

**🗣️ SAY TO AIDE:**
> "Add sound effects: jump sound, land sound, stomp sound, hurt sound, level complete fanfare. Use the Web Audio API or simple Audio objects."

---

### Step 4: Camera

**🗣️ SAY TO AIDE:**
> "For levels larger than screen, add camera that follows player. Player stays roughly centered while the world scrolls. Camera should have some smoothing."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add polish - screen shake, particles, sound, camera'"

---

## 🏆 Graduation Checklist

Before claiming your reward, verify:

- [ ] Player moves and jumps
- [ ] Collision with platforms works
- [ ] Player can reach goal
- [ ] Enemies patrol and can be stomped
- [ ] Game states work (menu, play, pause, win/lose)
- [ ] At least 2 complete levels
- [ ] Some polish effects added

---

## 🎓 What You Learned (In This Project AND The Whole Curriculum)

### Game-Specific:
- ✅ **Game loop** - update/render cycle
- ✅ **Physics** - gravity, velocity, collision
- ✅ **State machines** - organized state logic
- ✅ **Data-driven design** - levels from data

### Curriculum-Wide Skills:

| Skill | You Learned In... |
|-------|-------------------|
| AI Prompting | Every project |
| Reading Code | Project 1 |
| Separation of Concerns | Projects 2, 12 |
| State Management | Projects 4, 5, 12 |
| CRUD Operations | Projects 5, 7, 8, 11 |
| Data Visualization | Projects 8, 9 |
| Complex Data | Projects 10, 11 |
| Project Organization | Projects 3, 12 |
| Git Workflows | Every project |
| Debugging | Every project |
| Recognizing AI Problems | Every project |

---

## 📚 AI MANAGEMENT SKILLS - Complete Reference

You've learned these skills progressively. Here's your complete toolkit:

### Project Management Files

| File | Purpose | Introduced In |
|------|---------|---------------|
| **NOTES.md** | Personal project journal | Project 1 |
| **README.md** | Project documentation for humans & AI | Project 3 |
| **CONTEXT.md** | Context feeder for AI sessions | Project 4 |
| **TASKS.md** | Current work & backlog tracking | Project 7 |
| **SCOPE.md** | Feature boundaries & scope control | Project 8 |

### AI Problem Patterns

| Problem | Signs | Solution | Learned In |
|---------|-------|----------|------------|
| **Context Loss** | AI forgets project, references wrong files | Paste CONTEXT.md, re-establish | Project 1 |
| **Loops** | Same "fix" over and over | "Stop. Show me the code. Don't change it." | Project 1 |
| **Tangents** | AI writes tons of unrequested code | "Undo. Only do what I asked." | Project 1 |
| **Feature Creep** | AI suggests endless features | "Out of scope. Focus on [task]." | Project 8 |

### Development Practices

| Practice | What It Means | Introduced In |
|----------|---------------|---------------|
| **Comments/Docstrings** | Document code for future you & AI | Project 2 |
| **Experiments Folder** | Test ideas without breaking main code | Project 6 |
| **Git Branches** | Work on features safely | Project 5 |
| **Code Review** | Have AI check your work | Project 11 |
| **Catching Mistakes** | Validate AI output before using | Project 10 |
| **GitHub & CI/CD** | Professional deployment | Project 12 |

### Good Prompting Patterns

```markdown
✅ GOOD: Specific, contextual, focused
"In js/player.js, the jump function should set vy to -12 and 
isOnGround to false. Add a console.log showing the player's 
y position before and after."

❌ BAD: Vague, no context, too broad
"Fix the jumping"
"Make it better"
"It's broken"
```

### Recovery Techniques

| Situation | Recovery |
|-----------|----------|
| AI broke working code | `git checkout -- filename` or "Undo" |
| Lost track of project | Paste README + CONTEXT + current task |
| Stuck in debugging loop | Add console.logs, trace manually |
| AI suggests wrong approach | "That's not what I want. Let me explain..." |
| Feature creep temptation | Add to SCOPE.md "Maybe Later" section |

---

## 🎁 CLAIM YOUR REWARDS!

**You've completed the entire AIDE curriculum!**

### Immediate Reward:
Dede gets a **Graduation Cap!** 🎓

### Master Achievement:
You've unlocked the **"Master Coder" badge** - displayed on your AIDE profile showing you've completed all 12 projects!

### You've Learned:
- How to work WITH AI as a partner
- How to guide AI when it goes wrong
- How to build real projects from scratch
- How to think like a developer
- How to manage complexity
- **How to manage AI through documentation and structure**

---

## 🚀 What's Next?

You're not done learning - you're ready for the real world!

**Ideas:**
1. **Combine projects** - Recipe app with health tracking
2. **Go mobile** - Convert a project to a mobile app
3. **Add backends** - Real databases and servers
4. **Start YOUR dream project** - You have the skills now!

Remember: The AI is your partner. You're the director. 

**Go build something amazing.** 🚀

---

## 📖 Curriculum Quick Reference

| Project | Focus | Key Concept |
|---------|-------|-------------|
| 1. Floor is Lava | Basics | Reading code, brackets, colors |
| 2. Word Calculator | Functions | Separation of concerns |
| 3. Portfolio | Multi-page | File organization, layouts |
| 4. Smart Home | Toggles | State management |
| 5. Kanban | Drag/Drop | CRUD, event delegation |
| 6. Chat App | Real-time | Scope management |
| 7. Journal | Rich Text | Content editing, export |
| 8. Budget | Charts | Calculations, visualization |
| 9. Health | Gamification | Streaks, achievements |
| 10. Store | E-commerce | Cart logic, checkout |
| 11. Recipe | Nested Data | Complex CRUD |
| 12. Platformer | Game Dev | Loops, physics, polish |
| **13. Your Dream** | **Extra Credit** | **Production pipeline** |

---

*Congratulations, Graduate! 🎓🎉*

*Ready for more? Try the Extra Credit Project 13 - Build Your Dream!*
