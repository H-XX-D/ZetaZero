# 📋 Project 5: Kanban Board

## 🎁 Reward: Cape for Dede!
Complete this project and Dede becomes a superhero with a flowing cape! 🦸

---

## 🌟 What You'll Learn

Build a project management board like Trello - drag and drop tasks between columns. This project teaches:

- ✅ **Drag and Drop API** - native browser functionality
- ✅ **CRUD operations** - Create, Read, Update, Delete
- ✅ **Complex state** - arrays of objects with relationships
- ✅ **AI debugging** - fixing multi-step interactions
- ✅ **UX considerations** - making interactions feel good
- ✅ **Git branching** - working on features safely

---

## 📓 NEW SKILL: Git Branches

Branches let you experiment without breaking working code.

```
main ────●────●────●────●  (stable, working code)
              \
feature ───────●────●      (experimental work)
```

**🗣️ SAY TO AIDE:**
> "Create a new branch called 'add-drag-drop'"

Now you can experiment. If it breaks everything, just switch back to main!

**Basic Git Branch Workflow:**
```bash
# Create and switch to new branch
git checkout -b feature-name

# Work, commit, work, commit...

# When feature works, merge to main
git checkout main
git merge feature-name

# Delete the feature branch (optional)
git branch -d feature-name
```

**🧠 WHEN TO BRANCH:**
- ✅ Adding a new feature (drag-drop, editing, etc.)
- ✅ Trying something risky
- ✅ Working on something that might take multiple sessions
- ❌ Small fixes (just commit directly)

**Good commit messages:**
```bash
# ❌ Bad
git commit -m "stuff"
git commit -m "fix"
git commit -m "asdfasdf"

# ✅ Good  
git commit -m "Add drag start and drop handlers"
git commit -m "Fix task not appearing after drag"
git commit -m "Style drop zone highlight on dragover"
```

**Rule: Your commit message should finish this sentence:**
> "If applied, this commit will... [your message]"

---

## 🧠 New Concept: CRUD

Every data app does 4 things:

| Letter | Action | Example |
|--------|--------|---------|
| **C** | Create | Add a new task |
| **R** | Read | Display existing tasks |
| **U** | Update | Edit a task's title |
| **D** | Delete | Remove a task |

Before building, think: "How will users do each CRUD action?"

---

## 🧠 New Concept: Data Modeling

Your tasks need a structure. Think about:

```javascript
const task = {
    id: "unique-id-here",     // To identify this specific task
    title: "My Task",          // What the user sees
    column: "todo",            // Which column it's in
    createdAt: Date.now()      // When it was created
};
```

> 💡 **Why IDs?** If you have 3 tasks called "Fix bug", how do you know which one to delete? IDs make each task unique.

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: Define Your Columns

Standard Kanban boards have 3 columns, but you can customize.

Choose your columns and their meanings:

| Column ID | Display Name | Purpose |
|-----------|--------------|---------|
| `todo` | To Do | Tasks not started |
| `progress` | In Progress | Currently working on |
| `done` | Done | Completed tasks |

Or get creative: `backlog`, `review`, `blocked`, etc.

---

## 🗣️ Phase 2: Project Setup

### Step 1: Create Files

**🗣️ SAY TO AIDE:**
> "Create a kanban-board project with index.html, css/styles.css, and js/app.js"

---

### Step 2: Git Init

**🗣️ SAY TO AIDE:**
> "Initialize git with a .gitignore"

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Board Layout

**🗣️ SAY TO AIDE:**
> "In index.html, create a Kanban board layout. Add a header with title 'My Kanban' and a button to add new tasks. Below, create a board container with three columns: To Do, In Progress, and Done. Each column needs a header with the column title and a droppable area for tasks."

**👀 CHECK:**

✅ Each column has a unique ID:
```html
<div class="column" id="todo">
<div class="column" id="progress">
<div class="column" id="done">
```

✅ Each column has a droppable container for tasks:
```html
<div class="column" id="todo">
    <h2>To Do</h2>
    <div class="task-container"></div>
</div>
```

---

### Step 2: Task Template

