# 📒 JavaScript Mastery
## *The Language of the Web*

---

## 🎁 Reward: Browser Badge for Dede!
Complete this book and Dede gets a cool browser window accessory!

---

## 🌟 Why JavaScript?

```
┌─────────────────────────────────────────────────────────────────┐
│                  WHY DEVELOPERS LOVE JAVASCRIPT                 │
├─────────────────────────────────────────────────────────────────┤
│  ✅ Everywhere       - Browser, server, mobile, desktop        │
│  ✅ Dynamic          - Flexible and forgiving                  │
│  ✅ Huge Ecosystem   - npm has millions of packages            │
│  ✅ Async Power      - Handle multiple things at once          │
│  ✅ JSON Native      - Data interchange is natural             │
│  ✅ Community        - Largest developer community             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 CHAPTER 1: JavaScript Basics

### 1.1 Variables - let, const, var

```javascript
// const - cannot be reassigned (preferred for values that don't change)
const PI = 3.14159;
const APP_NAME = "AIDE";

// let - can be reassigned (use for values that change)
let score = 0;
score = 100;

// var - old way (avoid in modern JS)
var oldWay = "don't use this";

// When to use what:
// const by default
// let when you need to reassign
// var never (unless legacy code)
```

### 1.2 Data Types

```javascript
// Primitives
const name = "Dede";           // String
const age = 5;                 // Number (no int/float distinction)
const isHappy = true;          // Boolean
const nothing = null;          // Null (intentionally empty)
let unknown;                   // Undefined (not yet assigned)
const unique = Symbol("id");   // Symbol (unique identifier)
const big = 9007199254740991n; // BigInt (large numbers)

// Objects (everything else)
const person = { name: "Dede", age: 5 };
const fruits = ["apple", "banana"];
const greet = function() { console.log("Hi!"); };
```

### 🧪 Fill-in-the-Blank: Variables

```javascript
// Create a constant for the app version
_______ APP_VERSION = "1.0.0";

// Create a variable for user's points
_______ points = 0;

// Create an object for game settings
_______ settings = {
    difficulty: _______,
    soundEnabled: _______,
    playerName: _______
};

// Update points (this should work!)
points = _______;

// Try to update APP_VERSION (this should fail!)
// APP_VERSION = "2.0.0";  // Uncomment to see error
```

<details>
<summary>💡 Solution</summary>

```javascript
const APP_VERSION = "1.0.0";
let points = 0;
const settings = {
    difficulty: "normal",
    soundEnabled: true,
    playerName: "Dede"
};
points = 100;
```
</details>

---

## 📚 CHAPTER 2: Strings and Template Literals

### 2.1 String Basics

```javascript
const name = "Dede";

// Template literals (backticks) - MODERN WAY
const message = `Hello, ${name}!`;
const multiline = `
    This is a
    multi-line
    string!
`;

// String methods
console.log(name.toUpperCase());    // DEDE
console.log(name.toLowerCase());    // dede
console.log(name.length);           // 4
console.log(name.charAt(0));        // D
console.log(name.includes("ed"));   // true
console.log(name.split(""));        // ["D", "e", "d", "e"]
```

### 🧪 Fill-in-the-Blank: Strings

```javascript
const firstName = "Dede";
const lastName = "Bot";
const age = 5;

// Create a greeting using template literal
const greeting = `Hello, my name is ${_______} ${_______}!`;

// Create a multi-line bio
const bio = _______
    Name: ${firstName} ${lastName}
    Age: ${age}
    Status: Happy
_______;

// Check if name includes "De"
const hasDe = firstName._______(____);

// Get first 2 characters
const initials = firstName._______(0, _____);
```

<details>
<summary>💡 Solution</summary>

```javascript
const greeting = `Hello, my name is ${firstName} ${lastName}!`;

const bio = `
    Name: ${firstName} ${lastName}
    Age: ${age}
    Status: Happy
`;

const hasDe = firstName.includes("De");
const initials = firstName.slice(0, 2);
```
</details>

---

## 📚 CHAPTER 3: Arrays

### 3.1 Array Basics

```javascript
const fruits = ["apple", "banana", "cherry"];

// Access
console.log(fruits[0]);        // apple
console.log(fruits.at(-1));    // cherry (last item)

// Modify
fruits.push("date");           // Add to end
fruits.unshift("avocado");     // Add to beginning
fruits.pop();                  // Remove last
fruits.shift();                // Remove first

// Find
const index = fruits.indexOf("banana");
const hasCherry = fruits.includes("cherry");
```

### 3.2 Array Methods (The Big Three)

```javascript
const numbers = [1, 2, 3, 4, 5];

// MAP - transform each item
const doubled = numbers.map(n => n * 2);
// [2, 4, 6, 8, 10]

// FILTER - keep items that pass test
const evens = numbers.filter(n => n % 2 === 0);
// [2, 4]

// REDUCE - combine into single value
const sum = numbers.reduce((acc, n) => acc + n, 0);
// 15

// Chain them!
const result = numbers
    .filter(n => n > 2)
    .map(n => n * 2)
    .reduce((acc, n) => acc + n, 0);
