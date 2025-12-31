# 💰 Project 8: Budgeting App

## 🎁 Reward: Calculator Watch for Dede!
Complete this project and Dede gets a nerdy calculator watch! 🔢

---

## 🌟 What You'll Learn

Build a personal budget tracker with categories, charts, and insights. This project teaches:

- ✅ **Data visualization** - charts and graphs
- ✅ **Complex calculations** - sums, percentages, trends
- ✅ **Form validation** - ensuring clean data
- ✅ **Category systems** - organizing data
- ✅ **AI for math** - prompting for calculations
- ✅ **Scope control** - preventing feature creep

---

## 📓 NEW SKILL: Preventing Feature Creep

"Feature creep" = your project keeps growing until it's overwhelming.

AI loves to add features. You ask for a budget tracker, AI suggests:
- "Let's add bank sync!"
- "How about investment tracking?"
- "We could add AI spending predictions!"
- "What about multi-currency support?"

**🚨 THIS IS HOW PROJECTS DIE.**

**Create a SCOPE.md file:**

**🗣️ SAY TO AIDE:**
> "Create a SCOPE.md file in my project"

```markdown
# Budget App - Project Scope

## ✅ IN SCOPE (We ARE building this)
- Add income/expense transactions
- View transaction history
- See spending by category (pie chart)
- Monthly spending bar chart
- Calculate remaining balance
- Save to localStorage

## ❌ OUT OF SCOPE (We are NOT building this)
- Bank account sync
- Investment tracking
- Multi-currency
- Bill reminders
- AI predictions
- Mobile app version
- User accounts/login

## 🤔 MAYBE LATER (After core is done)
- Export to CSV
- Budget goals per category
- Recurring transactions

## 📏 Definition of Done
The project is COMPLETE when:
1. User can add income and expenses
2. Charts display correctly
3. Data persists on refresh
4. Balance calculates correctly
```

**🧠 WHEN AI SUGGESTS NEW FEATURES:**

**🗣️ SAY:**
> "That's out of scope for now. Let's focus on [current task]. I've noted the idea for later."

Then add it to "MAYBE LATER" in SCOPE.md.

**Rule: Finish the core before adding extras.**

---

## 🧠 New Concept: Data Visualization

Numbers are hard to understand. Charts make patterns visible.

| Data Type | Best Chart |
|-----------|------------|
| Parts of a whole | Pie/Donut |
| Change over time | Line |
| Comparing amounts | Bar |
| Distribution | Histogram |

You'll use Chart.js or build simple visualizations.

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: Categories

What spending categories will you track?

**Common categories:**
- Housing (rent, utilities)
- Food (groceries, dining)
- Transportation (gas, transit)
- Entertainment
- Shopping
- Healthcare
- Savings

**Decide your categories and assign each a color.**

### Data Structure

```javascript
const transaction = {
    id: 'unique-id',
    type: 'expense',        // or 'income'
    amount: 45.99,
    category: 'food',
    description: 'Grocery shopping',
    date: '2024-01-15'
};
```

---

## 🗣️ Phase 2: Project Setup

**🗣️ SAY TO AIDE:**
> "Create a budget-app project with index.html, css/styles.css, and js/app.js. Also include Chart.js from a CDN for data visualization. Initialize git."

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Dashboard Layout

**🗣️ SAY TO AIDE:**
> "Create a budget dashboard layout. Top section shows summary cards: total income, total expenses, and remaining balance. Below that, a chart area on one side and transaction list on the other. Bottom has an add transaction form."

---

### Step 2: Summary Cards

**🗣️ SAY TO AIDE:**
> "Create three summary cards: Income showing total income in green, Expenses showing total expenses in red, and Balance showing the difference. The balance should change color based on positive or negative."

---

### Step 3: Charts Area

**🗣️ SAY TO AIDE:**
> "Add two chart containers: one for a donut chart showing expense breakdown by category, and one for a bar chart showing monthly spending comparison."

---

### Step 4: Transaction List

**🗣️ SAY TO AIDE:**
> "Create a transaction list that shows recent transactions with date, description, category icon, and amount. Income should show as green plus, expenses as red minus."

---

### Step 5: Add Transaction Form

**🗣️ SAY TO AIDE:**
> "Create a form to add transactions with fields for: type toggle between income and expense, amount input, category dropdown that changes options based on type, description text input, and date picker defaulting to today."

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add budget app HTML structure'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Dashboard Grid