**🗣️ SAY TO AIDE:**
> "Add a sample task card inside the To Do column so we can see how tasks should look. The task should have a title, and small buttons for edit and delete. Add a data attribute for the task ID."

**👀 CHECK:**

✅ Task has `draggable="true"` attribute
✅ Task has a `data-id` attribute
✅ Edit and delete buttons are present

---

### Step 3: Add Task Modal

**🗣️ SAY TO AIDE:**
> "Create a modal overlay for adding new tasks. It should have an input field for the task title, a dropdown to select which column to add it to, and Add and Cancel buttons. The modal should be hidden by default."

**👀 CHECK:**

✅ Modal has a class that can be toggled to show/hide
✅ Input has an ID for JavaScript to access
✅ Buttons have clear purposes

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add Kanban board HTML with columns and modal'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Board Layout

**🗣️ SAY TO AIDE:**
> "Style the board as a horizontal flex layout with equal-width columns. Use a light background for the board and slightly different backgrounds for each column to distinguish them. Add padding and gaps between columns."

---

### Step 2: Column Styling

**🗣️ SAY TO AIDE:**
> "Style the columns with a minimum height of 500px so they're tall enough even when empty. The column header should be sticky at the top. Add a subtle background and rounded corners."

---

### Step 3: Task Cards

**🗣️ SAY TO AIDE:**
> "Style task cards with a white background, padding, rounded corners, and a subtle shadow. On hover, add a slight lift effect. The edit and delete buttons should only appear on hover - hidden otherwise."

**👀 CHECK:**

✅ Buttons hidden by default: `opacity: 0;`
✅ Buttons show on hover: `.task:hover .buttons { opacity: 1; }`
✅ Cursor indicates draggable: `cursor: grab;`

---

### Step 4: Drag Visual Feedback

**🗣️ SAY TO AIDE:**
> "Add a visual style for when a task is being dragged - maybe reduce opacity or add a rotation. Also style the drop zone when a dragged task is over it - maybe a dashed border or background color change."

**👀 CHECK:**

✅ `.task.dragging` style exists
✅ `.column.drag-over` style exists

---

### Step 5: Modal Styling

**🗣️ SAY TO AIDE:**
> "Style the modal with a semi-transparent dark overlay covering the whole screen. The modal content should be centered, white, with padding and rounded corners. Add smooth fade-in animation when it appears."

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add Kanban board styling with drag feedback and modal'"

---

## 🗣️ Phase 5: JavaScript - State & Rendering

### Step 1: State Structure

**🗣️ SAY TO AIDE:**
> "In app.js, create a state object that holds an array of tasks. Each task should have an id, title, and column property. Initialize with 2-3 sample tasks spread across different columns."

**👀 CHECK:**

```javascript
const state = {
    tasks: [
        { id: '1', title: 'Learn Kanban', column: 'done' },
        { id: '2', title: 'Build board', column: 'progress' },
        { id: '3', title: 'Add drag drop', column: 'todo' }
    ]
};
```

---

### Step 2: Generate Unique IDs

**🗣️ SAY TO AIDE:**
> "Create a function called generateId that returns a unique string ID. Use Date.now combined with a random number to ensure uniqueness."

---

### Step 3: Render Function

**🗣️ SAY TO AIDE:**
> "Create a render function that clears all task containers, then loops through state.tasks and creates a task element for each one, adding it to the correct column based on the task's column property."

**👀 CHECK:**

✅ Clears containers first (prevents duplicates)
✅ Creates elements dynamically
✅ Adds to correct column based on `task.column`
✅ Attaches event listeners to new elements

---

### 📝 Your Turn: The Render Flow

Put these steps in order:

1. Add new element to container
2. Create element from task data
3. Clear old elements
4. Loop through state.tasks
5. Find correct column container

<details>
<summary>Click for answer</summary>

1. **Clear old elements** (start fresh)
2. **Loop through state.tasks** (for each task...)
3. **Create element from task data** (build the HTML)
4. **Find correct column container** (where does it go?)
5. **Add new element to container** (put it there)
</details>

---

### Commit State & Render

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add state structure and render function'"

---

