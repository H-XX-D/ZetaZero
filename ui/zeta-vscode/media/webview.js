(function(){
  const vscode = acquireVsCodeApi();
  const form = document.getElementById('input-form');
  const input = document.getElementById('input');
  const messages = document.getElementById('messages');
  const toolsBtn = document.getElementById('tools-btn');
  const shellBtn = document.getElementById('shell-btn');

  function addMessage(text, cls){
    const div = document.createElement('div');
    div.className = `message ${cls||''}`;
    div.textContent = text;
    messages.appendChild(div);
    messages.scrollTop = messages.scrollHeight;
  }

  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const text = input.value.trim();
    if (!text) return;
    addMessage(text, 'user');
    vscode.postMessage({ type: 'send', text });
    input.value='';
  });

  toolsBtn?.addEventListener('click', () => {
    vscode.postMessage({ type: 'listTools' });
  });

  shellBtn?.addEventListener('click', () => {
    const cmd = prompt('Enter shell command to run:');
    if (!cmd) return;
    const confirmRun = confirm('Run this command locally?\n\n' + cmd);
    if (!confirmRun) return;
    vscode.postMessage({ type: 'runShell', command: cmd });
  });

  let buffer = '';

  window.addEventListener('message', (event) => {
    const msg = event.data;
    if (msg.type === 'response') {
      addMessage(msg.text, 'model');
    } else if (msg.type === 'chunk') {
      buffer += msg.text;
      // render progressively (simple approach)
      if (buffer.length > 0) {
        if (!messages.lastChunk) {
          const div = document.createElement('div');
          div.className = 'message model';
          messages.appendChild(div);
          messages.lastChunk = div;
        }
        messages.lastChunk.textContent = buffer;
        messages.scrollTop = messages.scrollHeight;
      }
    } else if (msg.type === 'done') {
      buffer = '';
      messages.lastChunk = null;
    } else if (msg.type === 'error') {
      addMessage('Error: ' + msg.text, 'model');
    } else if (msg.type === 'tools') {
      const list = Array.isArray(msg.tools) ? msg.tools : [];
      addMessage('Tools: ' + list.map(t => t.name || t).join(', '), 'model');
      // Prompt user to execute a tool
      const name = prompt('Execute tool by name:');
      if (name) {
        try {
          const argsStr = prompt('Tool args as JSON (optional):') || '{}';
          const args = JSON.parse(argsStr);
          vscode.postMessage({ type: 'executeTool', name, args });
        } catch (e) {
          addMessage('Invalid JSON args', 'model');
        }
      }
    } else if (msg.type === 'toolResult') {
      addMessage('Tool Result (' + msg.name + '): ' + JSON.stringify(msg.result), 'model');
    } else if (msg.type === 'shellStart') {
      addMessage('Shell: ' + msg.command, 'model');
    } else if (msg.type === 'shellChunk') {
      addMessage(msg.text, 'model');
    } else if (msg.type === 'shellDone') {
      addMessage('Shell exited with code ' + msg.code, 'model');
    }
  });
})();