**🗣️ SAY TO AIDE:**
> "Style the dashboard with CSS Grid. Summary cards in a row at top, charts and transactions in columns below, form at the bottom. Responsive - stacks on mobile."

---

### Step 2: Summary Cards

**🗣️ SAY TO AIDE:**
> "Style summary cards with large numbers, subtle backgrounds matching their meaning - light green for income, light red for expenses, and contextual for balance. Add subtle shadows and rounded corners."

---

### Step 3: Transaction List

**🗣️ SAY TO AIDE:**
> "Style transaction items in a clean list with alternating backgrounds. Show amount prominently, category as a small colored badge. Add hover effect."

---

### Step 4: Form Styling

**🗣️ SAY TO AIDE:**
> "Style the form in a horizontal layout with fields inline. Toggle between income and expense should be visually clear - like tab buttons. Submit button should be prominent."

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add budget app styling'"

---

## 🗣️ Phase 5: JavaScript - Core Budget Logic

### Step 1: State and Categories

**🗣️ SAY TO AIDE:**
> "Create state with a transactions array. Define category objects with name, color, and icon for both income categories like salary, freelance, investments and expense categories like food, housing, transport, entertainment, shopping, healthcare."

---

### Step 2: Calculations

**🗣️ SAY TO AIDE:**
> "Create calculation functions: getTotalIncome sums all income transactions, getTotalExpenses sums all expense transactions, getBalance returns income minus expenses, getCategoryTotals returns an object with each category's total."

**👀 CHECK THE MATH:**

✅ Uses reduce for summing:
```javascript
const total = transactions
    .filter(t => t.type === 'income')
    .reduce((sum, t) => sum + t.amount, 0);
```

✅ Handles empty arrays (returns 0, not NaN)

---

### 📝 Your Turn: The Reduce Pattern

Complete this code to sum expenses:

```javascript
function getTotalExpenses() {
    return state.transactions
        .filter(t => t.type === '_______')
        .reduce((sum, t) => sum ___ t.amount, ___);
}
```

<details>
<summary>Click for answer</summary>

```javascript
function getTotalExpenses() {
    return state.transactions
        .filter(t => t.type === 'expense')
        .reduce((sum, t) => sum + t.amount, 0);
}
```
</details>

---

## ✏️ FILL IN THE BLANKS: Array Methods Deep Dive

### 🧠 CONCEPT: Filter - Selecting Items

`.filter()` creates a NEW array with only items that pass a test:

```javascript
const numbers = [1, 2, 3, 4, 5];
const evens = numbers.filter(n => n % 2 === 0);  // [2, 4]
const big = numbers.filter(n => n > 3);          // [4, 5]
```

**The pattern:** `array.filter(item => condition)` → new array

### Exercise 1: Filter Transactions

```javascript
function getIncomeTransactions() {
    return state.transactions._________(t => t.______ === 'income');
}

function getExpensesByCategory(category) {
    return state.transactions._________(t => 
        t.type === '________' && t.__________ === category
    );
}

function getTransactionsThisMonth() {
    const now = new Date();
    const currentMonth = now.___________();
    const currentYear = now._______________();
    
    return state.transactions.filter(t => {
        const date = new Date(t.date);
        return date.getMonth() === currentMonth && 
               date.getFullYear() === currentYear;
    });
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function getIncomeTransactions() {
    return state.transactions.filter(t => t.type === 'income');
}

function getExpensesByCategory(category) {
    return state.transactions.filter(t => 
        t.type === 'expense' && t.category === category
    );
}

function getTransactionsThisMonth() {
    const now = new Date();
    const currentMonth = now.getMonth();
    const currentYear = now.getFullYear();
    
    return state.transactions.filter(t => {
        const date = new Date(t.date);
        return date.getMonth() === currentMonth && 
               date.getFullYear() === currentYear;
    });
}
```

**Note:** `getMonth()` returns 0-11 (January = 0)
</details>

---

### 🧠 CONCEPT: Reduce - Combining into One Value

`.reduce()` takes an array and reduces it to a single value:

```javascript
const numbers = [1, 2, 3, 4, 5];
const sum = numbers.reduce((accumulator, current) => {
    return accumulator + current;
}, 0);  // 15
```