## 🗣️ Phase 6: CRUD Operations

### Step 1: CREATE - Adding Tasks

**🗣️ SAY TO AIDE:**
> "Create an addTask function. When the Add Task button in the header is clicked, show the modal. When the modal's Add button is clicked, get the title and column values, create a new task object with a unique ID, add it to state.tasks, save state, hide the modal, and render."

**👀 CHECK:**

✅ Creates task with all properties (id, title, column)
✅ Pushes to `state.tasks` array
✅ Calls `saveState()` and `render()`
✅ Clears input field after adding

---

### Step 2: READ - Already Done!

The render function IS the Read operation. It reads from state and displays.

---

### Step 3: UPDATE - Editing Tasks

**🗣️ SAY TO AIDE:**
> "Create an editTask function. When the edit button on a task is clicked, show a prompt with the current title. If the user enters a new title and clicks OK, find the task in state by ID, update its title, save state, and render."

**👀 CHECK:**

✅ Gets task ID from data attribute
✅ Finds task in array: `state.tasks.find(t => t.id === id)`
✅ Updates the found task
✅ Doesn't create a new task

---

### Step 4: DELETE - Removing Tasks

**🗣️ SAY TO AIDE:**
> "Create a deleteTask function. When the delete button is clicked, confirm with the user, then remove the task from state.tasks by filtering it out by ID, save state, and render."

**👀 CHECK:**

✅ Confirmation before delete
✅ Uses filter: `state.tasks = state.tasks.filter(t => t.id !== id)`
✅ Doesn't mutate - creates new array

---

### Commit CRUD

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add create, update, and delete functionality'"

---

## 🗣️ Phase 7: Drag and Drop

This is the tricky part. Drag and Drop has multiple events.

### Understanding Drag Events

| Event | Fires When | On Element |
|-------|------------|------------|
| `dragstart` | User starts dragging | The dragged item |
| `dragend` | User stops dragging | The dragged item |
| `dragover` | Dragged item is over | The drop zone |
| `drop` | User drops the item | The drop zone |

### Step 1: Make Tasks Draggable

**🗣️ SAY TO AIDE:**
> "Add dragstart and dragend event listeners to all tasks. On dragstart, add a 'dragging' class and store the task ID using dataTransfer. On dragend, remove the dragging class."

---

## ✏️ FILL IN THE BLANKS: Drag and Drop API

### 🧠 CONCEPT: How Drag and Drop Works

Drag and Drop has TWO sides:

1. **The DRAGGED item** - the thing being moved
2. **The DROP ZONE** - where it can be dropped

```
┌─────────┐     drag     ┌─────────┐
│  Task   │  ──────→   │ Column  │
│ (drag)  │            │ (drop)  │
└─────────┘            └─────────┘
```

**Events on the DRAGGED item:**
- `dragstart` - user starts dragging
- `dragend` - user stops dragging

**Events on the DROP ZONE:**
- `dragover` - dragged item is hovering over
- `drop` - user releases the item here

### Exercise 1: Make an Element Draggable

```html
<!-- In HTML, add the draggable attribute -->
<div class="task" ___________="true" data-id="task-1">
    Learn Drag and Drop
</div>
```

```javascript
function setupDragListeners() {
    const tasks = document.querySelectorAll('.task');
    
    tasks.forEach(task => {
        // When drag starts
        task.addEventListener('___________', function(e) {
            // Add visual feedback
            this.classList._____('dragging');
            
            // Store the task ID so drop zone knows what's being dragged
            e.dataTransfer.________('text/plain', this.dataset.id);
        });
        
        // When drag ends
        task.addEventListener('___________', function() {
            this.classList.________('dragging');
        });
    });
}
```

<details>
<summary>✅ Check Your Answers</summary>

```html
<div class="task" draggable="true" data-id="task-1">
```

```javascript
task.addEventListener('dragstart', function(e) {
    this.classList.add('dragging');
    e.dataTransfer.setData('text/plain', this.dataset.id);
});

task.addEventListener('dragend', function() {
    this.classList.remove('dragging');
});
```

