/**
 * AIDE - Renderer
 * File explorer + 4 quadrant layout with drag-drop rearrange
 * 
 * FREE: Editor, Terminal OR Debug (one), AI Chat, File Explorer
 * PRO ($20 or $3/mo): Voice, TODO List, Both Terminal AND Debug, Drag-drop
 */

// Catch all errors
window.onerror = function(msg, url, line, col, error) {
    console.error('RENDERER ERROR:', msg, 'at', url, line, col);
    return false;
};

const { ipcRenderer } = require('electron');
const fs = require('fs');
const path = require('path');
const os = require('os');

console.log('AIDE Renderer starting...');

// === STATE ===
let closeBehavior = 'single'; // 'single', 'double', 'minimize'
let currentDir = os.homedir();
let openFiles = new Map(); // path -> content
let currentFile = null;
let features = { isPro: false }; // Will be loaded from main process
let freeModePanelChoice = 'terminal'; // 'terminal' or 'debug' - user picks one in free mode
let monacoEditor = null; // Monaco editor instance

// === ELEMENTS ===
const eyeLeft = document.getElementById('eyeLeft');
const eyeRight = document.getElementById('eyeRight');
const mascotFace = document.getElementById('mascotFace');
const sidebar = document.getElementById('sidebar');
const todoSidebar = document.getElementById('todoSidebar');
const mainContainer = document.getElementById('mainContainer');
const fileTree = document.getElementById('fileTree');
const welcomeOverlay = document.getElementById('welcomeOverlay');

// === MONACO EDITOR ===
function initMonaco() {
    if (!window.monaco) {
        console.log('Monaco not loaded yet, waiting...');
        return;
    }
    
    const container = document.getElementById('monacoEditor');
    if (!container) return;
    
    // Detect theme
    const isDark = !document.body.classList.contains('light-mode');
    
    monacoEditor = monaco.editor.create(container, {
        value: '',
        language: 'javascript',
        theme: isDark ? 'vs-dark' : 'vs',
        automaticLayout: true,
        minimap: { enabled: true },
        fontSize: 13,
        fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
        lineNumbers: 'on',
        roundedSelection: true,
        scrollBeyondLastLine: false,
        wordWrap: 'on',
        tabSize: 4,
        insertSpaces: true,
        folding: true,
        glyphMargin: true,
        renderLineHighlight: 'all',
        cursorBlinking: 'smooth',
        cursorSmoothCaretAnimation: 'on',
        smoothScrolling: true,
        padding: { top: 10 }
    });
    
    // Update status bar on cursor change
    monacoEditor.onDidChangeCursorPosition((e) => {
        const pos = e.position;
        document.getElementById('editorPos').textContent = `Ln ${pos.lineNumber}, Col ${pos.column}`;
    });
    
    // Track changes
    monacoEditor.onDidChangeModelContent(() => {
        if (currentFile) {
            openFiles.set(currentFile, monacoEditor.getValue());
            markTabDirty(currentFile);
        }
    });
    
    console.log('Monaco Editor initialized!');
}

// Wait for Monaco to load
if (window.monacoLoaded) {
    initMonaco();
} else {
    window.addEventListener('monaco-ready', initMonaco);
}

// Get language from file extension
function getLanguageFromPath(filePath) {
    const ext = path.extname(filePath).toLowerCase();
    const langMap = {
        '.js': 'javascript',
        '.jsx': 'javascript',
        '.ts': 'typescript',
        '.tsx': 'typescript',
        '.py': 'python',
        '.rb': 'ruby',
        '.go': 'go',
        '.rs': 'rust',
        '.java': 'java',
        '.c': 'c',
        '.cpp': 'cpp',
        '.h': 'cpp',
        '.cs': 'csharp',
        '.php': 'php',
        '.html': 'html',
        '.htm': 'html',
        '.css': 'css',
        '.scss': 'scss',
        '.less': 'less',
        '.json': 'json',
        '.xml': 'xml',
        '.yaml': 'yaml',
        '.yml': 'yaml',
        '.md': 'markdown',
        '.sql': 'sql',
        '.sh': 'shell',
        '.bash': 'shell',
        '.zsh': 'shell',
        '.dockerfile': 'dockerfile',
        '.swift': 'swift',
        '.kt': 'kotlin',
        '.lua': 'lua',
        '.r': 'r',
        '.vue': 'vue',
        '.svelte': 'svelte'
    };
    return langMap[ext] || 'plaintext';
}

// Hide welcome overlay when file is opened
function hideWelcome() {
    if (welcomeOverlay) {
        welcomeOverlay.classList.add('hidden');
    }
}

// Show welcome when no files open
function showWelcome() {
    if (welcomeOverlay && openFiles.size === 0) {
        welcomeOverlay.classList.remove('hidden');
    }
}

// === TODO STATE ===
let todos = JSON.parse(localStorage.getItem('aide-todos') || '[]');

// === API SETTINGS STATE ===
let apiSettings = JSON.parse(localStorage.getItem('aide-api-settings') || JSON.stringify({
    provider: 'openai',
    apiKey: '',
    model: 'gpt-4',
    customEndpoint: ''
}));
let userTier = localStorage.getItem('aide-tier') || 'free'; // 'free', 'pro', 'proplus'

// === CLOSE BEHAVIOR (eyes) ===
// No glow = single click off hides
// One eye = double click off hides
// Both eyes = only minimize hides (stays on top)

function updateEyes() {
    console.log('updateEyes called with mode:', closeBehavior);
    eyeLeft.classList.remove('glowing');
    eyeRight.classList.remove('glowing');
    
    switch (closeBehavior) {
        case 'single': 
            // No glow - single click off
            console.log('Setting: no eyes glowing');
            break;
        case 'double': 
            // One eye glowing - double click off
            console.log('Setting: left eye glowing');
            eyeLeft.classList.add('glowing'); 
            break;
        case 'minimize': 
            // Both eyes glowing - minimize only
            console.log('Setting: both eyes glowing');
            eyeLeft.classList.add('glowing');
            eyeRight.classList.add('glowing');
            break;
    }
    console.log('Eye classes after update - left:', eyeLeft.classList.contains('glowing'), 'right:', eyeRight.classList.contains('glowing'));
    
    // Update tooltip
    const tips = {
        'single': 'Single click off to hide',
        'double': 'Double click off to hide',
        'minimize': 'Minimize only (stays on top)'
    };
    mascotFace.title = tips[closeBehavior] || 'Click to change hide behavior';
}

mascotFace.addEventListener('click', () => {
    console.log('Mascot face clicked! Current mode:', closeBehavior);
    closeBehavior = closeBehavior === 'single' ? 'double' : closeBehavior === 'double' ? 'minimize' : 'single';
    console.log('New mode:', closeBehavior);
    updateEyes();
    ipcRenderer.send('set-close-behavior', closeBehavior);
    log('info', `Hide mode: ${closeBehavior}`);
});

// Right-click context menu for sidebar management
mascotFace.addEventListener('contextmenu', (e) => {
    e.preventDefault();
    showSidebarContextMenu(e);
});

updateEyes();

// === WINDOW CONTROLS ===
document.getElementById('closeBtn').addEventListener('click', () => ipcRenderer.send('close-ide'));
document.getElementById('minimizeBtn').addEventListener('click', () => ipcRenderer.send('minimize-ide'));

// === THEME TOGGLE ===
let isDarkMode = localStorage.getItem('aide-theme') !== 'light';
const themeToggle = document.getElementById('themeToggle');

function applyTheme() {
    if (isDarkMode) {
        document.body.classList.remove('light-mode');
        themeToggle.innerHTML = '<i class="fas fa-sun"></i>';
        themeToggle.title = 'Switch to Light Mode';
        // Update Monaco theme
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs-dark');
        }
    } else {
        document.body.classList.add('light-mode');
        themeToggle.innerHTML = '<i class="fas fa-moon"></i>';
        themeToggle.title = 'Switch to Dark Mode';
        // Update Monaco theme
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs');
        }
    }
    localStorage.setItem('aide-theme', isDarkMode ? 'dark' : 'light');
}

themeToggle.addEventListener('click', () => {
    isDarkMode = !isDarkMode;
    applyTheme();
});

applyTheme();

// === API SETTINGS MODAL ===
const settingsModal = document.getElementById('settingsModal');
const settingsBtn = document.getElementById('settingsBtn');
const aiProviderSelect = document.getElementById('aiProvider');
const apiKeyInput = document.getElementById('apiKeyInput');
const aiModelSelect = document.getElementById('aiModel');
const customEndpointInput = document.getElementById('customEndpoint');
const customEndpointSection = document.getElementById('customEndpointSection');
const apiStatus = document.getElementById('apiStatus');
const tierBadge = document.getElementById('tierBadge');
const tierDescription = document.getElementById('tierDescription');
const apiKeySection = document.getElementById('apiKeySection');

// Model options per provider
const providerModels = {
    openai: ['gpt-4', 'gpt-4-turbo', 'gpt-4o', 'gpt-4o-mini', 'gpt-3.5-turbo'],
    anthropic: ['claude-3-opus-20240229', 'claude-3-sonnet-20240229', 'claude-3-haiku-20240307', 'claude-3-5-sonnet-20241022'],
    openrouter: ['openai/gpt-4', 'anthropic/claude-3-opus', 'google/gemini-pro', 'meta-llama/llama-3-70b'],
    ollama: ['llama3', 'mistral', 'codellama', 'phi3', 'gemma'],
    custom: ['default']
};

function openSettingsModal() {
    // Load current settings
    aiProviderSelect.value = apiSettings.provider;
    apiKeyInput.value = apiSettings.apiKey;
    customEndpointInput.value = apiSettings.customEndpoint;
    updateModelOptions();
    aiModelSelect.value = apiSettings.model;
    updateTierDisplay();
    customEndpointSection.style.display = apiSettings.provider === 'custom' ? 'block' : 'none';
    
    settingsModal.classList.add('show');
}

function closeSettingsModal() {
    settingsModal.classList.remove('show');
}

window.closeSettingsModal = closeSettingsModal;

function updateTierDisplay() {
    tierBadge.className = 'tier-badge ' + userTier;
    
    if (userTier === 'proplus') {
        tierBadge.textContent = 'PRO+';
        tierDescription.textContent = 'API credits included - no key needed!';
        apiKeySection.style.display = 'none';
    } else if (userTier === 'pro') {
        tierBadge.textContent = 'PRO';
        tierDescription.textContent = 'Bring your own API key to use AI features';
        apiKeySection.style.display = 'block';
    } else {
        tierBadge.textContent = 'FREE';
        tierDescription.textContent = 'Bring your own API key to use AI features';
        apiKeySection.style.display = 'block';
    }
}

// Update usage counter for Pro+ users
function updateUsageCounter() {
    const counter = document.getElementById('usageCounter');
    if (!counter || !features.isProPlus) return;
    
    const used = features.budgetUsed || 0;
    const total = features.totalBudget || 10;
    const remaining = total - used;
    
    counter.textContent = `$${used.toFixed(2)} / $${total.toFixed(2)}`;
    
    // Color based on usage
    counter.classList.remove('warning', 'danger');
    const percentUsed = (used / total) * 100;
    if (percentUsed >= 90) {
        counter.classList.add('danger');
    } else if (percentUsed >= 70) {
        counter.classList.add('warning');
    }
}

// Track API usage cost (called after each API call)
function trackUsage(cost) {
    if (!features.isProPlus) return;
    
    features.budgetUsed = (features.budgetUsed || 0) + cost;
    updateUsageCounter();
    
    // Persist to main process
    ipcRenderer.invoke('track-usage', cost);
}

function updateModelOptions() {
    const provider = aiProviderSelect.value;
    const models = providerModels[provider] || ['default'];
    
    aiModelSelect.innerHTML = models.map(m => 
        `<option value="${m}">${m}</option>`
    ).join('');
    
    customEndpointSection.style.display = provider === 'custom' ? 'block' : 'none';
}

aiProviderSelect.addEventListener('change', () => {
    updateModelOptions();
    saveApiSettings();
});

apiKeyInput.addEventListener('change', saveApiSettings);
aiModelSelect.addEventListener('change', saveApiSettings);
customEndpointInput.addEventListener('change', saveApiSettings);

function saveApiSettings() {
    apiSettings = {
        provider: aiProviderSelect.value,
        apiKey: apiKeyInput.value,
        model: aiModelSelect.value,
        customEndpoint: customEndpointInput.value
    };
    localStorage.setItem('aide-api-settings', JSON.stringify(apiSettings));
    log('info', 'API settings saved');
}

window.toggleKeyVisibility = function() {
    const input = apiKeyInput;
    const icon = document.getElementById('keyEyeIcon');
    if (input.type === 'password') {
        input.type = 'text';
        icon.classList.remove('fa-eye');
        icon.classList.add('fa-eye-slash');
    } else {
        input.type = 'password';
        icon.classList.remove('fa-eye-slash');
        icon.classList.add('fa-eye');
    }
};

window.testApiConnection = async function() {
    apiStatus.className = 'api-status';
    apiStatus.style.display = 'none';
    
    const key = apiKeyInput.value.trim();
    const provider = aiProviderSelect.value;
    
    if (provider !== 'ollama' && !key) {
        apiStatus.className = 'api-status error';
        apiStatus.textContent = '❌ Please enter an API key';
        apiStatus.style.display = 'block';
        return;
    }
    
    apiStatus.className = 'api-status';
    apiStatus.textContent = '⏳ Testing connection...';
    apiStatus.style.display = 'block';
    
    try {
        // Test based on provider
        let testUrl, headers, body;
        
        switch (provider) {
            case 'openai':
                testUrl = 'https://api.openai.com/v1/models';
                headers = { 'Authorization': `Bearer ${key}` };
                break;
            case 'anthropic':
                testUrl = 'https://api.anthropic.com/v1/messages';
                headers = { 
                    'x-api-key': key,
                    'anthropic-version': '2023-06-01',
                    'content-type': 'application/json'
                };
                body = JSON.stringify({ model: 'claude-3-haiku-20240307', max_tokens: 1, messages: [{ role: 'user', content: 'hi' }] });
                break;
            case 'ollama':
                testUrl = 'http://localhost:11434/api/tags';
                headers = {};
                break;
            case 'openrouter':
                testUrl = 'https://openrouter.ai/api/v1/models';
                headers = { 'Authorization': `Bearer ${key}` };
                break;
            case 'custom':
                testUrl = customEndpointInput.value || 'http://localhost:8080/v1/models';
                headers = key ? { 'Authorization': `Bearer ${key}` } : {};
                break;
        }
        
        const response = await fetch(testUrl, { 
            method: body ? 'POST' : 'GET', 
            headers,
            body 
        });
        
        if (response.ok || response.status === 200) {
            apiStatus.className = 'api-status success';
            apiStatus.textContent = '✅ Connection successful!';
            saveApiSettings();
        } else {
            const errText = await response.text();
            apiStatus.className = 'api-status error';
            apiStatus.textContent = `❌ Error: ${response.status} - ${errText.slice(0, 100)}`;
        }
    } catch (err) {
        apiStatus.className = 'api-status error';
        apiStatus.textContent = `❌ Connection failed: ${err.message}`;
    }
};

window.upgradeTier = function(tier) {
    // In production, this would open a payment flow
    // For now, simulate upgrade
    if (tier === 'proplus') {
        window.open('https://aide.dev/upgrade?tier=proplus', '_blank');
    } else {
        window.open('https://aide.dev/upgrade?tier=pro', '_blank');
    }
};

settingsBtn.addEventListener('click', openSettingsModal);

// Close modal on overlay click
settingsModal.addEventListener('click', (e) => {
    if (e.target === settingsModal) {
        closeSettingsModal();
    }
});

// === SIDEBAR CONTROLS ===
document.getElementById('sidebarToggle').addEventListener('click', () => {
    const willOpen = sidebar.classList.contains('collapsed');
    
    // Free users: only one sidebar at a time
    if (willOpen && !features.bothSidebars && !todoSidebar.classList.contains('collapsed')) {
        todoSidebar.classList.add('collapsed');
        document.getElementById('todoToggle').classList.remove('active');
    }
    
    sidebar.classList.toggle('collapsed');
    document.getElementById('sidebarToggle').classList.toggle('active', !sidebar.classList.contains('collapsed'));
});

document.getElementById('todoToggle').addEventListener('click', () => {
    const willOpen = todoSidebar.classList.contains('collapsed');
    
    // Free users: only one sidebar at a time
    if (willOpen && !features.bothSidebars && !sidebar.classList.contains('collapsed')) {
        sidebar.classList.add('collapsed');
        document.getElementById('sidebarToggle').classList.remove('active');
    }
    
    todoSidebar.classList.toggle('collapsed');
    document.getElementById('todoToggle').classList.toggle('active', !todoSidebar.classList.contains('collapsed'));
});

document.getElementById('sidebarSide').addEventListener('click', () => {
    mainContainer.classList.toggle('sidebar-right');
});

// === TODO LIST ===
function saveTodos() {
    localStorage.setItem('aide-todos', JSON.stringify(todos));
    renderTodos();
}

function renderTodos() {
    const list = document.getElementById('todoList');
    list.innerHTML = '';
    
    todos.forEach((todo, idx) => {
        const el = document.createElement('div');
        el.className = 'todo-item' + (todo.done ? ' completed' : '');
        el.innerHTML = `
            <div class="checkbox"><i class="fas fa-check"></i></div>
            <span class="text">${todo.text}</span>
            <span class="delete" onclick="deleteTodo(${idx}); event.stopPropagation();"><i class="fas fa-times"></i></span>
        `;
        el.onclick = () => toggleTodo(idx);
        list.appendChild(el);
    });
    
    const done = todos.filter(t => t.done).length;
    document.getElementById('todoStats').textContent = `${done}/${todos.length} done`;
}

window.addTodo = function() {
    const input = document.getElementById('todoInput');
    const text = input.value.trim();
    if (!text) return;
    
    todos.push({ text, done: false });
    input.value = '';
    saveTodos();
};

window.toggleTodo = function(idx) {
    todos[idx].done = !todos[idx].done;
    saveTodos();
};

window.deleteTodo = function(idx) {
    todos.splice(idx, 1);
    saveTodos();
};

