# 🍳 Project 11: Recipe Book App

## 🎁 Reward: Chef Hat for Dede!
Complete this project and Dede becomes a master chef! 👨‍🍳

---

## 🌟 What You'll Learn

Build a recipe collection app with ingredients, steps, and meal planning. This project teaches:

- ✅ **Complex data structures** - nested arrays, related data
- ✅ **CRUD on nested data** - editing arrays within objects
- ✅ **Scaling calculations** - math for serving adjustments
- ✅ **Print-friendly design** - CSS for printing
- ✅ **Related data** - meal plans referencing recipes
- ✅ **Code review with AI** - having AI check your work

---

## 📓 NEW SKILL: Code Review with AI

You've been telling AI what to write. Now flip it: have AI **review** your code.

**Technique 1: Ask for Problems**

**🗣️ SAY TO AIDE:**
> "Review my app.js file. What bugs, edge cases, or improvements do you see? Don't fix them yet - just list them."

This helps you learn to spot issues yourself.

**Technique 2: Ask About Specific Concerns**

**🗣️ SAY:**
> "Look at my deleteIngredient function. Will it cause problems if the ingredient doesn't exist? What about if ingredients array is empty?"

**Technique 3: Request a Refactor Review**

After your app works, ask:
> "My code works but feels messy. Suggest refactoring to make it cleaner. Explain each suggestion but don't change anything yet."

**Example AI Review Response:**
```
I see a few opportunities:

1. **Repeated DOM selection** - You call getElementById('recipe-list') 
   in 5 places. Cache it in a variable at the top.

2. **Magic numbers** - What's 4 in `servings * 4`? Name it:
   const BASE_SERVINGS = 4;

3. **Missing error handling** - loadRecipe(id) doesn't check 
   if recipe exists before using it.

4. **Long function** - saveRecipe() is 60 lines. Split into 
   validateRecipe(), updateRecipe(), and saveToStorage().
```

**🧠 LEARNING FROM REVIEWS:**
Don't just let AI fix everything. Read the review, understand WHY, then decide what to change yourself.

**Technique 4: Compare to Best Practices**

**🗣️ SAY:**
> "Does my code follow JavaScript best practices? Check for: const vs let usage, error handling, naming conventions, and code organization."

---

## 🧠 New Concept: Nested Data

A recipe has arrays inside it:

```javascript
const recipe = {
    id: 'r1',
    name: 'Pancakes',
    servings: 4,
    ingredients: [
        { name: 'flour', amount: 2, unit: 'cups' },
        { name: 'eggs', amount: 2, unit: 'whole' },
        { name: 'milk', amount: 1.5, unit: 'cups' }
    ],
    steps: [
        'Mix dry ingredients',
        'Add wet ingredients',
        'Cook on griddle'
    ]
};
```

**Editing this means editing arrays inside the object.**

---

## ✏️ FILL IN THE BLANKS: Data Structure Practice

Before we start, let's make sure you understand how to work with nested data!

### Exercise 1: Accessing Nested Data

```javascript
// Given this recipe object:
const recipe = {
    name: 'Pancakes',
    servings: 4,
    ingredients: [
        { name: 'flour', amount: 2, unit: 'cups' }
    ]
};

// How do you get the first ingredient's name?
const firstIngredientName = recipe._____________[0]._____________;

// How do you get the amount?
const firstAmount = recipe.ingredients[___]._____________;
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const firstIngredientName = recipe.ingredients[0].name;
const firstAmount = recipe.ingredients[0].amount;
```
</details>

---

### Exercise 2: Adding to Nested Arrays

```javascript
// Add a new ingredient to the recipe
const newIngredient = { name: 'sugar', amount: 0.5, unit: 'cups' };

// Which method adds to the end of an array?
recipe.ingredients._____________(newIngredient);

// What if you want to add at the beginning?
recipe.ingredients._____________(newIngredient);
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
recipe.ingredients.push(newIngredient);    // end
recipe.ingredients.unshift(newIngredient); // beginning
```
</details>

---

### Exercise 3: Updating Nested Data

```javascript
// Update the second ingredient's amount to 3
recipe.ingredients[___].__________ = 3;

// Update using find (if you know the name)
const flour = recipe.ingredients._______(i => i.name === 'flour');
if (flour) {
    flour.__________ = 2.5;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
recipe.ingredients[1].amount = 3;

const flour = recipe.ingredients.find(i => i.name === 'flour');
if (flour) {
    flour.amount = 2.5;
}
```
</details>

