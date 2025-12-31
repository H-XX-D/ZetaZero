# 🏠 Project 4: Virtual Smart Home

## 🎁 Reward: Bow Tie for Dede!
Complete this project and Dede looks dapper with a bow tie! Fancy! 🎀

---

## 🌟 What You'll Learn

Build an interactive smart home dashboard where you control virtual lights, thermostats, and devices. You'll master:

- ✅ **State management** - tracking what's on, what's off, current values
- ✅ **Component thinking** - breaking UI into reusable pieces
- ✅ **Event handling** - responding to user interactions
- ✅ **Local storage** - saving data in the browser
- ✅ **Prompting for complex logic** - multi-step AI instructions
- ✅ **Context feeder files** - keeping AI aligned with your project

---

## 📓 NEW SKILL: The Context Feeder File

As projects get complex, AI loses track. Create a file specifically to "feed" context back to AI.

**🗣️ SAY TO AIDE:**
> "Create a file called CONTEXT.md in my project root"

**Structure your CONTEXT.md like this:**

```markdown
# Smart Home Dashboard - AI Context

## Quick Summary
Interactive dashboard to control virtual smart home devices.

## Current State Structure
```js
state = {
    rooms: {
        'living-room': { light: true, thermostat: 72 },
        'bedroom': { light: false },
        'kitchen': { light: true }
    },
    frontDoor: { locked: true }
}
```

## File Purposes
- index.html: Dashboard UI with room cards
- css/styles.css: Dark theme dashboard styling
- js/app.js: State management and device control logic

## Current Task
Working on: Thermostat up/down buttons

## Key Decisions Made
- Using data-room attributes to link buttons to state
- Saving state to localStorage on every change
- Temperature range: 60-80°F

## What's Working
- Light toggles update state
- State saves to localStorage

## What's Broken / TODO
- Thermostat buttons not connected yet
- Lock animation TODO
```

**🧠 HOW TO USE IT:**

When AI loses context, paste this:
> "Here's my project context: [paste CONTEXT.md contents]. Now, help me with [specific task]."

**Update CONTEXT.md every time you:**
- Make a key decision
- Finish a feature
- Change the state structure
- Hit a bug

---

## 🧠 New Concept: State

**State** = the current condition of your app.

Think of a light switch:
- State: `on` or `off`
- When you flip the switch, the state changes

In code:
```javascript
let lightOn = false;     // State: off
lightOn = true;          // State: on
```

Your smart home will track LOTS of state:
- Is each light on/off?
- What's the thermostat set to?
- Is the door locked?
- Is the music playing?

> 💡 **Why state matters:** When you save/load, you're saving STATE. When you display, you're showing STATE. Everything revolves around state.

---

## 🗺️ Phase 1: Planning Your Smart Home

### 📝 Your Turn: Draw Your Floor Plan

Before code, SKETCH (paper or digital) a simple floor plan with:

- 3-4 rooms (Living Room, Bedroom, Kitchen, etc.)
- Devices in each room:
  - Lights (on/off)
  - Thermostat (temperature)
  - Smart Lock (locked/unlocked)
  - Maybe: Music, TV, Blinds

### State Planning

List every piece of state your app needs:

| Device | Room | State Type | Possible Values |
|--------|------|------------|-----------------|
| Light | Living Room | Boolean | true/false |
| Thermostat | Whole House | Number | 60-80 |
| Lock | Front Door | Boolean | true/false |
| ... | ... | ... | ... |

**This planning will make your prompts to AIDE so much clearer.**

---

## 🗣️ Phase 2: Project Setup

### Step 1: Create Files

**🗣️ SAY TO AIDE:**
> "Create a smart-home-dashboard project with index.html, css/styles.css, and js/app.js. The structure should be clean with separate folders for css and js."

---

### Step 2: Git Init

**🗣️ SAY TO AIDE:**
> "Initialize git and create a .gitignore"

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Dashboard Layout

**🗣️ SAY TO AIDE:**
> "In index.html, create a dashboard layout. Add a header with the title 'My Smart Home' and the current date/time that will update. Below that, create a main section with a grid that will hold room cards."

---

### Step 2: Room Cards

**🗣️ SAY TO AIDE:**
> "Create a room card template. Each room card should have: a room name heading, an icon placeholder, and a container for device controls. Create cards for Living Room, Bedroom, Kitchen, and Bathroom."

**👀 CHECK:**

