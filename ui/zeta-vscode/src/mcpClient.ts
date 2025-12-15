import * as vscode from 'vscode';
import WebSocket, { RawData } from 'ws';

interface JsonRpcRequest {
  jsonrpc: '2.0';
  id: number;
  method: string;
  params?: any;
}
interface JsonRpcResponse {
  jsonrpc: '2.0';
  id: number;
  result?: any;
  error?: { code: number; message: string; data?: any };
}

export class MCPClient {
  private ws?: WebSocket;
  private reqId = 1;
  private pending = new Map<number, { resolve: (v: any)=>void; reject: (e: any)=>void }>();

  constructor(private readonly endpoint: string) {}

  async connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(this.endpoint);
      this.ws.on('open', () => {
        resolve();
      });
      this.ws.on('error', (err: Error) => {
        reject(err);
      });
      this.ws.on('message', (data: RawData) => {
        try {
          const msg = JSON.parse(String(data)) as JsonRpcResponse;
          if (msg && msg.id && this.pending.has(msg.id)) {
            const p = this.pending.get(msg.id)!;
            this.pending.delete(msg.id);
            if (msg.error) p.reject(new Error(msg.error.message));
            else p.resolve(msg.result);
          }
        } catch (e) {
          // ignore
        }
      });
    });
  }

  private call(method: string, params?: any): Promise<any> {
    return new Promise((resolve, reject) => {
      if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
        reject(new Error('MCP socket not open'));
        return;
      }
      const id = this.reqId++;
      const req: JsonRpcRequest = { jsonrpc: '2.0', id, method, params };
      this.pending.set(id, { resolve, reject });
      this.ws.send(JSON.stringify(req));
    });
  }

  async listTools(): Promise<any[]> {
    // Many MCP servers expose 'tools/list' or 'list_tools'. We'll try both.
    try {
      return await this.call('tools/list');
    } catch {
      return await this.call('list_tools');
    }
  }

  async executeTool(name: string, args: any): Promise<any> {
    // Common patterns: 'tools/execute' or 'execute_tool'
    try {
      return await this.call('tools/execute', { name, arguments: args });
    } catch {
      return await this.call('execute_tool', { name, arguments: args });
    }
  }
}
