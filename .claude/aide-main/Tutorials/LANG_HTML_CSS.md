# 📙 HTML & CSS Fundamentals
## *The Building Blocks of the Web*

---

## 🎁 Reward: Painter's Palette for Dede!
Complete this book and Dede gets an artistic palette accessory!

---

## 🌟 Why HTML & CSS?

```
┌─────────────────────────────────────────────────────────────────┐
│               THE FOUNDATION OF EVERY WEBSITE                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   HTML = Structure (The Skeleton)                               │
│   "What things ARE on the page"                                 │
│   • Headings, paragraphs, images, links                        │
│   • Forms, buttons, lists, tables                              │
│                                                                 │
│   CSS = Style (The Clothes)                                     │
│   "How things LOOK on the page"                                 │
│   • Colors, fonts, sizes, spacing                              │
│   • Layouts, animations, responsiveness                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📚 PART 1: HTML

### CHAPTER 1: HTML Basics

#### 1.1 Document Structure

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>My Website</title>
    <link rel="stylesheet" href="styles.css">
</head>
<body>
    <!-- Your content goes here -->
    <h1>Hello, World!</h1>
</body>
</html>
```

#### 1.2 Common Elements

```html
<!-- Headings (h1 is most important, h6 least) -->
<h1>Main Title</h1>
<h2>Section Title</h2>
<h3>Subsection</h3>

<!-- Paragraphs and text -->
<p>This is a paragraph of text.</p>
<strong>Bold text</strong>
<em>Italic text</em>
<br> <!-- Line break -->

<!-- Links -->
<a href="https://example.com">Click here</a>
<a href="#section">Jump to section</a>

<!-- Images -->
<img src="image.jpg" alt="Description of image">
```

### 🧪 Fill-in-the-Blank: HTML Structure

```html
<!________ html>
<________ lang="en">
<______>
    <meta ________="UTF-8">
    <________>My Cool Website</______>
</head>
<______>
    <____>Welcome!</__>
    <____>This is my first website.</____>
    <____ href="about.html">About Me</____>
</body>
</html>
```

<details>
<summary>💡 Solution</summary>

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>My Cool Website</title>
</head>
<body>
    <h1>Welcome!</h1>
    <p>This is my first website.</p>
    <a href="about.html">About Me</a>
</body>
</html>
```
</details>

---

### CHAPTER 2: Semantic HTML

```html
<!-- Use meaningful tags for better accessibility and SEO -->

<header>
    <nav>
        <ul>
            <li><a href="/">Home</a></li>
            <li><a href="/about">About</a></li>
        </ul>
    </nav>
</header>

<main>
    <article>
        <h1>Article Title</h1>
        <p>Article content...</p>
    </article>
    
    <aside>
        <h3>Related Links</h3>
    </aside>
</main>

<footer>
    <p>&copy; 2024 My Website</p>
</footer>
```

---

### CHAPTER 3: Forms

```html
<form action="/submit" method="POST">
    <!-- Text input -->
    <label for="name">Name:</label>
    <input type="text" id="name" name="name" required>
    
    <!-- Email -->
    <input type="email" placeholder="your@email.com">
    
    <!-- Password -->
    <input type="password" minlength="8">
    
    <!-- Number -->
    <input type="number" min="0" max="100" step="5">
    
    <!-- Textarea -->
    <textarea rows="4" cols="50"></textarea>
    
    <!-- Select dropdown -->
    <select name="color">
        <option value="red">Red</option>
        <option value="blue" selected>Blue</option>
    </select>
    
    <!-- Checkbox -->
    <input type="checkbox" id="agree" checked>
    <label for="agree">I agree</label>
    
    <!-- Radio buttons -->
    <input type="radio" name="size" value="small"> Small
    <input type="radio" name="size" value="large"> Large
    
    <!-- Submit -->
    <button type="submit">Submit</button>
</form>
```

### 🧪 Fill-in-the-Blank: Forms

```html
<form ________="/signup" ________="POST">
    <________ for="email">Email:</label>
    <________ type="________" id="email" name="email" ________>
    
    <label for="password">Password:</label>
    <input ________="password" id="password" ________="8">
    
    <input type="________" id="terms">
    <label ________="terms">I accept the terms</label>
    
    <________ type="________">Create Account</button>
</form>
```

<details>
<summary>💡 Solution</summary>

```html
<form action="/signup" method="POST">
    <label for="email">Email:</label>
    <input type="email" id="email" name="email" required>
    
    <label for="password">Password:</label>
    <input type="password" id="password" minlength="8">
    
    <input type="checkbox" id="terms">
    <label for="terms">I accept the terms</label>
    
    <button type="submit">Create Account</button>
</form>
```
</details>

---

### CHAPTER 4: Lists and Tables

```html
<!-- Unordered list -->
<ul>
    <li>First item</li>
    <li>Second item</li>
</ul>

<!-- Ordered list -->
<ol>
    <li>Step one</li>
    <li>Step two</li>
