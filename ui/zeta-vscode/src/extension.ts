import * as vscode from 'vscode';
import { MCPClient } from './mcpClient';

export function activate(context: vscode.ExtensionContext) {
  const provider = new ZetaChatViewProvider(context);
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider('zetaZero.chatView', provider)
  );

  context.subscriptions.push(
    vscode.commands.registerCommand('zetaZero.openChat', () => {
      provider.reveal();
    })
  );
}

export function deactivate() {}

class ZetaChatViewProvider implements vscode.WebviewViewProvider {
  private view?: vscode.WebviewView;
  private mcp?: MCPClient;
  private shellRunning = false;

  constructor(private readonly context: vscode.ExtensionContext) {}

  resolveWebviewView(view: vscode.WebviewView): void | Thenable<void> {
    this.view = view;
    view.webview.options = {
      enableScripts: true,
    };
    view.webview.html = this.getHtml(view.webview);

    view.webview.onDidReceiveMessage(async (message) => {
      if (message.type === 'send') {
        const serverUrl = vscode.workspace.getConfiguration().get<string>('zetaZero.serverUrl', 'http://localhost:9000');
        const stream = vscode.workspace.getConfiguration().get<boolean>('zetaZero.stream', true);
        try {
          const response = await fetch(`${serverUrl}/chat`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ prompt: message.text, stream })
          });
          if (stream && response.body) {
            // Stream chunks to the webview
            const reader = response.body.getReader();
            const decoder = new TextDecoder();
            while (true) {
              const { done, value } = await reader.read();
              if (done) break;
              const chunk = decoder.decode(value);
              view.webview.postMessage({ type: 'chunk', text: chunk });
            }
            view.webview.postMessage({ type: 'done' });
          } else {
            const text = await response.text();
            view.webview.postMessage({ type: 'response', text });
          }
        } catch (err: any) {
          view.webview.postMessage({ type: 'error', text: String(err) });
        }
      } else if (message.type === 'listTools') {
        try {
          await this.ensureMcp();
          const tools = await this.mcp!.listTools();
          view.webview.postMessage({ type: 'tools', tools });
        } catch (err: any) {
          view.webview.postMessage({ type: 'error', text: 'MCP listTools failed: ' + String(err) });
        }
      } else if (message.type === 'executeTool') {
        try {
          await this.ensureMcp();
          const result = await this.mcp!.executeTool(message.name, message.args || {});
          view.webview.postMessage({ type: 'toolResult', name: message.name, result });
        } catch (err: any) {
          view.webview.postMessage({ type: 'error', text: 'MCP executeTool failed: ' + String(err) });
        }
      } else if (message.type === 'runShell') {
        try {
          await this.runShell(message.command);
        } catch (err: any) {
          view.webview.postMessage({ type: 'error', text: 'Shell failed: ' + String(err) });
        }
      }
    });
  }

  reveal() {
    if (this.view) {
      this.view.show?.(true);
    }
  }

  private getHtml(webview: vscode.Webview): string {
    const scriptUri = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'media', 'webview.js'));
    const styleUri = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'media', 'webview.css'));
    const iconUri = webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, 'media', 'zeta.svg'));
    const nonce = Date.now().toString();
    return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src ${webview.cspSource}; style-src ${webview.cspSource} 'unsafe-inline'; script-src 'nonce-${nonce}';">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<link rel="stylesheet" href="${styleUri}">
<title>Zeta Zero Chat</title>
</head>
<body>
<header class="header">
  <img src="${iconUri}" class="logo" alt="Zeta Zero">
  <span>Zeta Zero Chat</span>
  <div class="spacer"></div>
  <button id="tools-btn" title="List Tools">Tools</button>
  <button id="shell-btn" title="Run Shell">Shell</button>
</header>
<main id="chat">
  <div id="messages" class="messages"></div>
  <form id="input-form">
    <input id="input" type="text" placeholder="Ask ZetaLm..." autocomplete="off" />
    <button type="submit">Send</button>
  </form>
</main>
<script nonce="${nonce}" src="${scriptUri}"></script>
</body>
</html>`;
  }

  private async ensureMcp(): Promise<void> {
    const enabled = vscode.workspace.getConfiguration().get<boolean>('zetaZero.toolsEnabled', true);
    if (!enabled) throw new Error('Tools are disabled in settings');
    if (!this.mcp) {
      const endpoint = vscode.workspace.getConfiguration().get<string>('zetaZero.mcpEndpoint', 'ws://localhost:9001');
      this.mcp = new MCPClient(endpoint);
      await this.mcp.connect();
    }
  }

  private async runShell(command: string): Promise<void> {
    if (this.shellRunning) throw new Error('A shell command is already running');
    const cfg = vscode.workspace.getConfiguration();
    const allow = cfg.get<boolean>('zetaZero.allowShell', false);
    if (!allow) throw new Error('Shell bridge is disabled in settings');
    if (!command || typeof command !== 'string') throw new Error('Invalid command');
    const allowlist = cfg.get<string[]>('zetaZero.shellAllowlist', ['echo','ls','cat','pwd']);
    const allowed = allowlist.some(p => command.trim().startsWith(p + ' ') || command.trim() === p);
    if (!allowed) throw new Error('Command not in allowlist');
    const cwdSetting = cfg.get<string>('zetaZero.shellCwd', '${workspaceFolder}');
    let cwd = process.cwd();
    if (cwdSetting && cwdSetting.includes('${workspaceFolder}')) {
      const wf = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
      if (wf) cwd = wf;
    } else if (cwdSetting) {
      cwd = cwdSetting;
    }
    const view = this.view;
    if (!view) throw new Error('View not ready');
    const cp = await import('child_process');
    this.shellRunning = true;
    view.webview.postMessage({ type: 'shellStart', command });
    const proc = cp.spawn('bash', ['-lc', command], { cwd });
    proc.stdout.on('data', (d: Buffer) => {
      view.webview.postMessage({ type: 'shellChunk', text: d.toString() });
    });
    proc.stderr.on('data', (d: Buffer) => {
      view.webview.postMessage({ type: 'shellChunk', text: d.toString() });
    });
    proc.on('close', (code: number) => {
      this.shellRunning = false;
      view.webview.postMessage({ type: 'shellDone', code });
    });
    proc.on('error', (err: Error) => {
      this.shellRunning = false;
      view.webview.postMessage({ type: 'error', text: 'Shell error: ' + String(err) });
    });
  }
}
