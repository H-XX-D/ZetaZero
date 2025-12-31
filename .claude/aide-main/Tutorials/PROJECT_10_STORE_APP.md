# 🛒 Project 10: Store App

## 🎁 Reward: Jetpack for Dede!
Complete this project and Dede gets a jetpack for flying adventures! 🚀

---

## 🌟 What You'll Learn

Build an e-commerce storefront with products, cart, and checkout. This project teaches:

- ✅ **Shopping cart logic** - adding, removing, quantities
- ✅ **Product management** - display, filtering, search
- ✅ **Checkout flow** - multi-step process
- ✅ **Currency formatting** - proper money display
- ✅ **Inventory concepts** - stock management
- ✅ **Catching AI mistakes** - validation techniques

---

## 📓 NEW SKILL: Catching AI Mistakes

AI makes mistakes. Good developers catch them BEFORE they cause problems.

**Technique 1: The Sanity Check Question**

After AI writes code, ask:
> "Walk me through what happens when a user adds an item to cart. Trace the function calls."

If AI can't explain it clearly, the code probably has issues.

**Technique 2: Edge Case Testing**

**🗣️ SAY TO AIDE:**
> "What happens if someone tries to add more items than are in stock?"

> "What if cart quantity goes below 0?"

> "What if the product ID doesn't exist?"

Make AI think about edge cases BEFORE you test manually.

**Technique 3: The Console.log Audit**

**🗣️ SAY TO AIDE:**
> "Add console.log statements to addToCart showing: what product is being added, current cart state before add, and cart state after add."

Watch the console. If numbers don't make sense, you've found a bug.

**Technique 4: Compare to Your Notes**

Remember your CONTEXT.md and SCOPE.md files?
Check: Does AI's code match YOUR decisions?

```markdown
## Key Decisions Made
- Cart stored in localStorage
- Max quantity per item: 10
- Prices in USD, two decimal places
```

If AI stores cart in memory only, or allows quantity 999, or shows prices as "29.99000001" - catch it!

**🧠 TRUST BUT VERIFY:** AI is helpful, but YOU are responsible for the code.

---

## 🧠 New Concept: Cart as Separate State

The cart is NOT just a copy of products. It references products by ID:

```javascript
// Products (your inventory)
const products = [
    { id: 'p1', name: 'T-Shirt', price: 29.99, stock: 50 }
];

// Cart (what customer wants to buy)
const cart = [
    { productId: 'p1', quantity: 2 }  // References product, has its own quantity
];
```

> 💡 **Why separate?** Product info can change (price update). Cart just tracks what they want.

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: What Do You Sell?

Pick a theme for your store:
- Clothing
- Electronics
- Books
- Food/Drinks
- Custom: ____________

**List 8-10 products with:**
- Name
- Price
- Description
- Category
- Image (placeholder OK)

---

## 🗣️ Phase 2: Project Setup

**🗣️ SAY TO AIDE:**
> "Create a store-app project with index.html, css/styles.css, js/app.js, and a js/products.js file to hold product data. Initialize git."

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Store Layout

**🗣️ SAY TO AIDE:**
> "Create a store layout with: header containing logo, search bar, and cart icon with item count badge. Main area has sidebar with category filters and main product grid. Footer has basic info."

---

### Step 2: Product Cards

**🗣️ SAY TO AIDE:**
> "Create product card structure: image, product name, price, short description, and 'Add to Cart' button. If out of stock, show 'Out of Stock' instead of button."

---

### Step 3: Cart Sidebar/Modal

**🗣️ SAY TO AIDE:**
> "Create a cart panel that slides in from the right when the cart icon is clicked. Show list of cart items with image, name, price, quantity controls, and remove button. Bottom shows subtotal and checkout button."

---

### Step 4: Checkout Modal

**🗣️ SAY TO AIDE:**
> "Create a multi-step checkout modal: Step 1 shows order summary, Step 2 collects shipping info, Step 3 collects payment info (simulated), Step 4 shows confirmation. Include progress indicator."

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add store HTML structure with cart and checkout'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Store Theme

**🗣️ SAY TO AIDE:**
> "Style the store with a clean, professional e-commerce look. Clear typography, product images prominent, price visible. Use a consistent color scheme."

---

### Step 2: Product Grid

**🗣️ SAY TO AIDE:**
> "Style the product grid as a responsive CSS grid - 4 columns on desktop, 3 on tablet, 2 on phone, 1 on small phone. Cards should have equal height with image at consistent aspect ratio."

---

### Step 3: Cart Slide

**🗣️ SAY TO AIDE:**
> "Style the cart as a slide-in panel from the right side. Full height, overlaps content with a backdrop. Smooth slide animation on open/close. Cart items in a scrollable list with fixed bottom section for total."

---

### Step 4: Quantity Controls

**🗣️ SAY TO AIDE:**
> "Style quantity controls as a compact plus/minus stepper with the number in the middle. Buttons should be easy to click but not take too much space."

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add store styling with responsive grid and cart'"

---

## 🗣️ Phase 5: JavaScript - Products

### Step 1: Product Data

**🗣️ SAY TO AIDE:**
> "In products.js, create an array of 8-10 products. Each product has id, name, price, description, category, imageUrl (use placeholder image service), and stock quantity. Export or attach to window."

---

### Step 2: Render Products

**🗣️ SAY TO AIDE:**
> "Create renderProducts function that takes a product array and displays them in the grid. Each card shows product info and has data-id attribute. Add click handler for add to cart button."

---

### Step 3: Category Filter

**🗣️ SAY TO AIDE:**
> "Add category filter functionality. When a category button is clicked, filter products to show only that category. 'All' shows everything. Update visual state of filter buttons."