window.clearCompletedTodos = function() {
    todos = todos.filter(t => !t.done);
    saveTodos();
};

document.getElementById('todoInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') addTodo();
});

renderTodos();

// === SIDEBAR TABS ===
function switchSidebarTab(tabName) {
    const tab = document.querySelector(`.sidebar-tab[data-tab="${tabName}"]`);
    const content = document.getElementById(tabName + 'Tab');
    const isActive = tab.classList.contains('active');
    const activeCount = document.querySelectorAll('.sidebar-tab.active').length;
    
    // Determine max sidebars based on tier
    const maxSidebars = features.maxSidebars || 1;
    
    if (isActive) {
        // Clicking active tab - deselect it (but keep at least one if free)
        if (activeCount > 1 || maxSidebars > 1) {
            tab.classList.remove('active');
            content.classList.remove('active');
        }
    } else {
        // Clicking inactive tab
        if (activeCount < maxSidebars) {
            // Room for more - just add it
            tab.classList.add('active');
            content.classList.add('active');
        } else {
            // At max - swap (deselect oldest/first active)
            if (maxSidebars === 1) {
                // Free: simple swap
                document.querySelectorAll('.sidebar-tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.sidebar-tab-content').forEach(c => c.classList.remove('active'));
            } else {
                // Pro: remove first active to make room
                const firstActive = document.querySelector('.sidebar-tab.active');
                if (firstActive) {
                    firstActive.classList.remove('active');
                    document.getElementById(firstActive.dataset.tab + 'Tab').classList.remove('active');
                }
            }
            tab.classList.add('active');
            content.classList.add('active');
        }
    }
    
    // Load data for specific tabs
    if (tabName === 'git' && content.classList.contains('active')) {
        gitRefresh();
    }
}
window.switchSidebarTab = switchSidebarTab;

// Collapse/expand sidebar
function toggleSidebarCollapse() {
    sidebar.classList.toggle('collapsed');
}
window.toggleSidebarCollapse = toggleSidebarCollapse;

// Expand sidebar and switch to specific tab
function expandToTab(tabName) {
    sidebar.classList.remove('collapsed');
    switchSidebarTab(tabName);
}
window.expandToTab = expandToTab;


// === GIT PANEL ===
const { execSync } = require('child_process');

function isGitRepo() {
    try {
        execSync('git rev-parse --is-inside-work-tree', { cwd: currentDir, encoding: 'utf8' });
        return true;
    } catch {
        return false;
    }
}

function gitRefresh() {
    if (!isGitRepo()) {
        document.getElementById('gitBranch').innerHTML = '<i class="fas fa-code-branch"></i> Not a Git repo';
        document.getElementById('stagedFiles').innerHTML = '<div class="git-empty">Not a Git repository<br><small>Use the Files tab to browse all files</small></div>';
        document.getElementById('changedFiles').innerHTML = '';
        document.getElementById('stagedCount').textContent = '0';
        document.getElementById('changesCount').textContent = '0';
        return;
    }
    
    try {
        // Get current branch
        const branch = execSync('git branch --show-current', { cwd: currentDir, encoding: 'utf8' }).trim();
        document.getElementById('gitBranch').innerHTML = `<i class="fas fa-code-branch"></i> ${branch || 'HEAD'}`;
        
        // Get status
        const status = execSync('git status --porcelain', { cwd: currentDir, encoding: 'utf8' });
        const lines = status.split('\n').filter(l => l.trim());
        
        const staged = [];
        const changes = [];
        
        lines.forEach(line => {
            const indexStatus = line[0];
            const workStatus = line[1];
            const filename = line.substring(3);
            
            if (indexStatus !== ' ' && indexStatus !== '?') {
                staged.push({ status: indexStatus, file: filename });
            }
            if (workStatus !== ' ' || indexStatus === '?') {
                changes.push({ status: workStatus === ' ' ? indexStatus : workStatus, file: filename });
            }
        });
        
        renderGitFiles('stagedFiles', staged, true);
        renderGitFiles('changedFiles', changes, false);
        document.getElementById('stagedCount').textContent = staged.length;
        document.getElementById('changesCount').textContent = changes.length;
        
    } catch (e) {
        console.error('Git error:', e);
    }
}
window.gitRefresh = gitRefresh;

function renderGitFiles(containerId, files, isStaged) {
    const container = document.getElementById(containerId);
    
    if (files.length === 0) {
        container.innerHTML = '<div class="git-empty">No changes</div>';
        return;
    }
    
    container.innerHTML = files.map(f => {
        const iconClass = getGitStatusIcon(f.status);
        const action = isStaged 
            ? `<button onclick="gitUnstage('${f.file}')" title="Unstage"><i class="fas fa-minus"></i></button>`
            : `<button onclick="gitStage('${f.file}')" title="Stage"><i class="fas fa-plus"></i></button>
               <button onclick="gitDiscard('${f.file}')" title="Discard"><i class="fas fa-undo"></i></button>`;
        
        return `
            <div class="git-file" ondblclick="openFile('${path.join(currentDir, f.file).replace(/'/g, "\\'")}')">
                <span class="git-file-icon ${iconClass}"><i class="fas fa-circle"></i></span>
                <span class="git-file-name">${f.file}</span>
                <div class="git-file-actions">${action}</div>
            </div>
        `;
    }).join('');
}

function getGitStatusIcon(status) {
    switch (status) {
        case 'A': return 'added';
        case 'M': return 'modified';
        case 'D': return 'deleted';
        case '?': return 'untracked';
        default: return 'modified';
    }
}

function gitStage(file) {
    try {
        execSync(`git add "${file}"`, { cwd: currentDir });
        gitRefresh();
        log('info', `Staged: ${file}`);
    } catch (e) {
        log('error', `Failed to stage: ${e.message}`);
    }
}
window.gitStage = gitStage;

function gitUnstage(file) {
    try {
        execSync(`git reset HEAD "${file}"`, { cwd: currentDir });
        gitRefresh();
        log('info', `Unstaged: ${file}`);
    } catch (e) {
        log('error', `Failed to unstage: ${e.message}`);
    }
}
window.gitUnstage = gitUnstage;

function gitDiscard(file) {
    if (!confirm(`Discard changes to ${file}?`)) return;
    try {
        execSync(`git checkout -- "${file}"`, { cwd: currentDir });
        gitRefresh();
        log('info', `Discarded: ${file}`);
    } catch (e) {
        log('error', `Failed to discard: ${e.message}`);
    }
}
window.gitDiscard = gitDiscard;

function gitCommit() {
    const msg = document.getElementById('commitMessage').value.trim();
    if (!msg) {
        log('warn', 'Please enter a commit message');
        return;
    }
    
    try {
        execSync(`git commit -m "${msg.replace(/"/g, '\\"')}"`, { cwd: currentDir });
        document.getElementById('commitMessage').value = '';
        gitRefresh();
        log('info', `Committed: ${msg}`);
    } catch (e) {
        log('error', `Commit failed: ${e.message}`);
    }
}
window.gitCommit = gitCommit;

function gitPull() {
    try {
        log('info', 'Pulling...');
        const result = execSync('git pull', { cwd: currentDir, encoding: 'utf8' });
        gitRefresh();
        log('info', result.trim() || 'Pull complete');
    } catch (e) {
        log('error', `Pull failed: ${e.message}`);
    }
}
window.gitPull = gitPull;

function gitPush() {
    try {
        log('info', 'Pushing...');
        const result = execSync('git push', { cwd: currentDir, encoding: 'utf8' });
        gitRefresh();
        log('info', result.trim() || 'Push complete');
    } catch (e) {
        log('error', `Push failed: ${e.message}`);
    }
}
window.gitPush = gitPush;

function toggleGitSection(section) {
    const el = document.querySelector(`#${section}Files`).closest('.git-section');
    el.classList.toggle('collapsed');
}
window.toggleGitSection = toggleGitSection;

// === FILE EXPLORER ===
function getFileIcon(filename, isDir) {
    if (isDir) return { class: 'fas', icon: 'fa-folder' };
    const ext = path.extname(filename).toLowerCase();
    const icons = {
        '.py': { class: 'fab', icon: 'fa-python' },
        '.js': { class: 'fab', icon: 'fa-js' },
        '.ts': { class: 'fab', icon: 'fa-js' },
        '.html': { class: 'fab', icon: 'fa-html5' },
        '.css': { class: 'fab', icon: 'fa-css3-alt' },
        '.json': { class: 'fas', icon: 'fa-file-code' },
        '.md': { class: 'fas', icon: 'fa-file-lines' },
        '.txt': { class: 'fas', icon: 'fa-file-lines' },
        '.yml': { class: 'fas', icon: 'fa-file-code' },
        '.yaml': { class: 'fas', icon: 'fa-file-code' },
        '.svg': { class: 'fas', icon: 'fa-image' },
        '.png': { class: 'fas', icon: 'fa-image' },
        '.jpg': { class: 'fas', icon: 'fa-image' },
        '.gif': { class: 'fas', icon: 'fa-image' },
        '.sh': { class: 'fas', icon: 'fa-terminal' },
        '.bash': { class: 'fas', icon: 'fa-terminal' },
        '.zsh': { class: 'fas', icon: 'fa-terminal' },
    };
    return icons[ext] || { class: 'fas', icon: 'fa-file-code' };
}

function formatSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
}

async function loadDirectory(dir) {
    // Check directory access permissions
    const accessAllowed = isDirectoryAccessAllowed(dir);
    if (accessAllowed === false) {
        // Access denied
        showDirectoryAccessDenied(dir);
        return;
    } else if (accessAllowed === null) {
        // Unknown - request permission
        const granted = await requestDirectoryAccess(dir);
        if (!granted) {
            showDirectoryAccessDenied(dir);
            return;
        }
    }

    // Access granted - proceed with loading
    currentDir = dir;
    const shortPath = dir.replace(os.homedir(), '~');
    document.getElementById('currentPath').textContent = shortPath;
    document.getElementById('terminalPath').textContent = shortPath;
    fileTree.innerHTML = '';
    
    try {
        // Parent directory
        if (dir !== '/') {
            const parentItem = document.createElement('div');
            parentItem.className = 'file-item folder';
            parentItem.innerHTML = '<i class="fas fa-level-up-alt"></i> ..';
            parentItem.onclick = () => loadDirectory(path.dirname(dir));
            fileTree.appendChild(parentItem);
        }
        
        const items = fs.readdirSync(dir, { withFileTypes: true });
        
        // Sort: folders first, then files
        items.sort((a, b) => {
            if (a.isDirectory() && !b.isDirectory()) return -1;
            if (!a.isDirectory() && b.isDirectory()) return 1;
            return a.name.localeCompare(b.name);
        });
        
        items.forEach(item => {
            if (item.name.startsWith('.')) return; // Skip hidden
            
            const fullPath = path.join(dir, item.name);
            const isDir = item.isDirectory();
            const icon = getFileIcon(item.name, isDir);
            
            const el = document.createElement('div');
            el.className = 'file-item' + (isDir ? ' folder' : '');
            el.dataset.path = fullPath;
            
            let sizeStr = '';
            if (!isDir) {
                try {
                    const stats = fs.statSync(fullPath);
                    sizeStr = `<span class="size">${formatSize(stats.size)}</span>`;
                } catch (e) {}
            }
            
            el.innerHTML = `<i class="${icon.class} ${icon.icon}"></i> ${item.name} ${sizeStr}`;
            
            // Add context menu for favorites
            el.oncontextmenu = (e) => {
                e.preventDefault();
                showContextMenu(e, fullPath, isDir, 'sidebar');
            };
            
            el.onclick = () => {
                if (isDir) {
                    loadDirectory(fullPath);
                } else {
                    openFile(fullPath);
                }
            };
            
            fileTree.appendChild(el);
        });
    } catch (e) {
        log('error', `Cannot read directory: ${e.message}`);
    }
}

window.refreshFiles = () => loadDirectory(currentDir);
window.goHome = () => loadDirectory(os.homedir());

// === FILE OPERATIONS ===
function openFile(filePath) {
    try {
        const content = fs.readFileSync(filePath, 'utf8');
        openFiles.set(filePath, content);
        currentFile = filePath;
        
        // Use Monaco if available, fallback to textarea
        if (monacoEditor) {
            const language = getLanguageFromPath(filePath);
            monaco.editor.setModelLanguage(monacoEditor.getModel(), language);
            monacoEditor.setValue(content);
            hideWelcome();
        }
        
        document.getElementById('editorFile').textContent = path.basename(filePath);
        
        // Update tabs
        updateTabs();
        
        // Highlight in tree
        document.querySelectorAll('.file-item').forEach(el => {
            el.classList.toggle('active', el.dataset.path === filePath);
        });
        
        log('info', `Opened: ${path.basename(filePath)}`);
    } catch (e) {
        log('error', `Cannot open file: ${e.message}`);
    }
}

let dirtyFiles = new Set(); // Track unsaved changes

function markTabDirty(filePath) {
    dirtyFiles.add(filePath);
    updateTabs();
}

function clearTabDirty(filePath) {
    dirtyFiles.delete(filePath);
    updateTabs();
}

function updateTabs() {
    const tabBar = document.getElementById('editorTabs');
    tabBar.innerHTML = '';
    
    openFiles.forEach((content, filePath) => {
        const tab = document.createElement('div');
        const isDirty = dirtyFiles.has(filePath);
        tab.className = 'tab' + (filePath === currentFile ? ' active' : '') + (isDirty ? ' dirty' : '');
        tab.innerHTML = `
            ${isDirty ? '● ' : ''}${path.basename(filePath)}
            <span class="close-tab" onclick="closeTab('${filePath.replace(/'/g, "\\'")}'); event.stopPropagation();">
                <i class="fas fa-times"></i>
            </span>
        `;
        tab.onclick = () => switchToTab(filePath);
        tabBar.appendChild(tab);
    });
    
    if (openFiles.size === 0) {
        tabBar.innerHTML = '<div class="tab active">untitled</div>';
        currentFile = null;
        if (monacoEditor) monacoEditor.setValue('');
        document.getElementById('editorFile').textContent = 'untitled';
        showWelcome();
    }
}

function switchToTab(filePath) {
    currentFile = filePath;
    const content = openFiles.get(filePath) || '';
    if (monacoEditor) {
        const language = getLanguageFromPath(filePath);
        monaco.editor.setModelLanguage(monacoEditor.getModel(), language);
        monacoEditor.setValue(content);
    }
    document.getElementById('editorFile').textContent = path.basename(filePath);
    updateTabs();
}

window.closeTab = function(filePath) {
    openFiles.delete(filePath);
    dirtyFiles.delete(filePath);
    
    if (currentFile === filePath) {
        // Switch to another tab
        const remaining = Array.from(openFiles.keys());
        if (remaining.length > 0) {
            switchToTab(remaining[remaining.length - 1]);
        } else {
            currentFile = null;
            if (monacoEditor) monacoEditor.setValue('');
            document.getElementById('editorFile').textContent = 'untitled';
            showWelcome();
        }
    }
    updateTabs();
};

window.newTab = function() {
    const name = 'untitled-' + Date.now();
    openFiles.set(name, '');
    switchToTab(name);
};

// Save on Ctrl+S / Cmd+S
document.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        e.preventDefault();
        if (currentFile) {
            try {
                const content = monacoEditor ? monacoEditor.getValue() : '';
                fs.writeFileSync(currentFile, content, 'utf8');
                openFiles.set(currentFile, content);
                clearTabDirty(currentFile);
                log('info', `Saved: ${path.basename(currentFile)}`);
            } catch (e) {
                log('error', `Cannot save: ${e.message}`);
            }
        }
    }
});

// Editor cursor position is handled by Monaco's onDidChangeCursorPosition
// No need for codeEditor listeners since we're using Monaco

// === PANEL COLLAPSE ===
window.togglePanel = function(panelName) {
    const panel = document.querySelector(`[data-panel="${panelName}"]`);
    if (panel) {
        panel.classList.toggle('collapsed');
        
        // If terminal is collapsed/expanded, adjust top row
        if (panelName === 'terminal') {
            const topRow = document.getElementById('topRow');
            const bottomRow = document.getElementById('bottomRow');
            
            if (panel.classList.contains('collapsed')) {
                bottomRow.style.flex = '0 0 30px';
                topRow.style.flex = '1';
            } else {
                bottomRow.style.flex = '1';
                topRow.style.flex = '2';
            }
        }
    }
};

// === DRAG & DROP PANEL REARRANGE ===
let draggedPanel = null;

document.querySelectorAll('.quad-panel').forEach(panel => {
    const header = panel.querySelector('.quad-header > span');
    if (!header) return;
    
    header.setAttribute('draggable', true);
    
    header.addEventListener('dragstart', (e) => {
        draggedPanel = panel;
        panel.style.opacity = '0.5';
        e.dataTransfer.effectAllowed = 'move';
    });
    
    header.addEventListener('dragend', () => {
        draggedPanel = null;
        panel.style.opacity = '1';
        document.querySelectorAll('.quad-panel').forEach(p => p.classList.remove('drag-over'));
    });
    
    panel.addEventListener('dragover', (e) => {
        e.preventDefault();
        if (draggedPanel && draggedPanel !== panel) {
            panel.classList.add('drag-over');
        }
    });
    
    panel.addEventListener('dragleave', () => {
        panel.classList.remove('drag-over');
    });
    
    panel.addEventListener('drop', (e) => {
        e.preventDefault();
        panel.classList.remove('drag-over');
        
        if (draggedPanel && draggedPanel !== panel) {
            // Swap panels
            const parent1 = draggedPanel.parentNode;
            const parent2 = panel.parentNode;
            const next1 = draggedPanel.nextSibling;
            const next2 = panel.nextSibling;
            
            // Handle resizers
            if (parent1 === parent2) {
                // Same row - swap positions
                parent1.insertBefore(draggedPanel, panel);
                parent1.insertBefore(panel, next1 === panel ? draggedPanel.nextSibling : next1);
            } else {
                // Different rows - swap between rows
                parent2.insertBefore(draggedPanel, next2);
                parent1.insertBefore(panel, next1);
            }
            
            log('info', 'Panels rearranged');
        }
    });
});

// Add drag-over style
const style = document.createElement('style');
style.textContent = '.quad-panel.drag-over { border: 2px dashed var(--primary) !important; }';
document.head.appendChild(style);