**Breaking it down:**
- `accumulator` - the running total (starts at 0)
- `current` - each item as we loop
- `0` at the end - the starting value

```
Step 1: accumulator=0,  current=1 → return 0+1=1
Step 2: accumulator=1,  current=2 → return 1+2=3
Step 3: accumulator=3,  current=3 → return 3+3=6
Step 4: accumulator=6,  current=4 → return 6+4=10
Step 5: accumulator=10, current=5 → return 10+5=15
```

### Exercise 2: Sum with Reduce

```javascript
function getTotalIncome() {
    // First, get only income transactions
    const incomes = state.transactions._________(t => t.type === 'income');
    
    // Then reduce to sum
    return incomes._________(
        (sum, transaction) => sum ___ transaction.________,
        ___  // starting value
    );
}

function getBalance() {
    const income = getTotalIncome();
    const expenses = getTotalExpenses();
    
    return income _____ expenses;
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function getTotalIncome() {
    const incomes = state.transactions.filter(t => t.type === 'income');
    
    return incomes.reduce(
        (sum, transaction) => sum + transaction.amount,
        0
    );
}

function getBalance() {
    const income = getTotalIncome();
    const expenses = getTotalExpenses();
    
    return income - expenses;
}
```
</details>

---

### 🧠 CONCEPT: Reduce to Build Objects

Reduce can also build objects, not just numbers:

```javascript
const transactions = [
    { category: 'food', amount: 20 },
    { category: 'food', amount: 15 },
    { category: 'gas', amount: 40 }
];

const totals = transactions.reduce((acc, t) => {
    if (acc[t.category]) {
        acc[t.category] += t.amount;
    } else {
        acc[t.category] = t.amount;
    }
    return acc;
}, {});

// Result: { food: 35, gas: 40 }
```

### Exercise 3: Category Totals

```javascript
function getCategoryTotals() {
    // Get only expenses
    const expenses = state.transactions.filter(t => t.type === 'expense');
    
    // Build an object with category totals
    return expenses._________(
        (totals, transaction) => {
            const cat = transaction.__________;
            
            // If category exists, add to it
            if (totals[_____]) {
                totals[cat] ___= transaction.amount;
            } else {
                // Otherwise, start with this amount
                totals[cat] = transaction.________;
            }
            
            // IMPORTANT: Always return the accumulator!
            return ________;
        },
        ____  // Start with empty object
    );
}

// Usage:
getCategoryTotals();  
// { food: 150, transport: 80, entertainment: 45 }
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function getCategoryTotals() {
    const expenses = state.transactions.filter(t => t.type === 'expense');
    
    return expenses.reduce(
        (totals, transaction) => {
            const cat = transaction.category;
            
            if (totals[cat]) {
                totals[cat] += transaction.amount;
            } else {
                totals[cat] = transaction.amount;
            }
            
            return totals;
        },
        {}
    );
}
```

**Why return totals?**
- Reduce needs to pass the accumulator to the next iteration
- Forgetting `return` is a common bug!
</details>

---

### 🧠 CONCEPT: Formatting Currency

Numbers look better as currency:

```javascript
const amount = 1234.5;
const formatted = amount.toLocaleString('en-US', {
    style: 'currency',
    currency: 'USD'
});
// "$1,234.50"
```

### Exercise 4: Format and Display

```javascript
function formatCurrency(amount) {
    return amount.________________('en-US', {
        _______: 'currency',
        currency: '_____'
    });
}

function updateBalanceDisplay() {
    const balance = getBalance();
    const element = document.getElementById('balance');
    
    // Set the text
    element._____________ = formatCurrency(Math._____(balance));
    
    // Set color based on positive/negative
    element.style._______ = balance >= 0 ? 'green' : 'red';
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function formatCurrency(amount) {
    return amount.toLocaleString('en-US', {
        style: 'currency',
        currency: 'USD'
    });
}

function updateBalanceDisplay() {
    const balance = getBalance();
    const element = document.getElementById('balance');
    
    element.textContent = formatCurrency(Math.abs(balance));
    element.style.color = balance >= 0 ? 'green' : 'red';
}
```

**Why Math.abs()?**
- `abs` = absolute value (removes negative sign)
- We show the color to indicate positive/negative
- The number itself stays readable: "$50.00" not "-$50.00"
</details>