**Key Concepts:**
- `draggable="true"` - HTML attribute that enables dragging
- `e.dataTransfer.setData()` - stores data to pass to the drop zone
- `this.dataset.id` - reads the `data-id` attribute from HTML
</details>

---

### 🧠 CONCEPT: Preventing Default Behavior

By default, browsers DON'T allow dropping. You must:

```javascript
element.addEventListener('dragover', function(e) {
    e.preventDefault();  // THIS IS REQUIRED!
});
```

Without `preventDefault()`, the `drop` event will never fire!

### Exercise 2: Set Up Drop Zones

```javascript
function setupDropZones() {
    const columns = document.querySelectorAll('.column');
    
    columns.forEach(column => {
        // MUST prevent default or drop won't work!
        column.addEventListener('dragover', function(e) {
            e.________________();
            
            // Add visual feedback
            this.classList.add('drag-over');
        });
        
        // Remove highlight when leaving
        column.addEventListener('___________', function() {
            this.classList.remove('drag-over');
        });
        
        // Handle the actual drop
        column.addEventListener('______', function(e) {
            e.preventDefault();
            this.classList.remove('drag-over');
            
            // Get the task ID we stored in dragstart
            const taskId = e.dataTransfer.________('text/plain');
            
            // Get the column ID (this is where we dropped)
            const newColumn = this.____;
            
            // Move the task in our state
            moveTask(taskId, newColumn);
        });
    });
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
column.addEventListener('dragover', function(e) {
    e.preventDefault();
    this.classList.add('drag-over');
});

column.addEventListener('dragleave', function() {
    this.classList.remove('drag-over');
});

column.addEventListener('drop', function(e) {
    e.preventDefault();
    this.classList.remove('drag-over');
    
    const taskId = e.dataTransfer.getData('text/plain');
    const newColumn = this.id;
    moveTask(taskId, newColumn);
});
```

**The Flow:**
1. `dragover` + `preventDefault()` = "dropping is allowed here"
2. `dragleave` = "item left without dropping, remove highlight"
3. `drop` = "item dropped! Get the data and update state"
</details>

---

### 🧠 CONCEPT: Finding and Updating in Arrays

To move a task, we need to:
1. FIND the task in the array
2. UPDATE its `column` property

```javascript
// Find returns the actual object (or undefined)
const task = array.find(item => item.id === targetId);

// Now we can modify it directly
task.column = 'done';
```

### Exercise 3: Move Task Function

```javascript
function moveTask(taskId, newColumn) {
    // Find the task in our state array
    const task = state.tasks._______(t => t.____ === taskId);
    
    // If found, update its column
    if (______) {
        task.________ = newColumn;
        
        // Save to localStorage
        ___________();
        
        // Re-render the board
        ________();
    }
}
```

<details>
<summary>✅ Check Your Answers</summary>

```javascript
function moveTask(taskId, newColumn) {
    const task = state.tasks.find(t => t.id === taskId);
    
    if (task) {
        task.column = newColumn;
        saveState();
        render();
    }
}
```

**Why check `if (task)`?**
- `.find()` returns `undefined` if nothing matches
- Trying to access `.column` on `undefined` would crash
- Always check before using the result!
</details>

✅ Uses `event.dataTransfer.setData('text/plain', taskId)`
✅ Adds/removes `.dragging` class

---

### Step 2: Make Columns Drop Zones

**🗣️ SAY TO AIDE:**
> "Add dragover and drop event listeners to all column task containers. On dragover, prevent default to allow drops and add a 'drag-over' class. Also add a dragleave listener to remove the drag-over class when leaving."

**👀 CHECK:**

✅ `event.preventDefault()` in dragover (required!)
✅ Drag-over class added/removed properly

---

### Step 3: Handle the Drop

**🗣️ SAY TO AIDE:**
> "On drop, get the task ID from dataTransfer, get the target column ID from the drop zone's parent, find the task in state, update its column property to the new column, save state, and render."

**👀 CHECK:**

✅ Gets data: `event.dataTransfer.getData('text/plain')`
✅ Gets column: could be `event.target.closest('.column').id`
✅ Updates task's column property
✅ Saves and renders

---