// === RESIZERS ===
function initResizer(resizerId, getPanels, isVertical) {
    const resizer = document.getElementById(resizerId);
    if (!resizer) return;
    
    let isResizing = false;
    let startPos, sizes;
    
    resizer.addEventListener('mousedown', (e) => {
        isResizing = true;
        const panels = getPanels();
        startPos = isVertical ? e.clientY : e.clientX;
        sizes = panels.map(p => isVertical ? p.offsetHeight : p.offsetWidth);
        document.body.style.cursor = isVertical ? 'row-resize' : 'col-resize';
        document.body.style.userSelect = 'none';
        e.preventDefault();
    });
    
    document.addEventListener('mousemove', (e) => {
        if (!isResizing) return;
        const panels = getPanels();
        const currentPos = isVertical ? e.clientY : e.clientX;
        const delta = currentPos - startPos;
        
        const newSize0 = Math.max(80, sizes[0] + delta);
        const newSize1 = Math.max(80, sizes[1] - delta);
        
        panels[0].style.flex = `0 0 ${newSize0}px`;
        panels[1].style.flex = `0 0 ${newSize1}px`;
    });
    
    document.addEventListener('mouseup', () => {
        if (isResizing) {
            isResizing = false;
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
        }
    });
}

initResizer('sidebarResizer', () => [sidebar, document.querySelector('.content-area')], false);
initResizer('todoResizer', () => [document.querySelector('.content-area'), todoSidebar], false);
initResizer('topResizer', () => [document.getElementById('editorPanel'), document.getElementById('chatPanel')], false);
// bottomResizer removed - no debug panel in current layout
initResizer('rowResizer', () => [document.getElementById('topRow'), document.getElementById('bottomRow')], true);

// === CHAT ===
const chatInput = document.getElementById('chatInput');
const chatMessages = document.getElementById('chatMessages');

function addMessage(role, content) {
    const msg = document.createElement('div');
    msg.className = `message ${role}`;
    msg.innerHTML = `
        <div class="avatar"><i class="fas fa-${role === 'assistant' ? 'robot' : 'user'}"></i></div>
        <div class="content">${content}</div>
    `;
    chatMessages.appendChild(msg);
    chatMessages.scrollTop = chatMessages.scrollHeight;
}

async function sendMessage() {
    const text = chatInput.value.trim();
    if (!text) return;
    
    // Check for API key (unless Pro+ with included credits)
    if (userTier !== 'proplus' && !apiSettings.apiKey && apiSettings.provider !== 'ollama') {
        addMessage('assistant', `⚠️ <strong>API Key Required</strong><br><br>
            To use AI features, please configure your API key in settings.<br><br>
            <button onclick="openSettingsModal()" style="padding: 6px 12px; background: var(--primary); border: none; border-radius: 4px; color: white; cursor: pointer;">
                <i class="fas fa-key"></i> Open Settings
            </button><br><br>
            <small>Or upgrade to Pro+ for included API credits!</small>`);
        return;
    }
    
    addMessage('user', text);
    chatInput.value = '';
    
    // Show typing indicator
    const typingId = 'typing-' + Date.now();
    addMessage('assistant', `<span id="${typingId}" class="typing-indicator">Thinking<span class="dots">...</span></span>`);
    
    try {
        const response = await callAI(text);
        // Remove typing indicator and show response
        const typingEl = document.getElementById(typingId);
        if (typingEl) {
            typingEl.parentElement.innerHTML = response;
        }
    } catch (err) {
        const typingEl = document.getElementById(typingId);
        if (typingEl) {
            typingEl.parentElement.innerHTML = `❌ Error: ${err.message}<br><small>Check your API settings.</small>`;
        }
    }
}

// Make openSettingsModal available globally
window.openSettingsModal = openSettingsModal;

async function callAI(prompt) {
    const { provider, apiKey, model, customEndpoint } = apiSettings;
    
    // Pro+ uses our backend
    if (userTier === 'proplus') {
        // In production, this would call your backend
        const res = await fetch('https://api.aide.dev/v1/chat', {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${localStorage.getItem('aide-license-key')}`
            },
            body: JSON.stringify({ prompt, model })
        });
        if (!res.ok) throw new Error('Pro+ API error');
        const data = await res.json();
        return data.response;
    }
    
    // User's own API key
    let url, headers, body;
    
    switch (provider) {
        case 'openai':
            url = 'https://api.openai.com/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json'
            };
            body = {
                model,
                messages: [{ role: 'user', content: prompt }],
                max_tokens: 2048
            };
            break;
            
        case 'anthropic':
            url = 'https://api.anthropic.com/v1/messages';
            headers = { 
                'x-api-key': apiKey,
                'anthropic-version': '2023-06-01',
                'Content-Type': 'application/json'
            };
            body = {
                model,
                max_tokens: 2048,
                messages: [{ role: 'user', content: prompt }]
            };
            break;
            
        case 'openrouter':
            url = 'https://openrouter.ai/api/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json',
                'HTTP-Referer': 'https://aide.dev',
                'X-Title': 'AIDE'
            };
            body = {
                model,
                messages: [{ role: 'user', content: prompt }]
            };
            break;
            
        case 'ollama':
            url = 'http://localhost:11434/api/chat';
            headers = { 'Content-Type': 'application/json' };
            body = {
                model,
                messages: [{ role: 'user', content: prompt }],
                stream: false
            };
            break;
            
        case 'custom':
            url = customEndpoint;
            headers = { 
                'Content-Type': 'application/json',
                ...(apiKey ? { 'Authorization': `Bearer ${apiKey}` } : {})
            };
            body = {
                model,
                messages: [{ role: 'user', content: prompt }]
            };
            break;
    }
    
    const res = await fetch(url, {
        method: 'POST',
        headers,
        body: JSON.stringify(body)
    });
    
    if (!res.ok) {
        const errText = await res.text();
        throw new Error(`API error: ${res.status} - ${errText.slice(0, 200)}`);
    }
    
    const data = await res.json();
    
    // Parse response based on provider
    if (provider === 'anthropic') {
        return data.content?.[0]?.text || 'No response';
    } else if (provider === 'ollama') {
        return data.message?.content || 'No response';
    } else {
        return data.choices?.[0]?.message?.content || 'No response';
    }
}

document.getElementById('sendBtn').addEventListener('click', sendMessage);
chatInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        sendMessage();
    }
});

// Auto-resize textarea
chatInput.addEventListener('input', () => {
    chatInput.style.height = 'auto';
    chatInput.style.height = Math.min(chatInput.scrollHeight, 100) + 'px';
});

// === TERMINAL ===
const terminalInput = document.getElementById('terminalInput');
const terminalOutput = document.getElementById('terminalOutput');
let cmdHistory = [], historyIdx = -1;

function execCmd(cmd) {
    if (!cmd.trim()) return;
    cmdHistory.push(cmd);
    historyIdx = cmdHistory.length;
    
    const cmdLine = document.createElement('div');
    cmdLine.className = 'terminal-line cmd';
    cmdLine.textContent = `$ ${cmd}`;
    terminalOutput.appendChild(cmdLine);
    
    // FREE feature: Detect --help and show popout docs
    if (cmd.includes('--help')) {
        const baseCmd = cmd.split(' ')[0];
        showHelpPopout(baseCmd, cmd);
    }
    
    // Execute real commands
    const { execSync } = require('child_process');
    try {
        const output = execSync(cmd, { 
            cwd: currentDir, 
            encoding: 'utf8',
            timeout: 10000,
            maxBuffer: 1024 * 1024
        });
        
        if (output.trim()) {
            const outLine = document.createElement('div');
            outLine.className = 'terminal-line';
            outLine.textContent = output;
            terminalOutput.appendChild(outLine);
        }
        
        // Handle cd command
        if (cmd.trim().startsWith('cd ')) {
            const newDir = cmd.trim().slice(3).replace('~', os.homedir());
            const resolved = path.resolve(currentDir, newDir);
            if (fs.existsSync(resolved) && fs.statSync(resolved).isDirectory()) {
                currentDir = resolved;
                loadDirectory(currentDir);
            }
        }
    } catch (e) {
        const errLine = document.createElement('div');
        errLine.className = 'terminal-line err';
        errLine.textContent = e.message || 'Command failed';
        terminalOutput.appendChild(errLine);
    }
    
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

terminalInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') { execCmd(terminalInput.value); terminalInput.value = ''; }
    else if (e.key === 'ArrowUp' && historyIdx > 0) { historyIdx--; terminalInput.value = cmdHistory[historyIdx]; }
    else if (e.key === 'ArrowDown') { historyIdx = Math.min(historyIdx + 1, cmdHistory.length); terminalInput.value = cmdHistory[historyIdx] || ''; }
});

// === HELP POPOUT (Pro feature) ===
let helpPopout = null;

function showHelpPopout(command, fullCmd) {
    // Close existing popout
    if (helpPopout) {
        helpPopout.remove();
    }
    
    // Create popout panel
    helpPopout = document.createElement('div');
    helpPopout.className = 'help-popout';
    helpPopout.innerHTML = `
        <div class="help-popout-header" id="helpPopoutHeader">
            <span><i class="fas fa-book"></i> ${command} --help</span>
            <div class="help-popout-controls">
                <button onclick="minimizeHelpPopout()" title="Minimize"><i class="fas fa-minus"></i></button>
                <button onclick="closeHelpPopout()" title="Close"><i class="fas fa-times"></i></button>
            </div>
        </div>
        <div class="help-popout-content" id="helpPopoutContent">
            <div class="help-loading"><i class="fas fa-spinner fa-spin"></i> Loading docs...</div>
        </div>
        <div class="help-popout-footer">
            <button onclick="openExternalDocs('${command}')"><i class="fas fa-external-link-alt"></i> Open Full Docs</button>
            <button onclick="copyHelpToChat('${command}')"><i class="fas fa-comment"></i> Ask Dee Dee</button>
        </div>
    `;
    
    document.body.appendChild(helpPopout);
    
    // Make it draggable
    makeHelpPopoutDraggable();
    
    // Fetch and display help content
    fetchHelpContent(command, fullCmd);
}

function makeHelpPopoutDraggable() {
    const header = document.getElementById('helpPopoutHeader');
    if (!header || !helpPopout) return;
    
    let isDragging = false;
    let startX, startY, startLeft, startTop;
    
    header.addEventListener('mousedown', (e) => {
        if (e.target.closest('button')) return; // Don't drag when clicking buttons
        isDragging = true;
        startX = e.clientX;
        startY = e.clientY;
        const rect = helpPopout.getBoundingClientRect();
        startLeft = rect.left;
        startTop = rect.top;
        helpPopout.style.transition = 'none';
    });
    
    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const dx = e.clientX - startX;
        const dy = e.clientY - startY;
        helpPopout.style.left = (startLeft + dx) + 'px';
        helpPopout.style.top = (startTop + dy) + 'px';
        helpPopout.style.right = 'auto';
    });
    
    document.addEventListener('mouseup', () => {
        isDragging = false;
        if (helpPopout) helpPopout.style.transition = '';
    });
}

let helpPopoutMinimized = false;

function minimizeHelpPopout() {
    if (!helpPopout) return;
    
    helpPopoutMinimized = !helpPopoutMinimized;
    
    if (helpPopoutMinimized) {
        helpPopout.classList.add('minimized');
    } else {
        helpPopout.classList.remove('minimized');
    }
}
window.minimizeHelpPopout = minimizeHelpPopout;

function closeHelpPopout() {
    if (helpPopout) {
        helpPopout.remove();
        helpPopout = null;
    }
}
window.closeHelpPopout = closeHelpPopout;

async function fetchHelpContent(command, fullCmd) {
    const content = document.getElementById('helpPopoutContent');
    
    try {
        // Run the actual --help command
        const { execSync } = require('child_process');
        const output = execSync(fullCmd, { 
            encoding: 'utf8', 
            timeout: 5000,
            cwd: currentDir 
        });
        
        // Format the output nicely
        content.innerHTML = `<pre class="help-text">${escapeHtml(output)}</pre>`;
        
        // Try to parse and highlight sections
        highlightHelpSections(content);
        
    } catch (e) {
        // If command fails, show error but offer alternatives
        content.innerHTML = `
            <div class="help-error">
                <p><i class="fas fa-exclamation-triangle"></i> Could not fetch help for <code>${command}</code></p>
                <p class="help-suggestion">Try searching online or ask Dee Dee!</p>
            </div>
        `;
    }
}

function highlightHelpSections(container) {
    const pre = container.querySelector('pre');
    if (!pre) return;
    
    let html = pre.innerHTML;
    
    // Highlight section headers (USAGE:, OPTIONS:, etc.)
    html = html.replace(/^([A-Z][A-Z\s]+:)/gm, '<span class="help-section">$1</span>');
    
    // Highlight flags (--flag, -f)
    html = html.replace(/(\s)(--?[a-zA-Z][-a-zA-Z0-9]*)/g, '$1<span class="help-flag">$2</span>');
    
    // Highlight <placeholders>
    html = html.replace(/(&lt;[^&]+&gt;)/g, '<span class="help-placeholder">$1</span>');
    
    pre.innerHTML = html;
}

function escapeHtml(text) {
    return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

function openExternalDocs(command) {
    // Common documentation URLs
    const docUrls = {
        'git': 'https://git-scm.com/docs/',
        'npm': 'https://docs.npmjs.com/cli/',
        'node': 'https://nodejs.org/docs/',
        'python': 'https://docs.python.org/3/',
        'pip': 'https://pip.pypa.io/en/stable/',
        'docker': 'https://docs.docker.com/reference/',
        'kubectl': 'https://kubernetes.io/docs/reference/kubectl/',
        'cargo': 'https://doc.rust-lang.org/cargo/',
        'go': 'https://golang.org/doc/',
    };
    
    const baseUrl = docUrls[command] || `https://www.google.com/search?q=${command}+documentation`;
    require('electron').shell.openExternal(baseUrl);
}
window.openExternalDocs = openExternalDocs;

function copyHelpToChat(command) {
    chatInput.value = `How do I use ${command}? Can you explain the main options?`;
    chatInput.focus();
    closeHelpPopout();
}
window.copyHelpToChat = copyHelpToChat;

// === DEBUG LOG ===
function log(level, msg) {
    const time = new Date().toLocaleTimeString('en-US', { hour12: false });
    console.log(`[${level.toUpperCase()}] ${msg}`);
    
    // Also output to terminal if available
    const terminalOutput = document.getElementById('terminalOutput');
    if (terminalOutput && level !== 'info') {
        const line = document.createElement('div');
        line.className = `terminal-line ${level === 'error' ? 'err' : ''}`;
        line.textContent = `[${level.toUpperCase()}] ${msg}`;
        terminalOutput.appendChild(line);
        terminalOutput.scrollTop = terminalOutput.scrollHeight;
    }
}

// === CLOSE HANDLERS ===
document.addEventListener('click', (e) => {
    if (closeBehavior !== 'single') return;
    if (e.target.closest('.titlebar') && !e.target.closest('button, .mascot-face, .antenna-close')) {
        ipcRenderer.send('close-ide');
    }
});

// Double-click on titlebar or empty areas to hide (when in double-click mode)
document.addEventListener('dblclick', (e) => {
    if (closeBehavior !== 'double') return;
    // Double-click on titlebar (not buttons/face) or on empty content areas
    const isOnTitlebar = e.target.closest('.titlebar') && !e.target.closest('button, .mascot-face');
    const isOnEmptyArea = e.target.closest('.quad-header') && !e.target.closest('button, input, textarea');
    if (isOnTitlebar || isOnEmptyArea) {
        ipcRenderer.send('close-ide');
    }
});

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && closeBehavior !== 'minimize') ipcRenderer.send('close-ide');
    if (e.ctrlKey && e.key === 'b') { e.preventDefault(); mascotFace.click(); }
});

// === VOICE COMMANDS ===
// FREE commands - available to all users
const FREE_VOICE_COMMANDS = {
    wake: {
        phrases: ['aye dee dee', 'hey dee dee', 'a d d', 'aide', 'ay dee dee', 'aydeedee'],
        action: () => ipcRenderer.send('voice-wake'),
        feedback: { color: 'rgba(168, 85, 247, 0.8)', log: 'Waking Dee Dee!' }
    },
    hide: {
        phrases: ['dee dee hide', 'deedee hide', 'hide dee dee', 'bye dee dee', 'bye deedee', 'go away dee dee'],
        action: () => ipcRenderer.send('voice-hide'),
        feedback: { color: 'rgba(100, 100, 100, 0.8)', log: 'Hiding Dee Dee!' }
    }
};

// PRO commands - require Pro subscription
const PRO_VOICE_COMMANDS = {
    newFile: {
        phrases: ['dee dee new file', 'new file', 'create file', 'dee dee create'],
        action: () => newTab(),
        feedback: { color: 'rgba(34, 197, 94, 0.8)', log: 'Creating new file!' }
    },
    saveFile: {
        phrases: ['dee dee save', 'save file', 'save this', 'dee dee save this'],
        action: () => document.dispatchEvent(new KeyboardEvent('keydown', { key: 's', ctrlKey: true })),
        feedback: { color: 'rgba(59, 130, 246, 0.8)', log: 'Saving file!' }
    },
    toggleTheme: {
        phrases: ['dee dee dark mode', 'dee dee light mode', 'toggle theme', 'switch theme', 'dee dee theme'],
        action: () => themeToggle.click(),
        feedback: { color: 'rgba(251, 191, 36, 0.8)', log: 'Toggling theme!' }
    },
    openExplorer: {
        phrases: ['dee dee files', 'show files', 'open explorer', 'dee dee explorer', 'show explorer'],
        action: () => { if (sidebar.classList.contains('collapsed')) document.getElementById('sidebarToggle').click(); },
        feedback: { color: 'rgba(139, 92, 246, 0.8)', log: 'Opening explorer!' }
    },
    openTodo: {
        phrases: ['dee dee todo', 'show todo', 'open todo', 'dee dee tasks', 'show tasks'],
        action: () => { if (todoSidebar.classList.contains('collapsed')) document.getElementById('todoToggle').click(); },
        feedback: { color: 'rgba(236, 72, 153, 0.8)', log: 'Opening TODO!' }
    },
    focusChat: {
        phrases: ['dee dee chat', 'open chat', 'dee dee help', 'ask dee dee', 'hey dee dee help'],
        action: () => chatInput.focus(),
        feedback: { color: 'rgba(6, 182, 212, 0.8)', log: 'Ready to chat!' }
    },
    focusTerminal: {
        phrases: ['dee dee terminal', 'open terminal', 'show terminal', 'dee dee shell'],
        action: () => document.getElementById('terminalInput')?.focus(),
        feedback: { color: 'rgba(16, 185, 129, 0.8)', log: 'Opening terminal!' }
    },
    runCode: {
        phrases: ['dee dee run', 'run code', 'execute', 'dee dee execute'],
        action: () => log('info', '▶️ Run code (coming soon)'),
        feedback: { color: 'rgba(249, 115, 22, 0.8)', log: 'Running code!' }
    },
    clearChat: {
        phrases: ['dee dee clear', 'clear chat', 'dee dee reset', 'start over'],
        action: () => { document.getElementById('chatMessages').innerHTML = ''; log('info', 'Chat cleared!'); },
        feedback: { color: 'rgba(239, 68, 68, 0.8)', log: 'Clearing chat!' }
    },
    whatCanYouDo: {
        phrases: ['dee dee what can you do', 'dee dee help me', 'dee dee commands', 'what can you do'],
        action: () => showVoiceCommandsHelp(),
        feedback: { color: 'rgba(168, 85, 247, 0.8)', log: 'Showing commands!' }
    }
};