// 24
```

### 🧪 Fill-in-the-Blank: Arrays

```javascript
const scores = [85, 92, 78, 95, 88];

// Add a score to the end
scores._______(90);

// Get scores above 85
const highScores = scores._______(score => score _______ 85);

// Double all scores (for bonus points!)
const bonusScores = scores._______(score => score _______ 2);

// Get the total of all scores
const total = scores._______(
    (_______, score) => _______ + score,
    _______
);

// Find the average
const average = total / scores._______;
```

<details>
<summary>💡 Solution</summary>

```javascript
scores.push(90);
const highScores = scores.filter(score => score > 85);
const bonusScores = scores.map(score => score * 2);
const total = scores.reduce(
    (acc, score) => acc + score,
    0
);
const average = total / scores.length;
```
</details>

---

## 📚 CHAPTER 4: Objects

### 4.1 Object Basics

```javascript
const person = {
    name: "Dede",
    age: 5,
    hobbies: ["coding", "dancing"],
    greet() {
        console.log(`Hi, I'm ${this.name}!`);
    }
};

// Access
console.log(person.name);          // Dede
console.log(person["age"]);        // 5 (bracket notation)
console.log(person.hobbies[0]);    // coding

// Modify
person.age = 6;
person.color = "purple";
delete person.hobbies;
```

### 4.2 Destructuring

```javascript
const person = { name: "Dede", age: 5, color: "purple" };

// Object destructuring
const { name, age } = person;
console.log(name);  // Dede

// With rename
const { name: personName } = person;

// With default
const { nickname = "Anonymous" } = person;

// Array destructuring
const [first, second, third] = [1, 2, 3];

// Spread operator
const newPerson = { ...person, mood: "happy" };
const allNumbers = [...[1, 2], ...[3, 4]];
```

### 🧪 Fill-in-the-Blank: Objects

```javascript
const game = {
    title: "Floor is Lava",
    score: 0,
    isRunning: false,
    player: {
        name: "Hero",
        health: 100
    }
};

// Destructure title and score
const { _______, _______ } = game;

// Destructure player name with rename
const { player: { name: _______ } } = game;

// Create a new game object with updated score using spread
const updatedGame = {
    _______game,
    score: _______
};

// Add a method to the game
game.start = _______ () {
    this.isRunning = _______;
    console.log(`${this._______} started!`);
};
```

<details>
<summary>💡 Solution</summary>

```javascript
const { title, score } = game;
const { player: { name: playerName } } = game;

const updatedGame = {
    ...game,
    score: 100
};

game.start = function () {
    this.isRunning = true;
    console.log(`${this.title} started!`);
};
```
</details>

---

## 📚 CHAPTER 5: Functions

### 5.1 Function Types

```javascript
// Function declaration (hoisted)
function greet(name) {
    return `Hello, ${name}!`;
}

// Function expression
const greet2 = function(name) {
    return `Hi, ${name}!`;
};

// Arrow function (modern way)
const greet3 = (name) => {
    return `Hey, ${name}!`;
};

// Arrow function shorthand (single expression)
const greet4 = name => `Yo, ${name}!`;

// Arrow with multiple parameters
const add = (a, b) => a + b;
```

### 5.2 Higher-Order Functions

```javascript
// Function that takes a function
function doTwice(fn) {
    fn();
    fn();
}

doTwice(() => console.log("Hello!"));

// Function that returns a function
function multiplier(factor) {
    return (number) => number * factor;
}

const double = multiplier(2);
const triple = multiplier(3);
console.log(double(5));  // 10
console.log(triple(5));  // 15
```

### 🧪 Fill-in-the-Blank: Functions

```javascript
// Create an arrow function that calculates area
const calculateArea = (_______, _______) _______ width * height;

// Create a function that greets with optional loudness
const greet = (name, loud _______ false) => {
    const message = `Hello, ${name}!`;
    return loud _______ message.toUpperCase() _______ message;
};

// Create a higher-order function that repeats an action
const repeat = (times, _______) => {
    for (let i = 0; i < times; i++) {
        _______(i);
    }
};

// Use it
repeat(3, (i) _______ console.log(`Iteration ${i}`));
```

<details>
<summary>💡 Solution</summary>

```javascript
const calculateArea = (width, height) => width * height;

const greet = (name, loud = false) => {
    const message = `Hello, ${name}!`;
    return loud ? message.toUpperCase() : message;
};

const repeat = (times, action) => {
    for (let i = 0; i < times; i++) {
        action(i);
    }
};

repeat(3, (i) => console.log(`Iteration ${i}`));
```
</details>

---

## 📚 CHAPTER 6: Async JavaScript

### 6.1 Promises

```javascript
// Creating a promise
const fetchData = () => {
    return new Promise((resolve, reject) => {
        setTimeout(() => {
            const success = true;
            if (success) {
                resolve({ data: "Hello!" });
            } else {
                reject(new Error("Failed!"));
            }
        }, 1000);
    });
};

// Using promises
fetchData()
    .then(result => console.log(result))
    .catch(error => console.error(error))
    .finally(() => console.log("Done!"));