### ⚠️ Common AI Problem: Drag Events on Wrong Elements

Drag and drop bugs are common. AI might:
- Attach events to wrong elements
- Forget `preventDefault()`
- Use wrong dataTransfer methods

**🚨 DEBUGGING APPROACH:**

**🗣️ SAY:**
> "Add console.log to each drag event: dragstart, dragover, dragleave, and drop. Log the event type and the element it fired on."

Then drag something and watch the console. You'll see exactly what's happening (or not happening).

---

### Test Drag and Drop

1. Drag a task from To Do
2. It should visually show as dragging
3. Hover over another column - should show drop feedback
4. Drop - task should move
5. Refresh - task should stay in new column

---

### Commit Drag & Drop

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add drag and drop between columns'"

---

## 🗣️ Phase 8: Polish

### Step 1: Task Count Badges

**🗣️ SAY TO AIDE:**
> "Add a count badge next to each column title showing how many tasks are in that column. Update the counts in the render function."

---

### Step 2: Empty State

**🗣️ SAY TO AIDE:**
> "When a column has no tasks, show a placeholder message like 'No tasks yet' or 'Drop tasks here' with a subtle style."

---

### Step 3: Keyboard Shortcuts

**🗣️ SAY TO AIDE:**
> "Add keyboard shortcuts: pressing 'n' opens the add task modal, pressing Escape closes any modal."

---

### Step 4: Due Dates (Optional)

**🗣️ SAY TO AIDE:**
> "Add an optional due date field to tasks. In the modal, add a date picker. Display the due date on the task card, and highlight tasks that are past due in red."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add task counts, empty states, and keyboard shortcuts'"

---

## ⚠️ Common AI Problem: Event Listener Buildup

When rendering creates new elements with listeners, you might get duplicate listeners or listeners on old elements.

**🚨 SIGNS:**
- Click fires twice
- Deleted items' events still fire
- Performance gets worse

**🗣️ FIX:**

> "Use event delegation. Instead of adding listeners to each task, add one listener to the column container that checks which task was clicked."

```javascript
// Instead of: task.addEventListener('click', ...)
// Do this:
container.addEventListener('click', (e) => {
    if (e.target.matches('.delete-btn')) {
        const taskId = e.target.closest('.task').dataset.id;
        deleteTask(taskId);
    }
});
```

---

## 🎓 What You Learned

### AI Communication:
- ✅ Debug complex interactions with console.log
- ✅ Break multi-step features into individual steps
- ✅ Describe event flow clearly

### Code Concepts:
- ✅ **CRUD operations** - full data lifecycle
- ✅ **Drag and Drop API** - native browser feature
- ✅ **Event delegation** - efficient event handling
- ✅ **Data attributes** - connecting UI to data
- ✅ **Array methods** - find, filter, push

### Project Skills:
- ✅ Model your data before coding
- ✅ Test each feature independently
- ✅ Handle edge cases (empty states)

---

## 🎁 Unlock Your Reward!

Dede now has a superhero cape! Ready to save the day! 🦸

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Task priorities** - "Add high/medium/low priority with color coding"
2. **Multiple boards** - "Let users create multiple kanban boards"
3. **Subtasks** - "Add checklists inside tasks"
4. **Collaboration** - "Sync with localStorage across tabs"

---

## 📖 Quick Reference

### CRUD Pattern

| Action | Array Method | Example |
|--------|--------------|---------|
| Create | `push()` | `tasks.push(newTask)` |
| Read | `find()` | `tasks.find(t => t.id === id)` |
| Update | `find()` + assign | `task.title = "New"` |
| Delete | `filter()` | `tasks.filter(t => t.id !== id)` |

### Drag & Drop Events

```javascript
// On draggable element:
dragstart → dragend

// On drop zone:
dragenter → dragover (repeatedly) → dragleave
                              or → drop
```

### Event Delegation

```javascript
// Efficient: one listener, many targets
container.addEventListener('click', (e) => {
    const button = e.target.closest('.my-button');
    if (button) {
        // Handle click
    }
});
```

---

*Next up: Project 6 - Real-Time Chat App! 💬 + 🚀*