---

### Exercise 4: Removing from Nested Arrays

```javascript
// Remove ingredient at index 2
recipe.ingredients.__________(2, 1);  // (start index, count to remove)

// Remove ingredient by name - filter creates a new array WITHOUT the item
recipe.ingredients = recipe.ingredients.__________(
    i => i.name !== 'sugar'
);
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
recipe.ingredients.splice(2, 1);

recipe.ingredients = recipe.ingredients.filter(
    i => i.name !== 'sugar'
);
```
</details>

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: Recipe Structure

Decide what your recipes will track:
- [ ] Name, description
- [ ] Prep time, cook time
- [ ] Servings
- [ ] Ingredients (name, amount, unit)
- [ ] Steps/instructions
- [ ] Categories (breakfast, dinner, etc.)
- [ ] Tags (vegetarian, quick, etc.)
- [ ] Notes
- [ ] Image

---

## 🗣️ Phase 2: Project Setup

**🗣️ SAY TO AIDE:**
> "Create a recipe-book project with index.html, css/styles.css, js/app.js, and js/sample-recipes.js with some starter recipes. Initialize git."

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Main Layout

**🗣️ SAY TO AIDE:**
> "Create a recipe app layout. Sidebar has category filter and recipe list. Main area shows selected recipe details or recipe editor. Header has search and 'Add Recipe' button."

---

### Step 2: Recipe List

**🗣️ SAY TO AIDE:**
> "Recipe list sidebar shows recipe cards with name, category badge, prep time, and small image thumbnail. Clicking a recipe loads it in the main view."

---

### Step 3: Recipe Detail View

**🗣️ SAY TO AIDE:**
> "Recipe detail view shows: large image, name, description, timing info, serving size with adjuster controls, ingredients list that updates with servings, numbered steps, and edit/delete buttons."

---

### Step 4: Servings Adjuster

**🗣️ SAY TO AIDE:**
> "Add serving size controls - minus and plus buttons with current serving count. When changed, ALL ingredient amounts should recalculate proportionally."

---

### Step 5: Recipe Editor

**🗣️ SAY TO AIDE:**
> "Create a recipe editor form with: text inputs for name and description, number inputs for times and servings, a dynamic ingredient list where you can add/remove ingredients each with name amount and unit fields, a dynamic steps list where you can add/remove/reorder steps, category selector, and save/cancel buttons."

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add recipe book HTML structure'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Cookbook Theme

**🗣️ SAY TO AIDE:**
> "Style with a warm, kitchen-friendly theme. Earthy colors, maybe a subtle paper texture for recipe cards. Good typography for readability. Food photos should look appetizing."

---

### Step 2: Recipe Cards

**🗣️ SAY TO AIDE:**
> "Style recipe list cards to show image as background with a gradient overlay for text. Quick-glance info visible. Hover effect invites clicking."

---

### Step 3: Ingredients List

**🗣️ SAY TO AIDE:**
> "Style ingredients as a clean checklist. Each ingredient on its own line with amount and unit clearly formatted. Maybe add checkbox for cooking along."

---

### Step 4: Steps Styling

**🗣️ SAY TO AIDE:**
> "Style recipe steps as numbered cards or large numbered list. Each step should be easy to read with good spacing. Maybe add ability to check off completed steps."

---

### Step 5: Print Styles

**🗣️ SAY TO AIDE:**
> "Add print styles using @media print. When printing a recipe, hide the sidebar and navigation. Show only the recipe content in a clean, ink-friendly format."

**🧠 PRINT CSS:**
```css
@media print {
    .sidebar, .header, .buttons {
        display: none;
    }
    .recipe-content {
        width: 100%;
    }
}
```

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add cookbook styling with print support'"

---

## 🗣️ Phase 5: JavaScript - Data & Display

### Step 1: State Structure

**🗣️ SAY TO AIDE:**
> "Create state with recipes array, currentRecipeId for selected recipe, and editMode boolean. Load recipes from localStorage with sample recipes as fallback."

---

### Step 2: Render Recipe List

**🗣️ SAY TO AIDE:**
> "Create renderRecipeList that displays all recipes in sidebar. Support filtering by category and search term. Highlight currently selected recipe."

---

### Step 3: Render Recipe Detail