✅ Each room has a unique identifier (id or data attribute)
✅ Structure is consistent across all rooms

---

### Step 3: Device Controls Template

**🗣️ SAY TO AIDE:**
> "Inside each room card, add device controls. Each room should have a light toggle with an on/off switch. The Living Room should also have a thermostat control with temperature display and up/down buttons. Add a front door section in the sidebar with a lock toggle."

**👀 WATCH FOR:**

✅ Toggles have data attributes linking them to rooms:
```html
<button class="light-toggle" data-room="living-room">
```

✅ Thermostat has display element we can update:
```html
<span id="thermostat-value">72</span>
```

---

### 📝 Your Turn: Data Attributes

Data attributes connect HTML elements to JavaScript logic.

Fill in what `data-room` should be for each:

```html
<button class="light-toggle" data-room="________">  <!-- Living Room -->
<button class="light-toggle" data-room="________">  <!-- Bedroom -->
<button class="light-toggle" data-room="________">  <!-- Kitchen -->
```

<details>
<summary>✅ Check Your Answers</summary>

```html
<button class="light-toggle" data-room="living-room">
<button class="light-toggle" data-room="bedroom">
<button class="light-toggle" data-room="kitchen">
```

**Naming convention:** Use lowercase with hyphens (kebab-case) for data attributes.
</details>

---

## ✏️ FILL IN THE BLANKS: State Management

### 🧠 CONCEPT: What is State?

State is like the "current status" of everything in your app.

Think of your home right now:
- Is the living room light ON or OFF?
- What temperature is the thermostat set to?
- Is the front door LOCKED or UNLOCKED?

All of these are **state**. In JavaScript, we store state in objects:

```javascript
const state = {
    livingRoom: { light: true },    // light is ON
    thermostat: 72,                  // set to 72°F
    frontDoor: { locked: true }      // door is LOCKED
};
```

### Exercise 1: Initialize State

```javascript
// Create the initial state for our smart home
const state = {
    rooms: {
        'living-room': { 
            light: _____,      // Start with light OFF
            thermostat: ____   // Start at 72 degrees
        },
        'bedroom': { 
            light: _____ 
        },
        'kitchen': { 
            light: _____ 
        }
    },
    frontDoor: { 
        locked: _____          // Start LOCKED for safety
    }
};
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const state = {
    rooms: {
        'living-room': { 
            light: false,
            thermostat: 72
        },
        'bedroom': { light: false },
        'kitchen': { light: false }
    },
    frontDoor: { locked: true }
};
```

**Why booleans?**
- `true` = ON/LOCKED
- `false` = OFF/UNLOCKED
- Easy to toggle: `light = !light` flips the value
</details>

---

### 🧠 CONCEPT: Reading from State

To read nested data, chain the property names:

```javascript
state.rooms['living-room'].light     // true or false
state.rooms['bedroom'].light         // true or false
state.frontDoor.locked               // true or false
```

**Bracket vs Dot notation:**
- `state.frontDoor` - works when property name is simple
- `state.rooms['living-room']` - needed when name has hyphens

### Exercise 2: Read State Values

```javascript
function isLightOn(roomName) {
    // Access the room's light state
    return state._______[roomName].________;
}

function getThermostat() {
    return state.rooms['living-room'].____________;
}

function isDoorLocked() {
    return state.____________.________;
}

// Test:
isLightOn('bedroom')  // returns true or false
getThermostat()       // returns 72
isDoorLocked()        // returns true or false
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function isLightOn(roomName) {
    return state.rooms[roomName].light;
}

function getThermostat() {
    return state.rooms['living-room'].thermostat;
}

function isDoorLocked() {
    return state.frontDoor.locked;
}
```
</details>

---

### 🧠 CONCEPT: Updating State

To change state, assign a new value:

```javascript
// Turn on a light
state.rooms['bedroom'].light = true;

// Toggle (flip) a light
state.rooms['bedroom'].light = !state.rooms['bedroom'].light;

// Change thermostat
state.rooms['living-room'].thermostat = 75;
```

**The `!` operator:** Flips booleans
- `!true` → `false`
- `!false` → `true`

### Exercise 3: Toggle Functions

