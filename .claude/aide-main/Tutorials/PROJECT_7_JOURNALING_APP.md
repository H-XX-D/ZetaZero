# 📔 Project 7: Journaling App

## 🎁 Reward: Wizard Hat for Dede!
Complete this project and Dede becomes a mystical wizard! 🧙‍♂️

---

## 🌟 What You'll Learn

Build a personal journaling app with rich text, moods, and reflection features. This project teaches:

- ✅ **Rich text editing** - bold, italic, formatting
- ✅ **Date-based organization** - calendars, filtering
- ✅ **Search functionality** - finding entries
- ✅ **Data export** - backup and portability
- ✅ **Privacy patterns** - local-only sensitive data
- ✅ **TODO tracking** - managing tasks within your project

---

## 📓 NEW SKILL: The TASKS.md File

As projects grow, you need to track what's done and what's next.

**🗣️ SAY TO AIDE:**
> "Create a TASKS.md file in my project root"

**Structure it like this:**

```markdown
# Journal App - Task Tracker

## 🎯 Current Focus
Working on: Rich text toolbar formatting

## ✅ Completed
- [x] Basic HTML structure
- [x] Sidebar layout
- [x] Content editable area
- [x] Mood selector UI

## 🔄 In Progress
- [ ] Bold/Italic/Underline buttons
- [ ] Save entry to localStorage

## 📋 Backlog (Future)
- [ ] Search functionality
- [ ] Export to PDF
- [ ] Calendar view
- [ ] Tags system

## 🐛 Known Bugs
- Mood not saving with entry
- Scroll jumps when loading entries

## 💡 Ideas (Maybe Later)
- Password protection?
- Cloud sync?
- Writing prompts?
```

**🧠 WHY TASKS.md HELPS WITH AI:**

When starting a session, tell AI:
> "Here are my current tasks: [paste TASKS.md]. I want to work on [specific task]."

This keeps AI focused on ONE thing instead of trying to do everything.

**Update TASKS.md:**
- ✅ Move task to "Completed" when done
- 🔄 Update "Current Focus" when switching
- 🐛 Log bugs as you find them

**🗣️ END EACH SESSION BY SAYING:**
> "Update my TASKS.md to reflect what we accomplished"

---

## 🧠 New Concept: Content Editable

Instead of a plain `<textarea>`, you can make any element editable:

```html
<div contenteditable="true">
    Type anything here! Even <b>bold</b> text!
</div>
```

This lets users format their text naturally.

---

## 🗺️ Phase 1: Planning

### 📝 Your Turn: Journal Entry Structure

What should each journal entry contain?

- Date (when was it written?)
- Title (optional summary?)
- Content (the actual entry)
- Mood (how were they feeling?)
- Tags (categorization?)

**Decide your structure before coding.**

---

## 🗣️ Phase 2: Project Setup

**🗣️ SAY TO AIDE:**
> "Create a journaling-app project with index.html, css/styles.css, and js/app.js. Initialize git."

---

## 🗣️ Phase 3: HTML Structure

### Step 1: Main Layout

**🗣️ SAY TO AIDE:**
> "Create a journal app layout. Left sidebar shows a list of entries by date. Main area has the editor. Header has search and a 'New Entry' button."

---

### Step 2: Entry Editor

**🗣️ SAY TO AIDE:**
> "The editor area should have: a date display showing the current date, an editable title field, a mood selector with emoji options like happy, sad, neutral, excited, tired, and a contenteditable div for the main journal content with a placeholder."

**👀 CHECK:**

✅ Content div has `contenteditable="true"`
✅ Mood selector has clickable options
✅ Date displays nicely

---

### Step 3: Entry List

**🗣️ SAY TO AIDE:**
> "The sidebar entry list should show each saved entry with its date, title preview, and mood emoji. Most recent entries at top. Clicking an entry loads it in the editor."

---

### Step 4: Toolbar

**🗣️ SAY TO AIDE:**
> "Add a formatting toolbar above the content editor with buttons for bold, italic, and underline. Maybe add heading and list buttons too."

---

### Commit HTML

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add journal app HTML structure'"

---

## 🗣️ Phase 4: CSS Styling

### Step 1: Peaceful Design

**🗣️ SAY TO AIDE:**
> "Style the journal with a calm, peaceful design. Soft colors, good typography, plenty of whitespace. The writing area should feel inviting like a real journal page."

---

### Step 2: Entry List

**🗣️ SAY TO AIDE:**
> "Style the entry list items to show date prominently, title as preview, and mood emoji as a small indicator. Selected entry should be highlighted."

---

### Step 3: Mood Selector

**🗣️ SAY TO AIDE:**
> "Style the mood selector as a horizontal row of emoji buttons. Selected mood should have a ring or highlight around it."

---

### Step 4: Toolbar

**🗣️ SAY TO AIDE:**
> "Style the toolbar buttons as small icon buttons with tooltips. Active formatting should show as pressed/highlighted."

---

### Commit CSS

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add peaceful journal styling'"

---

## 🗣️ Phase 5: JavaScript - Rich Text

### Step 1: Formatting Commands

**🗣️ SAY TO AIDE:**
> "Add click handlers to toolbar buttons. Use document.execCommand for bold, italic, and underline. When a button is clicked, apply that formatting to selected text in the content editor."

**🧠 UNDERSTAND THIS:**
```javascript
document.execCommand('bold');      // Makes selected text bold
document.execCommand('italic');    // Makes selected text italic
```

This is a browser API for rich text editing.

---

### Step 2: Toolbar State