**🗣️ SAY TO AIDE:**
> "Create renderRecipeDetail that displays the selected recipe with all its information. Use the original servings as base and calculate adjusted amounts based on current serving selection."

---

### Step 4: Serving Calculator

**🗣️ SAY TO AIDE:**
> "Create calculateIngredients function that takes original recipe and desired servings, returns ingredients array with adjusted amounts. Formula: newAmount = originalAmount × (desiredServings / originalServings)."

**👀 CHECK:**

✅ Maintains original recipe data unchanged
✅ Returns NEW array with calculated values
✅ Handles fractions nicely (maybe round to 1 decimal)

---

## ✏️ FILL IN THE BLANKS: Scaling Function

### Exercise 5: Write the Scaling Logic

Complete this function that scales ingredients:

```javascript
function scaleIngredients(recipe, newServings) {
    // Calculate the scale factor
    const scaleFactor = newServings / recipe.______________;
    
    // Return a NEW array with scaled amounts (don't modify original!)
    return recipe.ingredients._______(ingredient => {
        return {
            ...ingredient,  // spread to copy all properties
            amount: ingredient.__________ * ______________
        };
    });
}

// Usage:
const pancakes = { servings: 4, ingredients: [...] };
const scaledForSix = scaleIngredients(pancakes, ___);
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function scaleIngredients(recipe, newServings) {
    const scaleFactor = newServings / recipe.servings;
    
    return recipe.ingredients.map(ingredient => {
        return {
            ...ingredient,
            amount: ingredient.amount * scaleFactor
        };
    });
}

const scaledForSix = scaleIngredients(pancakes, 6);
```