</ol>

<!-- Tables -->
<table>
    <thead>
        <tr>
            <th>Name</th>
            <th>Age</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>Dede</td>
            <td>5</td>
        </tr>
    </tbody>
</table>
```

---

## 📚 PART 2: CSS

### CHAPTER 5: CSS Basics

#### 5.1 Three Ways to Add CSS

```html
<!-- 1. Inline (avoid) -->
<p style="color: red;">Red text</p>

<!-- 2. Internal (in <head>) -->
<style>
    p { color: blue; }
</style>

<!-- 3. External (best!) -->
<link rel="stylesheet" href="styles.css">
```

#### 5.2 Selectors

```css
/* Element selector */
p {
    color: blue;
}

/* Class selector (.) */
.highlight {
    background: yellow;
}

/* ID selector (#) */
#header {
    font-size: 24px;
}

/* Descendant selector */
article p {
    line-height: 1.6;
}

/* Child selector */
ul > li {
    list-style: none;
}

/* Multiple selectors */
h1, h2, h3 {
    font-family: Arial;
}

/* Pseudo-classes */
a:hover {
    color: red;
}

button:active {
    transform: scale(0.95);
}
```

### 🧪 Fill-in-the-Blank: Selectors

```css
/* Select all paragraphs */
________ {
    font-size: 16px;
}

/* Select element with class "card" */
________ {
    padding: 20px;
    border: 1px solid gray;
}

/* Select element with ID "main-title" */
________ {
    font-size: 32px;
}

/* Select links when hovered */
a________ {
    text-decoration: underline;
}

/* Select all h1, h2, and h3 */
________, ________, ________ {
    font-family: sans-serif;
}
```

<details>
<summary>💡 Solution</summary>

```css
p {
    font-size: 16px;
}

.card {
    padding: 20px;
    border: 1px solid gray;
}

#main-title {
    font-size: 32px;
}

a:hover {
    text-decoration: underline;
}

h1, h2, h3 {
    font-family: sans-serif;
}
```
</details>

---

### CHAPTER 6: The Box Model

```
┌─────────────────────────────────────────────────────────┐
│                        MARGIN                           │
│   ┌─────────────────────────────────────────────────┐   │
│   │                    BORDER                       │   │
│   │   ┌─────────────────────────────────────────┐   │   │
│   │   │               PADDING                   │   │   │
│   │   │   ┌─────────────────────────────────┐   │   │   │
│   │   │   │                                 │   │   │   │
│   │   │   │            CONTENT              │   │   │   │
│   │   │   │                                 │   │   │   │
│   │   │   └─────────────────────────────────┘   │   │   │
│   │   │                                         │   │   │
│   │   └─────────────────────────────────────────┘   │   │
│   │                                                 │   │
│   └─────────────────────────────────────────────────┘   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

```css
.box {
    /* Content size */
    width: 300px;
    height: 200px;
    
    /* Padding (inside) */
    padding: 20px;           /* all sides */
    padding: 10px 20px;      /* vertical horizontal */
    padding: 5px 10px 15px 20px; /* top right bottom left */
    
    /* Border */
    border: 2px solid black;
    border-radius: 10px;     /* rounded corners */
    
    /* Margin (outside) */
    margin: 20px;
    margin: 0 auto;          /* center horizontally */
    
    /* Box sizing (include padding/border in width) */
    box-sizing: border-box;
}
```

### 🧪 Fill-in-the-Blank: Box Model

```css
.card {
    /* Make the card 400px wide including padding and border */
    ________-sizing: ________;
    width: 400px;
    
    /* Add 20px padding on all sides */
    ________: 20px;
    
    /* Add a 2px solid gray border */
    ________: ________ ________ ________;
    
    /* Round the corners by 8px */
    border-________: 8px;
    
    /* Add 20px margin on top/bottom, center horizontally */
    ________: 20px ________;
}
```

<details>
<summary>💡 Solution</summary>

```css
.card {
    box-sizing: border-box;
    width: 400px;
    padding: 20px;
    border: 2px solid gray;
    border-radius: 8px;
    margin: 20px auto;
}
```
</details>

---

### CHAPTER 7: Flexbox

```css
.container {
    display: flex;
    
    /* Main axis direction */
    flex-direction: row;        /* or column */
    
    /* Main axis alignment */
    justify-content: center;    /* flex-start, flex-end, space-between, space-around */
    
    /* Cross axis alignment */
    align-items: center;        /* flex-start, flex-end, stretch */
    
    /* Wrap items */
    flex-wrap: wrap;
    
    /* Gap between items */
    gap: 20px;
}

.item {
    flex: 1;                    /* grow to fill space equally */
    flex-grow: 1;               /* grow factor */
    flex-shrink: 0;             /* don't shrink */
    flex-basis: 200px;          /* initial size */
}
```

### Common Flexbox Patterns

