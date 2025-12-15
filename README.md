# ZetaLm Chat - VS Code Extension

A Cyberpunk 2077-themed VS Code extension for chatting with your local ZetaLm AI model.

## Features

- **Cyberpunk Aesthetic**: CD Projekt Red-inspired UI with neon colors, grid backgrounds, and scan line effects
- **Streaming Responses**: Real-time token-by-token streaming from ZetaLm
- **Sidebar Integration**: Chat interface accessible from the VS Code activity bar
- **Conversation History**: Maintains full conversation context
- **Error Handling**: Clear error messages when server is unreachable

## Requirements

- ZetaLm server running on `http://localhost:9000`
- Node.js and npm

## Installation

1. Compile the extension:
   ```bash
   npm install
   npm run compile
   ```

2. Press `F5` in VS Code to launch the extension in debug mode

3. Look for the **⚡ ZETAZERO** icon in the activity bar

## Usage

1. Click the ZetaLm icon in the sidebar
2. Type your message in the input box (placeholder: `>> QUERY_PROTOCOL_ACTIVE...`)
3. Press Enter or click **TRANSMIT**
4. Watch the AI response stream in real-time
5. Use **PURGE_MEM** to clear chat history

## Cyberpunk Theme Elements

- **Colors**: 
  - Primary: `#00ff9f` (cyan/green)
  - Secondary: `#ffed00` (yellow)
  - Accent: `#ff003c` (red/pink)
- **Effects**:
  - Animated scan line
  - Grid background
  - Glowing borders
  - Message slide-in animations
  - Blinking cursor during streaming

## Development

- `npm run compile`: Compile TypeScript
- `npm run watch`: Watch mode for development
- `npm run lint`: Run ESLint
- Press `F5` to debug

## Server Configuration

The extension connects to the ZetaLm server at `http://localhost:9000/v1/chat/completions`. 

Start your ZetaLm server:
```bash
cd llama.cpp
./launch_hippocampus.sh
```

## License

This repository is dual-licensed:
- **AGPL-3.0-or-later** OR **Commercial** (see `LICENSE` and `COMMERCIAL_LICENSE.md`).
- Enterprise: if your company has annual gross revenue > USD $2,000,000 and you use ZetaZero for revenue-generating products/services, contact todd@hendrixxdesign.com (see `COMMERCIAL_LICENSE.md`).
- Third-party components (e.g., `llama.cpp/`) remain under their original licenses (see `THIRD_PARTY_NOTICES.md`).