```javascript
function toggleLight(roomName) {
    // Flip the light state (if on, turn off; if off, turn on)
    state.rooms[roomName].light = ___state.rooms[roomName].light;
    
    // Update the display
    renderRoom(roomName);
    
    // Save to localStorage
    ____________();
}

function adjustThermostat(change) {
    // change will be +1 or -1
    const currentTemp = state.rooms['living-room'].thermostat;
    const newTemp = currentTemp ____ change;
    
    // Keep temperature between 60 and 80
    if (newTemp >= ____ && newTemp <= ____) {
        state.rooms['living-room'].thermostat = newTemp;
        renderThermostat();
        saveState();
    }
}

function toggleDoorLock() {
    state.frontDoor.locked = ___state.frontDoor.________;
    renderDoor();
    saveState();
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function toggleLight(roomName) {
    state.rooms[roomName].light = !state.rooms[roomName].light;
    renderRoom(roomName);
    saveState();
}

function adjustThermostat(change) {
    const currentTemp = state.rooms['living-room'].thermostat;
    const newTemp = currentTemp + change;
    
    if (newTemp >= 60 && newTemp <= 80) {
        state.rooms['living-room'].thermostat = newTemp;
        renderThermostat();
        saveState();
    }
}

function toggleDoorLock() {
    state.frontDoor.locked = !state.frontDoor.locked;
    renderDoor();
    saveState();
}
```

**Pattern to remember:**
1. Update state
2. Re-render the UI
3. Save to localStorage
</details>

---

### 🧠 CONCEPT: localStorage

The browser can save data that survives page refresh:

```javascript
// Save (must convert to string)
localStorage.setItem('key', JSON.stringify(data));

// Load (must convert back)
const data = JSON.parse(localStorage.getItem('key'));
```

**Why JSON.stringify/parse?**
- localStorage only stores strings
- Objects must be converted to string (stringify) to save
- Strings must be converted back to object (parse) to use

### Exercise 4: Save and Load

```javascript
function saveState() {
    // Convert state object to string and save
    localStorage.________('smartHome', JSON.___________(state));
}

function loadState() {
    const saved = localStorage.________('smartHome');
    
    if (saved) {
        // Parse the string back into an object
        const loaded = JSON._______(saved);
        
        // Copy loaded data into our state
        Object.assign(state, loaded);
    }
}

// Call loadState when page loads
window._______________('load', loadState);
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function saveState() {
    localStorage.setItem('smartHome', JSON.stringify(state));
}

function loadState() {
    const saved = localStorage.getItem('smartHome');
    
    if (saved) {
        const loaded = JSON.parse(saved);
        Object.assign(state, loaded);
    }
}

window.addEventListener('load', loadState);
```

**Object.assign():** Copies properties from one object to another
- `Object.assign(target, source)` - copies source INTO target
</details>
```

<details>
<summary>Click for answer</summary>

```html
data-room="living-room"
data-room="bedroom"
data-room="kitchen"
```

Use lowercase with hyphens. This will match keys in your state object.
</details>

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add dashboard HTML structure with room cards and device controls'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Dashboard Layout

**🗣️ SAY TO AIDE:**
> "Style the dashboard with a dark theme - dark backgrounds with accent colors for active devices. Use CSS Grid to arrange the room cards. Make the header sticky at the top."

---

### Step 2: Room Cards

**🗣️ SAY TO AIDE:**
> "Style room cards with rounded corners, padding, and subtle borders. When a room has a light on, it should have a glowing border effect. The room name should be clearly visible."

**👀 CHECK FOR:**

✅ `.room-card.light-on` styling for active state
✅ Uses CSS classes that JavaScript can add/remove

---

### Step 3: Toggle Switches

**🗣️ SAY TO AIDE:**
> "Create a custom toggle switch style - a pill-shaped background with a circle that slides left and right. When on, the background should be a bright accent color. Animate the toggle smoothly."

**🧠 UNDERSTAND THIS:**
```css
.toggle {
    width: 60px;
    height: 30px;
    background: #666;              /* Off state */
    border-radius: 15px;
    transition: background 0.3s;
}

.toggle.on {
    background: #00ff00;           /* On state */
}

.toggle::before {                  /* The circle */
    content: '';
    position: absolute;
    width: 26px;
    height: 26px;
    background: white;
    border-radius: 50%;
    transform: translateX(2px);
    transition: transform 0.3s;
}

