# 🎨 Project 3: Portfolio Website

## 🎁 Reward: Beret for Dede!
Complete this project and Dede becomes an artist with a fancy French beret! Très chic! 🇫🇷

---

## 🌟 What You'll Learn

You're building a personal portfolio website - a place to show off your work. This project teaches:

- ✅ **Multi-page navigation** - how pages connect to each other
- ✅ **Folder organization** - keeping projects clean and logical
- ✅ **CSS layouts** - flexbox and grid for modern designs
- ✅ **Responsive design** - looks good on phone AND computer
- ✅ **Refactoring prompts** - improving code through iteration
- ✅ **README files** - documenting your project for humans AND AI

---

## 📓 NEW SKILL: The README File

Every professional project has a README.md file. It's the first thing people (and AI) see.

**🗣️ SAY TO AIDE:**
> "Create a README.md file for my portfolio project"

**Your README should include:**

```markdown
# My Portfolio Website

## Description
A personal portfolio website showcasing my projects and skills.

## Pages
- **Home** (index.html) - Hero section, featured work
- **About** (about.html) - Bio, skills, photo
- **Projects** (projects.html) - Full project gallery
- **Contact** (contact.html) - Contact form

## Tech Stack
- HTML5
- CSS3 (Flexbox, Grid, Custom Properties)
- Vanilla JavaScript

## File Structure
portfolio/
├── index.html
├── about.html
├── projects.html
├── contact.html
├── css/styles.css
├── js/main.js
└── images/

## How to Run
Open index.html in a browser

## Current Status
- [x] Homepage complete
- [ ] About page in progress
- [ ] Projects page TODO
- [ ] Contact form TODO
```

**🧠 WHY README MATTERS FOR AI:**
When you paste README content to AI, it instantly understands:
- What your project IS
- What files exist
- What's done vs TODO
- What tech you're using

**📓 UPDATE README as you work!** It becomes your context feeder.

---

## 🧠 New Concept: File Organization

Professional projects have a structure:

```
portfolio/
├── index.html          ← Homepage
├── about.html          ← About page
├── projects.html       ← Projects page
├── contact.html        ← Contact page
├── css/
│   └── styles.css      ← All styles
├── js/
│   └── main.js         ← All scripts
└── images/
    ├── profile.jpg     ← Your photo
    └── project1.png    ← Project screenshots
```

> 💡 **Why organize?**
> - Finding files is instant
> - Other developers understand your project
> - Easier to update/maintain
> - Looks professional

---

## 🗺️ Phase 1: Planning Your Portfolio

### 📝 Your Turn: Content First

Before ANY code, answer these:

1. **Who are you?** (One sentence bio)
2. **What do you make?** (Type of work)
3. **What projects will you show?** (List 3-4)
4. **How can people contact you?** (Email, social links)

Write these down. You'll tell AIDE this info later.

### Page Planning

| Page | Purpose | Main Content |
|------|---------|--------------|
| Home | First impression | Hero banner, quick intro, featured work |
| About | Who you are | Bio, skills, photo |
| Projects | Your work | Project cards with images and descriptions |
| Contact | Get in touch | Contact form or links |

---

## 🗣️ Phase 2: Project Setup

### Step 1: Create the Folder Structure

**🗣️ SAY TO AIDE:**
> "Create a portfolio project with this structure: index.html at root, plus about.html, projects.html, and contact.html. Create folders for css, js, and images. Put styles.css in css folder and main.js in js folder."

**👀 WATCH FOR:**
- All files created in correct folders
- Folder structure matches the plan

**⚠️ IF AIDE PUTS FILES IN WRONG PLACES:**

**🗣️ SAY:**
> "Move styles.css into the css folder. It should be at css/styles.css, not in the root."

---

### Step 2: Git Init

**🗣️ SAY TO AIDE:**
> "Initialize git and add a .gitignore that ignores .DS_Store, node_modules, and any .env files"

---

## 🗣️ Phase 3: The Homepage

### Step 1: HTML Boilerplate with Correct Paths

**🗣️ SAY TO AIDE:**
> "In index.html, create an HTML5 document. Link to css/styles.css in the head and js/main.js at the end of body. The title should be 'Your Name - Portfolio'"

**👀 CHECK THE PATHS:**

✅ CSS path is `css/styles.css` (with folder)
✅ JS path is `js/main.js` (with folder)

```html
<link rel="stylesheet" href="css/styles.css">
...
<script src="js/main.js"></script>
```

**🧠 PATH BASICS:**
- `css/styles.css` = look in css folder, find styles.css
- `./file.html` = same folder
- `../file.html` = parent folder

---

### Step 2: Navigation

**🗣️ SAY TO AIDE:**
> "Add a navigation bar with links to all four pages: Home, About, Projects, and Contact. Use a nav element with an unordered list inside. Add a class 'active' to the Home link since we're on the homepage."

**👀 CHECK:**