// Combined commands based on tier
function getVoiceCommands() {
    if (features.isPro) {
        return { ...FREE_VOICE_COMMANDS, ...PRO_VOICE_COMMANDS };
    }
    return FREE_VOICE_COMMANDS;
}

let recognition = null;
let isListening = false;
let audioContext = null;
let analyser = null;
let microphone = null;
let voiceActivityTimeout = null;
let voiceActivityThreshold = 0.01; // Adjust based on environment

// Voice analytics
let voiceAnalytics = {
    commandsRecognized: 0,
    wakeWordsDetected: 0,
    falsePositives: 0,
    averageConfidence: 0,
    errors: [],
    startTime: Date.now()
};

// Privacy and security settings
let voicePrivacySettings = {
    enableAnalytics: true,
    allowRemoteProcessing: false, // Default to local-only
    storeTranscripts: false,
    enableVoiceActivityDetection: true,
    wakeWordRequired: true,
    confidenceThreshold: 0.6,
    maxRetentionHours: 24
};

// File system permissions
let grantedDirectories = new Set();
let deniedDirectories = new Set();

// Favorites system
let favoriteDirectories = new Set();
let sidebarInstances = ['sidebar']; // Track multiple sidebar instances
let todoSidebarInstances = ['todoSidebar']; // Track multiple todo sidebar instances

// Saved todo lists system
let savedTodoLists = new Map(); // name -> {todos: [], projectRoot: string, lastModified: Date}
let recentTodoLists = []; // Array of {name, projectRoot, lastModified}

// Load saved todo lists from localStorage
function loadSavedTodoLists() {
    const saved = localStorage.getItem('aide-saved-todo-lists');
    if (saved) {
        try {
            const parsed = JSON.parse(saved);
            savedTodoLists = new Map(parsed.lists || []);
            recentTodoLists = parsed.recent || [];
        } catch (e) {
            savedTodoLists = new Map();
            recentTodoLists = [];
        }
    }
    updateSavedTodoListsUI();
}

// Save todo lists to localStorage
function saveTodoListsToStorage() {
    const data = {
        lists: Array.from(savedTodoLists.entries()),
        recent: recentTodoLists
    };
    localStorage.setItem('aide-saved-todo-lists', JSON.stringify(data));
}

// Save current todos for a sidebar with a name
function saveTodoList(sidebarId, listName) {
    const todos = window[`todos_${sidebarId}`] || [];
    const projectRoot = currentDir ? path.dirname(currentDir) : os.homedir();

    const todoListData = {
        todos: todos,
        projectRoot: projectRoot,
        lastModified: new Date().toISOString(),
        sidebarId: sidebarId
    };

    savedTodoLists.set(listName, todoListData);

    // Update recent lists
    const recentEntry = {
        name: listName,
        projectRoot: projectRoot,
        lastModified: todoListData.lastModified
    };

    // Remove if already exists, then add to front
    recentTodoLists = recentTodoLists.filter(item => item.name !== listName);
    recentTodoLists.unshift(recentEntry);

    // Keep only last 10 recent lists
    recentTodoLists = recentTodoLists.slice(0, 10);

    saveTodoListsToStorage();
    updateSavedTodoListsUI();

    log('info', `Saved todo list "${listName}" with ${todos.length} items for project ${projectRoot}`);
    showSaveSuccess(listName);
}

// Load a saved todo list into a sidebar
function loadTodoList(sidebarId, listName) {
    const savedList = savedTodoLists.get(listName);
    if (!savedList) {
        log('error', `Todo list "${listName}" not found`);
        return;
    }

    window[`todos_${sidebarId}`] = [...savedList.todos];
    saveTodosForSidebar(sidebarId);

    log('info', `Loaded todo list "${listName}" with ${savedList.todos.length} items`);
    showLoadSuccess(listName);
}

// Delete a saved todo list
function deleteTodoList(listName) {
    if (savedTodoLists.delete(listName)) {
        recentTodoLists = recentTodoLists.filter(item => item.name !== listName);
        saveTodoListsToStorage();
        updateSavedTodoListsUI();
        log('info', `Deleted todo list "${listName}"`);
    }
}

// Search todos in a sidebar
function searchTodos(sidebarId, query) {
    const todos = window[`todos_${sidebarId}`] || [];
    if (!query.trim()) {
        renderTodosForSidebar(sidebarId);
        return;
    }

    const filteredTodos = todos.filter(todo =>
        todo.text.toLowerCase().includes(query.toLowerCase())
    );

    renderFilteredTodos(sidebarId, filteredTodos, query);
}

// Get saved todo lists for current project
function getProjectTodoLists(projectRoot) {
    return Array.from(savedTodoLists.entries())
        .filter(([name, data]) => data.projectRoot === projectRoot)
        .map(([name, data]) => ({ name, ...data }));
}

// Load favorites from localStorage
function loadFavorites() {
    const saved = localStorage.getItem('aide-favorites');
    if (saved) {
        try {
            favoriteDirectories = new Set(JSON.parse(saved));
        } catch (e) {
            favoriteDirectories = new Set();
        }
    }
    updateFavoritesBar();
}

// Save favorites to localStorage
function saveFavorites() {
    localStorage.setItem('aide-favorites', JSON.stringify([...favoriteDirectories]));
}

// Toggle favorite status for a directory
function toggleFavorite(dirPath) {
    if (favoriteDirectories.has(dirPath)) {
        favoriteDirectories.delete(dirPath);
        log('info', `Removed favorite: ${dirPath.replace(os.homedir(), '~')}`);
    } else {
        favoriteDirectories.add(dirPath);
        log('info', `Added favorite: ${dirPath.replace(os.homedir(), '~')}`);
    }
    saveFavorites();
    updateFavoritesBar();
}

// Check if directory is favorited
function isFavorite(dirPath) {
    return favoriteDirectories.has(dirPath);
}

// Update favorites bar UI
function updateFavoritesBar() {
    const favoritesList = document.getElementById('favoritesList');
    if (!favoritesList) return;

    favoritesList.innerHTML = '';

    if (favoriteDirectories.size === 0) {
        favoritesList.innerHTML = '<div class="no-favorites">Right-click directories to add favorites</div>';
        return;
    }

    for (const dirPath of favoriteDirectories) {
        const shortPath = dirPath.replace(os.homedir(), '~');
        const item = document.createElement('div');
        item.className = 'favorite-item';
        item.innerHTML = `
            <i class="fas fa-folder"></i>
            <span class="favorite-name" title="${shortPath}">${shortPath.split('/').pop() || '~'}</span>
            <button class="favorite-remove" onclick="toggleFavorite('${dirPath.replace(/'/g, "\\'")}'); event.stopPropagation();" title="Remove favorite">
                <i class="fas fa-times"></i>
            </button>
        `;
        item.onclick = () => loadDirectory(dirPath);
        favoritesList.appendChild(item);
    }
}

// Load granted directories from localStorage
function loadDirectoryPermissions() {
    const granted = localStorage.getItem('aide-granted-directories');
    if (granted) {
        try {
            grantedDirectories = new Set(JSON.parse(granted));
        } catch (e) {
            grantedDirectories = new Set();
        }
    }

    // Always grant access to home directory
    const homeDir = os.homedir();
    grantedDirectories.add(homeDir);

    // Save back to localStorage
    saveDirectoryPermissions();
}

// Save directory permissions
function saveDirectoryPermissions() {
    localStorage.setItem('aide-granted-directories', JSON.stringify([...grantedDirectories]));
}

// Show directory permissions management dialog
function showDirectoryPermissionsManager() {
    const dialog = document.createElement('div');
    dialog.id = 'directory-permissions-manager';
    dialog.innerHTML = `
        <div style="
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.7);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 10000;
            backdrop-filter: blur(5px);
        ">
            <div style="
                background: var(--surface, #1f2937);
                border: 1px solid var(--border, #374151);
                border-radius: 12px;
                padding: 24px;
                max-width: 500px;
                max-height: 80vh;
                overflow-y: auto;
                box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
            ">
                <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 20px;">
                    <h3 style="margin: 0; color: var(--text, white); font-size: 18px; font-weight: 600;">
                        <i class="fas fa-folder-shield" style="margin-right: 8px;"></i>
                        Directory Permissions
                    </h3>
                    <button id="close-permissions" style="
                        background: transparent;
                        color: var(--text-muted, #9ca3af);
                        border: none;
                        font-size: 20px;
                        cursor: pointer;
                        padding: 4px;
                        border-radius: 4px;
                        transition: all 0.2s;
                    ">✕</button>
                </div>

                <div style="margin-bottom: 20px;">
                    <p style="margin: 0 0 16px 0; color: var(--text-muted, #9ca3af); line-height: 1.5; font-size: 14px;">
                        Manage which directories AIDE can access. Changes take effect immediately.
                    </p>
                </div>

                <div id="permissions-list" style="margin-bottom: 20px;">
                    <!-- Permissions will be loaded here -->
                </div>

                <div style="display: flex; gap: 12px; justify-content: flex-end;">
                    <button id="reset-permissions" style="
                        background: #ef4444;
                        color: white;
                        border: none;
                        padding: 8px 16px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        transition: background 0.2s;
                    ">Reset All</button>
                    <button id="done-permissions" style="
                        background: #3b82f6;
                        color: white;
                        border: none;
                        padding: 8px 16px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        font-weight: 500;
                        transition: background 0.2s;
                    ">Done</button>
                </div>
            </div>
        </div>
    `;

    document.body.appendChild(dialog);

    // Load permissions list
    loadPermissionsList();

    // Handle close button
    document.getElementById('close-permissions').onclick = () => dialog.remove();

    // Handle done button
    document.getElementById('done-permissions').onclick = () => dialog.remove();

    // Handle reset button
    document.getElementById('reset-permissions').onclick = () => {
        if (confirm('Reset all directory permissions? This will only keep access to your home directory.')) {
            grantedDirectories.clear();
            deniedDirectories.clear();
            const homeDir = os.homedir();
            grantedDirectories.add(homeDir);
            saveDirectoryPermissions();
            loadPermissionsList();
            log('info', 'Directory permissions reset');
        }
    };

    // Add hover effects
    const resetBtn = document.getElementById('reset-permissions');
    const doneBtn = document.getElementById('done-permissions');
    const closeBtn = document.getElementById('close-permissions');

    resetBtn.onmouseover = () => resetBtn.style.background = '#dc2626';
    resetBtn.onmouseout = () => resetBtn.style.background = '#ef4444';

    doneBtn.onmouseover = () => doneBtn.style.background = '#2563eb';
    doneBtn.onmouseout = () => doneBtn.style.background = '#3b82f6';

    closeBtn.onmouseover = () => closeBtn.style.color = 'var(--text, white)';
    closeBtn.onmouseout = () => closeBtn.style.color = 'var(--text-muted, #9ca3af)';
}

function loadPermissionsList() {
    const listEl = document.getElementById('permissions-list');
    listEl.innerHTML = '';

    if (grantedDirectories.size === 0 && deniedDirectories.size === 0) {
        listEl.innerHTML = '<p style="color: var(--text-muted, #9ca3af); font-style: italic;">No custom permissions set</p>';
        return;
    }

    // Show granted directories
    if (grantedDirectories.size > 0) {
        listEl.innerHTML += '<h4 style="color: var(--text, white); margin: 16px 0 8px 0; font-size: 14px;">✅ Allowed Directories</h4>';
        for (const dir of grantedDirectories) {
            const shortPath = dir.replace(os.homedir(), '~');
            listEl.innerHTML += `
                <div style="display: flex; align-items: center; justify-content: space-between; padding: 8px 12px; background: rgba(16, 185, 129, 0.1); border-radius: 6px; margin-bottom: 4px;">
                    <span style="color: var(--text, white); font-size: 13px;">${shortPath}</span>
                    <button onclick="revokeDirectoryPermission('${dir.replace(/'/g, "\\'")}')" style="
                        background: transparent;
                        color: #ef4444;
                        border: 1px solid #ef4444;
                        padding: 2px 8px;
                        border-radius: 4px;
                        cursor: pointer;
                        font-size: 12px;
                    ">Revoke</button>
                </div>
            `;
        }
    }

    // Show denied directories
    if (deniedDirectories.size > 0) {
        listEl.innerHTML += '<h4 style="color: var(--text, white); margin: 16px 0 8px 0; font-size: 14px;">❌ Denied Directories</h4>';
        for (const dir of deniedDirectories) {
            const shortPath = dir.replace(os.homedir(), '~');
            listEl.innerHTML += `
                <div style="display: flex; align-items: center; justify-content: space-between; padding: 8px 12px; background: rgba(239, 68, 68, 0.1); border-radius: 6px; margin-bottom: 4px;">
                    <span style="color: var(--text, white); font-size: 13px;">${shortPath}</span>
                    <button onclick="allowDirectoryPermission('${dir.replace(/'/g, "\\'")}')" style="
                        background: transparent;
                        color: #10b981;
                        border: 1px solid #10b981;
                        padding: 2px 8px;
                        border-radius: 4px;
                        cursor: pointer;
                        font-size: 12px;
                    ">Allow</button>
                </div>
            `;
        }
    }
}

// Duplicate sidebar management
function addDuplicateSidebar() {
    // Check if user has unlimited sidebars (Pro+)
    if (features.bothSidebars === false) {
        showMaxSidebarsWarning();
        return;
    }

    const sidebarId = `sidebar-${Date.now()}`;
    sidebarInstances.push(sidebarId);

    createDuplicateSidebar(sidebarId);
    log('info', `Created duplicate file explorer: ${sidebarId}`);
}

function addDuplicateTodoSidebar() {
    const sidebarId = `todo-sidebar-${Date.now()}`;
    todoSidebarInstances = todoSidebarInstances || [];
    todoSidebarInstances.push(sidebarId);

    createDuplicateTodoSidebar(sidebarId);
    log('info', `Created duplicate todo sidebar: ${sidebarId}`);
}

function createDuplicateTodoSidebar(sidebarId) {
    // Create new todo sidebar element
    const todoSidebar = document.createElement('div');
    todoSidebar.className = 'todo-sidebar duplicate-todo-sidebar';
    todoSidebar.id = sidebarId;
    todoSidebar.innerHTML = `
        <div class="sidebar-header">
            <div class="todo-header-info">
                <span><i class="fas fa-list-check"></i> Todo ${todoSidebarInstances.length}</span>
                <small class="todo-item-name" id="todoItemName-${sidebarId}">No item selected</small>
            </div>
            <div class="sidebar-actions">
                <button onclick="showTodoSaveLoadMenu('${sidebarId}')" title="Save/Load todo lists">
                    <i class="fas fa-save"></i>
                </button>
            </div>
        </div>
        <!-- Action Buttons -->
        <div class="todo-actions-row">
            <button onclick="createNewItem('${sidebarId}')" title="Create new item" class="action-btn primary">
                <i class="fas fa-plus"></i> New
            </button>
            <button onclick="showRecentItems('${sidebarId}')" title="Recent items" class="action-btn">
                <i class="fas fa-clock"></i> Recent
            </button>
            <button onclick="deleteCurrentItem('${sidebarId}')" title="Delete current item" class="action-btn danger">
                <i class="fas fa-trash"></i> Delete
            </button>
        </div>

        <!-- Search Row -->
        <div class="todo-search-row">
            <input type="text" id="todoSearch-${sidebarId}" placeholder="Search items..." autocomplete="off">
            <button onclick="clearTodoSearch('${sidebarId}')" title="Clear search">
                <i class="fas fa-times"></i>
            </button>
        </div>

        <!-- Content Input Row (shown/hidden based on item type) -->
        <div class="todo-input-row" id="todoInputRow-${sidebarId}">
            <input type="text" id="todoInput-${sidebarId}" placeholder="Add task..." autocomplete="off">
            <button onclick="addTodoForSidebar('${sidebarId}')"><i class="fas fa-plus"></i></button>
        </div>
        <div class="todo-list" id="todoList-${sidebarId}">
            <!-- Todos rendered here -->
        </div>
        <div class="todo-stats" id="todoStats-${sidebarId}">0 tasks</div>
    `;

    // Add to main container
    const mainContainer = document.querySelector('.main-container');
    mainContainer.appendChild(todoSidebar);

    // Initialize the duplicate todo sidebar
    initDuplicateTodoSidebar(sidebarId);

    // Make it resizable
    initResizer(`resizer-${sidebarId}`, () => [document.querySelector('.content-area'), todoSidebar], false);
}