---

### Step 4: Search

**🗣️ SAY TO AIDE:**
> "Add search functionality. As user types in search bar, filter products to show only those whose name or description contains the search term. Case insensitive. Combine with category filter."

---

### 📝 Your Turn: Filter Logic

Complete the filter:

```javascript
function filterProducts(products, category, searchTerm) {
    return products.filter(p => {
        const matchesCategory = category === 'all' || p.category === ________;
        const matchesSearch = searchTerm === '' || 
            p.name.toLowerCase().includes(searchTerm.____________());
        return matchesCategory ____ matchesSearch;
    });
}
```

<details>
<summary>Click for answer</summary>

```javascript
function filterProducts(products, category, searchTerm) {
    return products.filter(p => {
        const matchesCategory = category === 'all' || p.category === category;
        const matchesSearch = searchTerm === '' || 
            p.name.toLowerCase().includes(searchTerm.toLowerCase());
        return matchesCategory && matchesSearch;
    });
}
```
</details>

---

### Commit Products

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add product display with filtering and search'"

---

## 🗣️ Phase 6: Cart Logic

### Step 1: Cart State

**🗣️ SAY TO AIDE:**
> "Create cart state as an array of items with productId and quantity. Create functions: addToCart takes product id and adds to cart or increases quantity if already there. removeFromCart removes item. updateQuantity changes quantity for an item."

---

### Step 2: Cart Calculations

**🗣️ SAY TO AIDE:**
> "Create calculation functions: getCartCount returns total number of items. getCartSubtotal returns sum of price times quantity for all items. getCartItems returns cart items with full product info attached."

**👀 CHECK:**

✅ Lookups product by ID to get price:
```javascript
const product = products.find(p => p.id === item.productId);
const lineTotal = product.price * item.quantity;
```

---

### Step 3: Currency Formatting

**🗣️ SAY TO AIDE:**
> "Create formatCurrency function that takes a number and returns it formatted as USD like $29.99. Use Intl.NumberFormat for proper formatting."

**🧠 PROPER CURRENCY:**
```javascript
const formatter = new Intl.NumberFormat('en-US', {
    style: 'currency',
    currency: 'USD'
});
formatter.format(29.99);  // "$29.99"
```

---

### Step 4: Render Cart

**🗣️ SAY TO AIDE:**
> "Create renderCart function that displays all cart items with product details, quantity controls, line totals, and the subtotal. Update cart count badge in header."

---

### Step 5: Cart Persistence

**🗣️ SAY TO AIDE:**
> "Save cart to localStorage whenever it changes. Load cart from localStorage on page load. This way cart persists across sessions."

---

### Commit Cart

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add cart functionality with persistence'"

---

## ⚠️ Common AI Problem: Stock Validation

AI might forget to check stock when adding to cart:

**🚨 IF STOCK NOT CHECKED:**

**🗣️ SAY:**
> "Before adding to cart, check if product has stock. Don't allow adding more than available stock. Show a message if trying to add more than available."

---

## 🗣️ Phase 7: Checkout Flow

### Step 1: Checkout State

**🗣️ SAY TO AIDE:**
> "Create checkout state to track: current step (1-4), shipping info, and payment info. Create functions to move between steps with validation."

---

### Step 2: Step 1 - Order Summary

**🗣️ SAY TO AIDE:**
> "Render the order summary step showing all cart items, subtotal, estimated tax, shipping cost, and total. Have back to cart and continue buttons."

---

### Step 3: Step 2 - Shipping

**🗣️ SAY TO AIDE:**
> "Create shipping form with name, address, city, state, zip, and phone. Validate fields before allowing continue. Show helpful error messages."

---

### Step 4: Step 3 - Payment

**🗣️ SAY TO AIDE:**
> "Create simulated payment form with card number, expiration, and CVV fields. Add input masks for formatting. This is a simulation - don't store real card data."

---

### Step 5: Step 4 - Confirmation

**🗣️ SAY TO AIDE:**
> "Show order confirmation with a success message, order number generated randomly, summary of order, and estimated delivery. Clear the cart after successful order."

---

### Commit Checkout

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add multi-step checkout flow'"

---

## 🗣️ Phase 8: Polish

### Step 1: Loading States

**🗣️ SAY TO AIDE:**
> "Add loading states: skeleton loaders for products while loading, button loading state during checkout submit, and smooth transitions between states."

---

### Step 2: Empty States

**🗣️ SAY TO AIDE:**
> "Handle empty states: empty cart shows a friendly message with link to shop, no search results shows helpful message, empty category shows message."

---

### Step 3: Quick View Modal

**🗣️ SAY TO AIDE:**
> "Add quick view - clicking a product opens a modal with larger image, full description, and add to cart with quantity selection."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add loading states, empty states, and quick view'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Remind AI about validations (stock, required fields)
- ✅ Specify multi-step flows clearly
- ✅ Request proper formatting (currency)

### Code Concepts:
- ✅ **Cart data model** - references, not copies
- ✅ **Intl.NumberFormat** - localized formatting
- ✅ **Multi-step forms** - flow management
- ✅ **Input validation** - user feedback

### Project Skills:
- ✅ E-commerce patterns
- ✅ State persistence
- ✅ Professional polish (loading, empty states)

---

## 🎁 Unlock Your Reward!

Dede now has a jetpack! Ready for liftoff! 🚀

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Wishlist** - "Add save for later/wishlist functionality"
2. **Product reviews** - "Add ability to rate and review products"
3. **Discount codes** - "Add coupon code system with discounts"
4. **Recently viewed** - "Track and show recently viewed products"

---

*Next up: Project 11 - Recipe Book App! 🍳 + 📖*