✅ Uses semantic `<nav>` element
✅ Links have correct hrefs:
```html
<a href="index.html">Home</a>
<a href="about.html">About</a>
<a href="projects.html">Projects</a>
<a href="contact.html">Contact</a>
```
✅ Home link has `class="active"`

---

### Step 3: Hero Section

**🗣️ SAY TO AIDE:**
> "Add a hero section with a large heading that has my name, a short tagline paragraph underneath, and a call-to-action button that links to the projects page."

**👀 WATCH FOR:**

✅ Uses `<section>` with a class like `hero`
✅ Heading is `<h1>` (only one h1 per page!)
✅ Button links to projects.html

---

### Step 4: Featured Work Preview

**🗣️ SAY TO AIDE:**
> "Add a section below the hero with heading 'Featured Work' and three project card divs. Each card should have a placeholder image, project title, and short description. Add a 'View All' link to the projects page."

---

### Step 5: Footer

**🗣️ SAY TO AIDE:**
> "Add a footer with copyright text and social media links. Include placeholders for GitHub, LinkedIn, and Twitter."

---

### First Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add homepage structure with nav, hero, featured work, and footer'"

---

## 🗣️ Phase 4: CSS - Making It Beautiful

### Step 1: CSS Reset and Base Styles

**🗣️ SAY TO AIDE:**
> "In css/styles.css, add a modern CSS reset. Set box-sizing to border-box on everything. Use a clean sans-serif font stack. Define CSS custom properties for your color scheme - a primary color, secondary color, text color, and background color."

**👀 CHECK FOR:**

✅ Box-sizing reset:
```css
*, *::before, *::after {
    box-sizing: border-box;
}
```

✅ CSS Custom Properties (variables):
```css
:root {
    --primary-color: #3498db;
    --secondary-color: #2c3e50;
    --text-color: #333;
    --background-color: #fff;
}
```

**🧠 WHY CSS VARIABLES?**
Want to change your color scheme? Change ONE place, updates everywhere.

---

### Step 2: Navigation Styling

**🗣️ SAY TO AIDE:**
> "Style the navigation as a horizontal bar. Use flexbox to space the nav items. The active link should have a different color or underline. On hover, links should have a visual change. Make it stick to the top of the page when scrolling."

**👀 CHECK:**

✅ Uses `display: flex` with `justify-content`
✅ Has `:hover` styles
✅ Has `.active` styles
✅ Uses `position: sticky; top: 0;` for sticky nav

---

### 📝 Your Turn: Flexbox Properties

Match each property to what it does:

| Property | Effect |
|----------|--------|
| `justify-content: space-between` | ___ |
| `align-items: center` | ___ |
| `gap: 20px` | ___ |

**A.** Vertical centering
**B.** Space between items (first/last at edges)
**C.** Fixed space between items

<details>
<summary>Click for answer</summary>

- `justify-content: space-between` = **B** (first and last at edges, equal space between)
- `align-items: center` = **A** (centers vertically)
- `gap: 20px` = **C** (20px between each item)
</details>

---

### Step 3: Hero Section Styling

**🗣️ SAY TO AIDE:**
> "Style the hero section to be full viewport height minus the nav height. Center everything vertically and horizontally. The heading should be large, the tagline smaller, and the button should be styled as a prominent call-to-action with hover effects."

**👀 CHECK:**

✅ Uses `min-height: calc(100vh - 60px)` or similar
✅ Centers with flexbox
✅ Button has padding, background color, and `:hover`

---

### Step 4: Project Cards with Grid

**🗣️ SAY TO AIDE:**
> "Style the featured work section. Use CSS Grid to display the project cards in a 3-column layout. Cards should have an image at top, padding, subtle shadow, and rounded corners. On hover, cards should lift slightly with a shadow change."

**👀 CHECK FOR:**

✅ Grid layout: `display: grid; grid-template-columns: repeat(3, 1fr);`
✅ Card images fill width: `width: 100%;`
✅ Hover effect with `transform` and `box-shadow`

---

### Step 5: Responsive Design

**🗣️ SAY TO AIDE:**
> "Add media queries to make the design responsive. On tablets under 900px, make the project grid 2 columns. On phones under 600px, make it 1 column. Also make the navigation stack vertically on phones."

**👀 CHECK:**

✅ Has at least two `@media` breakpoints
✅ Grid columns change at each breakpoint
✅ Nav behavior changes on mobile

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add responsive CSS with flexbox nav and grid project cards'"

---

## 🗣️ Phase 5: Other Pages

### The DRY Problem

DRY = Don't Repeat Yourself

If you copy-paste the nav and footer to every page, you have 4 copies. Change the nav? Change it 4 times. Forget one? Broken links.

### Option 1: Copy Carefully (Simple)

**🗣️ SAY TO AIDE:**
> "Copy the navigation and footer from index.html to about.html, projects.html, and contact.html. Update the 'active' class on each page's corresponding nav link."

**👀 CHECK:**
- about.html has `class="active"` on About link
- projects.html has `class="active"` on Projects link
- contact.html has `class="active"` on Contact link

### Option 2: JavaScript Include (Advanced)