```

### 6.2 Async/Await

```javascript
// The modern way (much cleaner!)
async function getData() {
    try {
        const response = await fetch("https://api.example.com/data");
        const data = await response.json();
        console.log(data);
        return data;
    } catch (error) {
        console.error("Error:", error);
    }
}

// Arrow function version
const getData2 = async () => {
    try {
        const response = await fetch("https://api.example.com/data");
        return await response.json();
    } catch (error) {
        console.error("Error:", error);
        throw error;
    }
};
```

### 🧪 Fill-in-the-Blank: Async

```javascript
// Create an async function to fetch user data
_______ function fetchUser(userId) {
    _______ {
        const response = _______ fetch(`/api/users/${userId}`);
        
        if (!response._______) {
            _______ new Error("User not found");
        }
        
        const user = _______ response.json();
        _______ user;
    } _______ (error) {
        console.error("Failed to fetch user:", error);
        return _______;
    }
}

// Use the function
const user = _______ fetchUser(123);
```

<details>
<summary>💡 Solution</summary>

```javascript
async function fetchUser(userId) {
    try {
        const response = await fetch(`/api/users/${userId}`);
        
        if (!response.ok) {
            throw new Error("User not found");
        }
        
        const user = await response.json();
        return user;
    } catch (error) {
        console.error("Failed to fetch user:", error);
        return null;
    }
}

const user = await fetchUser(123);
```
</details>

---

## 📚 CHAPTER 7: Classes

### 7.1 Class Basics

```javascript
class Player {
    // Private field (# prefix)
    #secret = "hidden";
    
    constructor(name, score = 0) {
        this.name = name;
        this.score = score;
    }
    
    // Instance method
    addPoints(points) {
        this.score += points;
    }
    
    // Getter
    get displayName() {
        return `🎮 ${this.name}`;
    }
    
    // Setter
    set displayName(value) {
        this.name = value.replace("🎮 ", "");
    }
    
    // Static method
    static createGuest() {
        return new Player("Guest", 0);
    }
}

const player = new Player("Dede", 100);
player.addPoints(50);
console.log(player.displayName);  // 🎮 Dede
```

### 7.2 Inheritance

```javascript
class Character {
    constructor(name, health = 100) {
        this.name = name;
        this.health = health;
    }
    
    takeDamage(amount) {
        this.health -= amount;
        if (this.health < 0) this.health = 0;
    }
}

class Hero extends Character {
    constructor(name, health, superPower) {
        super(name, health);  // Call parent constructor
        this.superPower = superPower;
    }
    
    usePower() {
        console.log(`${this.name} uses ${this.superPower}!`);
    }
}

const hero = new Hero("Dede", 100, "Code Blast");
hero.usePower();
```

---

## 📚 CHAPTER 8: DOM Manipulation

### 8.1 Selecting Elements

```javascript
// By ID
const header = document.getElementById("header");

// By class (returns collection)
const buttons = document.getElementsByClassName("btn");

// Query selector (CSS-style, returns first match)
const firstButton = document.querySelector(".btn");
const allButtons = document.querySelectorAll(".btn");
```

### 8.2 Modifying Elements

```javascript
const element = document.querySelector("#myElement");

// Content
element.textContent = "New text";
element.innerHTML = "<strong>Bold text</strong>";

// Attributes
element.setAttribute("data-id", "123");
element.classList.add("active");
element.classList.remove("hidden");
element.classList.toggle("selected");

// Style
element.style.backgroundColor = "blue";
element.style.display = "none";
```

### 8.3 Events

```javascript
const button = document.querySelector("#myButton");

// Add event listener
button.addEventListener("click", (event) => {
    console.log("Button clicked!");
    event.preventDefault();  // Prevent default action
});

// Remove event listener
const handleClick = () => console.log("Clicked!");
button.addEventListener("click", handleClick);
button.removeEventListener("click", handleClick);
```

---

## 📚 CHAPTER 9: Modules

### 9.1 ES6 Modules

```javascript
// math.js - exporting
export const PI = 3.14159;

export function add(a, b) {
    return a + b;
}

export default class Calculator {
    // default export
}

// main.js - importing
import Calculator, { PI, add } from "./math.js";
import * as MathUtils from "./math.js";

console.log(PI);
console.log(add(2, 3));
console.log(MathUtils.PI);
```

---

## 🏆 FINAL CHALLENGE: Build a Web App

**🗣️ SAY TO AIDE:**
> "Help me build a vanilla JavaScript todo app with:
> - Add, complete, and delete todos
> - Filter by all/active/completed
> - Local storage persistence
> - Clean class-based architecture
> - Event delegation for performance
> - Template literals for HTML generation
> - Proper error handling"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ Use const by default, let when needed, never var      │
│  ✅ Template literals with ${} for string interpolation   │
│  ✅ Arrow functions for cleaner syntax                    │
│  ✅ map/filter/reduce for array transformations           │
│  ✅ Destructuring for cleaner data access                 │
│  ✅ async/await for readable async code                   │
│  ✅ Classes for object-oriented patterns                  │
│  ✅ Modules for code organization                         │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've built your vanilla JS todo app!