```css
/* Center everything */
.center-all {
    display: flex;
    justify-content: center;
    align-items: center;
}

/* Space items evenly */
.navbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
}

/* Stack vertically, centered */
.card {
    display: flex;
    flex-direction: column;
    align-items: center;
}
```

### 🧪 Fill-in-the-Blank: Flexbox

```css
/* Navigation bar with logo left, links right */
.navbar {
    ________: flex;
    ________-content: ________;
    ________-items: center;
    padding: 10px 20px;
}

/* Card grid with 3 equal columns */
.card-grid {
    display: ________;
    flex-________: wrap;
    ________: 20px;
}

.card {
    ________: 1;
    ________-basis: 300px;
}

/* Perfectly centered content */
.hero {
    display: flex;
    ________: center;
    ________: center;
    min-height: 100vh;
}
```

<details>
<summary>💡 Solution</summary>

```css
.navbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 20px;
}

.card-grid {
    display: flex;
    flex-wrap: wrap;
    gap: 20px;
}

.card {
    flex: 1;
    flex-basis: 300px;
}

.hero {
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
}
```
</details>

---

### CHAPTER 8: CSS Grid

```css
.grid-container {
    display: grid;
    
    /* Define columns */
    grid-template-columns: 1fr 1fr 1fr;     /* 3 equal columns */
    grid-template-columns: repeat(3, 1fr);  /* same as above */
    grid-template-columns: 200px 1fr 200px; /* fixed-flex-fixed */
    
    /* Define rows */
    grid-template-rows: 100px auto 100px;
    
    /* Gap */
    gap: 20px;
    row-gap: 10px;
    column-gap: 20px;
}

.grid-item {
    /* Span multiple columns/rows */
    grid-column: span 2;
    grid-row: 1 / 3;
}
```

### Common Grid Patterns

```css
/* 12-column grid */
.grid-12 {
    display: grid;
    grid-template-columns: repeat(12, 1fr);
    gap: 20px;
}

/* Responsive card grid */
.cards {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 20px;
}
```

---

### CHAPTER 9: Responsive Design

```css
/* Mobile-first approach */
.container {
    width: 100%;
    padding: 10px;
}

/* Tablet and up */
@media (min-width: 768px) {
    .container {
        max-width: 750px;
        margin: 0 auto;
    }
}

/* Desktop and up */
@media (min-width: 1024px) {
    .container {
        max-width: 960px;
    }
}

/* Large screens */
@media (min-width: 1200px) {
    .container {
        max-width: 1140px;
    }
}
```

### 🧪 Fill-in-the-Blank: Responsive

```css
/* Base styles (mobile) */
.card-grid {
    display: grid;
    grid-template-columns: ________(1fr);
    gap: 15px;
}

/* Tablet: 2 columns */
@________ (________-width: 768px) {
    .card-grid {
        grid-template-columns: repeat(________, 1fr);
    }
}

/* Desktop: auto-fit with minimum 300px */
@media (min-width: ________px) {
    .card-grid {
        grid-template-columns: repeat(________, minmax(300px, ________));
    }
}
```

<details>
<summary>💡 Solution</summary>

```css
.card-grid {
    display: grid;
    grid-template-columns: repeat(1fr);
    gap: 15px;
}

@media (min-width: 768px) {
    .card-grid {
        grid-template-columns: repeat(2, 1fr);
    }
}

@media (min-width: 1024px) {
    .card-grid {
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    }
}
```
</details>

---

### CHAPTER 10: CSS Variables & Modern Features

```css
:root {
    /* Define variables */
    --primary-color: #3b82f6;
    --secondary-color: #10b981;
    --spacing: 1rem;
    --border-radius: 8px;
}

.button {
    /* Use variables */
    background: var(--primary-color);
    padding: var(--spacing);
    border-radius: var(--border-radius);
    
    /* Transitions */
    transition: all 0.3s ease;
}

.button:hover {
    background: var(--secondary-color);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}
```

---

## 🏆 FINAL CHALLENGE: Build a Landing Page

**🗣️ SAY TO AIDE:**
> "Help me build a responsive landing page with:
> - Fixed navigation bar with logo and links
> - Hero section with centered text and CTA button
> - Features section with 3-column grid
> - Testimonials with cards
> - Footer with multiple columns
> - CSS variables for colors and spacing
> - Mobile-first responsive design
> - Smooth hover transitions"

---

## 🎯 KEY TAKEAWAYS

```
┌────────────────────────────────────────────────────────────┐
│  ✅ HTML is structure, CSS is style                        │
│  ✅ Use semantic HTML for accessibility                    │
│  ✅ External CSS is best for maintainability               │
│  ✅ box-sizing: border-box makes sizing intuitive          │
│  ✅ Flexbox for 1D layouts, Grid for 2D layouts            │
│  ✅ Mobile-first with min-width media queries              │
│  ✅ CSS variables for consistent theming                   │
│  ✅ Always include alt text on images                      │
└────────────────────────────────────────────────────────────┘
```

---

**[  ] Mark Complete** when you've built your responsive landing page!