.toggle.on::before {
    transform: translateX(32px);   /* Slide right when on */
}
```

---

### Step 4: Thermostat Control

**🗣️ SAY TO AIDE:**
> "Style the thermostat control. Display the temperature in a large font. The up/down buttons should be circular and clearly indicate their function."

---

### Step 5: Responsive Design

**🗣️ SAY TO AIDE:**
> "Make the dashboard responsive. On mobile, room cards should stack vertically. The header should collapse to just the title."

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add dark theme dashboard styling with custom toggles'"

---

## 🗣️ Phase 5: JavaScript State Management

This is the heart of the app - managing state.

### Step 1: Define Initial State

**🗣️ SAY TO AIDE:**
> "In app.js, create a state object that tracks all devices. Include lights for each room as booleans, the thermostat temperature as a number, and the front door lock status as a boolean. Initialize with sensible defaults."

**👀 CHECK:**

✅ Clear, organized state object:
```javascript
const state = {
    lights: {
        'living-room': false,
        'bedroom': false,
        'kitchen': false,
        'bathroom': false
    },
    thermostat: 72,
    lock: true
};
```

---

### Step 2: Get DOM References

**🗣️ SAY TO AIDE:**
> "Get references to all the toggle buttons, the thermostat display and buttons, and the lock toggle. Use document.querySelectorAll for groups and getElementById for unique elements."

---

### Step 3: Render Function

**🗣️ SAY TO AIDE:**
> "Create a render function that updates ALL the UI to match the current state. For each room, check if the light is on and add/remove the appropriate classes. Update the thermostat display. Update the lock toggle."

**👀 CHECK:**

✅ Function reads FROM state, writes TO DOM
✅ Handles every piece of state

**🧠 ONE-WAY DATA FLOW:**
```
State changes → render() → UI updates

Never: UI changes → State (directly)
Instead: UI click → update state → render()
```

---

### ⚠️ Common AI Problem: Direct DOM Manipulation

AI sometimes writes code that updates DOM directly without updating state:

```javascript
// BAD - updates button but not state
button.classList.add('on');

// GOOD - updates state, then renders
state.lights['living-room'] = true;
render();
```

**🚨 IF AI DOES THIS:**

**🗣️ SAY:**
> "Don't update the DOM directly. Update the state object, then call render to update the UI from state."

This pattern prevents state and UI from getting out of sync.

---

### Step 4: Light Toggle Handler

**🗣️ SAY TO AIDE:**
> "Add click event listeners to all light toggles. When clicked, get the room from the data attribute, toggle that light's state from on to off or off to on, then call render. Also log the action to console."

**👀 CHECK:**

✅ Gets room from data attribute: `button.dataset.room`
✅ Toggles state: `state.lights[room] = !state.lights[room]`
✅ Calls render after state change

---

### Step 5: Thermostat Controls

**🗣️ SAY TO AIDE:**
> "Add click handlers for the thermostat up and down buttons. Up increases temperature by 1 degree with a max of 85. Down decreases by 1 with a min of 55. Update state and render after each change."

**👀 CHECK:**

✅ Boundary checks (min/max)
✅ State updates before render

---

### 📝 Your Turn: Write the Boundary Check

Complete this code:

```javascript
function increaseTemp() {
    if (state.thermostat < ___) {
        state.thermostat ___ 1;
        render();
    }
}

function decreaseTemp() {
    if (state.thermostat > ___) {
        state.thermostat ___ 1;
        render();
    }
}
```

<details>
<summary>Click for answer</summary>

```javascript
function increaseTemp() {
    if (state.thermostat < 85) {
        state.thermostat += 1;
        render();
    }
}