function initDuplicateTodoSidebar(sidebarId) {
    // Create unique variables for this sidebar instance
    window[`todos_${sidebarId}`] = JSON.parse(localStorage.getItem(`aide-todos-${sidebarId}`) || '[]');
    window[`currentItemName_${sidebarId}`] = null;
    window[`currentItemType_${sidebarId}`] = null;

    // Initialize search
    const searchInput = document.getElementById(`todoSearch-${sidebarId}`);
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            searchTodos(sidebarId, e.target.value);
        });
    }

    // Initialize todo input (initially hidden)
    const input = document.getElementById(`todoInput-${sidebarId}`);
    if (input) {
        input.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') addTodoForSidebar(sidebarId);
        });
    }

    // Start with todo input hidden
    hideTodoInput(sidebarId);
    resetTodoItemName(sidebarId);

    // Load and render todos
    renderTodosForSidebar(sidebarId);
}

function clearTodoSearch(sidebarId) {
    const searchInput = document.getElementById(`todoSearch-${sidebarId}`);
    searchInput.value = '';
    searchTodos(sidebarId, '');
}

// Item creation and management
function createNewItem(sidebarId) {
    showItemTypeSelector(sidebarId);
}

function showItemTypeSelector(sidebarId) {
    document.querySelectorAll('.item-type-selector').forEach(el => el.remove());

    const selector = document.createElement('div');
    selector.className = 'item-type-selector';
    selector.innerHTML = `
        <div class="selector-overlay">
            <div class="selector-content">
                <h3>Create New Item</h3>
                <input type="text" id="itemNameInput-${sidebarId}" class="item-name-input" placeholder="Name your item..." autocomplete="off">
                <div class="type-options">
                    <button class="type-option plan-option" onclick="createPlanItem('${sidebarId}')">
                        <i class="fas fa-project-diagram"></i>
                        <div class="type-info">
                            <strong>Plan</strong>
                            <small>Phased todo list with headers and notes</small>
                        </div>
                    </button>
                    <button class="type-option note-option" onclick="createNoteItem('${sidebarId}')">
                        <i class="fas fa-sticky-note"></i>
                        <div class="type-info">
                            <strong>Note</strong>
                            <small>Freeform notepad</small>
                        </div>
                    </button>
                    <button class="type-option todo-option" onclick="createTodoItem('${sidebarId}')">
                        <i class="fas fa-list-ul"></i>
                        <div class="type-info">
                            <strong>Todo</strong>
                            <small>Bullet list with check/cross functionality</small>
                        </div>
                    </button>
                </div>
                <button class="cancel-btn" onclick="closeItemTypeSelector()">Cancel</button>
            </div>
        </div>
    `;

    document.body.appendChild(selector);
}

function createPlanItem(sidebarId) {
    const name = getNewItemName(sidebarId);
    if (!name) return;
    closeItemTypeSelector();
    setTodoItemName(sidebarId, name, 'plan');
    hideTodoInput(sidebarId);
    window[`todos_${sidebarId}`] = [];
    saveTodosForSidebar(sidebarId);
    showNotification('Plan creation coming soon!', 'info');
}

function createNoteItem(sidebarId) {
    const name = getNewItemName(sidebarId);
    if (!name) return;
    closeItemTypeSelector();
    setTodoItemName(sidebarId, name, 'note');
    hideTodoInput(sidebarId);
    window[`todos_${sidebarId}`] = [];
    saveTodosForSidebar(sidebarId);
    showNotification('Note creation coming soon!', 'info');
}

function createTodoItem(sidebarId) {
    const name = getNewItemName(sidebarId);
    if (!name) return;
    closeItemTypeSelector();
    setTodoItemName(sidebarId, name, 'todo');
    window[`todos_${sidebarId}`] = [];
    saveTodosForSidebar(sidebarId);
    showTodoInput(sidebarId);
    const input = document.getElementById(`todoInput-${sidebarId}`);
    if (input) {
        input.focus();
    }
}

function showTodoInput(sidebarId) {
    const inputRow = document.getElementById(`todoInputRow-${sidebarId}`);
    if (inputRow) {
        inputRow.style.display = 'flex';
    }
    const input = document.getElementById(`todoInput-${sidebarId}`);
    if (input) {
        const name = window[`currentItemName_${sidebarId}`] || 'todo';
        input.placeholder = `Add tasks for ${name}...`;
    }
}

function hideTodoInput(sidebarId) {
    const inputRow = document.getElementById(`todoInputRow-${sidebarId}`);
    if (inputRow) {
        inputRow.style.display = 'none';
    }
}

function getNewItemName(sidebarId) {
    const input = document.getElementById(`itemNameInput-${sidebarId}`);
    if (!input) return '';
    const name = input.value.trim();
    if (!name) {
        showNotification('Please enter a name for this item', 'error');
        input.focus();
        return '';
    }
    return name;
}

function closeItemTypeSelector() {
    document.querySelectorAll('.item-type-selector').forEach(el => el.remove());
}

function setTodoItemName(sidebarId, name, type) {
    const typeLabels = { plan: 'Plan', note: 'Note', todo: 'Todo' };
    window[`currentItemName_${sidebarId}`] = name;
    window[`currentItemType_${sidebarId}`] = type;

    const label = document.getElementById(`todoItemName-${sidebarId}`);
    if (label) {
        label.textContent = `${name} · ${typeLabels[type] || 'Item'}`;
    }
}

function resetTodoItemName(sidebarId) {
    window[`currentItemName_${sidebarId}`] = null;
    window[`currentItemType_${sidebarId}`] = null;
    const label = document.getElementById(`todoItemName-${sidebarId}`);
    if (label) {
        label.textContent = 'No item selected';
    }
}

function showRecentItems(sidebarId) {
    showTodoSaveLoadMenu(sidebarId, 'recent');
}

function deleteCurrentItem(sidebarId) {
    const currentName = window[`currentItemName_${sidebarId}`];
    if (!currentName) {
        showNotification('No item selected to delete', 'error');
        return;
    }

    if (confirm(`Delete "${currentName}"? This action cannot be undone.`)) {
        window[`todos_${sidebarId}`] = [];
        saveTodosForSidebar(sidebarId);
        hideTodoInput(sidebarId);
        resetTodoItemName(sidebarId);
        showNotification('Item deleted', 'success');
    }
}

function addTodoForSidebar(sidebarId) {
    if (window[`currentItemType_${sidebarId}`] !== 'todo') {
        showNotification('Create a Todo item first', 'error');
        return;
    }

    const input = document.getElementById(`todoInput-${sidebarId}`);
    const text = input.value.trim();
    if (!text) return;

    const todos = window[`todos_${sidebarId}`];
    todos.push({ text, done: false });
    input.value = '';

    saveTodosForSidebar(sidebarId);
    log('info', `Added todo to ${sidebarId}: ${text}`);
}

function toggleTodoForSidebar(sidebarId, idx) {
    const todos = window[`todos_${sidebarId}`];
    todos[idx].done = !todos[idx].done;
    saveTodosForSidebar(sidebarId);
}

function deleteTodoForSidebar(sidebarId, idx) {
    const todos = window[`todos_${sidebarId}`];
    todos.splice(idx, 1);
    saveTodosForSidebar(sidebarId);
}

function clearCompletedTodosForSidebar(sidebarId) {
    const todos = window[`todos_${sidebarId}`];
    window[`todos_${sidebarId}`] = todos.filter(t => !t.done);
    saveTodosForSidebar(sidebarId);
}

function saveTodosForSidebar(sidebarId) {
    const todos = window[`todos_${sidebarId}`];
    localStorage.setItem(`aide-todos-${sidebarId}`, JSON.stringify(todos));
    renderTodosForSidebar(sidebarId);
}

function renderTodosForSidebar(sidebarId) {
    const todos = window[`todos_${sidebarId}`];
    renderTodoItems(sidebarId, todos);
}

function renderFilteredTodos(sidebarId, filteredTodos, query) {
    renderTodoItems(sidebarId, filteredTodos, query);
}

function renderTodoItems(sidebarId, todos, searchQuery = '') {
    const list = document.getElementById(`todoList-${sidebarId}`);
    const stats = document.getElementById(`todoStats-${sidebarId}`);

    list.innerHTML = '';

    if (todos.length === 0 && searchQuery) {
        list.innerHTML = '<div class="no-results">No todos match your search</div>';
        stats.textContent = '0 results';
        return;
    }

    todos.forEach((todo, originalIdx) => {
        const el = document.createElement('div');
        el.className = 'todo-item' + (todo.done ? ' completed' : '');

        // Highlight search query if present
        let displayText = todo.text;
        if (searchQuery) {
            const regex = new RegExp(`(${searchQuery})`, 'gi');
            displayText = todo.text.replace(regex, '<mark>$1</mark>');
        }

        el.innerHTML = `
            <div class="checkbox"><i class="fas fa-check"></i></div>
            <span class="text">${displayText}</span>
            <span class="delete" onclick="deleteTodoForSidebar('${sidebarId}', ${originalIdx}); event.stopPropagation();"><i class="fas fa-times"></i></span>
        `;
        el.onclick = () => toggleTodoForSidebar(sidebarId, originalIdx);
        list.appendChild(el);
    });

    const done = todos.filter(t => t.done).length;
    if (searchQuery) {
        stats.textContent = `${done}/${todos.length} results`;
    } else {
        stats.textContent = `${done}/${todos.length} done`;
    }
}

function updateSavedTodoListsUI() {
    // This will be called when we add the UI components
    // For now, just ensure the data is available
}

function showSaveSuccess(listName) {
    showNotification(`Saved todo list "${listName}"`, 'success');
}

function showLoadSuccess(listName) {
    showNotification(`Loaded todo list "${listName}"`, 'info');
}

function showTodoSaveLoadMenu(sidebarId, focusSection = 'project') {
    // Remove any existing menu
    const existing = document.querySelector('.todo-save-menu');
    if (existing) existing.remove();

    const sidebar = document.getElementById(sidebarId);
    const header = sidebar.querySelector('.sidebar-header');
    const rect = header.getBoundingClientRect();

    const menu = document.createElement('div');
    menu.className = 'todo-save-menu';
    menu.style.cssText = `
        position: fixed;
        left: ${rect.right + 5}px;
        top: ${rect.top}px;
        background: var(--surface, #1f2937);
        border: 1px solid var(--border, #374151);
        border-radius: 6px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        z-index: 10000;
        min-width: 200px;
        padding: 8px 0;
    `;

    const projectRoot = currentDir ? path.dirname(currentDir) : os.homedir();
    const projectName = path.basename(projectRoot);

    menu.innerHTML = `
        <div class="menu-section">
            <div class="menu-item save-section">
                <input type="text" id="saveTodoName-${sidebarId}" placeholder="List name..." style="
                    width: 100%;
                    padding: 4px 8px;
                    border: 1px solid var(--border);
                    border-radius: 4px;
                    background: var(--bg);
                    color: var(--text);
                    font-size: 12px;
                ">
                <button onclick="saveCurrentTodoList('${sidebarId}')" style="
                    margin-top: 4px;
                    width: 100%;
                    padding: 4px 8px;
                    background: #10b981;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                    font-size: 12px;
                ">Save List</button>
            </div>
        </div>
        <div class="menu-divider"></div>
        <div class="menu-section project-section">
            <div class="menu-header">Project: ${projectName}</div>
            ${getProjectTodoListsHTML(sidebarId, projectRoot)}
        </div>
        <div class="menu-divider"></div>
        <div class="menu-section recent-section">
            <div class="menu-header">Recent Lists</div>
            ${getRecentTodoListsHTML(sidebarId)}
        </div>
    `;

    document.body.appendChild(menu);

    // Focus the input or scroll to section
    setTimeout(() => {
        const input = document.getElementById(`saveTodoName-${sidebarId}`);
        if (input) input.focus();

        if (focusSection === 'recent') {
            const recentSection = menu.querySelector('.recent-section');
            if (recentSection) recentSection.scrollIntoView({ block: 'start' });
        }
    }, 10);

    // Close menu when clicking elsewhere
    const closeMenu = (e) => {
        if (!menu.contains(e.target) && !header.contains(e.target)) {
            menu.remove();
            document.removeEventListener('click', closeMenu);
        }
    };
    setTimeout(() => document.addEventListener('click', closeMenu), 1);
}

function getProjectTodoListsHTML(sidebarId, projectRoot) {
    const projectLists = getProjectTodoLists(projectRoot);
    if (projectLists.length === 0) {
        return '<div class="menu-item no-lists">No saved lists for this project</div>';
    }

    return projectLists.slice(0, 5).map(list => `
        <div class="menu-item saved-list" onclick="loadTodoList('${sidebarId}', '${list.name.replace(/'/g, "\\'")}')">
            <span>${list.name}</span>
            <small>${list.todos.length} items</small>
        </div>
    `).join('');
}

function getRecentTodoListsHTML(sidebarId) {
    if (recentTodoLists.length === 0) {
        return '<div class="menu-item no-lists">No recent lists</div>';
    }

    return recentTodoLists.slice(0, 5).map(list => `
        <div class="menu-item saved-list" onclick="loadTodoList('${sidebarId}', '${list.name.replace(/'/g, "\\'")}')">
            <span>${list.name}</span>
            <small>${path.basename(list.projectRoot)}</small>
        </div>
    `).join('');
}

function saveCurrentTodoList(sidebarId) {
    const input = document.getElementById(`saveTodoName-${sidebarId}`);
    const listName = input.value.trim();

    if (!listName) {
        showNotification('Please enter a list name', 'error');
        return;
    }

    saveTodoList(sidebarId, listName);
    input.value = '';

    // Close the menu
    const menu = document.querySelector('.todo-save-menu');
    if (menu) menu.remove();
}