**Key Points:**
- `map()` creates a NEW array (doesn't mutate original)
- `...ingredient` spreads existing properties
- We only override `amount` with the new value
</details>

---

### Exercise 6: Rounding for Nice Display

```javascript
function scaleIngredients(recipe, newServings) {
    const scaleFactor = newServings / recipe.servings;
    
    return recipe.ingredients.map(ingredient => {
        // Round to 1 decimal place
        const scaledAmount = ingredient.amount * scaleFactor;
        const roundedAmount = Math._________(scaledAmount * 10) / 10;
        
        return {
            ...ingredient,
            amount: roundedAmount
        };
    });
}
```

<details>
<summary>✅ Check Your Answer</summary>

```javascript
const roundedAmount = Math.round(scaledAmount * 10) / 10;
```

**The Trick:**
- Multiply by 10: `2.847 → 28.47`
- Round: `28.47 → 28`
- Divide by 10: `28 → 2.8`
</details>

---

### 📝 Your Turn: Scaling Math

If a recipe for 4 servings needs 2 cups flour, how much for 6 servings?

```
newAmount = 2 × (6 / 4) = _____ cups
```

<details>
<summary>Click for answer</summary>

```
newAmount = 2 × (6 / 4) = 2 × 1.5 = 3 cups
```
</details>

---

### Commit Display

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add recipe display with serving calculator'"

---

## 🗣️ Phase 6: Recipe CRUD

### Step 1: Add Recipe

**🗣️ SAY TO AIDE:**
> "Create addRecipe function. Open the editor form empty. On save, validate required fields, create recipe object with unique ID, add to state, save to localStorage, and display the new recipe."

---

### Step 2: Edit Recipe

**🗣️ SAY TO AIDE:**
> "Create editRecipe function. Populate editor form with current recipe data including all ingredients and steps. On save, update the recipe in state, save, and re-render."

---

### Step 3: Dynamic Ingredients Editor

**🗣️ SAY TO AIDE:**
> "In the editor, add ability to add/remove ingredients dynamically. 'Add Ingredient' button adds a new row with empty fields. Each ingredient has a remove button. Ensure at least one ingredient remains."

**👀 WATCH FOR:**

✅ New rows have working inputs
✅ Remove buttons work on each row
✅ Data is collected correctly on save

---

## ✏️ FILL IN THE BLANKS: Dynamic Form Handling

### Exercise 7: Add Ingredient to Form

```javascript
function addIngredientRow() {
    const container = document.getElementById('ingredients-container');
    
    const row = document.____________('div');
    row.className = 'ingredient-row';
    
    row.innerHTML = `
        <input type="text" placeholder="Name" class="ing-name">
        <input type="number" placeholder="Amount" class="ing-amount">
        <input type="text" placeholder="Unit" class="ing-unit">
        <button onclick="removeIngredientRow(this)">🗑️</button>
    `;
    
    container.______________(row);
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const row = document.createElement('div');
container.appendChild(row);
```
</details>

---

### Exercise 8: Remove Ingredient Row

```javascript
function removeIngredientRow(button) {
    // Get all ingredient rows
    const container = document.getElementById('ingredients-container');
    const rows = container.querySelectorAll('.ingredient-row');
    
    // Don't remove if only one left
    if (rows.__________ <= 1) {
        alert('Recipe needs at least one ingredient!');
        return;
    }
    
    // Remove this row (button is inside the row)
    button._____________.remove();
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
if (rows.length <= 1) {
    alert('Recipe needs at least one ingredient!');
    return;
}

button.parentElement.remove();
```

**Why parentElement?**
- `button` is the delete button
- `button.parentElement` is the row `<div>` containing it
</details>

---

### Exercise 9: Collecting Form Data

```javascript
function collectIngredients() {
    const rows = document.querySelectorAll('.ingredient-row');
    const ingredients = [];
    
    rows._____________(row => {
        const name = row.querySelector('.ing-name')._________;
        const amount = row.querySelector('.ing-amount').value;
        const unit = row.querySelector('.ing-unit').value;
        
        // Only add if name exists
        if (name.________()) {  // checks if not empty/whitespace
            ingredients.______({
                name: name,
                amount: ____________(amount) || 0,  // convert to number
                unit: unit
            });
        }
    });
    
    return ingredients;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
rows.forEach(row => {
    const name = row.querySelector('.ing-name').value;
    const amount = row.querySelector('.ing-amount').value;
    const unit = row.querySelector('.ing-unit').value;
    
    if (name.trim()) {
        ingredients.push({
            name: name,
            amount: parseFloat(amount) || 0,
            unit: unit
        });
    }
});
```
</details>

---

### Step 4: Dynamic Steps Editor

**🗣️ SAY TO AIDE:**
> "Add ability to add/remove/reorder steps. Each step is a text area. Add and remove buttons work. Maybe add drag to reorder or up/down buttons."

---

### Step 5: Delete Recipe

**🗣️ SAY TO AIDE:**
> "Create deleteRecipe function. Confirm before deleting. Remove from state, save, and clear the detail view or select another recipe."

---

### Commit CRUD

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add recipe create, edit, and delete'"

---

## ⚠️ Common AI Problem: Losing Nested Data

When editing nested arrays (ingredients, steps), AI might:
- Replace the whole array accidentally
- Lose other properties during edit
- Mix up indices

**🚨 IF DATA IS LOST:**

**🗣️ SAY:**
> "Console.log the recipe object before and after the edit. Let me see exactly what's changing."

Then you can spot where data is being lost.

---

## 🗣️ Phase 7: Meal Planning

### Step 1: Meal Plan Structure

**🗣️ SAY TO AIDE:**
> "Add meal planning feature. Create a meal plan state that tracks recipe IDs assigned to days and meals. Structure: { 'Monday': { breakfast: 'recipe-id', lunch: 'recipe-id', dinner: 'recipe-id' } }."

---

## ✏️ FILL IN THE BLANKS: Meal Plan Data

### Exercise 10: Initialize Meal Plan

```javascript
// Create a week of empty meal slots
function createEmptyMealPlan() {
    const days = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'];
    const mealPlan = {};
    
    days._____________(day => {
        mealPlan[day] = {
            breakfast: _____,
            lunch: _____,
            dinner: _____
        };
    });
    
    return mealPlan;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
days.forEach(day => {
    mealPlan[day] = {
        breakfast: null,
        lunch: null,
        dinner: null
    };
});
```
</details>

---

### Exercise 11: Assign Recipe to Meal

```javascript
function assignRecipe(day, meal, recipeId) {
    // Update the meal plan state
    mealPlan[_____][______] = recipeId;
    
    // Save to localStorage
    localStorage.____________('mealPlan', JSON.stringify(mealPlan));
    
    // Re-render the meal plan view
    renderMealPlan();
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function assignRecipe(day, meal, recipeId) {
    mealPlan[day][meal] = recipeId;
    localStorage.setItem('mealPlan', JSON.stringify(mealPlan));
    renderMealPlan();
}
```
</details>

---

### Step 2: Meal Plan View

**🗣️ SAY TO AIDE:**
> "Create a meal plan view showing the week as a grid. Days as columns, meals as rows. Each cell can have a recipe assigned or be empty. Clicking empty cell lets you select a recipe."

---

### Step 3: Shopping List

**🗣️ SAY TO AIDE:**
> "Add 'Generate Shopping List' that aggregates all ingredients from all recipes in the meal plan. Combine same ingredients - if two recipes need milk, add the amounts. Show as printable checklist."

---

## ✏️ FILL IN THE BLANKS: Aggregating Ingredients

### Exercise 12: Combine Shopping List

```javascript
function generateShoppingList() {
    const shoppingList = {};  // { 'flour': { amount: 5, unit: 'cups' }, ... }
    const days = Object._______(mealPlan);  // ['Monday', 'Tuesday', ...]
    
    days.forEach(day => {
        const meals = Object._______(mealPlan[day]);  // ['breakfast', 'lunch', 'dinner']
        
        meals.forEach(meal => {
            const recipeId = mealPlan[day][meal];
            if (!recipeId) return;  // skip empty slots
            
            // Find the recipe
            const recipe = recipes._______(r => r.id === recipeId);
            if (!recipe) return;
            
            // Add each ingredient to shopping list
            recipe.ingredients.forEach(ing => {
                const key = ing.name.toLowerCase();
                
                if (shoppingList[key]) {
                    // Already exists - add amounts
                    shoppingList[key].amount ____ ing.amount;
                } else {
                    // New ingredient
                    shoppingList[key] = {
                        name: ing.name,
                        amount: ing.amount,
                        unit: ing.unit
                    };
                }
            });
        });
    });
    
    return Object._______(shoppingList);  // convert to array
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const days = Object.keys(mealPlan);
const meals = Object.keys(mealPlan[day]);
const recipe = recipes.find(r => r.id === recipeId);

// Add amounts:
shoppingList[key].amount += ing.amount;

// Convert to array:
return Object.values(shoppingList);
```

**Key Concepts:**
- `Object.keys()` - get array of object's property names
- `Object.values()` - get array of object's values
- `+=` - add and assign in one operation
</details>

---

### Exercise 13: Display Shopping List

```javascript
function renderShoppingList(items) {
    const container = document.getElementById('shopping-list');
    
    container.innerHTML = items._______(item => {
        return `
            <div class="shopping-item">
                <input type="checkbox" id="item-${item.name}">
                <label for="item-${item.name}">
                    ${item.amount} ${item.______} ${item.name}
                </label>
            </div>
        `;
    }).______('');  // join array into single string
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
container.innerHTML = items.map(item => {
    return `
        <div class="shopping-item">
            <input type="checkbox" id="item-${item.name}">
            <label for="item-${item.name}">
                ${item.amount} ${item.unit} ${item.name}
            </label>
        </div>
    `;
}).join('');
```

**The Pattern:**
1. `map()` - transforms each item into HTML string
2. `join('')` - combines all strings into one (no separator)
3. Assign to `innerHTML` - renders it
</details>

---

### Commit Meal Planning

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add meal planning and shopping list'"

---

## 🗣️ Phase 8: Polish

### Step 1: Import/Export

**🗣️ SAY TO AIDE:**
> "Add ability to export recipes as JSON for backup. Add import that reads JSON and adds recipes to collection."

---

### Step 2: Recipe Sharing

**🗣️ SAY TO AIDE:**
> "Add a share button that copies a recipe's data to clipboard as formatted text that could be pasted into a message or document."

---

### Step 3: Cooking Mode

**🗣️ SAY TO AIDE:**
> "Add a 'Start Cooking' mode that shows steps one at a time in a large, easy-to-read format. Navigate with next/prev buttons. Timer controls for each step if it mentions time."

---

## ✏️ FILL IN THE BLANKS: Cooking Mode

### Exercise 14: Step Navigation

```javascript
let cookingState = {
    recipe: null,
    currentStep: 0
};

function startCooking(recipeId) {
    cookingState.recipe = recipes._______(r => r.id === recipeId);
    cookingState.currentStep = _____;  // start at first step
    showCookingModal();
    renderCookingStep();
}

function nextStep() {
    const maxStep = cookingState.recipe.steps._________ - 1;
    
    if (cookingState.currentStep < maxStep) {
        cookingState.currentStep_____;  // increment by 1
        renderCookingStep();
    }
}

function prevStep() {
    if (cookingState.currentStep > _____) {
        cookingState.currentStep_____;  // decrement by 1
        renderCookingStep();
    }
}

function renderCookingStep() {
    const step = cookingState.recipe.steps[cookingState._____________];
    const stepNumber = cookingState.currentStep + 1;
    const totalSteps = cookingState.recipe.steps.length;
    
    document.getElementById('cooking-step').innerHTML = `
        <div class="step-counter">Step ${stepNumber} of ${totalSteps}</div>
        <div class="step-text">${step}</div>
    `;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
cookingState.recipe = recipes.find(r => r.id === recipeId);
cookingState.currentStep = 0;

const maxStep = cookingState.recipe.steps.length - 1;

cookingState.currentStep++;  // increment
cookingState.currentStep--;  // decrement

if (cookingState.currentStep > 0) {

const step = cookingState.recipe.steps[cookingState.currentStep];
```
</details>

---

### Exercise 15: Timer Detection

```javascript
function extractTime(stepText) {
    // Look for patterns like "5 minutes" or "10 mins" or "1 hour"
    const timePattern = /(\d+)\s*(minutes?|mins?|hours?|hrs?|seconds?|secs?)/i;
    const match = stepText._______(timePattern);
    
    if (match) {
        const amount = ___________(match[1]);  // convert to number
        const unit = match[2].toLowerCase();
        
        // Convert everything to seconds
        if (unit.startsWith('hour') || unit.startsWith('hr')) {
            return amount * _______;  // 60 * 60
        } else if (unit.startsWith('min')) {
            return amount * _____;  // 60
        } else {
            return amount;  // already seconds
        }
    }
    
    return null;  // no time found
}

// Usage:
extractTime("Cook for 5 minutes until golden")  // returns 300
extractTime("Mix ingredients well")              // returns null
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
const match = stepText.match(timePattern);
const amount = parseInt(match[1]);

// Hours to seconds:
return amount * 3600;

// Minutes to seconds:
return amount * 60;
```

**Regex Breakdown:**
- `(\d+)` - capture one or more digits
- `\s*` - any whitespace
- `(minutes?|mins?|...)` - capture the unit word
- `i` flag - case insensitive
</details>

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add import/export, sharing, and cooking mode'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Debug nested data carefully
- ✅ Describe dynamic form fields
- ✅ Explain calculation requirements
- ✅ **Code review with AI** - let AI find bugs before you fix them

### Code Concepts:
- ✅ **Nested data structures** - arrays in objects
- ✅ **Scaling/proportions** - mathematical relationships
- ✅ **Dynamic forms** - add/remove inputs
- ✅ **Print CSS** - different media styles
- ✅ **Data aggregation** - combining from multiple sources

### Project Skills:
- ✅ Complex data modeling
- ✅ Feature interconnection (meal plan uses recipes)
- ✅ User-focused features (cooking mode)

---

## 📊 Fill-in-the-Blank Progress Tracker

Check off the exercises as you complete them:

- [ ] Exercise 1: Accessing Nested Data
- [ ] Exercise 2: Adding to Nested Arrays
- [ ] Exercise 3: Updating Nested Data
- [ ] Exercise 4: Removing from Nested Arrays
- [ ] Exercise 5: Write the Scaling Logic
- [ ] Exercise 6: Rounding for Nice Display
- [ ] Exercise 7: Add Ingredient to Form
- [ ] Exercise 8: Remove Ingredient Row
- [ ] Exercise 9: Collecting Form Data
- [ ] Exercise 10: Initialize Meal Plan
- [ ] Exercise 11: Assign Recipe to Meal
- [ ] Exercise 12: Combine Shopping List
- [ ] Exercise 13: Display Shopping List
- [ ] Exercise 14: Step Navigation
- [ ] Exercise 15: Timer Detection

**🏆 All 15 complete? You truly understand this code!**

---

## 🎁 Unlock Your Reward!

Dede is now a master chef! 👨‍🍳

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Nutritional info** - "Calculate calories and macros per serving"
2. **Recipe scaling** - "Convert between metric and imperial"
3. **Voice steps** - "Read steps aloud using text-to-speech"
4. **Recipe variations** - "Track variations of the same base recipe"

---

*Next up: Project 12 - Platformer Game! 🎮 + 🏃*