function decreaseTemp() {
    if (state.thermostat > 55) {
        state.thermostat -= 1;
        render();
    }
}
```
</details>

---

### Step 6: Lock Toggle

**🗣️ SAY TO AIDE:**
> "Add a click handler for the lock toggle. Toggle the lock state between locked and unlocked, update the UI to show a lock or unlock icon, and call render."

---

### Commit Logic

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add state management and device control handlers'"

---

## 🗣️ Phase 6: Local Storage (Persistence)

Right now, refresh the page and everything resets. Let's save state!

### Step 1: Save State Function

**🗣️ SAY TO AIDE:**
> "Create a saveState function that saves the entire state object to localStorage as a JSON string. Call this function every time state changes."

**👀 CHECK:**

✅ Uses `JSON.stringify(state)` to convert object to string
✅ Uses `localStorage.setItem('smartHomeState', ...)`

---

### Step 2: Load State Function

**🗣️ SAY TO AIDE:**
> "Create a loadState function that checks localStorage for saved state, parses it from JSON, and returns it. If nothing is saved, return null. Call this when the app starts and use the saved state if available."

**👀 CHECK:**

✅ Uses `localStorage.getItem('smartHomeState')`
✅ Uses `JSON.parse()` with try-catch (in case data is corrupted)
✅ Returns fallback if nothing saved

---

### Step 3: Integrate Save/Load

**🗣️ SAY TO AIDE:**
> "Modify the app initialization to load saved state if available, otherwise use defaults. Modify all handlers to call saveState after updating state."

---

### Test Persistence

1. Toggle some lights, change thermostat
2. Refresh the page
3. Your settings should persist!

If not:

**🗣️ SAY:**
> "When I refresh, state doesn't persist. Add a console.log in loadState to see if saved data is being found."

---

### Commit Persistence

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add localStorage persistence for state'"

---

## 🗣️ Phase 7: Polish & Features

### Step 1: Live Clock

**🗣️ SAY TO AIDE:**
> "Add a live clock to the header that shows current time and updates every second. Also show the current date."

---

### Step 2: All Lights Toggle

**🗣️ SAY TO AIDE:**
> "Add a master toggle in the header that turns all lights on or off. If any lights are on, clicking it turns all off. If all are off, clicking turns all on."

---

### Step 3: Preset Scenes

**🗣️ SAY TO AIDE:**
> "Add scene preset buttons: 'Good Morning' sets living room and kitchen lights on, thermostat to 70. 'Good Night' turns all lights off, thermostat to 68, lock to locked. 'Movie Mode' turns only living room on with thermostat at 72."

**🧠 UNDERSTAND THIS:**
Scenes are just preset state values:
```javascript
const scenes = {
    morning: {
        lights: { 'living-room': true, 'bedroom': false, 'kitchen': true, 'bathroom': false },
        thermostat: 70,
        lock: false
    },
    // ...
};
```

---

### Step 4: Energy Usage Display

**🗣️ SAY TO AIDE:**
> "Add an energy usage meter that calculates usage based on what's on. Each light adds 10 watts, AC/heating adds based on how far thermostat is from 70. Display the current usage."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add clock, master toggle, scenes, and energy display'"

---

## ⚠️ Common AI Problem: Overcomplicating State

For this project, AI might suggest:
- Redux
- State management libraries
- Complex observer patterns

**🚨 IF AI OVERCOMPLICATES:**

**🗣️ SAY:**
> "Keep it simple. Just a plain JavaScript object for state and a render function. No libraries or frameworks."

Simple is better when learning. Libraries come later.

---

## 🎓 What You Learned

### AI Communication:
- ✅ Describe multi-step logic clearly
- ✅ Enforce state management patterns
- ✅ Keep AI from overcomplicating

### Code Concepts:
- ✅ **State** - single source of truth
- ✅ **One-way data flow** - state → render → UI
- ✅ **Local Storage** - persisting data
- ✅ **Data attributes** - connecting HTML to logic
- ✅ **Event delegation** - handling dynamic elements

### Project Skills:
- ✅ Plan state before coding
- ✅ Test state persistence
- ✅ Build incrementally

---

## 🎁 Unlock Your Reward!

Dede now looks dapper with a bow tie! 🎀

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Room animations** - "Add smooth animations when lights turn on/off"
2. **Voice control** - "Add a voice command button using Web Speech API"
3. **Automation rules** - "Turn on kitchen light automatically at 7am"
4. **Multiple users** - "Add a dropdown to switch between user profiles with different preferences"

---

## 📖 Quick Reference

### State Management Pattern

```javascript
// 1. Define state
const state = { ... };

// 2. Render reads state → updates UI
function render() { ... }

// 3. Handlers update state → call render
function handleClick() {
    state.something = newValue;
    saveState();
    render();
}

// 4. Init loads state → calls render
function init() {
    const saved = loadState();
    if (saved) Object.assign(state, saved);
    render();
}
```

### Local Storage

| Action | Code |
|--------|------|
| Save | `localStorage.setItem('key', JSON.stringify(obj))` |
| Load | `JSON.parse(localStorage.getItem('key'))` |
| Delete | `localStorage.removeItem('key')` |
| Clear all | `localStorage.clear()` |

### Data Attributes

```html
<button data-room="bedroom" data-device="light">

<script>
const room = button.dataset.room;     // "bedroom"
const device = button.dataset.device; // "light"
</script>
```

---

*Next up: Project 5 - Kanban Board! 📋 + 🚀*