**🗣️ SAY TO AIDE (if you want to learn):**
> "Instead of copying, create nav.html and footer.html as partial files. Write JavaScript in main.js that loads these partials into placeholder divs on each page."

---

### About Page Content

**🗣️ SAY TO AIDE:**
> "In about.html, after the nav add a section with a two-column layout: left column has my profile image placeholder, right column has an h1 with 'About Me', a few paragraphs of bio text, and a skills list."

---

### Projects Page Content

**🗣️ SAY TO AIDE:**
> "In projects.html, add a section with heading 'My Projects' and a grid of 6 project cards. Each card needs: image, title, description, technologies used tags, and links to live demo and GitHub."

---

### Contact Page Content

**🗣️ SAY TO AIDE:**
> "In contact.html, add a section with heading 'Get In Touch'. Include a contact form with fields for name, email, and message, plus a submit button. Below the form, show alternative contact methods like email and social links."

**⚠️ NOTE:** The form won't actually send emails without a backend. For now it's just the design.

---

### Commit All Pages

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add about, projects, and contact pages'"

---

## ⚠️ Common AI Problem: Refactoring Gone Wrong

You might ask AIDE to "improve" or "refactor" something and it breaks.

**🚨 SIGNS:**
- Code that was working now breaks
- Styling looks completely different
- Features disappear

**🗣️ HOW TO RECOVER:**

Option 1 - Undo:
> "Undo the last change"

Option 2 - Git to the rescue:
> "Discard all changes since the last commit"

Option 3 - Be specific:
> "Only change the hover effect on project cards. Don't touch anything else."

> 💡 **AI Lesson:** "Improve this" is too vague. AI might "improve" things you liked. Be specific about what to change.

---

## 🧪 Phase 6: Testing & Polish

### Cross-Page Testing

Test each navigation link:
1. Click Home → should go to index.html
2. Click About → should go to about.html
3. Click Projects → should go to projects.html
4. Click Contact → should go to contact.html

### Responsive Testing

**🗣️ SAY TO AIDE:**
> "Open the site in the browser and show me how to test responsive design"

Or manually:
1. Open in browser
2. Right-click → Inspect
3. Toggle device toolbar (phone icon)
4. Test different screen sizes

### Fix What's Broken

**Specific prompts work best:**

❌ "The mobile version looks bad"
✅ "On mobile, the nav items overlap. Add more spacing between them."

❌ "Fix the layout"
✅ "On tablet width, the project cards are too narrow. Make them 2 columns instead of 3."

---

## 🗣️ Phase 7: Final Polish

### Add Smooth Scrolling

**🗣️ SAY TO AIDE:**
> "Add smooth scrolling behavior to the whole site and a subtle fade-in animation for sections as the page loads"

---

### Add Favicon

**🗣️ SAY TO AIDE:**
> "Add a link in the head for a favicon. Just use a placeholder emoji favicon for now."

---

### Optimize Images

**🗣️ SAY TO AIDE:**
> "Add width and height attributes to all images to prevent layout shift while loading"

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add animations, favicon, and image optimizations'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Be specific about what to change vs what to leave alone
- ✅ Use Git to recover from bad changes
- ✅ Test across pages and screen sizes

### Code Concepts:
- ✅ **File organization** - folders for css, js, images
- ✅ **Multi-page navigation** - linking pages together
- ✅ **Flexbox** - 1D layouts (nav, centering)
- ✅ **CSS Grid** - 2D layouts (card grids)
- ✅ **Responsive design** - media queries
- ✅ **CSS Variables** - consistent theming

### Project Skills:
- ✅ Plan content before code
- ✅ Commit after each working feature
- ✅ Test navigation between pages
- ✅ Test on different screen sizes

---

## 🎁 Unlock Your Reward!

Dede is now rocking an artistic beret! Très magnifique! 🎨

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Dark Mode** - "Add a toggle that switches between light and dark color schemes"
2. **Animated Background** - "Add subtle animated shapes or gradients to the hero section"
3. **Project Filtering** - "Add filter buttons to show only certain types of projects"
4. **Contact Form Validation** - "Add JavaScript validation for email format before submit"

---

## 📖 Quick Reference

### File Paths

| From | To | Path |
|------|----|------|
| index.html | css/styles.css | `css/styles.css` |
| index.html | about.html | `about.html` |
| about.html | index.html | `index.html` |
| css/styles.css | images/photo.jpg | `../images/photo.jpg` |

### Flexbox vs Grid

| Use Case | Tool |
|----------|------|
| Navigation bar | Flexbox |
| Centering something | Flexbox |
| Card grid layout | Grid |
| Two-column layout | Either (Grid is easier) |
| Unknown number of items in a row | Flexbox |

### Common Breakpoints

| Width | Device |
|-------|--------|
| 600px | Phones |
| 900px | Tablets |
| 1200px | Laptops |
| 1400px+ | Large screens |

---

*Next up: Project 4 - Virtual Smart Home! 🏠 + 🤖*
