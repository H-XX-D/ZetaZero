# Zeta Zero VS Code Extension

A VS Code extension to chat with the local Zeta Zero server.

- Server URL: `http://localhost:9000`
- Streaming supported via chunked HTTP responses
- Sidebar view with WebView chat UI

## Development

```bash
# from ui/zeta-vscode
npm install
npm run watch
# Press F5 in VS Code to launch Extension Development Host
```

## Settings
- `zetaZero.serverUrl`: Base URL (default `http://localhost:9000`)
- `zetaZero.stream`: Enable streaming (default `true`)

## Commands
- `Zeta Zero: Open Chat` opens the chat view