**🗣️ SAY TO AIDE:**
> "Create renderSummary function that calculates totals and updates the summary cards. Format numbers as currency. Balance card should have green text if positive, red if negative."

---

### Step 4: Render Transactions

**🗣️ SAY TO AIDE:**
> "Create renderTransactions function that displays transactions sorted by date, newest first. Each shows formatted date, description, category badge, and amount. Limit to most recent 10 with a 'show all' option."

---

### Commit Core Logic

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add budget calculations and rendering'"

---

## 🗣️ Phase 6: Charts

### Step 1: Expense Breakdown Chart

**🗣️ SAY TO AIDE:**
> "Create a donut chart using Chart.js showing expense breakdown by category. Each slice is a category with its color. Show category names and percentages in the legend or on hover."

**👀 CHECK:**

✅ Chart.js is properly initialized
✅ Data comes from getCategoryTotals function
✅ Colors match category definitions

---

### Step 2: Monthly Comparison

**🗣️ SAY TO AIDE:**
> "Create a bar chart showing income vs expenses for the last 6 months. Two bars per month - green for income, red for expenses. X-axis shows month names."

---

### Step 3: Update Charts

**🗣️ SAY TO AIDE:**
> "Create an updateCharts function that refreshes both charts with current data. Call this whenever transactions change."

---

### Commit Charts

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add Chart.js visualizations'"

---

## ⚠️ Common AI Problem: Chart Confusion

Chart.js has many options. AI might:
- Use wrong chart type
- Overcomplicate configuration
- Mix up data formats

**🚨 DEBUGGING CHARTS:**

**🗣️ SAY:**
> "Console.log the data being passed to the chart before creating it. I want to see the exact array and values."

Then you can see if the data is right. Usually data issues, not chart issues.

---

## 🗣️ Phase 7: Adding Transactions

### Step 1: Form Handling

**🗣️ SAY TO AIDE:**
> "Handle form submission. Validate that amount is positive, category is selected, and description isn't empty. If valid, create transaction object, add to state, save to localStorage, re-render everything, and clear the form."

---

### Step 2: Type Toggle

**🗣️ SAY TO AIDE:**
> "When toggling between income and expense, change the category dropdown options to show only relevant categories. Update form styling to reflect the type."

---

### Step 3: Editing Transactions

**🗣️ SAY TO AIDE:**
> "Add edit functionality. Clicking a transaction opens it in the form for editing. On submit, update the existing transaction instead of creating new. Add a cancel edit button."

---

### Step 4: Deleting Transactions

**🗣️ SAY TO AIDE:**
> "Add delete button to transactions. Confirm before deleting. Remove from state, save, and re-render."

---

### Commit Transaction Management

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add transaction CRUD operations'"

---

## 🗣️ Phase 8: Insights

### Step 1: Budget Goals

**🗣️ SAY TO AIDE:**
> "Add ability to set monthly budget goals per category. Show progress bars on categories showing how much of the budget is used. Warn when approaching or exceeding budget."

---

### Step 2: Spending Trends

**🗣️ SAY TO AIDE:**
> "Calculate and display insights like: highest spending category this month, spending change from last month as percentage, average daily spending, days until budget runs out at current rate."

---

### Step 3: Recurring Transactions

**🗣️ SAY TO AIDE:**
> "Add option to mark transactions as recurring - daily, weekly, or monthly. At start of each period, automatically create these recurring transactions."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add budget goals, insights, and recurring transactions'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Request calculation verification
- ✅ Debug data issues before blaming libraries
- ✅ Specify chart types clearly

### Code Concepts:
- ✅ **Reduce** - summing/aggregating arrays
- ✅ **Chart.js** - data visualization
- ✅ **Form validation** - ensuring data quality
- ✅ **Computed values** - derived from state

### Project Skills:
- ✅ Financial logic accuracy
- ✅ User feedback (warnings, progress)
- ✅ Data visualization

---

## 🎁 Unlock Your Reward!

Dede now has a nerdy calculator watch! 🔢

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **CSV Import** - "Import transactions from bank CSV exports"
2. **Split transactions** - "Allow splitting one transaction across categories"
3. **Currency support** - "Support multiple currencies with conversion"
4. **Reports** - "Generate monthly PDF reports"

---

*Next up: Project 9 - Health Monitoring App! 🏥 + 📊*
