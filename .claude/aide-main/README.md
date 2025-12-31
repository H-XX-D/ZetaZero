# AIDE - AI Development Environment

<p align="center">
  <img src="assets/icon.png" width="120" alt="Dee Dee">
</p>

**A cute, floating mini-IDE with AI integration.** Dee Dee lives at the edge of your screen, ready to help when you need him.

## ✨ Features

### 🆓 Free & Open Source (GPL v2)
- 📝 **Tabbed Code Editor** with syntax highlighting
- 💬 **AI Chat** panel for assistance
- 💻 **Terminal** OR 🐛 **Debug Console** (pick one)
- 📁 **File Explorer** sidebar
- 🤖 **Dee Dee mascot** - docks to any screen edge
- 🎨 Beautiful dark theme

### ⭐ Pro Features ($5 or $15)per month 
- 🎤 **Voice Activation** - say "Aye Dee Dee" to summon
- ✅ **TODO List** sidebar
- 🐛 **Both Terminal AND Debug Console** 
- ↔️ **Drag & drop** panel rearrangement

## 🔧 Open Source Philosophy

AIDE follows the **VS Code model**:

- **Core is fully open source** under GPL v2
- Anyone can fork, modify, and redistribute
- Community contributions welcome
- Pro features are **convenience, not lock-out**
- You can implement any Pro feature yourself from the source

**Why Pro exists:**
- Supports ongoing development
- Pre-built convenience for those who want it
- Totally optional - free version is fully functional

## 🚀 Installation

```bash
# Clone the repo
git clone https://github.com/yourusername/aide.git
cd aide

# Install dependencies
npm install

# Run
npm start
```

## 📦 Building

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions.

```bash
# Build renderer bundle
npm run build:renderer

# Build for your platform
npm run build:mac      # macOS
npm run build:win      # Windows
npm run build:linux    # Linux
npm run build:all      # All platforms
```

## 🎮 Usage

1. **Dee Dee appears** at the edge of your screen (default: right)
2. **Hover** over the purple antenna ball to peek
3. **Click** to open the IDE
4. **Double-click** outside or press Escape to hide

### Keyboard Shortcuts
- `Ctrl+S` - Save current file
- `Ctrl+B` - Cycle close behavior
- `Escape` - Hide window (if not pinned)

### Dock Position
Right-click the menu bar icon to change which edge Dee Dee lives on.

## 🎤 Voice Activation (Pro)

Say **"Aye Dee Dee"** or **"Hey Dee Dee"** to summon the IDE from anywhere.

Requires microphone permission.

## 📄 License

**GPL v2** - Free and open source.

See [LICENSE](LICENSE) for details.

---

Made with 💜 by the community