**🗣️ SAY TO AIDE:**
> "Update toolbar button states based on current selection. If the cursor is in bold text, the bold button should appear pressed. Use document.queryCommandState to check."

---

### 📝 Your Turn: Command State

Match the commands:

| To Check If... | Use |
|----------------|-----|
| Text is bold | `queryCommandState('____')` |
| Text is italic | `queryCommandState('____')` |

<details>
<summary>Click for answer</summary>

- `queryCommandState('bold')`
- `queryCommandState('italic')`
</details>

---

## 🗣️ Phase 6: Entry Management

### Step 1: State Structure

**🗣️ SAY TO AIDE:**
> "Create state with an entries array and currentEntryId. Each entry has id, date, title, content as HTML, and mood. Load entries from localStorage on start."

---

### Step 2: Creating Entries

**🗣️ SAY TO AIDE:**
> "The New Entry button creates a fresh entry with today's date, empty title and content, and neutral mood. Add it to entries array, save to localStorage, and load it into the editor."

---

### Step 3: Auto-Save

**🗣️ SAY TO AIDE:**
> "Auto-save the current entry as the user types. Use a debounce pattern - wait 1 second after they stop typing before saving. Update the entry list when content changes."

**🧠 DEBOUNCE:**
```javascript
let saveTimeout;
function scheduleAutoSave() {
    clearTimeout(saveTimeout);
    saveTimeout = setTimeout(() => {
        saveCurrentEntry();
    }, 1000);
}
```
This prevents saving on every keystroke.

---

### Step 4: Loading Entries

**🗣️ SAY TO AIDE:**
> "When clicking an entry in the sidebar, load its content into the editor. Update the date display, title, content HTML, and mood selector to match the entry."

---

### Step 5: Deleting Entries

**🗣️ SAY TO AIDE:**
> "Add a delete button to entries or the editor. Confirm before deleting. Remove from state, save to localStorage, and if the deleted entry was current, clear the editor or load another entry."

---

### Commit Entry Management

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add entry CRUD and auto-save'"

---

## ⚠️ Common AI Problem: Over-Engineering Auto-Save

AI might suggest complex solutions:
- Real-time sync
- Conflict resolution
- Version history

**🗣️ KEEP IT SIMPLE:**
> "Just localStorage auto-save is fine for now. No need for sync or versions."

---

## 🗣️ Phase 7: Search & Filter

### Step 1: Search

**🗣️ SAY TO AIDE:**
> "Add search functionality. When user types in the search box, filter the entry list to show only entries where the title or content contains the search term. Case insensitive."

---

### Step 2: Date Filter

**🗣️ SAY TO AIDE:**
> "Add a date filter dropdown with options: All Time, This Week, This Month, This Year. Filter the entry list based on selection."

---

### Step 3: Mood Filter

**🗣️ SAY TO AIDE:**
> "Add ability to filter by mood. Maybe a small row of mood emojis that work as filter toggles. Clicking one shows only entries with that mood."

---

### Commit Search

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add search and filter functionality'"

---

## 🗣️ Phase 8: Export & Backup

### Step 1: Export to JSON

**🗣️ SAY TO AIDE:**
> "Add an Export button that downloads all entries as a JSON file. Use a Blob and createObjectURL to trigger a download with a filename like journal-backup-2024-01-15.json"

---

### Step 2: Import from JSON

**🗣️ SAY TO AIDE:**
> "Add an Import button that accepts a JSON file. Read the file, parse the entries, merge them with existing entries avoiding duplicates by ID, save to localStorage, and refresh the UI."

---

### Step 3: Export to Markdown

**🗣️ SAY TO AIDE:**
> "Add option to export all entries as a single Markdown file. Each entry should have a heading with the date, the title, mood, and the content converted from HTML to Markdown."

---

### Commit Export

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add export and import functionality'"

---

## 🗣️ Phase 9: Polish

### Step 1: Word Count

**🗣️ SAY TO AIDE:**
> "Show a live word count at the bottom of the editor. Update as the user types."

---

### Step 2: Streak Counter

**🗣️ SAY TO AIDE:**
> "Track journaling streaks. Show how many consecutive days the user has written an entry. Display prominently to encourage daily writing."

---

### Step 3: Random Past Entry

**🗣️ SAY TO AIDE:**
> "Add a 'Memory' feature that shows a random entry from the past, like 'On this day last year you wrote...' Great for reflection."

---

### Final Commit

**🗣️ SAY TO AIDE:**
> "Commit with message 'Add word count, streaks, and memory feature'"

---

## 🎓 What You Learned

### AI Communication:
- ✅ Ask for specific patterns (debounce, auto-save)
- ✅ Decline over-engineering
- ✅ Request privacy-respecting patterns

### Code Concepts:
- ✅ **contenteditable** - rich text in browser
- ✅ **execCommand** - formatting commands
- ✅ **Debounce** - delaying actions
- ✅ **Blob/URL** - file downloads
- ✅ **FileReader** - file uploads

### Project Skills:
- ✅ User experience focus
- ✅ Data portability (import/export)
- ✅ Engagement features (streaks)

---

## 🎁 Unlock Your Reward!

Dede is now a mystical wizard! 🧙‍♂️

**Go to the Wardrobe in AIDE to equip it!**

---

## 🚀 Bonus Challenges

1. **Prompts** - "Add writing prompts for when users don't know what to write"
2. **Tags** - "Add tagging system for entries"
3. **Encryption** - "Add optional password protection"
4. **Statistics** - "Show writing statistics over time"

---

*Next up: Project 8 - Budgeting App! 💰 + 📊*