function showNotification(message, type = 'info') {
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.innerHTML = `
        <i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'info-circle'}"></i>
        ${message}
    `;

    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${type === 'success' ? '#10b981' : type === 'error' ? '#ef4444' : '#3b82f6'};
        color: white;
        padding: 12px 16px;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        z-index: 10001;
        font-size: 14px;
        animation: slideInRight 0.3s ease-out;
        max-width: 300px;
    `;

    document.body.appendChild(notification);

    setTimeout(() => {
        notification.style.animation = 'slideOutRight 0.3s ease-in';
        setTimeout(() => notification.remove(), 300);
    }, 3000);
}

function createDuplicateSidebar(sidebarId) {
    // Create new sidebar element
    const sidebar = document.createElement('div');
    sidebar.className = 'sidebar duplicate-sidebar';
    sidebar.id = sidebarId;
    sidebar.innerHTML = `
        <div class="sidebar-header">
            <span><i class="fas fa-folder-tree"></i> Explorer ${sidebarInstances.length}</span>
        </div>

        <div class="sidebar-tabs">
            <button class="sidebar-tab active" data-tab="files-${sidebarId}" onclick="switchSidebarTab('files-${sidebarId}', '${sidebarId}')">
                <i class="fas fa-folder"></i>
            </button>
        </div>

        <div class="sidebar-tab-content active" id="filesTab-${sidebarId}">
            <div class="sidebar-path" id="currentPath-${sidebarId}">~</div>

            <div class="favorites-bar" id="favoritesBar-${sidebarId}">
                <div class="favorites-header">
                    <i class="fas fa-star"></i>
                    <span>Favorites</span>
                </div>
                <div class="favorites-list" id="favoritesList-${sidebarId}">
                    <!-- Favorite directories will appear here -->
                </div>
            </div>

            <div class="file-tree" id="fileTree-${sidebarId}">
                <!-- Files will be loaded here -->
            </div>
        </div>
    `;

    // Add to main container
    const mainContainer = document.querySelector('.main-container');
    mainContainer.appendChild(sidebar);

    // Initialize the duplicate sidebar
    initDuplicateSidebar(sidebarId);

    // Make it resizable
    initResizer(`resizer-${sidebarId}`, () => [document.querySelector('.content-area'), sidebar], true);
}

function initDuplicateSidebar(sidebarId) {
    // Create unique variables for this sidebar instance
    window[`currentDir_${sidebarId}`] = os.homedir();
    window[`fileTree_${sidebarId}`] = document.getElementById(`fileTree-${sidebarId}`);

    // Load initial directory
    loadDirectoryForSidebar(os.homedir(), sidebarId);

    // Update favorites for this sidebar
    updateFavoritesBarForSidebar(sidebarId);
}

function loadDirectoryForSidebar(dir, sidebarId) {
    window[`currentDir_${sidebarId}`] = dir;
    const shortPath = dir.replace(os.homedir(), '~');
    document.getElementById(`currentPath-${sidebarId}`).textContent = shortPath;

    const fileTree = window[`fileTree_${sidebarId}`];
    fileTree.innerHTML = '';

    try {
        const items = fs.readdirSync(dir, { withFileTypes: true });

        // Sort: folders first, then files
        items.sort((a, b) => {
            if (a.isDirectory() && !b.isDirectory()) return -1;
            if (!a.isDirectory() && b.isDirectory()) return 1;
            return a.name.localeCompare(b.name);
        });

        items.forEach(item => {
            if (item.name.startsWith('.')) return; // Skip hidden

            const fullPath = path.join(dir, item.name);
            const isDir = item.isDirectory();
            const icon = getFileIcon(item.name, isDir);

            const el = document.createElement('div');
            el.className = 'file-item' + (isDir ? ' folder' : '');
            el.dataset.path = fullPath;
            el.draggable = true;

            let sizeStr = '';
            if (!isDir) {
                try {
                    const stats = fs.statSync(fullPath);
                    sizeStr = `<span class="size">${formatSize(stats.size)}</span>`;
                } catch (e) {}
            }

            el.innerHTML = `<i class="${icon.class} ${icon.icon}"></i> ${item.name} ${sizeStr}`;

            // Add context menu for favorites
            el.oncontextmenu = (e) => {
                e.preventDefault();
                showContextMenu(e, fullPath, isDir, sidebarId);
            };

            el.onclick = () => {
                if (isDir) {
                    loadDirectoryForSidebar(fullPath, sidebarId);
                } else {
                    openFile(fullPath);
                }
            };

            // Add drag functionality
            el.ondragstart = (e) => {
                e.dataTransfer.setData('text/plain', JSON.stringify({
                    path: fullPath,
                    isDir: isDir,
                    sourceSidebar: sidebarId
                }));
            };

            fileTree.appendChild(el);
        });
    } catch (e) {
        log('error', `Cannot read directory for sidebar ${sidebarId}: ${e.message}`);
    }
}

function updateFavoritesBarForSidebar(sidebarId) {
    const favoritesList = document.getElementById(`favoritesList-${sidebarId}`);
    if (!favoritesList) return;

    favoritesList.innerHTML = '';

    if (favoriteDirectories.size === 0) {
        favoritesList.innerHTML = '<div class="no-favorites">Right-click directories to add favorites</div>';
        return;
    }

    for (const dirPath of favoriteDirectories) {
        const shortPath = dirPath.replace(os.homedir(), '~');
        const item = document.createElement('div');
        item.className = 'favorite-item';
        item.innerHTML = `
            <i class="fas fa-folder"></i>
            <span class="favorite-name" title="${shortPath}">${shortPath.split('/').pop() || '~'}</span>
        `;
        item.onclick = () => loadDirectoryForSidebar(dirPath, sidebarId);
        favoritesList.appendChild(item);
    }
}

function showSidebarContextMenu(e) {
    // Remove any existing context menu
    const existing = document.querySelector('.context-menu');
    if (existing) existing.remove();

    const menu = document.createElement('div');
    menu.className = 'context-menu sidebar-context-menu';
    menu.style.cssText = `
        position: fixed;
        left: ${e.clientX}px;
        top: ${e.clientY}px;
        background: var(--surface, #1f2937);
        border: 1px solid var(--border, #374151);
        border-radius: 6px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        z-index: 10000;
        min-width: 140px;
    `;

    menu.innerHTML = `
        <div class="context-menu-item" onclick="toggleLeftSidebar(); document.querySelector('.sidebar-context-menu').remove();">
            <i class="fas fa-eye-slash"></i>
            Toggle Left Sidebar
        </div>
        <div class="context-menu-item" onclick="toggleRightSidebar(); document.querySelector('.sidebar-context-menu').remove();">
            <i class="fas fa-eye-slash"></i>
            Toggle Right Sidebar
        </div>
        <div class="context-menu-divider"></div>
        <div class="context-menu-item" onclick="addDuplicateSidebar(); document.querySelector('.sidebar-context-menu').remove();">
            <i class="fas fa-clone"></i>
            Duplicate Explorer
        </div>
        <div class="context-menu-item" onclick="addDuplicateTodoSidebar(); document.querySelector('.sidebar-context-menu').remove();">
            <i class="fas fa-copy"></i>
            Duplicate Todo List
        </div>
    `;

    document.body.appendChild(menu);

    // Remove menu when clicking elsewhere
    const removeMenu = (e) => {
        if (!menu.contains(e.target)) {
            menu.remove();
            document.removeEventListener('click', removeMenu);
        }
    };
    setTimeout(() => document.addEventListener('click', removeMenu), 1);
}

function showContextMenu(e, filePath, isDir, sidebarId) {
    // Remove any existing context menu
    const existing = document.querySelector('.context-menu');
    if (existing) existing.remove();

    const menu = document.createElement('div');
    menu.className = 'context-menu';
    menu.style.cssText = `
        position: fixed;
        left: ${e.clientX}px;
        top: ${e.clientY}px;
        background: var(--surface, #1f2937);
        border: 1px solid var(--border, #374151);
        border-radius: 6px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        z-index: 10000;
        min-width: 150px;
    `;

    const isFavorited = isFavorite(filePath);
    const favoriteText = isFavorited ? 'Remove from Favorites' : 'Add to Favorites';

    menu.innerHTML = `
        <div class="context-menu-item" onclick="toggleFavorite('${filePath.replace(/'/g, "\\'")}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-star"></i>
            ${favoriteText}
        </div>
        ${isDir ? `
        <div class="context-menu-item" onclick="loadDirectory('${filePath.replace(/'/g, "\\'")}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-folder-open"></i>
            Open
        </div>
        ` : `
        <div class="context-menu-item" onclick="openFile('${filePath.replace(/'/g, "\\'")}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-edit"></i>
            Open File
        </div>
        `}
    `;

    document.body.appendChild(menu);

    // Remove menu when clicking elsewhere
    const removeMenu = (e) => {
        if (!menu.contains(e.target)) {
            menu.remove();
            document.removeEventListener('click', removeMenu);
        }
    };
    setTimeout(() => document.addEventListener('click', removeMenu), 1);
}


function showContextMenu(e, filePath, isDir, sidebarId) {
    // Remove any existing context menu
    const existing = document.querySelector('.context-menu');
    if (existing) existing.remove();

    const menu = document.createElement('div');
    menu.className = 'context-menu';
    menu.style.cssText = `
        position: fixed;
        left: ${e.clientX}px;
        top: ${e.clientY}px;
        background: var(--surface, #1f2937);
        border: 1px solid var(--border, #374151);
        border-radius: 6px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        z-index: 10000;
        min-width: 150px;
    `;

    const isFavorited = isFavorite(filePath);
    const favoriteText = isFavorited ? 'Remove from Favorites' : 'Add to Favorites';

    menu.innerHTML = `
        <div class="context-menu-item" onclick="toggleFavorite('${filePath.replace(/'/g, "\\'")}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-star"></i>
            ${favoriteText}
        </div>
        ${isDir ? `
        <div class="context-menu-item" onclick="loadDirectoryForSidebar('${filePath.replace(/'/g, "\\'")}', '${sidebarId}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-folder-open"></i>
            Open
        </div>
        ` : `
        <div class="context-menu-item" onclick="openFile('${filePath.replace(/'/g, "\\'")}'); document.querySelector('.context-menu').remove();">
            <i class="fas fa-edit"></i>
            Open File
        </div>
        `}
    `;

    document.body.appendChild(menu);

    // Remove menu when clicking elsewhere
    const removeMenu = (e) => {
        if (!menu.contains(e.target)) {
            menu.remove();
            document.removeEventListener('click', removeMenu);
        }
    };
    setTimeout(() => document.addEventListener('click', removeMenu), 1);
}

// Sidebar management functions for context menu
function toggleLeftSidebar() {
    const sidebarToggle = document.getElementById('sidebarToggle');
    if (sidebarToggle) {
        sidebarToggle.click();
    }
}

function toggleRightSidebar() {
    const todoToggle = document.getElementById('todoToggle');
    if (todoToggle) {
        todoToggle.click();
    }
}

// Global functions for permission management
window.revokeDirectoryPermission = function(dirPath) {
    grantedDirectories.delete(dirPath);
    saveDirectoryPermissions();
    loadPermissionsList();
    log('info', `Directory access revoked: ${dirPath.replace(os.homedir(), '~')}`);
};

window.allowDirectoryPermission = function(dirPath) {
    deniedDirectories.delete(dirPath);
    grantedDirectories.add(dirPath);
    saveDirectoryPermissions();
    loadPermissionsList();
    log('info', `Directory access granted: ${dirPath.replace(os.homedir(), '~')}`);
};

// Check if directory access is allowed
function isDirectoryAccessAllowed(dirPath) {
    // Normalize path
    const normalizedPath = path.resolve(dirPath);

    // Always allow access to already granted directories and their subdirectories
    for (const granted of grantedDirectories) {
        if (normalizedPath.startsWith(path.resolve(granted))) {
            return true;
        }
    }

    // Deny access to denied directories
    for (const denied of deniedDirectories) {
        if (normalizedPath.startsWith(path.resolve(denied))) {
            return false;
        }
    }

    return null; // Unknown - need to request permission
}

// Request directory access permission
function requestDirectoryAccess(dirPath) {
    return new Promise((resolve) => {
        const normalizedPath = path.resolve(dirPath);
        const shortPath = normalizedPath.replace(os.homedir(), '~');

        const dialog = document.createElement('div');
        dialog.id = 'directory-permission-dialog';
        dialog.innerHTML = `
            <div style="
                position: fixed;
                top: 0;
                left: 0;
                right: 0;
                bottom: 0;
                background: rgba(0, 0, 0, 0.7);
                display: flex;
                align-items: center;
                justify-content: center;
                z-index: 10000;
                backdrop-filter: blur(5px);
            ">
                <div style="
                    background: var(--surface, #1f2937);
                    border: 1px solid var(--border, #374151);
                    border-radius: 12px;
                    padding: 24px;
                    max-width: 450px;
                    box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
                ">
                    <div style="display: flex; align-items: center; margin-bottom: 16px;">
                        <div style="font-size: 32px; margin-right: 12px;">📁</div>
                        <div>
                            <h3 style="margin: 0 0 4px 0; color: var(--text, white); font-size: 18px; font-weight: 600;">
                                Access Directory
                            </h3>
                            <p style="margin: 0; color: var(--text-muted, #9ca3af); font-size: 14px;">
                                Allow access to: <code style="background: rgba(0,0,0,0.3); padding: 2px 4px; border-radius: 3px;">${shortPath}</code>
                            </p>
                        </div>
                    </div>
                    <p style="margin: 0 0 20px 0; color: var(--text-muted, #9ca3af); line-height: 1.5; font-size: 14px;">
                        AIDE needs permission to browse files in this directory. You can change this permission later in settings.
                    </p>
                    <div style="display: flex; gap: 12px; justify-content: flex-end;">
                        <button id="deny-directory" style="
                            background: transparent;
                            color: var(--text-muted, #9ca3af);
                            border: 1px solid var(--border, #374151);
                            padding: 8px 16px;
                            border-radius: 6px;
                            cursor: pointer;
                            font-size: 14px;
                            transition: all 0.2s;
                        ">Deny</button>
                        <button id="allow-directory" style="
                            background: #10b981;
                            color: white;
                            border: none;
                            padding: 8px 16px;
                            border-radius: 6px;
                            cursor: pointer;
                            font-size: 14px;
                            font-weight: 500;
                            transition: background 0.2s;
                        ">Allow Access</button>
                    </div>
                </div>
            </div>
        `;

        document.body.appendChild(dialog);

        // Handle allow button
        document.getElementById('allow-directory').onclick = () => {
            grantedDirectories.add(normalizedPath);
            saveDirectoryPermissions();
            dialog.remove();
            resolve(true);
            log('info', `✅ Directory access granted: ${shortPath}`);
        };

        // Handle deny button
        document.getElementById('deny-directory').onclick = () => {
            deniedDirectories.add(normalizedPath);
            dialog.remove();
            resolve(false);
            log('info', `❌ Directory access denied: ${shortPath}`);
        };

        // Add hover effects
        const allowBtn = document.getElementById('allow-directory');
        const denyBtn = document.getElementById('deny-directory');

        allowBtn.onmouseover = () => allowBtn.style.background = '#059669';
        allowBtn.onmouseout = () => allowBtn.style.background = '#10b981';

        denyBtn.onmouseover = () => {
            denyBtn.style.background = 'rgba(255, 255, 255, 0.05)';
            denyBtn.style.color = 'var(--text, white)';
        };
        denyBtn.onmouseout = () => {
            denyBtn.style.background = 'transparent';
            denyBtn.style.color = 'var(--text-muted, #9ca3af)';
        };
    });
}

// Load privacy settings from localStorage
function loadVoicePrivacySettings() {
    const saved = localStorage.getItem('aide-voice-privacy');
    if (saved) {
        try {
            voicePrivacySettings = { ...voicePrivacySettings, ...JSON.parse(saved) };
        } catch (e) {
            log('warn', 'Failed to load voice privacy settings');
        }
    }
}

// Microphone permission management
let micPermissionGranted = false;
let micPermissionRequested = false;

function showMicrophonePermissionDialog() {
    // Don't show if already requested this session or if permission granted
    if (micPermissionRequested || micPermissionGranted) return;

    micPermissionRequested = true;

    const dialog = document.createElement('div');
    dialog.id = 'mic-permission-dialog';
    dialog.innerHTML = `
        <div style="
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.7);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 10000;
            backdrop-filter: blur(5px);
        ">
            <div style="
                background: var(--surface, #1f2937);
                border: 1px solid var(--border, #374151);
                border-radius: 12px;
                padding: 24px;
                max-width: 400px;
                box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
                text-align: center;
            ">
                <div style="font-size: 48px; margin-bottom: 16px;">🎤</div>
                <h3 style="margin: 0 0 12px 0; color: var(--text, white); font-size: 18px; font-weight: 600;">
                    Enable Voice Commands
                </h3>
                <p style="margin: 0 0 20px 0; color: var(--text-muted, #9ca3af); line-height: 1.5; font-size: 14px;">
                    AIDE can listen for voice commands like "Hey Dee Dee" to help you code faster.
                    This requires microphone access to detect wake words and commands.
                </p>
                <div style="display: flex; gap: 12px; justify-content: center;">
                    <button id="enable-mic" style="
                        background: #3b82f6;
                        color: white;
                        border: none;
                        padding: 10px 20px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        font-weight: 500;
                        transition: background 0.2s;
                    ">Enable Microphone</button>
                    <button id="skip-mic" style="
                        background: transparent;
                        color: var(--text-muted, #9ca3af);
                        border: 1px solid var(--border, #374151);
                        padding: 10px 20px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        transition: all 0.2s;
                    ">Skip for Now</button>
                </div>
                <p style="margin: 16px 0 0 0; font-size: 12px; color: var(--text-muted, #6b7280);">
                    You can enable this later in settings, or say "Hey Dee Dee" to try again.
                </p>
            </div>
        </div>
    `;

    document.body.appendChild(dialog);

    // Handle enable button
    document.getElementById('enable-mic').onclick = async () => {
        dialog.remove();
        await requestMicrophonePermission();
    };

    // Handle skip button
    document.getElementById('skip-mic').onclick = () => {
        dialog.remove();
        log('info', 'User skipped microphone permission - voice features disabled');
        // Store preference to not ask again this session
        localStorage.setItem('aide-mic-permission-skipped', Date.now().toString());
    };

    // Add hover effects
    const enableBtn = document.getElementById('enable-mic');
    const skipBtn = document.getElementById('skip-mic');

    enableBtn.onmouseover = () => enableBtn.style.background = '#2563eb';
    enableBtn.onmouseout = () => enableBtn.style.background = '#3b82f6';

    skipBtn.onmouseover = () => {
        skipBtn.style.background = 'rgba(255, 255, 255, 0.05)';
        skipBtn.style.color = 'var(--text, white)';
    };
    skipBtn.onmouseout = () => {
        skipBtn.style.background = 'transparent';
        skipBtn.style.color = 'var(--text-muted, #9ca3af)';
    };
}

async function requestMicrophonePermission() {
    try {
        log('info', 'Requesting microphone permission...');

        const stream = await navigator.mediaDevices.getUserMedia({
            audio: {
                echoCancellation: true,
                noiseSuppression: true,
                autoGainControl: true,
                sampleRate: 44100,
                channelCount: 1
            }
        });

        // Stop the stream immediately - we just needed permission
        stream.getTracks().forEach(track => track.stop());

        micPermissionGranted = true;
        voicePrivacySettings.enableAnalytics = true; // Enable since user consented
        saveVoicePrivacySettings();

        log('info', '✅ Microphone permission granted');

        // Show success message
        showPermissionSuccess();

        // Initialize voice features
        setTimeout(initVoiceActivation, 500);

    } catch (error) {
        log('error', `❌ Microphone permission denied: ${error.message}`);
        micPermissionGranted = false;

        // Show error dialog
        showPermissionError(error);
    }
}

function showPermissionSuccess() {
    const success = document.createElement('div');
    success.id = 'permission-success';
    success.innerHTML = `
        <div style="
            position: fixed;
            top: 20px;
            right: 20px;
            background: #10b981;
            color: white;
            padding: 12px 16px;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            z-index: 10001;
            font-size: 14px;
            animation: slideInRight 0.3s ease-out;
        ">
            <i class="fas fa-check-circle" style="margin-right: 8px;"></i>
            Voice commands enabled! Try saying "Hey Dee Dee"
        </div>
    `;

    document.body.appendChild(success);

    setTimeout(() => {
        if (success.parentNode) {
            success.style.animation = 'slideOutRight 0.3s ease-in';
            setTimeout(() => success.remove(), 300);
        }
    }, 3000);
}

function showDirectoryAccessDenied(dirPath) {
    const shortPath = dirPath.replace(os.homedir(), '~');

    const denied = document.createElement('div');
    denied.id = 'directory-denied';
    denied.innerHTML = `
        <div style="
            position: fixed;
            top: 20px;
            right: 20px;
            background: #ef4444;
            color: white;
            padding: 12px 16px;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            z-index: 10001;
            font-size: 14px;
            max-width: 300px;
            animation: slideInRight 0.3s ease-out;
        ">
            <i class="fas fa-exclamation-triangle" style="margin-right: 8px;"></i>
            Access denied to: <code style="background: rgba(255,255,255,0.2); padding: 1px 3px; border-radius: 2px;">${shortPath}</code>
        </div>
    `;

    document.body.appendChild(denied);

    setTimeout(() => {
        if (denied.parentNode) {
            denied.style.animation = 'slideOutRight 0.3s ease-in';
            setTimeout(() => denied.remove(), 300);
        }
    }, 4000);
}

function showPermissionError(error) {
    const errorDialog = document.createElement('div');
    errorDialog.id = 'permission-error';
    errorDialog.innerHTML = `
        <div style="
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.7);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 10000;
            backdrop-filter: blur(5px);
        ">
            <div style="
                background: var(--surface, #1f2937);
                border: 1px solid var(--border, #374151);
                border-radius: 12px;
                padding: 24px;
                max-width: 400px;
                box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
                text-align: center;
            ">
                <div style="font-size: 48px; margin-bottom: 16px; color: #ef4444;">🚫</div>
                <h3 style="margin: 0 0 12px 0; color: var(--text, white); font-size: 18px; font-weight: 600;">
                    Microphone Access Required
                </h3>
                <p style="margin: 0 0 20px 0; color: var(--text-muted, #9ca3af); line-height: 1.5; font-size: 14px;">
                    ${error.name === 'NotAllowedError' ?
                        'Microphone access was denied. Please allow microphone access in your browser settings to use voice commands.' :
                        'Unable to access microphone. Please check your microphone settings and try again.'}
                </p>
                <div style="display: flex; gap: 12px; justify-content: center;">
                    <button id="retry-mic-error" style="
                        background: #3b82f6;
                        color: white;
                        border: none;
                        padding: 10px 20px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        font-weight: 500;
                        transition: background 0.2s;
                    ">Try Again</button>
                    <button id="cancel-mic-error" style="
                        background: transparent;
                        color: var(--text-muted, #9ca3af);
                        border: 1px solid var(--border, #374151);
                        padding: 10px 20px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        transition: all 0.2s;
                    ">Continue Without Voice</button>
                </div>
            </div>
        </div>
    `;

    document.body.appendChild(errorDialog);

    // Handle retry button
    document.getElementById('retry-mic-error').onclick = () => {
        errorDialog.remove();
        requestMicrophonePermission();
    };

    // Handle cancel button
    document.getElementById('cancel-mic-error').onclick = () => {
        errorDialog.remove();
        log('info', 'User chose to continue without microphone - voice features disabled');
    };
}

// Save privacy settings
function saveVoicePrivacySettings() {
    localStorage.setItem('aide-voice-privacy', JSON.stringify(voicePrivacySettings));
}

// Privacy-aware analytics tracking
function trackVoiceEvent(eventType, data = {}) {
    if (!voicePrivacySettings.enableAnalytics) return;

    const event = {
        type: eventType,
        timestamp: Date.now(),
        ...data
    };

    // Store locally only, never send to remote servers
    const events = JSON.parse(localStorage.getItem('aide-voice-events') || '[]');
    events.push(event);

    // Limit storage to prevent unlimited growth
    const cutoffTime = Date.now() - (voicePrivacySettings.maxRetentionHours * 60 * 60 * 1000);
    const filteredEvents = events.filter(e => e.timestamp > cutoffTime);

    localStorage.setItem('aide-voice-events', JSON.stringify(filteredEvents));
}

function showVoiceCommandsHelp() {
    const commands = [
        '"Aye Dee Dee" - Wake/summon',
        '"Dee Dee Hide" - Hide completely',
        '"Dee Dee Save" - Save current file',
        '"Dee Dee New File" - Create new file',
        '"Dee Dee Theme" - Toggle dark/light',
        '"Dee Dee Files" - Open explorer',
        '"Dee Dee Todo" - Open TODO list',
        '"Dee Dee Chat" - Focus chat input',
        '"Dee Dee Terminal" - Focus terminal',
        '"Dee Dee Clear" - Clear chat',
        '"Dee Dee Commands" - Show this help'
    ];
    addMessage('assistant', `<strong>🎤 Voice Commands:</strong><br><br>${commands.join('<br>')}`);
}

function initAudioContext() {
    try {
        // Initialize Web Audio API for voice activity detection and preprocessing
        audioContext = new (window.AudioContext || window.webkitAudioContext)();
        analyser = audioContext.createAnalyser();
        analyser.fftSize = 256;
        analyser.smoothingTimeConstant = 0.8;

        // Get microphone access with audio constraints for preprocessing
        const audioConstraints = {
            audio: {
                echoCancellation: true,
                noiseSuppression: true,
                autoGainControl: true,
                sampleRate: 44100,
                channelCount: 1,
                volume: 1.0
            }
        };

        navigator.mediaDevices.getUserMedia(audioConstraints)
            .then(stream => {
                // Apply additional audio preprocessing
                setupAudioPreprocessing(stream);
                log('info', 'Audio context initialized with preprocessing for voice activity detection');
                startVoiceActivityDetection();
            })
            .catch(err => {
                log('warn', `Could not access microphone for voice activity detection: ${err.message}`);
                // Try fallback without preprocessing
                navigator.mediaDevices.getUserMedia({ audio: true })
                    .then(stream => {
                        microphone = audioContext.createMediaStreamSource(stream);
                        microphone.connect(analyser);
                        log('info', 'Audio context initialized (fallback mode)');
                        startVoiceActivityDetection();
                    })
                    .catch(fallbackErr => {
                        log('error', 'Microphone access completely failed');
                    });
            });
    } catch (e) {
        log('warn', 'Web Audio API not supported - voice activity detection disabled');
    }
}

function setupAudioPreprocessing(stream) {
    try {
        microphone = audioContext.createMediaStreamSource(stream);

        // Create audio processing chain
        const gainNode = audioContext.createGain();
        gainNode.gain.value = 1.0;

        // Add a simple high-pass filter to reduce low-frequency noise
        const highPassFilter = audioContext.createBiquadFilter();
        highPassFilter.type = 'highpass';
        highPassFilter.frequency.value = 80; // Remove frequencies below 80Hz
        highPassFilter.Q.value = 0.7;

        // Add a low-pass filter to reduce high-frequency noise
        const lowPassFilter = audioContext.createBiquadFilter();
        lowPassFilter.type = 'lowpass';
        lowPassFilter.frequency.value = 8000; // Remove frequencies above 8kHz
        lowPassFilter.Q.value = 0.7;

        // Dynamic range compression to even out volume levels
        const compressor = audioContext.createDynamicsCompressor();
        compressor.threshold.value = -24; // dB
        compressor.knee.value = 30; // dB
        compressor.ratio.value = 12; // compression ratio
        compressor.attack.value = 0.003; // seconds
        compressor.release.value = 0.25; // seconds

        // Connect the audio processing chain
        microphone.connect(highPassFilter);
        highPassFilter.connect(lowPassFilter);
        lowPassFilter.connect(compressor);
        compressor.connect(gainNode);
        gainNode.connect(analyser);

        // Also connect to destination for speech recognition (with original constraints applied)
        microphone.connect(audioContext.destination);

        log('info', 'Audio preprocessing chain initialized: high-pass → low-pass → compressor → analyser');
    } catch (e) {
        log('warn', `Audio preprocessing failed: ${e.message} - falling back to basic setup`);
        // Fallback to basic connection
        microphone = audioContext.createMediaStreamSource(stream);
        microphone.connect(analyser);
    }
}

function startVoiceActivityDetection() {
    if (!analyser) return;

    const bufferLength = analyser.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);
    const timeDataArray = new Uint8Array(bufferLength);

    // Adaptive threshold variables
    let backgroundNoiseLevel = 0.05; // Initial estimate
    let adaptationRate = 0.01;
    let voiceActivityCounter = 0;
    let silenceCounter = 0;

    // Voice activity state
    let isVoiceActive = false;
    let lastVoiceActivityTime = 0;

    function checkVoiceActivity() {
        if (!analyser) return;

        analyser.getByteFrequencyData(dataArray);
        analyser.getByteTimeDomainData(timeDataArray);

        // Calculate RMS (Root Mean Square) for better volume detection
        let sum = 0;
        for (let i = 0; i < bufferLength; i++) {
            const sample = (dataArray[i] - 128) / 128; // Convert to -1 to 1 range
            sum += sample * sample;
        }
        const rms = Math.sqrt(sum / bufferLength);

        // Calculate frequency-weighted energy (emphasize speech frequencies)
        let speechEnergy = 0;
        let totalEnergy = 0;
        const nyquist = audioContext.sampleRate / 2;
        const speechBandStart = Math.floor((300 / nyquist) * bufferLength); // 300Hz
        const speechBandEnd = Math.floor((3400 / nyquist) * bufferLength); // 3400Hz

        for (let i = speechBandStart; i < speechBandEnd && i < bufferLength; i++) {
            speechEnergy += dataArray[i] * dataArray[i];
            totalEnergy += dataArray[i] * dataArray[i];
        }

        const speechRatio = speechEnergy / Math.max(totalEnergy, 1);

        // Adaptive background noise estimation
        if (rms < backgroundNoiseLevel) {
            backgroundNoiseLevel = backgroundNoiseLevel * (1 - adaptationRate) + rms * adaptationRate;
            silenceCounter++;
            voiceActivityCounter = Math.max(0, voiceActivityCounter - 1);
        } else {
            voiceActivityCounter++;
            silenceCounter = 0;
        }

        // Dynamic threshold based on background noise
        const dynamicThreshold = Math.max(backgroundNoiseLevel * 2.5, voiceActivityThreshold);

        // Voice activity detection with hysteresis
        const now = Date.now();

        if (!isVoiceActive && rms > dynamicThreshold && speechRatio > 0.3) {
            // Voice activity detected
            isVoiceActive = true;
            lastVoiceActivityTime = now;
            voiceActivityCounter = Math.max(voiceActivityCounter, 5); // Minimum activity count

            log('debug', `Voice activity detected (RMS: ${rms.toFixed(3)}, threshold: ${dynamicThreshold.toFixed(3)}, speech ratio: ${speechRatio.toFixed(2)})`);
        } else if (isVoiceActive && (rms < dynamicThreshold * 0.7 || speechRatio < 0.2)) {
            // Check if voice activity has ended
            if (now - lastVoiceActivityTime > 500) { // Minimum activity duration
                isVoiceActive = false;
                log('debug', 'Voice activity ended');
            }
        } else if (isVoiceActive) {
            lastVoiceActivityTime = now;
        }

        // Reset voice activity timeout if we're actively detecting voice
        if (isVoiceActive) {
            if (voiceActivityTimeout) {
                clearTimeout(voiceActivityTimeout);
            }
            voiceActivityTimeout = setTimeout(() => {
                isVoiceActive = false;
                log('debug', 'Voice activity timeout');
            }, 3000); // 3 second timeout after last detection
        }

        requestAnimationFrame(checkVoiceActivity);
    }

    // Initialize background noise estimation with first few samples
    let calibrationSamples = 0;
    const calibrationDuration = 60; // 1 second at 60fps

    function calibrateBackgroundNoise() {
        analyser.getByteFrequencyData(dataArray);

        let sum = 0;
        for (let i = 0; i < bufferLength; i++) {
            sum += (dataArray[i] - 128) * (dataArray[i] - 128);
        }
        const rms = Math.sqrt(sum / bufferLength) / 128;

        backgroundNoiseLevel = backgroundNoiseLevel * 0.9 + rms * 0.1;
        calibrationSamples++;

        if (calibrationSamples < calibrationDuration) {
            requestAnimationFrame(calibrateBackgroundNoise);
        } else {
            log('info', `Voice activity detection calibrated. Background noise: ${backgroundNoiseLevel.toFixed(3)}`);
            checkVoiceActivity(); // Start main detection loop
        }
    }

    calibrateBackgroundNoise();
}

function initVoiceActivation() {
    if (!('webkitSpeechRecognition' in window) && !('SpeechRecognition' in window)) {
        log('warn', 'Voice activation not supported');
        return;
    }

    // Initialize audio context for voice activity detection
    initAudioContext();
    
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    recognition = new SpeechRecognition();
    recognition.continuous = true;
    recognition.interimResults = true;
    recognition.lang = 'en-US';
    
    recognition.onresult = (event) => {
        for (let i = event.resultIndex; i < event.results.length; i++) {
            const result = event.results[i];
            const transcript = result[0].transcript.toLowerCase().trim();
            const confidence = result[0].confidence;

            // Skip low confidence results unless it's a wake word
            if (confidence < 0.3 && !isWakeWord(transcript)) {
                continue;
            }

            log('debug', `Voice result: "${transcript}" (confidence: ${confidence.toFixed(2)})`);

            // Check all voice commands with improved matching
            let handled = false;
            const availableCommands = getVoiceCommands();
            
            // Sort commands by phrase length (longer = more specific = check first)
            const sortedCommands = Object.entries(availableCommands).sort((a, b) => {
                const maxA = Math.max(...a[1].phrases.map(p => p.length));
                const maxB = Math.max(...b[1].phrases.map(p => p.length));
                return maxB - maxA;
            });
            
            for (const [cmdName, cmd] of sortedCommands) {
                const match = findBestPhraseMatch(transcript, cmd.phrases, confidence);

                if (match.found) {
                    // Check contextual relevance
                    const contextCheck = checkCommandContext(cmdName);
                    const adjustedConfidence = match.confidence * contextCheck.relevance;

                    // Apply privacy confidence threshold
                    if (adjustedConfidence < voicePrivacySettings.confidenceThreshold) {
                        log('debug', `Command "${cmdName}" below confidence threshold (${adjustedConfidence.toFixed(2)} < ${voicePrivacySettings.confidenceThreshold})`);
                        continue;
                    }

                    if (adjustedConfidence < 0.5) {
                        log('debug', `Command "${cmdName}" not relevant in current context (${contextCheck.reason})`);
                        continue;
                    }

                    // Privacy-aware analytics tracking
                    if (voicePrivacySettings.enableAnalytics) {
                        voiceAnalytics.commandsRecognized++;
                        voiceAnalytics.averageConfidence =
                            (voiceAnalytics.averageConfidence * (voiceAnalytics.commandsRecognized - 1) + adjustedConfidence) /
                            voiceAnalytics.commandsRecognized;

                        if (cmdName === 'wake') {
                            voiceAnalytics.wakeWordsDetected++;
                        }

                        // Track the command (without storing the actual transcript for privacy)
                        trackVoiceEvent('command_recognized', {
                            command: cmdName,
                            confidence: adjustedConfidence,
                            contextRelevance: contextCheck.relevance
                        });
                    }

                    log('info', `🎤 Heard command → ${cmd.feedback.log} (${adjustedConfidence.toFixed(2)})`);

                    // Enhanced visual feedback
                    showVoiceFeedback('success', cmd.feedback.log, 1500);

                    // Audio feedback for successful commands
                    if (voicePrivacySettings.enableAudioFeedback !== false) {
                        playSuccessSound();
                    }

                        cmd.action();
                        
                    // Enhanced visual feedback with confidence-based intensity
                    const intensity = Math.min(match.confidence * 1.5, 1);
                    document.body.style.boxShadow = `0 0 ${30 * intensity}px ${cmd.feedback.color}`;
                        setTimeout(() => {
                            document.body.style.boxShadow = '';
                    }, Math.max(400 * intensity, 200));

                    // Audio feedback for wake words
                    if (cmdName === 'wake') {
                        playWakeSound();
                    }
                        
                        handled = true;
                        break;
                    }
                }

            // If no command matched but confidence is high, could log for analysis
            if (!handled && confidence > 0.7) {
                log('debug', `High confidence but no match: "${transcript}"`);
            }
        }
    };
    
    recognition.onerror = (event) => {
        const error = event.error;
        log('warn', `Voice recognition error: ${error}`);

        // Handle different error types with appropriate recovery
        // Track error analytics
        voiceAnalytics.errors.push({
            error: error,
            timestamp: Date.now(),
            context: 'recognition'
        });

        switch (error) {
            case 'network':
                log('error', 'Network error - voice recognition unavailable');
                // Try to restart after delay
                setTimeout(() => {
                    if (isListening) {
                        log('info', 'Attempting to restart voice recognition after network error');
                        stopListening();
                        setTimeout(startListening, 2000);
                    }
                }, 5000);
                break;

            case 'not-allowed':
            case 'permission-denied':
                log('error', 'Microphone permission denied - please allow microphone access');
                stopListening();
                showMicrophonePermissionPrompt();
                break;

            case 'no-speech':
                // This is normal - user just didn't speak
                break;

            case 'audio-capture':
                log('error', 'Audio capture failed - check microphone');
                // Try restarting
                setTimeout(() => {
                    if (isListening) {
                        stopListening();
                        setTimeout(startListening, 1000);
                    }
                }, 2000);
                break;

            case 'service-not-allowed':
                log('error', 'Speech service not allowed');
                stopListening();
                break;

            default:
                log('error', `Unknown voice error: ${error}`);
                // Generic recovery - try restart
                setTimeout(() => {
                    if (isListening) {
                        stopListening();
                        setTimeout(startListening, 3000);
                    }
                }, 1000);
        }
    };
    
    recognition.onend = () => {
        // Restart listening
        if (isListening) {
            try {
                recognition.start();
            } catch (e) {}
        }
    };
    
    startListening();
}

let voiceRetryCount = 0;
const MAX_VOICE_RETRIES = 5;

function startListening() {
    if (recognition && !isListening) {
        try {
            recognition.start();
            isListening = true;
            voiceRetryCount = 0; // Reset retry count on success
            log('info', '🎤 Listening for "Aye Dee Dee"...');
        } catch (e) {
            log('warn', `Could not start voice recognition: ${e.message}`);
            handleVoiceStartFailure();
        }
    }
}

function handleVoiceStartFailure() {
    voiceRetryCount++;
    if (voiceRetryCount <= MAX_VOICE_RETRIES) {
        const delay = Math.min(1000 * Math.pow(2, voiceRetryCount), 30000); // Exponential backoff, max 30s
        log('info', `Retrying voice recognition in ${delay/1000}s (attempt ${voiceRetryCount}/${MAX_VOICE_RETRIES})`);
        setTimeout(() => {
            if (!isListening) {
                startListening();
            }
        }, delay);
    } else {
        log('error', 'Voice recognition failed after maximum retries - voice commands disabled');
        // Could show a user notification here
    }
}

function stopListening() {
    if (recognition && isListening) {
        recognition.stop();
        isListening = false;
    }
}

// Enhanced phrase matching with fuzzy logic
function findBestPhraseMatch(transcript, phrases, baseConfidence) {
    let bestMatch = { found: false, confidence: 0, phrase: '' };

    for (const phrase of phrases) {
        const similarity = calculateStringSimilarity(transcript, phrase);
        const combinedConfidence = (baseConfidence + similarity) / 2;

        if (combinedConfidence > bestMatch.confidence && combinedConfidence > 0.6) {
            bestMatch = {
                found: true,
                confidence: combinedConfidence,
                phrase: phrase
            };
        }
    }

    return bestMatch;
}

// Simple string similarity calculation (Levenshtein distance based)
function calculateStringSimilarity(str1, str2) {
    const longer = str1.length > str2.length ? str1 : str2;
    const shorter = str1.length > str2.length ? str2 : str1;

    if (longer.length === 0) return 1.0;

    const distance = levenshteinDistance(longer, shorter);
    return (longer.length - distance) / longer.length;
}

// Levenshtein distance calculation
function levenshteinDistance(str1, str2) {
    const matrix = [];
    for (let i = 0; i <= str2.length; i++) {
        matrix[i] = [i];
    }
    for (let j = 0; j <= str1.length; j++) {
        matrix[0][j] = j;
    }
    for (let i = 1; i <= str2.length; i++) {
        for (let j = 1; j <= str1.length; j++) {
            if (str2.charAt(i - 1) === str1.charAt(j - 1)) {
                matrix[i][j] = matrix[i - 1][j - 1];
            } else {
                matrix[i][j] = Math.min(
                    matrix[i - 1][j - 1] + 1, // substitution
                    matrix[i][j - 1] + 1,     // insertion
                    matrix[i - 1][j] + 1      // deletion
                );
            }
        }
    }
    return matrix[str2.length][str1.length];
}

// Enhanced wake word detection with context and confidence
function isWakeWord(transcript) {
    const wakeWords = [
        { phrase: 'aye dee dee', weight: 1.0 },
        { phrase: 'hey dee dee', weight: 1.0 },
        { phrase: 'a d d', weight: 0.8 },
        { phrase: 'aide', weight: 0.7 },
        { phrase: 'ay dee dee', weight: 0.9 },
        { phrase: 'aydeedee', weight: 0.6 },
        { phrase: 'dee dee', weight: 0.5 }, // Partial match
        { phrase: 'hey aide', weight: 0.8 }
    ];

    // Check for exact or near-exact matches
    for (const wakeWord of wakeWords) {
        if (transcript.includes(wakeWord.phrase)) {
            return { detected: true, confidence: wakeWord.weight, word: wakeWord.phrase };
        }

        // Check for fuzzy matches with high similarity
        const similarity = calculateStringSimilarity(transcript, wakeWord.phrase);
        if (similarity > 0.8) {
            return { detected: true, confidence: wakeWord.weight * similarity, word: wakeWord.phrase };
        }
    }

    // Check for phonetic variations and misspellings
    const phoneticVariations = [
        ['hey', 'hay', 'hi'],
        ['dee', 'dee', 'di', 'd'],
        ['aye', 'eye', 'i', 'a']
    ];

    let bestMatch = { detected: false, confidence: 0, word: '' };

    for (const wakeWord of wakeWords) {
        const words = wakeWord.phrase.split(' ');
        let transcriptWords = transcript.toLowerCase().split(/\s+/);

        // Try to match phonetic variations
        let matchScore = 0;
        let matchedWords = 0;

        for (const wakeWordPart of words) {
            for (const transcriptWord of transcriptWords) {
                // Check phonetic similarity
                if (calculatePhoneticSimilarity(wakeWordPart, transcriptWord) > 0.7) {
                    matchScore += wakeWord.weight;
                    matchedWords++;
                    break;
                }
            }
        }

        if (matchedWords === words.length && matchScore > bestMatch.confidence) {
            bestMatch = {
                detected: true,
                confidence: Math.min(matchScore / words.length, 1.0),
                word: wakeWord.phrase
            };
        }
    }

    return bestMatch.detected ? bestMatch : { detected: false, confidence: 0, word: '' };
}

// Simple phonetic similarity (could be enhanced with proper phonetics)
function calculatePhoneticSimilarity(word1, word2) {
    // Basic vowel/consonant pattern matching
    const vowels = 'aeiou';
    const pattern1 = word1.split('').map(c => vowels.includes(c.toLowerCase()) ? 'V' : 'C').join('');
    const pattern2 = word2.split('').map(c => vowels.includes(c.toLowerCase()) ? 'V' : 'C').join('');

    if (pattern1 === pattern2) {
        return calculateStringSimilarity(word1, word2);
    }

    return 0;
}

// Context-aware command validation
function checkCommandContext(commandName) {
    const context = {
        relevance: 1.0,
        reason: 'command is always relevant'
    };

    switch (commandName) {
        case 'saveFile':
            // Only relevant if there's an active file with unsaved changes
            const activeTab = document.querySelector('.tab.active');
            if (!activeTab) {
                context.relevance = 0.3;
                context.reason = 'no active file to save';
            } else {
                // Check if file has been modified (this would need more implementation)
                context.relevance = 0.8; // Assume it's usually relevant
            }
            break;

        case 'newFile':
            // Always relevant, but slightly less if user already has many tabs
            const tabCount = document.querySelectorAll('.tab').length;
            if (tabCount > 5) {
                context.relevance = 0.7;
                context.reason = 'many files already open';
            }
            break;

        case 'openExplorer':
        case 'openTodo':
            // Check if the panel is already open
            const sidebar = document.getElementById('sidebar');
            const todoSidebar = document.getElementById('todoSidebar');

            if (commandName === 'openExplorer' && sidebar && !sidebar.classList.contains('collapsed')) {
                context.relevance = 0.4;
                context.reason = 'explorer already open';
            } else if (commandName === 'openTodo' && todoSidebar && !todoSidebar.classList.contains('collapsed')) {
                context.relevance = 0.4;
                context.reason = 'todo list already open';
            }
            break;

        case 'focusChat':
            // Check if chat is already focused
            const chatInput = document.getElementById('chatInput');
            if (chatInput && chatInput === document.activeElement) {
                context.relevance = 0.5;
                context.reason = 'chat already focused';
            }
            break;

        case 'toggleTheme':
            // Always relevant but depends on user preference
            context.relevance = 0.9;
            break;

        case 'clearChat':
            // Check if there's anything to clear
            const chatMessages = document.getElementById('chatMessages');
            if (!chatMessages || chatMessages.children.length === 0) {
                context.relevance = 0.3;
                context.reason = 'chat already empty';
            }
            break;

        case 'runCode':
            // Only relevant if there's code to run
            const activeEditor = document.querySelector('.tab.active .monaco-editor');
            if (!activeEditor) {
                context.relevance = 0.4;
                context.reason = 'no code editor active';
            }
            break;

        case 'wake':
        case 'hide':
            // Always highly relevant
            context.relevance = 1.0;
            break;

        default:
            context.relevance = 0.8; // Default relevance
    }

    return context;
}

// Microphone permission management
function showMicrophonePermissionPrompt() {
    // Create a permission prompt overlay
    const prompt = document.createElement('div');
    prompt.id = 'mic-permission-prompt';
    prompt.innerHTML = `
        <div style="
            position: fixed;
            top: 20px;
            right: 20px;
            background: #1f2937;
            color: white;
            padding: 16px;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.5);
            border: 1px solid #374151;
            z-index: 10000;
            max-width: 300px;
        ">
            <h4 style="margin: 0 0 8px 0; color: #f59e0b;">🎤 Microphone Access Needed</h4>
            <p style="margin: 0 0 12px 0; font-size: 14px; color: #d1d5db;">
                Voice commands require microphone access. Please allow microphone access in your browser settings.
            </p>
            <button id="retry-mic" style="
                background: #3b82f6;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                cursor: pointer;
                font-size: 14px;
            ">Retry Access</button>
            <button id="dismiss-mic" style="
                background: transparent;
                color: #9ca3af;
                border: 1px solid #4b5563;
                padding: 8px 16px;
                border-radius: 4px;
                cursor: pointer;
                font-size: 14px;
                margin-left: 8px;
            ">Dismiss</button>
        </div>
    `;

    document.body.appendChild(prompt);

    // Handle retry button
    document.getElementById('retry-mic').onclick = () => {
        prompt.remove();
        requestMicrophonePermission();
    };

    // Handle dismiss button
    document.getElementById('dismiss-mic').onclick = () => {
        prompt.remove();
    };

    // Auto-dismiss after 10 seconds
    setTimeout(() => {
        if (prompt.parentNode) {
            prompt.remove();
        }
    }, 10000);
}

async function requestMicrophonePermission() {
    try {
        const stream = await navigator.mediaDevices.getUserMedia({
            audio: {
                echoCancellation: true,
                noiseSuppression: true,
                autoGainControl: true
            }
        });
        stream.getTracks().forEach(track => track.stop()); // Stop immediately

        log('info', 'Microphone permission granted');
        // Re-initialize voice activation
        setTimeout(initVoiceActivation, 500);
    } catch (err) {
        log('error', `Microphone permission request failed: ${err.message}`);
        showMicrophonePermissionPrompt();
    }
}

// Audio feedback functions
function playWakeSound() {
    playTone([800, 600], 0.1, 0.15);
}

function playSuccessSound() {
    playTone([600, 800], 0.08, 0.1);
}

function playErrorSound() {
    playTone([400, 300], 0.15, 0.2);
}

function playTone(frequencies, duration, volume = 0.1) {
    try {
        if (!audioContext || voicePrivacySettings.enableAudioFeedback === false) return;

        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);

        // Create frequency sweep
        oscillator.frequency.setValueAtTime(frequencies[0], audioContext.currentTime);
        if (frequencies.length > 1) {
            oscillator.frequency.exponentialRampToValueAtTime(frequencies[1], audioContext.currentTime + duration);
        }

        // Volume envelope
        gainNode.gain.setValueAtTime(0, audioContext.currentTime);
        gainNode.gain.linearRampToValueAtTime(volume, audioContext.currentTime + 0.01);
        gainNode.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + duration);

        oscillator.start(audioContext.currentTime);
        oscillator.stop(audioContext.currentTime + duration);
    } catch (e) {
        // Silently fail if audio context is not available
    }
}

// Voice analytics functions
function getVoiceAnalytics() {
    const uptime = Date.now() - voiceAnalytics.startTime;
    return {
        ...voiceAnalytics,
        uptime: uptime,
        commandsPerHour: voiceAnalytics.commandsRecognized / (uptime / (1000 * 60 * 60)),
        errorRate: voiceAnalytics.errors.length / Math.max(voiceAnalytics.commandsRecognized, 1),
        recentErrors: voiceAnalytics.errors.slice(-5) // Last 5 errors
    };
}

function resetVoiceAnalytics() {
    voiceAnalytics = {
        commandsRecognized: 0,
        wakeWordsDetected: 0,
        falsePositives: 0,
        averageConfidence: 0,
        errors: [],
        startTime: Date.now()
    };
    log('info', 'Voice analytics reset');
}

// Expose for debugging (can be called from console)
window.getVoiceAnalytics = getVoiceAnalytics;
window.resetVoiceAnalytics = resetVoiceAnalytics;

// Enhanced visual feedback for voice commands
function showVoiceFeedback(type, message, duration = 2000) {
    // Create or update feedback overlay
    let feedback = document.getElementById('voice-feedback');
    if (!feedback) {
        feedback = document.createElement('div');
        feedback.id = 'voice-feedback';
        feedback.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: rgba(0, 0, 0, 0.8);
            color: white;
            padding: 12px 16px;
            border-radius: 8px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            font-size: 14px;
            z-index: 10000;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            animation: slideIn 0.3s ease-out;
        `;
        document.body.appendChild(feedback);
    }

    feedback.textContent = message;
    feedback.style.background = type === 'success' ? 'rgba(34, 197, 94, 0.9)' :
                               type === 'error' ? 'rgba(239, 68, 68, 0.9)' :
                               'rgba(59, 130, 246, 0.9)';

    // Auto-hide after duration
    clearTimeout(feedback.hideTimeout);
    feedback.hideTimeout = setTimeout(() => {
        if (feedback.parentNode) {
            feedback.style.animation = 'slideOut 0.3s ease-in';
            setTimeout(() => {
                if (feedback.parentNode) {
                    feedback.remove();
                }
            }, 300);
        }
    }, duration);
}

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
@keyframes slideIn {
    from { transform: translateX(100%); opacity: 0; }
    to { transform: translateX(0); opacity: 1; }
}
@keyframes slideOut {
    from { transform: translateX(0); opacity: 1; }
    to { transform: translateX(100%); opacity: 0; }
}
@keyframes slideInRight {
    from { transform: translateX(100%); opacity: 0; }
    to { transform: translateX(0); opacity: 1; }
}
@keyframes slideOutRight {
    from { transform: translateX(0); opacity: 1; }
    to { transform: translateX(100%); opacity: 0; }
}
`;
document.head.appendChild(style);

// Check if we should show microphone permission dialog
function shouldShowMicPermissionDialog() {
    // Don't show if user already granted permission
    if (micPermissionGranted) return false;

    // Don't show if user skipped it recently (within last hour)
    const skippedTime = localStorage.getItem('aide-mic-permission-skipped');
    if (skippedTime && Date.now() - parseInt(skippedTime) < 60 * 60 * 1000) return false;

    // Show for free users (voice is a pro feature but we can still prompt)
    return true;
}

// Initialize voice after loading privacy settings
setTimeout(() => {
    loadVoicePrivacySettings();

    // Check if we need to show permission dialog
    if (shouldShowMicPermissionDialog()) {
        showMicrophonePermissionDialog();
    } else {
        // Initialize voice directly if permission already handled
        initVoiceActivation();
    }
}, 1000);

// === FEATURE FLAGS & PRO/FREE MODE ===
async function initFeatures() {
    features = await ipcRenderer.invoke('get-features');
    
    // Update tier from features
    userTier = features.tier || 'free';
    localStorage.setItem('aide-tier', userTier);
    
    const terminalPanel = document.getElementById('terminalPanel');
    const todoSidebarEl = document.getElementById('todoSidebar');
    const todoToggleBtn = document.getElementById('todoToggle');
    
    if (features.isProPlus) {
        // PRO+: All features + API credits
        console.log('⚡ AIDE Pro+ - Premium API & AI Terminal Access!');
        document.body.classList.add('pro-mode', 'proplus-mode');
        
        // Show model selector and usage counter
        const chatModelBar = document.getElementById('chatModelBar');
        if (chatModelBar) {
            chatModelBar.style.display = 'flex';
            updateUsageCounter();
        }
        
        // Pro+ gets both sidebars open
        if (todoSidebarEl) {
            todoSidebarEl.classList.remove('collapsed');
            document.getElementById('todoToggle')?.classList.add('active');
        }
    } else if (features.isPro) {
        // PRO: All features enabled
        console.log('⭐ AIDE Pro - All features unlocked!');
        document.body.classList.add('pro-mode');
        
        // Pro gets both sidebars open
        if (todoSidebarEl) {
            todoSidebarEl.classList.remove('collapsed');
            document.getElementById('todoToggle')?.classList.add('active');
        }
    } else {
        // FREE: Limited features
        console.log('🆓 AIDE Free - One sidebar at a time, upgrade for both!');
        document.body.classList.add('free-mode');
        
        // TODO sidebar is available in free mode, but only one sidebar at a time
        // Start with TODO open for better discoverability
        if (todoSidebarEl) {
            todoSidebarEl.classList.remove('collapsed');
        }
        if (todoToggleBtn) {
            todoToggleBtn.title = 'TODO List (Free: one sidebar at a time)';
        }
        
        // Disable drag-drop rearrange in free mode
        document.querySelectorAll('.quad-panel .quad-header > span').forEach(el => {
            el.setAttribute('draggable', false);
            el.style.cursor = 'default';
        });
    }
    
    // Show upgrade prompt
    if (!features.isPro) {
        showUpgradeHint();
    }
}

// Free panel switcher removed - terminal only in current layout

function showUpgradeHint() {
    // Don't show intrusive popups - user can find upgrade in menu
}

window.showUpgradeModal = async function() {
    const pricing = await ipcRenderer.invoke('get-pricing');
    
    const modal = document.createElement('div');
    modal.className = 'upgrade-modal';
    modal.innerHTML = `
        <div class="upgrade-content">
            <h2>⭐ Upgrade to AIDE Pro</h2>
            <p>Unlock all features:</p>
            <ul>
                <li>🎤 Voice Activation - "Aye Dee Dee"</li>
                <li>✅ TODO List sidebar</li>
                <li>🐛 Terminal AND Debug Console (both)</li>
                <li>↔️ Drag & drop panel rearrangement</li>
            </ul>
            <div class="pricing-options">
                <button class="price-btn" onclick="purchasePro('onetime')">
                    <span class="price">${pricing.oneTime.label}</span>
                    <span class="desc">One-time purchase</span>
                </button>
                <button class="price-btn" onclick="purchasePro('monthly')">
                    <span class="price">${pricing.monthly.label}</span>
                    <span class="desc">Monthly subscription</span>
                </button>
            </div>
            <div class="license-input">
                <input type="text" id="licenseKeyInput" placeholder="Enter license key...">
                <button onclick="activateLicense()">Activate</button>
            </div>
            <button class="close-modal" onclick="this.closest('.upgrade-modal').remove()">Maybe later</button>
        </div>
    `;
    document.body.appendChild(modal);
};

window.purchasePro = function(type) {
    // Open purchase page (you'd set this up with Gumroad, Lemonsqueezy, etc.)
    const urls = {
        onetime: 'https://your-store.com/aide-pro',
        monthly: 'https://your-store.com/aide-pro-monthly'
    };
    require('electron').shell.openExternal(urls[type] || urls.onetime);
};

window.activateLicense = async function() {
    const key = document.getElementById('licenseKeyInput').value.trim();
    if (!key) return;
    
    const result = await ipcRenderer.invoke('activate-license', key);
    
    if (result.success) {
        log('info', '✅ ' + result.message);
        alert(result.message);
        // Reload to apply pro features
        location.reload();
    } else {
        log('error', '❌ ' + result.message);
        alert(result.message);
    }
};

// === INIT ===
// Initialize directory permissions
loadDirectoryPermissions();
loadFavorites();
loadSavedTodoLists();

// Load initial directory
loadDirectory(currentDir);

// Initialize features (free vs pro)
initFeatures().then(() => {
    // Only init voice if Pro
    if (features.isPro) {
        setTimeout(initVoiceActivation, 1000);
        log('info', '🎤 Voice ready! Say "Dee Dee Commands" for full list.');
    } else {
        log('info', 'Dee Dee ready! (Voice commands are a Pro feature)');
    }
});

setTimeout(() => chatInput.focus(), 100);

function showMaxSidebarsWarning() {
    const warning = document.createElement("div");
    warning.id = "max-sidebars-warning";
    warning.innerHTML = `
        <div style="
            position: fixed;
            top: 20px;
            right: 20px;
            background: #f59e0b;
            color: white;
            padding: 12px 16px;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            z-index: 10001;
            font-size: 14px;
            animation: slideInRight 0.3s ease-out;
        ">
            <i class="fas fa-exclamation-triangle" style="margin-right: 8px;"></i>
            Upgrade to Pro+ for unlimited sidebars
        </div>
    `;

    document.body.appendChild(warning);

    setTimeout(() => {
        if (warning.parentNode) {
            warning.style.animation = "slideOutRight 0.3s ease-in";
            setTimeout(() => warning.remove(), 300);
        }
    }, 4000);
}
