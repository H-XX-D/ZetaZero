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

// Use nodeRequire saved before Monaco loaded, or fall back to require
const nodeRequire = window.nodeRequire || require;
const { ipcRenderer } = nodeRequire('electron');
const fs = nodeRequire('fs');
const path = nodeRequire('path');
const os = nodeRequire('os');
const { execSync, spawn } = nodeRequire('child_process');

// === EYE MODE CONTROL === (set up in index.html head script)

console.log('AIDE Renderer starting...');

// === DEE DEE PERSONALITY ===
// Personality module is loaded separately via script tag in index.html
// Access via window.deedeePersonality for separation of concerns
const deedeePersonality = window.deedeePersonality || {
    getPhrase: () => "I'm here to help! *beep*",
    enhanceMessage: (msg) => msg,
    getResponse: (ctx, msg) => msg || "I'm here to help! *beep*"
};

// === STATE ===
// closeBehavior is now managed by window.eyeMode in index.html inline script
let currentDir = os.homedir();
let openFiles = new Map(); // path -> content
let currentFile = null;
let features = { isPro: false }; // Will be loaded from main process
let freeModePanelChoice = 'terminal'; // 'terminal' or 'debug' - user picks one in free mode
let monacoEditor = null; // Monaco editor instance

// === GAMIFICATION TRACKING ===
let konamiSequence = [];
const KONAMI_CODE = ['ArrowUp', 'ArrowUp', 'ArrowDown', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'ArrowLeft', 'ArrowRight', 'b', 'a'];
let mouthClickCount = 0;
let mouthClickTimer = null;
let eyeClickCounts = { left: 0, right: 0 };
let eyeClickTimer = null;
let scrollLineBuffer = 0;

// === ELEMENTS ===
// NOTE: Eye/mouth click handlers are now in index.html inline script
// to avoid conflicts with renderer.js loading/execution issues
const sidebar = document.getElementById('sidebar');
const mainContainer = document.getElementById('mainContainer');
const fileTree = document.getElementById('fileTree');
const welcomeOverlay = document.getElementById('welcomeOverlay');
const mascotMouth = document.getElementById('mascotMouth');
const runBtn = document.getElementById('runBtn');
const interpreterSelect = document.getElementById('interpreterSelect');
const settingsInterpreterSelect = document.getElementById('settingsInterpreterSelect');
const customInterpreterInput = document.getElementById('customInterpreterInput');
const pathInfo = document.getElementById('pathInfo');

// === MONACO EDITOR ===
function initMonaco() {
    console.log('initMonaco called, checking Monaco...');
    console.log('window.monaco:', window.monaco);
    console.log('window.monacoLoaded:', window.monacoLoaded);
    
    if (!window.monaco) {
        console.log('Monaco not loaded yet, waiting...');
        return;
    }
    
    if (!window.monaco.editor) {
        console.error('Monaco loaded but editor API not available');
        console.log('Available properties:', Object.keys(window.monaco));
        return;
    }
    
    const container = document.getElementById('monacoEditor');
    if (!container) {
        console.error('Monaco container element not found');
        return;
    }
    
    // Check if editor already exists
    if (monacoEditor) {
        console.log('Monaco editor already initialized');
        return;
    }
    
    console.log('Creating Monaco editor instance...');
    
    // Detect theme
    const isDark = !document.body.classList.contains('light-mode');
    
    try {
        monacoEditor = window.monaco.editor.create(container, {
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
    
    // Make Monaco clickable - clicking anywhere on editor container focuses Monaco
    const editorContainer = document.querySelector('.editor-container');
    if (editorContainer) {
        editorContainer.addEventListener('click', (e) => {
            // If clicking on the editor area (not inside Monaco already)
            if (!e.target.closest('.monaco-editor')) {
                hideWelcome();
                if (monacoEditor) {
                    monacoEditor.focus();
                }
            }
        });
    }
    
    // Also handle clicks on the welcome overlay itself
    if (welcomeOverlay) {
        welcomeOverlay.style.cursor = 'text';
        welcomeOverlay.addEventListener('click', () => {
            hideWelcome();
            if (monacoEditor) {
                monacoEditor.focus();
            }
        });
    }
    
    console.log('Monaco Editor initialized successfully!', monacoEditor);
    } catch(error) {
        console.error('Error creating Monaco editor:', error);
    }
}

// Make initMonaco available globally for fallback
window.initMonaco = initMonaco;

// Wait for Monaco to load
function checkAndInitMonaco() {
    if (window.monacoLoaded && window.monaco && typeof window.monaco.editor !== 'undefined') {
        console.log('Monaco already loaded, initializing...');
        initMonaco();
        return true;
    }
    return false;
}

// Try immediate initialization if Monaco is already loaded
if (!checkAndInitMonaco()) {
    console.log('Waiting for Monaco to load...');
    
    // Listen for the monaco-ready event
    window.addEventListener('monaco-ready', function() {
        console.log('monaco-ready event received');
        if (checkAndInitMonaco()) {
            return;
        }
        // If still not ready, wait a bit more
        setTimeout(function() {
            if (checkAndInitMonaco()) {
                return;
            }
            console.warn('Monaco ready event fired but Monaco not fully initialized, retrying...');
            // Last resort: check every 100ms for up to 5 seconds
            var attempts = 0;
            var checkInterval = setInterval(function() {
                attempts++;
                if (checkAndInitMonaco() || attempts > 50) {
                    clearInterval(checkInterval);
                    if (attempts > 50) {
                        console.error('Monaco failed to initialize after 5 seconds');
                    }
                }
            }, 100);
        }, 200);
    }, { once: true });
    
    // Also check periodically in case event doesn't fire
    var pollAttempts = 0;
    var pollInterval = setInterval(function() {
        pollAttempts++;
        if (checkAndInitMonaco() || pollAttempts > 100) {
            clearInterval(pollInterval);
        }
    }, 100);
    
    // Also try after a delay in case event was missed
    setTimeout(() => {
        if (!monacoEditor && window.monaco) {
            console.log('Monaco found via timeout, initializing...');
            initMonaco();
        }
    }, 1000);
}

// === STATUS BAR ===
const statusPath = document.getElementById('statusPath');
const statusFile = document.getElementById('statusFile');
const statusLang = document.getElementById('statusLang');
const statusEncoding = document.getElementById('statusEncoding');
const statusCpu = document.getElementById('statusCpu');
const statusMemory = document.getElementById('statusMemory');
const statusDisk = document.getElementById('statusDisk');

function updateStatusBar() {
    // Update path
    if (statusPath) {
        const shortPath = currentDir.replace(os.homedir(), '~');
        statusPath.innerHTML = `<i class="fas fa-folder"></i> ${shortPath}`;
    }
    
    // Update file
    if (statusFile && currentFile) {
        statusFile.innerHTML = `<i class="fas fa-file"></i> ${path.basename(currentFile)}`;
    } else if (statusFile) {
        statusFile.innerHTML = `<i class="fas fa-file"></i> untitled`;
    }
    
    // Update language
    if (statusLang && currentFile) {
        const lang = getLanguageFromPath(currentFile);
        statusLang.innerHTML = `<i class="fas fa-code"></i> ${lang}`;
    }
}

function updateSystemStats() {
    // CPU usage (rough estimate using load average on Unix)
    if (statusCpu) {
        try {
            const cpus = os.cpus();
            let totalIdle = 0, totalTick = 0;
            cpus.forEach(cpu => {
                for (let type in cpu.times) {
                    totalTick += cpu.times[type];
                }
                totalIdle += cpu.times.idle;
            });
            const cpuPercent = Math.round(100 - (totalIdle / totalTick * 100));
            statusCpu.innerHTML = `<i class="fas fa-microchip"></i> CPU: ${cpuPercent}%`;
        } catch (e) {
            statusCpu.innerHTML = `<i class="fas fa-microchip"></i> CPU: --%`;
        }
    }
    
    // Memory usage
    if (statusMemory) {
        const totalMem = os.totalmem();
        const freeMem = os.freemem();
        const usedPercent = Math.round((1 - freeMem / totalMem) * 100);
        statusMemory.innerHTML = `<i class="fas fa-memory"></i> Mem: ${usedPercent}%`;
    }
    
    // Disk usage (approximate for home directory)
    if (statusDisk) {
        try {
            if (process.platform === 'darwin' || process.platform === 'linux') {
                const df = execSync('df -h ~ | tail -1').toString().trim();
                const parts = df.split(/\s+/);
                const usedPercent = parts[4] || '--%';
                statusDisk.innerHTML = `<i class="fas fa-hard-drive"></i> Disk: ${usedPercent}`;
            } else {
                statusDisk.innerHTML = `<i class="fas fa-hard-drive"></i> Disk: --%`;
            }
        } catch (e) {
            statusDisk.innerHTML = `<i class="fas fa-hard-drive"></i> Disk: --%`;
        }
    }
}

// Update status bar periodically
setInterval(updateSystemStats, 5000);
updateSystemStats();

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

// === RUN CONFIGURATION ===
let runConfig = JSON.parse(localStorage.getItem('aide-run-config') || JSON.stringify({
    interpreter: 'auto',
    customCommand: ''
}));

const interpreterTemplates = {
    node: 'node "{file}"',
    python: 'python3 "{file}"',
    deno: 'deno run "{file}"'
};

const autoInterpreterMap = {
    '.js': interpreterTemplates.node,
    '.ts': 'ts-node "{file}"',
    '.py': interpreterTemplates.python,
    '.rb': 'ruby "{file}"',
    '.sh': 'bash "{file}"',
    '.zsh': 'zsh "{file}"',
    '.ps1': 'powershell -ExecutionPolicy Bypass -File "{file}"'
};

function saveRunConfig() {
    localStorage.setItem('aide-run-config', JSON.stringify(runConfig));
}

function syncInterpreterSelects() {
    if (interpreterSelect) {
        interpreterSelect.value = runConfig.interpreter || 'auto';
    }
    if (settingsInterpreterSelect) {
        settingsInterpreterSelect.value = runConfig.interpreter || 'auto';
    }
    updateCustomInterpreterState();
}

function updateCustomInterpreterState() {
    if (customInterpreterInput) {
        const isCustom = (runConfig.interpreter === 'custom');
        customInterpreterInput.disabled = !isCustom;
        if (!isCustom) {
            customInterpreterInput.classList.remove('input-error');
        }
    }
}

// ============================================================
// TITLEBAR FACE - Close Behavior (Eyes) & Voice (Mouth)
// ============================================================
// NOTE: Eye/mouth click handlers are now defined in index.html
// inline script to avoid loading/execution conflicts.
// The inline script handles:
//   - window.cycleEyeMode() for eye clicks
//   - window.toggleMouth() for mouth clicks
//   - Sends IPC 'set-close-behavior' to main process

// Expose closeBehavior getter for other parts of the app
window.getCloseBehavior = () => window.eyeMode || 'single';

// === WINDOW CONTROLS ===
const closeBtn = document.getElementById('closeBtn');
const minimizeBtn = document.getElementById('minimizeBtn');

if (closeBtn) {
    closeBtn.addEventListener('click', () => ipcRenderer.send('close-ide'));
}
if (minimizeBtn) {
    minimizeBtn.addEventListener('click', () => ipcRenderer.send('minimize-ide'));
}

// === THEME ===
let currentTheme = localStorage.getItem('aide-theme') || 'dark';

function applyTheme(theme) {
    if (!theme) theme = currentTheme;
    currentTheme = theme;
    
    // Handle system theme
    if (theme === 'system') {
        const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
        theme = prefersDark ? 'dark' : 'light';
    }
    
    // Remove all theme classes
    document.body.classList.remove('light-mode', 'theme-midnight', 'theme-forest', 'theme-sunset');
    
    // Apply theme
    if (theme === 'light') {
        document.body.classList.add('light-mode');
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs');
        }
    } else if (theme === 'midnight') {
        document.body.classList.add('theme-midnight');
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs-dark');
        }
    } else if (theme === 'forest') {
        document.body.classList.add('theme-forest');
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs-dark');
        }
    } else if (theme === 'sunset') {
        document.body.classList.add('theme-sunset');
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs-dark');
        }
    } else {
        // Dark (default)
        if (monacoEditor && window.monaco) {
            monaco.editor.setTheme('vs-dark');
        }
    }
    
    localStorage.setItem('aide-theme', currentTheme);
}

// Listen for system theme changes
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', (e) => {
    if (currentTheme === 'system') {
        applyTheme('system');
    }
});

// Theme setting change handler
const themeSelect = document.getElementById('settingTheme');
if (themeSelect) {
    themeSelect.value = currentTheme;
    themeSelect.addEventListener('change', (e) => {
        applyTheme(e.target.value);
    });
}

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
    google: ['gemini-pro', 'gemini-pro-vision', 'gemini-1.5-pro', 'gemini-1.5-flash'],
    xai: ['grok-beta', 'grok-2'],
    openrouter: ['openai/gpt-4', 'anthropic/claude-3-opus', 'google/gemini-pro', 'x-ai/grok-beta', 'meta-llama/llama-3-70b'],
    ollama: ['llama3', 'mistral', 'codellama', 'phi3', 'gemma'],
    custom: ['default']
};

function openSettingsModal() {
    if (!settingsModal) {
        console.error('Settings modal not found');
        return;
    }
    
    // Load current settings
    if (aiProviderSelect) aiProviderSelect.value = apiSettings.provider;
    if (apiKeyInput) apiKeyInput.value = apiSettings.apiKey;
    if (customEndpointInput) customEndpointInput.value = apiSettings.customEndpoint;
    
    // Only call these if elements exist
    if (aiProviderSelect && aiModelSelect) {
        updateModelOptions();
    }
    updateSetupInstructions();
    
    if (aiModelSelect) aiModelSelect.value = apiSettings.model;
    updateTierDisplay();
    if (customEndpointSection) customEndpointSection.style.display = apiSettings.provider === 'custom' ? 'block' : 'none';
    // Sync quick dropdown
    if (aiProviderQuick) aiProviderQuick.value = apiSettings.provider;

    if (settingsInterpreterSelect) {
        settingsInterpreterSelect.value = runConfig.interpreter || 'auto';
    }
    if (customInterpreterInput) {
        customInterpreterInput.value = runConfig.customCommand || '';
        customInterpreterInput.disabled = (runConfig.interpreter !== 'custom');
    }
    if (pathInfo) {
        pathInfo.textContent = process.env.PATH || 'Unavailable';
    }
    
    settingsModal.classList.add('show');
}

function closeSettingsModal() {
    if (settingsModal) {
        settingsModal.classList.remove('show');
    }
}

window.closeSettingsModal = closeSettingsModal;

// Settings section switcher
function switchSettingsSection(section) {
    // Update nav buttons
    document.querySelectorAll('.settings-nav-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.section === section);
    });
    
    // Update content sections
    document.querySelectorAll('.settings-section-content').forEach(content => {
        content.classList.remove('active');
    });
    
    const sectionEl = document.getElementById('settings' + section.charAt(0).toUpperCase() + section.slice(1));
    if (sectionEl) {
        sectionEl.classList.add('active');
    }
    
    // Load section-specific data
    if (section === 'about') {
        loadAboutInfo();
    }
}
window.switchSettingsSection = switchSettingsSection;

// Load settings from localStorage
function loadSettings() {
    const settings = JSON.parse(localStorage.getItem('aide-settings') || '{}');
    
    // General
    if (document.getElementById('settingLaunchStartup')) {
        document.getElementById('settingLaunchStartup').checked = settings.launchStartup || false;
    }
    if (document.getElementById('settingStartMinimized')) {
        document.getElementById('settingStartMinimized').checked = settings.startMinimized || false;
    }
    if (document.getElementById('settingConfirmExit')) {
        document.getElementById('settingConfirmExit').checked = settings.confirmExit !== false;
    }
    if (document.getElementById('settingAutoSave')) {
        document.getElementById('settingAutoSave').value = settings.autoSave || 'off';
    }
    
    // Appearance
    if (document.getElementById('settingTheme')) {
        document.getElementById('settingTheme').value = settings.theme || 'dark';
    }
    if (document.getElementById('settingUIScale')) {
        document.getElementById('settingUIScale').value = settings.uiScale || '1';
    }
    if (document.getElementById('settingAnimations')) {
        document.getElementById('settingAnimations').checked = settings.animations !== false;
    }
    
    // Editor
    if (document.getElementById('settingEditorFont')) {
        document.getElementById('settingEditorFont').value = settings.editorFont || "'Fira Code', monospace";
    }
    if (document.getElementById('settingEditorFontSize')) {
        document.getElementById('settingEditorFontSize').value = settings.editorFontSize || 14;
    }
    if (document.getElementById('settingTabSize')) {
        document.getElementById('settingTabSize').value = settings.tabSize || '4';
    }
    if (document.getElementById('settingWordWrap')) {
        document.getElementById('settingWordWrap').value = settings.wordWrap || 'on';
    }
    if (document.getElementById('settingMinimap')) {
        document.getElementById('settingMinimap').checked = settings.minimap !== false;
    }
    
    // AI
    if (document.getElementById('settingTemperature')) {
        document.getElementById('settingTemperature').value = settings.temperature || 0.7;
        updateTemperatureDisplay();
    }
    if (document.getElementById('settingMaxTokens')) {
        document.getElementById('settingMaxTokens').value = settings.maxTokens || '2048';
    }
    if (document.getElementById('settingStreamResponses')) {
        document.getElementById('settingStreamResponses').checked = settings.streamResponses !== false;
    }
    
    // Dee Dee
    if (document.getElementById('settingVoiceActivation')) {
        document.getElementById('settingVoiceActivation').checked = settings.voiceActivation || false;
    }
    if (document.getElementById('settingSoundEffects')) {
        document.getElementById('settingSoundEffects').checked = settings.soundEffects !== false;
    }
    
    return settings;
}

// Save settings to localStorage
function saveSettings() {
    const settings = {
        // General
        launchStartup: document.getElementById('settingLaunchStartup')?.checked || false,
        startMinimized: document.getElementById('settingStartMinimized')?.checked || false,
        confirmExit: document.getElementById('settingConfirmExit')?.checked !== false,
        autoSave: document.getElementById('settingAutoSave')?.value || 'off',
        language: document.getElementById('settingLanguage')?.value || 'en',
        
        // Appearance
        theme: document.getElementById('settingTheme')?.value || 'dark',
        uiScale: document.getElementById('settingUIScale')?.value || '1',
        sidebarPosition: document.getElementById('settingSidebarPosition')?.value || 'left',
        showStatusBar: document.getElementById('settingShowStatusBar')?.checked !== false,
        animations: document.getElementById('settingAnimations')?.checked !== false,
        
        // Editor
        editorFont: document.getElementById('settingEditorFont')?.value || "'Fira Code', monospace",
        editorFontSize: parseInt(document.getElementById('settingEditorFontSize')?.value) || 14,
        tabSize: document.getElementById('settingTabSize')?.value || '4',
        insertSpaces: document.getElementById('settingInsertSpaces')?.checked !== false,
        wordWrap: document.getElementById('settingWordWrap')?.value || 'on',
        lineNumbers: document.getElementById('settingLineNumbers')?.checked !== false,
        minimap: document.getElementById('settingMinimap')?.checked !== false,
        bracketColors: document.getElementById('settingBracketColors')?.checked !== false,
        autoCloseBrackets: document.getElementById('settingAutoCloseBrackets')?.checked !== false,
        formatOnSave: document.getElementById('settingFormatOnSave')?.checked || false,
        
        // AI
        temperature: parseFloat(document.getElementById('settingTemperature')?.value) || 0.7,
        maxTokens: document.getElementById('settingMaxTokens')?.value || '2048',
        streamResponses: document.getElementById('settingStreamResponses')?.checked !== false,
        includeContext: document.getElementById('settingIncludeContext')?.checked !== false,
        
        // Terminal
        defaultShell: document.getElementById('settingDefaultShell')?.value || 'auto',
        terminalFontSize: parseInt(document.getElementById('settingTerminalFontSize')?.value) || 13,
        scrollback: document.getElementById('settingScrollback')?.value || '5000',
        copyOnSelect: document.getElementById('settingCopyOnSelect')?.checked || false,
        
        // Dee Dee
        voiceActivation: document.getElementById('settingVoiceActivation')?.checked || false,
        wakePhrase: document.getElementById('settingWakePhrase')?.value || 'hey-deedee',
        mascotPosition: document.getElementById('settingMascotPosition')?.value || 'right',
        mascotSize: document.getElementById('settingMascotSize')?.value || 'medium',
        showNotifications: document.getElementById('settingShowNotifications')?.checked !== false,
        soundEffects: document.getElementById('settingSoundEffects')?.checked !== false,
        
        // Privacy
        analytics: document.getElementById('settingAnalytics')?.checked || false,
        crashReports: document.getElementById('settingCrashReports')?.checked !== false,
    };
    
    localStorage.setItem('aide-settings', JSON.stringify(settings));
    applySettings(settings);
    log('info', 'Settings saved');
}

// Apply settings to the app
function applySettings(settings) {
    // Apply UI scale
    if (settings.uiScale) {
        document.body.style.fontSize = `${parseFloat(settings.uiScale) * 100}%`;
    }
    
    // Apply animations
    if (!settings.animations) {
        document.body.classList.add('no-animations');
    } else {
        document.body.classList.remove('no-animations');
    }
    
    // Apply status bar visibility
    const statusBar = document.getElementById('statusBar');
    if (statusBar) {
        statusBar.style.display = settings.showStatusBar !== false ? 'flex' : 'none';
    }
    
    // Apply to Monaco editor if available and fully initialized
    if (window.monacoEditor && typeof window.monacoEditor.updateOptions === 'function') {
        window.monacoEditor.updateOptions({
            fontSize: settings.editorFontSize || 14,
            fontFamily: settings.editorFont || "'Fira Code', monospace",
            tabSize: parseInt(settings.tabSize) || 4,
            insertSpaces: settings.insertSpaces !== false,
            wordWrap: settings.wordWrap || 'on',
            lineNumbers: settings.lineNumbers !== false ? 'on' : 'off',
            minimap: { enabled: settings.minimap !== false },
            bracketPairColorization: { enabled: settings.bracketColors !== false },
            autoClosingBrackets: settings.autoCloseBrackets !== false ? 'always' : 'never',
        });
    }
}

// Update temperature display
function updateTemperatureDisplay() {
    const slider = document.getElementById('settingTemperature');
    const display = document.getElementById('temperatureValue');
    if (slider && display) {
        display.textContent = slider.value;
    }
}

// Set accent color
function setAccentColor(color) {
    document.documentElement.style.setProperty('--accent', color);
    document.querySelectorAll('.color-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.color === color);
    });
    const settings = JSON.parse(localStorage.getItem('aide-settings') || '{}');
    settings.accentColor = color;
    localStorage.setItem('aide-settings', JSON.stringify(settings));
}
window.setAccentColor = setAccentColor;

// Browse for default path
function browseDefaultPath() {
    ipcRenderer.invoke('select-directory').then(result => {
        if (result && !result.cancelled) {
            document.getElementById('settingDefaultPath').value = result.path;
            saveSettings();
        }
    });
}
window.browseDefaultPath = browseDefaultPath;

// Load about info
function loadAboutInfo() {
    document.getElementById('appVersion').textContent = '1.0.0';
    document.getElementById('electronVersion').textContent = process.versions.electron || '--';
    document.getElementById('nodeVersion').textContent = process.versions.node || '--';
    document.getElementById('chromeVersion').textContent = process.versions.chrome || '--';
}

// Open external link
const { shell } = nodeRequire('electron');
function openExternal(url) {
    shell.openExternal(url);
}
window.openExternal = openExternal;

// === WARDROBE FUNCTIONS ===
let selectedWardrobeItem = null;
let equippedItems = JSON.parse(localStorage.getItem('aide-equipped-items') || '[]');
let unlockedItems = []; // Loaded from gamification
let allWardrobeItems = []; // Full wardrobe data from server

// Seeded random for consistent "hints" per session
const hintSeed = Math.floor(Math.random() * 1000);
function seededRandom(seed) {
    const x = Math.sin(seed) * 10000;
    return x - Math.floor(x);
}

// Load unlocked items from gamification
function loadUnlockedItems() {
    ipcRenderer.invoke('get-gamification-stats').then(stats => {
        if (stats && stats.unlockedAccessories) {
            unlockedItems = stats.unlockedAccessories;
            updateWardrobeUI();
        }
    }).catch(() => {});
}

// Update wardrobe UI based on unlocked items
function updateWardrobeUI() {
    document.querySelectorAll('.wardrobe-item').forEach(item => {
        const itemId = item.dataset.item;
        if (unlockedItems.includes(itemId)) {
            item.classList.add('unlocked');
        } else {
            item.classList.remove('unlocked');
        }
        
        if (equippedItems.includes(itemId)) {
            item.classList.add('equipped');
        }
    });
}

// Open wardrobe quick panel (from titlebar button)
function openWardrobeQuick() {
    // Open settings modal to the Dee Dee section
    openSettingsModal();
    setTimeout(() => {
        switchSettingsSection('deedee');
        renderWardrobeGrid();
    }, 100);
}
window.openWardrobeQuick = openWardrobeQuick;

// Render the full wardrobe grid dynamically
async function renderWardrobeGrid() {
    const grid = document.getElementById('wardrobeGrid');
    const stats = document.getElementById('wardrobeStats');
    if (!grid) return;
    
    try {
        // Get wardrobe data from main process
        const wardrobeData = await ipcRenderer.invoke('get-wardrobe');
        allWardrobeItems = wardrobeData || [];
        
        // Count stats
        const unlockedCount = allWardrobeItems.filter(i => i.unlocked).length;
        const totalCount = allWardrobeItems.length;
        
        // Update stats display
        if (stats) {
            stats.innerHTML = `<span style="color: var(--accent);">✓ ${unlockedCount}</span> / ${totalCount} drip unlocked`;
        }
        
        // Clear grid
        grid.innerHTML = '';
        
        // Determine which locked items to show as "hints" (~15%)
        const lockedItems = allWardrobeItems.filter(i => !i.unlocked);
        const hintCount = Math.max(3, Math.floor(lockedItems.length * 0.15));
        const hintIndices = new Set();
        
        // Use seeded random for consistent hints per session
        const seed = Math.floor(Date.now() / (1000 * 60 * 60)); // Changes every hour
        for (let i = 0; i < hintCount; i++) {
            const idx = Math.floor(seededRandom(seed + i * 7) * lockedItems.length);
            hintIndices.add(idx);
        }
        
        let lockedIdx = 0;
        
        // Render items
        allWardrobeItems.forEach((item, index) => {
            const div = document.createElement('div');
            div.className = 'wardrobe-item';
            div.dataset.item = item.id;
            
            if (item.unlocked) {
                // Unlocked item - show everything
                div.classList.add('unlocked');
                if (item.rarity) div.classList.add(item.rarity);
                if (equippedItems.includes(item.id)) div.classList.add('equipped');
                
                div.innerHTML = `
                    <div class="item-icon">${item.icon}</div>
                    <div class="item-name">${item.name}</div>
                    <div class="item-rarity ${item.rarity || 'common'}">${(item.rarity || 'common').toUpperCase()}</div>
                `;
                div.onclick = () => previewWardrobeItem(item.id);
            } else {
                // Locked item
                div.classList.add('locked');
                
                // Check if this is a hint item
                const isHint = hintIndices.has(lockedIdx);
                lockedIdx++;
                
                if (isHint && item.name && item.icon) {
                    // Show partial info as hint
                    div.classList.add('hint');
                    div.innerHTML = `
                        <div class="item-icon">${item.icon}</div>
                        <div class="item-name">${item.name}</div>
                        <div class="item-hint">${item.hint || '???'}</div>
                    `;
                } else {
                    // Fully hidden
                    const mysteryIcons = ['🔒', '❓', '🎁', '✨', '🔮'];
                    const randomIcon = mysteryIcons[Math.floor(seededRandom(seed + index) * mysteryIcons.length)];
                    div.innerHTML = `
                        <div class="item-icon">${randomIcon}</div>
                        <div class="item-name">???</div>
                        <div class="item-hint">Secret</div>
                    `;
                }
                
                div.onclick = () => {
                    // Show hint tooltip on click
                    if (isHint && item.hint) {
                        showNotification(`🔒 Hint: ${item.hint}`, 'info');
                    } else {
                        showNotification('🔒 Keep exploring to unlock!', 'info');
                    }
                };
            }
            
            grid.appendChild(div);
        });
        
    } catch (err) {
        console.error('Failed to load wardrobe:', err);
        if (stats) stats.textContent = 'Failed to load wardrobe';
    }
}

// Seeded random for consistent hints
function seededRandom(seed) {
    const x = Math.sin(seed) * 10000;
    return x - Math.floor(x);
}

// Initialize wardrobe when settings opens
window.initWardrobe = renderWardrobeGrid;
window.renderWardrobeGrid = renderWardrobeGrid;

function hideAllPreviewAccessories() {
    document.querySelectorAll('.mascot-preview [class^="accessory-"]').forEach(acc => {
        acc.classList.remove('show');
    });
}

// Preview a wardrobe item
function previewWardrobeItem(itemId) {
    // Remove selection from all items
    document.querySelectorAll('.wardrobe-item').forEach(item => {
        item.classList.remove('selected');
    });
    
    // Select clicked item
    const itemEl = document.querySelector(`.wardrobe-item[data-item="${itemId}"]`);
    if (itemEl) {
        itemEl.classList.add('selected');
    }
    
    selectedWardrobeItem = itemId;
    
    // Hide all accessories in preview (remove 'show' class)
    hideAllPreviewAccessories();
    
    // Show selected accessory
    const accessoryEl = document.querySelector(`.mascot-preview .accessory-${itemId}`);
    if (accessoryEl) {
        accessoryEl.classList.add('show');
    }
    
    // Update label
    const isUnlocked = unlockedItems.includes(itemId);
    const itemName = itemEl?.querySelector('.item-name')?.textContent || itemId;
    const label = document.getElementById('equippedLabel');
    if (label) {
        label.textContent = isUnlocked ? itemName : `🔒 ${itemName} (Locked)`;
    }
}
window.previewWardrobeItem = previewWardrobeItem;

// Reset the preview to base Dee Dee (no accessories)
function resetPreview() {
    // Clear selection from all items
    document.querySelectorAll('.wardrobe-item').forEach(item => {
        item.classList.remove('selected');
    });
    
    selectedWardrobeItem = null;
    hideAllPreviewAccessories();
    
    // Reset label
    const label = document.getElementById('equippedLabel');
    if (label) {
        label.textContent = 'Base Dee Dee';
    }
}
window.resetPreview = resetPreview;

// Equip selected item
function equipSelectedItem() {
    if (!selectedWardrobeItem) {
        alert('Please select an item first');
        return;
    }
    
    if (!unlockedItems.includes(selectedWardrobeItem)) {
        alert('This item is locked! Keep using AIDE to unlock it.');
        return;
    }
    
    // Toggle equipped state
    if (equippedItems.includes(selectedWardrobeItem)) {
        equippedItems = equippedItems.filter(i => i !== selectedWardrobeItem);
    } else {
        equippedItems.push(selectedWardrobeItem);
    }
    
    localStorage.setItem('aide-equipped-items', JSON.stringify(equippedItems));
    updateWardrobeUI();
    
    // Notify main process to update mascot
    ipcRenderer.send('equipped-accessories', equippedItems);
    
    log('info', `Equipped items: ${equippedItems.join(', ') || 'none'}`);
}
window.equipSelectedItem = equipSelectedItem;

// Reset wardrobe to default
function resetWardrobe() {
    if (confirm('Reset to default Dee Dee? This will unequip all items.')) {
        equippedItems = [];
        selectedWardrobeItem = null;
        localStorage.setItem('aide-equipped-items', JSON.stringify(equippedItems));
        
        hideAllPreviewAccessories();
        
        // Remove all selections
        document.querySelectorAll('.wardrobe-item').forEach(item => {
            item.classList.remove('selected', 'equipped');
        });
        
        document.getElementById('equippedLabel').textContent = 'Base Dee Dee';
        
        ipcRenderer.send('equipped-accessories', []);
        log('info', 'Wardrobe reset to default');
    }
}
window.resetWardrobe = resetWardrobe;

// Open 3D print generator
async function openPrintGenerator() {
    // Get current wardrobe data if available
    let currentUnlocked = unlockedItems || [];
    let currentEquipped = equippedItems || [];
    
    // If wardrobe data not loaded, fetch it
    if (currentUnlocked.length === 0 || allWardrobeItems.length === 0) {
        try {
            const wardrobeData = await ipcRenderer.invoke('get-wardrobe');
            if (wardrobeData && wardrobeData.length > 0) {
                allWardrobeItems = wardrobeData;
                currentUnlocked = wardrobeData.filter(i => i.unlocked).map(i => i.id);
            }
        } catch (e) {
            console.error('Failed to load wardrobe data:', e);
        }
    }
    
    const printSize = document.getElementById('printSize')?.value || 'small';
    const withAccessories = document.getElementById('printWithAccessories')?.checked !== false;
    const withBase = document.getElementById('printWithBase')?.checked !== false;
    
    // Generate unlock code for equipped items
    const code = btoa(JSON.stringify({
        unlocked: currentUnlocked,
        equipped: currentEquipped,
        size: printSize,
        base: withBase,
        withAccessories
    }));
    
    // Open the print generator via IPC
    // Send via IPC (ipcRenderer already imported at top)
    ipcRenderer.send('open-3d-print', { code });
}
window.openPrintGenerator = openPrintGenerator;

// Quick download STL
function downloadQuickSTL() {
    const printSize = document.getElementById('printSize')?.value || 'small';
    
    // Open print generator which handles STL export
    openPrintGenerator();
}
window.downloadQuickSTL = downloadQuickSTL;

// Initialize wardrobe on settings open
function initWardrobe() {
    loadUnlockedItems();
    updateWardrobeUI();
}

// Load wardrobe when settings modal opens
const settingsModalObserver = new MutationObserver((mutations) => {
    mutations.forEach((mutation) => {
        if (mutation.target.classList.contains('show')) {
            initWardrobe();
        }
    });
});

const settingsModalEl = document.getElementById('settingsModal');
if (settingsModalEl) {
    settingsModalObserver.observe(settingsModalEl, { attributes: true, attributeFilter: ['class'] });
}

// Check for updates
function checkForUpdates() {
    log('info', 'Checking for updates...');
    // TODO: Implement auto-update check
    alert('You are running the latest version!');
}
window.checkForUpdates = checkForUpdates;

// Show changelog
function showChangelog() {
    openExternal('https://aide.dev/changelog');
}
window.showChangelog = showChangelog;

// Show licenses
function showLicenses() {
    openExternal('https://aide.dev/licenses');
}
window.showLicenses = showLicenses;

// Export user data
function exportUserData() {
    const data = {
        settings: JSON.parse(localStorage.getItem('aide-settings') || '{}'),
        todos: JSON.parse(localStorage.getItem('aide-todos') || '[]'),
        favorites: JSON.parse(localStorage.getItem('aide-favorites') || '[]'),
        apiSettings: JSON.parse(localStorage.getItem('aide-api-settings') || '{}'),
    };
    
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'aide-data-export.json';
    a.click();
    URL.revokeObjectURL(url);
    log('info', 'Data exported');
}
window.exportUserData = exportUserData;

// Clear all data
function clearAllData() {
    if (confirm('Are you sure you want to clear all data? This cannot be undone.')) {
        localStorage.clear();
        log('warn', 'All data cleared');
        alert('All data has been cleared. The app will reload.');
        location.reload();
    }
}
window.clearAllData = clearAllData;

// Initialize settings listeners
function initSettingsListeners() {
    // Temperature slider
    const tempSlider = document.getElementById('settingTemperature');
    if (tempSlider) {
        tempSlider.addEventListener('input', updateTemperatureDisplay);
    }
    
    // Auto-save on change for all settings inputs
    document.querySelectorAll('.settings-section-content input, .settings-section-content select').forEach(el => {
        el.addEventListener('change', saveSettings);
    });
}

// Load settings on startup
setTimeout(() => {
    loadSettings();
    initSettingsListeners();
}, 100);

function updateTierDisplay() {
    if (tierBadge) {
        tierBadge.className = 'tier-badge ' + userTier;
    }
    
    if (userTier === 'proplus') {
        if (tierBadge) tierBadge.textContent = 'PRO+';
        if (tierDescription) tierDescription.textContent = 'API credits included - no key needed!';
        if (apiKeySection) apiKeySection.style.display = 'none';
    } else if (userTier === 'pro') {
        if (tierBadge) tierBadge.textContent = 'PRO';
        if (tierDescription) tierDescription.textContent = 'Bring your own API key to use AI features';
        if (apiKeySection) apiKeySection.style.display = 'block';
    } else {
        if (tierBadge) tierBadge.textContent = 'FREE';
        if (tierDescription) tierDescription.textContent = 'Bring your own API key to use AI features';
        if (apiKeySection) apiKeySection.style.display = 'block';
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
    if (!aiProviderSelect || !aiModelSelect) return;
    
    const provider = aiProviderSelect.value;
    const models = providerModels[provider] || ['default'];
    
    aiModelSelect.innerHTML = models.map(m => 
        `<option value="${m}">${m}</option>`
    ).join('');
    
    if (customEndpointSection) {
        customEndpointSection.style.display = provider === 'custom' ? 'block' : 'none';
    }
}

if (aiProviderSelect) {
    aiProviderSelect.addEventListener('change', () => {
        updateModelOptions();
        updateSetupInstructions();
        saveApiSettings();
        // Sync with quick dropdown
        if (aiProviderQuick) aiProviderQuick.value = aiProviderSelect.value;
    });
}

// Sync quick provider dropdown with settings
if (aiProviderQuick) {
    aiProviderQuick.addEventListener('change', () => {
        // Sync with settings dropdown
        if (aiProviderSelect) aiProviderSelect.value = aiProviderQuick.value;
        apiSettings.provider = aiProviderQuick.value;
        updateModelOptions();
        // Show/hide custom endpoint input if needed
        if (customEndpointSection) {
            customEndpointSection.style.display = aiProviderQuick.value === 'custom' ? 'block' : 'none';
        }
        saveApiSettings();
    });
}

// Update setup instructions based on selected provider
function updateSetupInstructions() {
    const provider = aiProviderSelect?.value || 'openai';
    
    // Hide all instruction blocks
    document.querySelectorAll('.instruction-block').forEach(el => {
        if (el) el.style.display = 'none';
    });
    
    // Show relevant instructions
    switch (provider) {
        case 'openai':
            const openaiInst = document.getElementById('instructOpenAI');
            if (openaiInst) openaiInst.style.display = 'block';
            break;
        case 'anthropic':
            const anthropicInst = document.getElementById('instructAnthropic');
            if (anthropicInst) anthropicInst.style.display = 'block';
            break;
        case 'ollama':
            const ollamaInst = document.getElementById('instructOllama');
            if (ollamaInst) ollamaInst.style.display = 'block';
            break;
        case 'google':
            const googleInst = document.getElementById('instructGoogle');
            if (googleInst) googleInst.style.display = 'block';
            break;
        case 'xai':
            const xaiInst = document.getElementById('instructXAI');
            if (xaiInst) xaiInst.style.display = 'block';
            break;
        case 'openrouter':
            const openrouterInst = document.getElementById('instructOpenRouter');
            if (openrouterInst) openrouterInst.style.display = 'block';
            break;
        case 'custom':
            const lmstudioInst = document.getElementById('instructLMStudio');
            if (lmstudioInst) lmstudioInst.style.display = 'block';
            break;
    }
}

if (apiKeyInput) apiKeyInput.addEventListener('change', saveApiSettings);
if (aiModelSelect) aiModelSelect.addEventListener('change', saveApiSettings);
if (customEndpointInput) customEndpointInput.addEventListener('change', saveApiSettings);

function saveApiSettings() {
    apiSettings = {
        provider: aiProviderSelect?.value || 'openai',
        apiKey: apiKeyInput?.value || '',
        model: aiModelSelect?.value || 'gpt-4',
        customEndpoint: customEndpointInput?.value || ''
    };
    localStorage.setItem('aide-api-settings', JSON.stringify(apiSettings));
    // Sync quick dropdown
    if (aiProviderQuick) aiProviderQuick.value = apiSettings.provider;
    log('info', 'API settings saved');
}

window.toggleKeyVisibility = function() {
    const input = apiKeyInput;
    const icon = document.getElementById('keyEyeIcon');
    if (!input || !icon) return;
    
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
    if (!apiStatus) return;
    
    apiStatus.className = 'api-status';
    apiStatus.style.display = 'none';
    
    const key = apiKeyInput?.value?.trim() || '';
    const provider = aiProviderSelect?.value || 'openai';
    
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
            case 'google':
                testUrl = `https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=${key}`;
                headers = { 'Content-Type': 'application/json' };
                body = JSON.stringify({ contents: [{ parts: [{ text: 'hi' }] }] });
                break;
            case 'xai':
                testUrl = 'https://api.x.ai/v1/models';
                headers = { 'Authorization': `Bearer ${key}` };
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

// === QUICK API KEY SETUP (in AI panel) ===
const apiSetupBar = document.getElementById('apiSetupBar');
const apiKeyQuick = document.getElementById('apiKeyQuick');
const aiProviderQuick = document.getElementById('aiProviderQuick');

function updateApiSetupBarVisibility() {
    if (apiSetupBar) {
        // Hide if Pro+ (has included credits) or if API key is already set
        if (userTier === 'proplus' || (apiSettings.apiKey && apiSettings.apiKey.trim())) {
            apiSetupBar.classList.add('hidden');
        } else {
            apiSetupBar.classList.remove('hidden');
        }
    }
}

window.toggleQuickKeyVisibility = function() {
    const input = apiKeyQuick;
    const icon = document.getElementById('quickKeyEye');
    if (!input || !icon) return;
    
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

window.saveQuickApiKey = function() {
    const key = apiKeyQuick?.value?.trim();
    const provider = aiProviderQuick?.value || 'openai';
    
    if (!key && provider !== 'ollama') {
        log('warn', 'Please enter an API key');
        return;
    }
    
    // Update settings
    apiSettings.apiKey = key;
    apiSettings.provider = provider;
    
    // Also update the modal inputs
    if (apiKeyInput) apiKeyInput.value = key;
    if (aiProviderSelect) aiProviderSelect.value = provider;
    
    // Update model options based on provider
    const models = providerModels[provider] || ['default'];
    apiSettings.model = models[0];
    if (aiModelSelect) {
        aiModelSelect.innerHTML = models.map(m => `<option value="${m}">${m}</option>`).join('');
        aiModelSelect.value = models[0];
    }
    
    // Save to localStorage
    localStorage.setItem('aide-api-settings', JSON.stringify(apiSettings));
    
    // Hide the setup bar
    updateApiSetupBarVisibility();
    
    log('info', `API key saved for ${provider}`);
    
    // Show confirmation in chat
        const successPhrase = deedeePersonality.getPhrase('success');
        addMessage('assistant', `${successPhrase}<br><br>✅ <strong>API key saved!</strong> Using ${provider.charAt(0).toUpperCase() + provider.slice(1)}. You're ready to chat!`);
        
        // Speak success phrase if TTS is enabled
        if (window.deedeeTTS && window.deedeeTTS.isSupported()) {
            window.deedeeTTS.speak(successPhrase, { volume: 0.7 }).catch(() => {});
        }
};

// Initialize quick API setup bar
if (apiKeyQuick && apiSettings.apiKey) {
    apiKeyQuick.value = apiSettings.apiKey;
}
if (aiProviderQuick && apiSettings.provider) {
    aiProviderQuick.value = apiSettings.provider;
}
updateApiSetupBarVisibility();

if (settingsBtn) {
    settingsBtn.addEventListener('click', openSettingsModal);
}

// Close modal on overlay click
if (settingsModal) {
    settingsModal.addEventListener('click', (e) => {
        if (e.target === settingsModal) {
            closeSettingsModal();
        }
    });
}

// === SHARE FUNCTIONS ===
const shareBtn = document.getElementById('shareBtn');
const importShareBtn = document.getElementById('importShareBtn');
const shareDeeDeeBtn = document.getElementById('shareDeeDeeBtn');

if (shareBtn) {
    shareBtn.addEventListener('click', shareProject);
}

if (importShareBtn) {
    importShareBtn.addEventListener('click', importSharedProject);
}

if (shareDeeDeeBtn) {
    shareDeeDeeBtn.addEventListener('click', shareDeeDee);
}

function shareDeeDee() {
    const appUrl = 'https://aide.dev'; // Update with actual URL
    const shareText = "Check out Dee Dee - the AI coding assistant that lives on your desktop! 🤖✨";
    const hashtags = 'DeeDee,AIDE,CodingAssistant,AI';
    
    // Create share modal
    const modal = document.createElement('div');
    modal.className = 'modal-overlay show';
    modal.id = 'shareDeeDeeModal';
    modal.innerHTML = `
        <div class="modal" style="max-width: 450px;">
            <div class="modal-header">
                <h3><i class="fas fa-heart" style="color: #ec4899;"></i> Share Dee Dee</h3>
                <button class="modal-close" onclick="document.getElementById('shareDeeDeeModal').remove()"><i class="fas fa-times"></i></button>
            </div>
            <div class="modal-body">
                <p style="margin-bottom: 20px; color: var(--text-muted); text-align: center;">
                    Help spread the word about Dee Dee! Share with your friends and unlock exclusive drip! 🎁
                </p>
                
                <div class="share-buttons" style="display: flex; flex-direction: column; gap: 10px;">
                    <button class="share-option-btn" onclick="shareViaTwitter()">
                        <i class="fab fa-twitter" style="color: #1da1f2;"></i>
                        Share on Twitter/X
                    </button>
                    <button class="share-option-btn" onclick="shareViaLinkedIn()">
                        <i class="fab fa-linkedin" style="color: #0077b5;"></i>
                        Share on LinkedIn
                    </button>
                    <button class="share-option-btn" onclick="shareViaReddit()">
                        <i class="fab fa-reddit" style="color: #ff4500;"></i>
                        Share on Reddit
                    </button>
                    <button class="share-option-btn" onclick="shareViaEmail()">
                        <i class="fas fa-envelope" style="color: #ea4335;"></i>
                        Share via Email
                    </button>
                    <button class="share-option-btn" onclick="copyShareLink()">
                        <i class="fas fa-link" style="color: #10b981;"></i>
                        Copy Link
                    </button>
                </div>
                
                <div class="share-stats" style="margin-top: 20px; padding-top: 15px; border-top: 1px solid var(--border); text-align: center;">
                    <p style="font-size: 12px; color: var(--text-muted);">
                        <i class="fas fa-gift"></i> Share to unlock exclusive Dee Dee drip!
                    </p>
                </div>
            </div>
        </div>
    `;
    document.body.appendChild(modal);
}

window.shareViaTwitter = function() {
    const appUrl = 'https://aide.dev';
    const text = encodeURIComponent("Check out Dee Dee - the AI coding assistant that lives on your desktop! 🤖✨ #DeeDee #AIDE");
    const url = `https://twitter.com/intent/tweet?text=${text}&url=${encodeURIComponent(appUrl)}`;
    shell.openExternal(url);
    trackShare('twitter');
};

window.shareViaLinkedIn = function() {
    const appUrl = 'https://aide.dev';
    const url = `https://www.linkedin.com/sharing/share-offsite/?url=${encodeURIComponent(appUrl)}`;
    shell.openExternal(url);
    trackShare('linkedin');
};

window.shareViaReddit = function() {
    const appUrl = 'https://aide.dev';
    const title = encodeURIComponent("Dee Dee - AI coding assistant that lives on your desktop");
    const url = `https://www.reddit.com/submit?url=${encodeURIComponent(appUrl)}&title=${title}`;
    shell.openExternal(url);
    trackShare('reddit');
};

window.shareViaEmail = function() {
    const appUrl = 'https://aide.dev';
    const subject = encodeURIComponent("Check out Dee Dee - AI Coding Assistant");
    const body = encodeURIComponent(`Hey!\n\nI've been using this awesome AI coding assistant called Dee Dee. It lives on your desktop and helps you code!\n\nCheck it out: ${appUrl}\n\n🤖✨`);
    const url = `mailto:?subject=${subject}&body=${body}`;
    shell.openExternal(url);
    trackShare('email');
};

window.copyShareLink = function() {
    const appUrl = 'https://aide.dev';
    navigator.clipboard.writeText(appUrl).then(() => {
        // Show copied feedback
        const btn = event.target.closest('button');
        const originalText = btn.innerHTML;
        btn.innerHTML = '<i class="fas fa-check" style="color: #10b981;"></i> Copied!';
        setTimeout(() => {
            btn.innerHTML = originalText;
        }, 2000);
        trackShare('copy');
    });
};

function trackShare(platform) {
    // Track share for gamification
    ipcRenderer.send('track-share');
    log('info', `Shared Dee Dee via ${platform}`);
    
    // Close modal after short delay
    setTimeout(() => {
        document.getElementById('shareDeeDeeModal')?.remove();
    }, 500);
}

async function shareProject() {
    // Get current project root
    const currentPath = document.getElementById('currentPath')?.textContent || '';
    if (!currentPath || currentPath === '~') {
        log('warn', 'No project open to share');
        alert('Please open a project folder first');
        return;
    }
    
    const projectPath = currentPath.replace('~', os.homedir());
    
    // Create share modal
    const modal = document.createElement('div');
    modal.className = 'modal-overlay show';
    modal.id = 'shareModal';
    modal.innerHTML = `
        <div class="modal" style="max-width: 500px;">
            <div class="modal-header">
                <h3><i class="fas fa-share-alt"></i> Share Project</h3>
                <button class="modal-close" onclick="document.getElementById('shareModal').remove()"><i class="fas fa-times"></i></button>
            </div>
            <div class="modal-body">
                <p style="margin-bottom: 15px; color: var(--text-muted);">Share your project as a compressed archive that others can import.</p>
                
                <div class="settings-section">
                    <label>Project: <strong>${path.basename(projectPath)}</strong></label>
                </div>
                
                <div class="settings-section">
                    <label>Include:</label>
                    <div style="display: flex; flex-direction: column; gap: 8px; margin-top: 8px;">
                        <label style="display: flex; align-items: center; gap: 8px; cursor: pointer;">
                            <input type="checkbox" id="shareIncludeGit" checked> .git folder (version history)
                        </label>
                        <label style="display: flex; align-items: center; gap: 8px; cursor: pointer;">
                            <input type="checkbox" id="shareIncludeNodeModules"> node_modules (large!)
                        </label>
                        <label style="display: flex; align-items: center; gap: 8px; cursor: pointer;">
                            <input type="checkbox" id="shareIncludeEnv"> .env files (secrets!)
                        </label>
                    </div>
                </div>
                
                <div class="settings-section" style="margin-top: 20px;">
                    <button class="primary-btn" onclick="executeShare('${projectPath.replace(/'/g, "\\'")}')">
                        <i class="fas fa-download"></i> Export as .zip
                    </button>
                </div>
            </div>
        </div>
    `;
    document.body.appendChild(modal);
    
    // Track share for gamification
    ipcRenderer.send('share-app');
}

window.executeShare = async function(projectPath) {
    const includeGit = document.getElementById('shareIncludeGit')?.checked ?? true;
    const includeNodeModules = document.getElementById('shareIncludeNodeModules')?.checked ?? false;
    const includeEnv = document.getElementById('shareIncludeEnv')?.checked ?? false;
    
    try {
        const result = await ipcRenderer.invoke('export-project', {
            projectPath,
            includeGit,
            includeNodeModules,
            includeEnv
        });
        
        if (result.success) {
            log('info', `Project exported to: ${result.outputPath}`);
            document.getElementById('shareModal')?.remove();
            
            // Show success message
            alert(`Project exported successfully!\n\nSaved to: ${result.outputPath}`);
        } else {
            log('error', `Export failed: ${result.error}`);
            alert(`Export failed: ${result.error}`);
        }
    } catch (err) {
        log('error', `Export error: ${err.message}`);
        alert(`Export error: ${err.message}`);
    }
};

async function importSharedProject() {
    try {
        const result = await ipcRenderer.invoke('import-project');
        
        if (result.success) {
            log('info', `Project imported to: ${result.projectPath}`);
            
            // Navigate to the imported project
            loadDirectory(result.projectPath);
            
            alert(`Project imported successfully!\n\nOpened: ${result.projectPath}`);
        } else if (result.cancelled) {
            // User cancelled, do nothing
        } else {
            log('error', `Import failed: ${result.error}`);
            alert(`Import failed: ${result.error}`);
        }
    } catch (err) {
        log('error', `Import error: ${err.message}`);
        alert(`Import error: ${err.message}`);
    }
}

// === SIDEBAR CONTROLS ===
document.getElementById('sidebarToggle').addEventListener('click', () => {
    sidebar.classList.toggle('collapsed');
    document.getElementById('sidebarToggle').classList.toggle('active', !sidebar.classList.contains('collapsed'));
});

const sidebarSide = document.getElementById('sidebarSide');
if (sidebarSide) {
    sidebarSide.addEventListener('click', () => {
        mainContainer.classList.toggle('sidebar-right');
    });
}

if (runBtn) {
    runBtn.addEventListener('click', runCurrentFile);
}

if (interpreterSelect) {
    interpreterSelect.value = runConfig.interpreter || 'auto';
    interpreterSelect.addEventListener('change', (e) => {
        runConfig.interpreter = e.target.value;
        saveRunConfig();
        syncInterpreterSelects();
        if (runConfig.interpreter === 'custom') {
            openSettingsModal();
        }
    });
}

if (settingsInterpreterSelect) {
    settingsInterpreterSelect.addEventListener('change', (e) => {
        runConfig.interpreter = e.target.value;
        saveRunConfig();
        syncInterpreterSelects();
    });
}

if (customInterpreterInput) {
    customInterpreterInput.addEventListener('input', (e) => {
        runConfig.customCommand = e.target.value;
        saveRunConfig();
    });
}

syncInterpreterSelects();

// NOTE: mascotMouth click handler is already set up above (around line 391)
// Do not add duplicate handlers here

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

// Toggle to todo tab in sidebar
window.toggleTodoSidebar = function() {
    switchSidebarTab('todo');
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
        // Track git usage for gamification
        ipcRenderer.send('record-feature-usage', 'git');
    }
    if (tabName === 'files' && content.classList.contains('active')) {
        // Track explorer usage for gamification
        ipcRenderer.send('record-feature-usage', 'explorer');
    }
    if (tabName === 'todo' && content.classList.contains('active')) {
        // Refresh todo display when switching to todo tab
        renderTodos();
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

// AI Sidebar collapse/expand
const aiSidebar = document.getElementById('aiSidebar');

function toggleAiSidebarCollapse() {
    if (aiSidebar) {
        aiSidebar.classList.toggle('collapsed');
    }
}
window.toggleAiSidebarCollapse = toggleAiSidebarCollapse;

function expandAiSidebar() {
    if (aiSidebar) {
        aiSidebar.classList.remove('collapsed');
    }
}
window.expandAiSidebar = expandAiSidebar;

function clearChat() {
    const chatMessages = document.getElementById('chatMessages');
    if (chatMessages) {
        const greeting = deedeePersonality.getPhrase('greetings');
        chatMessages.innerHTML = `
            <div class="message assistant">
                <div class="avatar"><i class="fas fa-robot"></i></div>
                <div class="content"><strong>${greeting}</strong><br><br>I'm Dee Dee, your AI coding assistant! Ask me anything or paste code. Say "Aye Dee Dee" to summon me anytime! 🎤</div>
            </div>
        `;
        
        // Speak greeting if TTS is enabled (after user interaction)
        // Initialize TTS on first user interaction
        const initTTSOnInteraction = () => {
            if (window.deedeeTTS && window.deedeeTTS.isSupported() && window.deedeeTTS.isEnabled()) {
                window.deedeeTTS.init().then(() => {
                    console.log('TTS initialized, speaking greeting');
                    window.deedeeTTS.speak(greeting, { volume: 0.8 }).catch((err) => {
                        console.log('TTS speak error:', err);
                    });
                }).catch((err) => {
                    console.log('TTS init error:', err);
                });
            }
        };
        
        // Try to initialize on any user interaction
        document.addEventListener('click', initTTSOnInteraction, { once: true });
        document.addEventListener('keydown', initTTSOnInteraction, { once: true });
    }
}
window.clearChat = clearChat;


// === GIT PANEL ===
function isGitRepo() {
    try {
        execSync('git rev-parse --is-inside-work-tree', { cwd: currentDir, encoding: 'utf8', stdio: 'pipe' });
        return true;
    } catch (e) {
        console.log('Not a git repo or git error:', e.message);
        return false;
    }
}

function gitRefresh() {
    console.log('gitRefresh called, currentDir:', currentDir);
    
    if (!isGitRepo()) {
        console.log('Not a git repo');
        document.getElementById('gitBranch').innerHTML = '<i class="fas fa-code-branch"></i> Not a Git repo';
        document.getElementById('stagedFiles').innerHTML = '<div class="git-empty">Not a Git repository<br><small>Use the Files tab to browse all files</small></div>';
        document.getElementById('changedFiles').innerHTML = '';
        document.getElementById('stagedCount').textContent = '0';
        document.getElementById('changesCount').textContent = '0';
        return;
    }
    
    try {
        console.log('Getting git branch...');
        // Get current branch
        const branch = execSync('git branch --show-current', { cwd: currentDir, encoding: 'utf8', stdio: 'pipe' }).trim();
        document.getElementById('gitBranch').innerHTML = `<i class="fas fa-code-branch"></i> ${branch || 'HEAD'}`;
        
        // Get status
        console.log('Getting git status...');
        const status = execSync('git status --porcelain', { cwd: currentDir, encoding: 'utf8', stdio: 'pipe' });
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
    console.log('openFile called with:', filePath);
    console.log('monacoEditor exists:', !!monacoEditor);
    console.log('window.monaco:', !!window.monaco);
    console.log('window.monaco.editor:', window.monaco ? !!window.monaco.editor : false);
    
    try {
        const content = fs.readFileSync(filePath, 'utf8');
        console.log('File content length:', content.length);
        
        openFiles.set(filePath, content);
        currentFile = filePath;
        
        // Use Monaco if available
        if (monacoEditor) {
            console.log('Setting Monaco content...');
            const language = getLanguageFromPath(filePath);
            console.log('Language:', language);
            
            // Create a new model instead of just setting value
            if (window.monaco && window.monaco.editor) {
                const model = window.monaco.editor.createModel(content, language);
                monacoEditor.setModel(model);
            } else {
                console.error('window.monaco.editor not available for creating model');
            }
            
            hideWelcome();
            
            // Focus the editor so user can start typing immediately
            setTimeout(() => {
                monacoEditor.focus();
            }, 50);
            
            console.log('Monaco content set successfully');
        } else {
            console.warn('Monaco editor not available!');
            console.warn('window.monaco:', window.monaco);
            console.warn('window.monacoLoaded:', window.monacoLoaded);
            if (window.monaco) {
                console.warn('Monaco properties:', Object.keys(window.monaco));
            }
        }
        
        document.getElementById('editorFile').textContent = path.basename(filePath);
        
        // Update tabs
        updateTabs();
        
        // Update status bar
        updateStatusBar();
        
        // Highlight in tree
        document.querySelectorAll('.file-item').forEach(el => {
            el.classList.toggle('active', el.dataset.path === filePath);
        });
        
        log('info', `Opened: ${path.basename(filePath)}`);
    } catch (e) {
        console.error('openFile error:', e);
        log('error', `Cannot open file: ${e.message}`);
    }
}
window.openFile = openFile;

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
        tab.dataset.filepath = filePath;
        tab.draggable = true;
        tab.innerHTML = `
            ${isDirty ? '● ' : ''}${path.basename(filePath)}
            <span class="close-tab" onclick="closeTab('${filePath.replace(/'/g, "\\'")}'); event.stopPropagation();">
                <i class="fas fa-times"></i>
            </span>
        `;
        tab.onclick = () => switchToTab(filePath);
        
        // Drag start - store the file path
        tab.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('text/plain', filePath);
            e.dataTransfer.effectAllowed = 'move';
            tab.classList.add('dragging');
            
            // Create a drag image
            const dragImg = document.createElement('div');
            dragImg.className = 'tab-drag-preview';
            dragImg.textContent = path.basename(filePath);
            dragImg.style.cssText = 'position: absolute; top: -1000px; background: var(--accent); color: white; padding: 6px 12px; border-radius: 4px; font-size: 12px;';
            document.body.appendChild(dragImg);
            e.dataTransfer.setDragImage(dragImg, 0, 0);
            setTimeout(() => dragImg.remove(), 0);
        });
        
        tab.addEventListener('dragend', (e) => {
            tab.classList.remove('dragging');
            
            // Check if dropped outside the tab bar (pop out)
            const tabBarRect = tabBar.getBoundingClientRect();
            const dropX = e.clientX;
            const dropY = e.clientY;
            
            // If dropped outside tab bar bounds, pop out the editor
            if (dropY > tabBarRect.bottom + 50 || dropY < tabBarRect.top - 50 ||
                dropX < tabBarRect.left - 50 || dropX > tabBarRect.right + 50) {
                popoutTabAsWindow(filePath, e.screenX, e.screenY);
            }
        });
        
        // Allow reordering within tab bar
        tab.addEventListener('dragover', (e) => {
            e.preventDefault();
            const draggingTab = tabBar.querySelector('.dragging');
            if (draggingTab && draggingTab !== tab) {
                const rect = tab.getBoundingClientRect();
                const midpoint = rect.left + rect.width / 2;
                if (e.clientX < midpoint) {
                    tabBar.insertBefore(draggingTab, tab);
                } else {
                    tabBar.insertBefore(draggingTab, tab.nextSibling);
                }
            }
        });
        
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

// Pop out a specific tab as a new window
function popoutTabAsWindow(filePath, screenX, screenY) {
    const content = openFiles.get(filePath) || '';
    const language = getLanguageFromPath(filePath);
    
    ipcRenderer.send('popout-editor', {
        filePath: filePath,
        content: content,
        language: language,
        x: screenX - 400, // Center the window on drop point
        y: screenY - 50
    });
    
    showNotification(`Popped out: ${path.basename(filePath)}`, 'info');
}

function switchToTab(filePath) {
    currentFile = filePath;
    const content = openFiles.get(filePath) || '';
    updateStatusBar();
    if (monacoEditor) {
        const language = getLanguageFromPath(filePath);
        monaco.editor.setModelLanguage(monacoEditor.getModel(), language);
        monacoEditor.setValue(content);
    }
    document.getElementById('editorFile').textContent = path.basename(filePath);
    updateTabs();
    hideWelcome();
    
    // Track editor usage for gamification
    ipcRenderer.send('record-feature-usage', 'editor');
}
window.switchToTab = switchToTab;

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

// Pop out current editor to its own always-on-top window
window.popoutEditor = function() {
    if (!currentFile && !monacoEditor) return;
    
    const content = monacoEditor ? monacoEditor.getValue() : '';
    const language = currentFile ? getLanguageFromPath(currentFile) : 'plaintext';
    
    ipcRenderer.send('popout-editor', {
        filePath: currentFile,
        content: content,
        language: language
    });
};

// Split view state
let isSplitView = false;
let splitEditor = null;

// Toggle split view in main editor
window.toggleSplitView = function() {
    const container = document.getElementById('monacoEditor');
    
    if (isSplitView) {
        // Remove split
        const splitPane = document.getElementById('splitEditorPane');
        if (splitPane) splitPane.remove();
        container.style.display = 'block';
        if (splitEditor) {
            splitEditor.dispose();
            splitEditor = null;
        }
        isSplitView = false;
    } else {
        // Create split view
        const content = monacoEditor ? monacoEditor.getValue() : '';
        const language = currentFile ? getLanguageFromPath(currentFile) : 'plaintext';
        
        // Wrap existing editor
        container.style.display = 'flex';
        container.style.flexDirection = 'row';
        
        // Create second editor pane
        const splitPane = document.createElement('div');
        splitPane.id = 'splitEditorPane';
        splitPane.style.cssText = 'flex: 1; border-left: 2px solid #3c3c3c;';
        container.appendChild(splitPane);
        
        // Create second Monaco editor (Monaco already loaded from CDN)
        if (window.monaco && window.monaco.editor) {
            splitEditor = window.monaco.editor.create(splitPane, {
                value: content,
                language: language,
                theme: document.body.classList.contains('light-mode') ? 'vs' : 'vs-dark',
                minimap: { enabled: false },
                fontSize: 13,
                automaticLayout: true
            });
        } else {
            console.error('Monaco editor not available for split view');
        }
        
        isSplitView = true;
    }
};

// === EDITOR MENU FUNCTIONS ===
window.toggleEditorMenu = function() {
    const dropdown = document.getElementById('editorDropdown');
    dropdown.classList.toggle('show');
    
    // Close when clicking outside
    if (dropdown.classList.contains('show')) {
        setTimeout(() => {
            document.addEventListener('click', closeEditorMenuOnClickOutside);
        }, 10);
    }
};

window.closeEditorMenu = function() {
    const dropdown = document.getElementById('editorDropdown');
    dropdown.classList.remove('show');
    document.removeEventListener('click', closeEditorMenuOnClickOutside);
};

function closeEditorMenuOnClickOutside(e) {
    const menu = document.querySelector('.editor-more-menu');
    if (!menu.contains(e.target)) {
        closeEditorMenu();
    }
}

window.formatCode = function() {
    if (!monacoEditor) return;
    monacoEditor.getAction('editor.action.formatDocument').run();
    log('info', 'Code formatted');
};

window.toggleWordWrap = function() {
    if (!monacoEditor) return;
    const current = monacoEditor.getOption(monaco.editor.EditorOption.wordWrap);
    monacoEditor.updateOptions({ wordWrap: current === 'on' ? 'off' : 'on' });
    log('info', `Word wrap: ${current === 'on' ? 'OFF' : 'ON'}`);
};

window.toggleMinimap = function() {
    if (!monacoEditor) return;
    const current = monacoEditor.getOption(monaco.editor.EditorOption.minimap);
    monacoEditor.updateOptions({ minimap: { enabled: !current.enabled } });
    log('info', `Minimap: ${current.enabled ? 'OFF' : 'ON'}`);
};

window.goToLine = function() {
    if (!monacoEditor) return;
    monacoEditor.getAction('editor.action.gotoLine').run();
};

window.findReplace = function() {
    if (!monacoEditor) return;
    monacoEditor.getAction('editor.action.startFindReplaceAction').run();
};

window.closeAllTabs = function() {
    if (!confirm('Close all tabs? Unsaved changes will be lost.')) return;
    openFiles.clear();
    dirtyFiles.clear();
    currentFile = null;
    updateTabs();
    if (monacoEditor) monacoEditor.setValue('');
    showWelcome();
    log('info', 'All tabs closed');
};

window.saveAllTabs = function() {
    let saved = 0;
    openFiles.forEach((content, filePath) => {
        if (filePath && !filePath.startsWith('untitled')) {
            try {
                fs.writeFileSync(filePath, content, 'utf8');
                dirtyFiles.delete(filePath);
                saved++;
            } catch (e) {
                log('error', `Failed to save ${path.basename(filePath)}: ${e.message}`);
            }
        }
    });
    updateTabs();
    log('info', `Saved ${saved} files`);
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
                
                // Track for gamification - check file content for secrets
                ipcRenderer.send('check-file-content', content);
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
initResizer('topResizer', () => [document.getElementById('editorPanel'), document.getElementById('chatPanel')], false);
initResizer('rowResizer', () => [document.getElementById('topRow'), document.getElementById('bottomRow')], true);
// Terminal/REPL horizontal resizer
initResizer('terminalReplResizer', () => [document.getElementById('terminalPanel'), document.getElementById('replPanel')], false);

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
        const errorPhrase = deedeePersonality.getPhrase('errors');
        addMessage('assistant', `${errorPhrase}<br><br>⚠️ <strong>API Key Required</strong><br><br>
            To use AI features, please configure your API key in settings.<br><br>
            <button onclick="openSettingsModal()" style="padding: 6px 12px; background: var(--primary); border: none; border-radius: 4px; color: white; cursor: pointer;">
                <i class="fas fa-key"></i> Open Settings
            </button><br><br>
            <small>Or upgrade to Pro+ for included API credits!</small>`);
        return;
    }
    
    addMessage('user', text);
    chatInput.value = '';
    
    // Show typing indicator with pop culture phrase
    const typingId = 'typing-' + Date.now();
    const processingPhrase = deedeePersonality.getPhrase('processing');
    addMessage('assistant', `<span id="${typingId}" class="typing-indicator">${processingPhrase}<span class="dots">...</span></span>`);
    
    // Speak processing phrase if TTS is enabled
    if (window.deedeeTTS && window.deedeeTTS.isSupported() && window.deedeeTTS.isEnabled()) {
        window.deedeeTTS.init().then(() => {
            window.deedeeTTS.speak(processingPhrase, { volume: 0.8 }).catch((err) => {
                console.log('TTS speak error:', err);
            });
        }).catch((err) => {
            console.log('TTS init error:', err);
        });
    }
    
    try {
        const response = await callAI(text);
        // Remove typing indicator and show response with pop culture enhancement
        const typingEl = document.getElementById(typingId);
        if (typingEl) {
            const enhancedResponse = deedeePersonality.enhanceMessage(response, 'helping');
            typingEl.parentElement.innerHTML = enhancedResponse;
            
            // Speak the response if TTS is enabled
            if (window.deedeeTTS && window.deedeeTTS.isSupported() && window.deedeeTTS.isEnabled()) {
                window.deedeeTTS.speakEnhanced(enhancedResponse, 'helping').catch((err) => {
                    console.log('TTS speakEnhanced error:', err);
                });
            }
        }
    } catch (err) {
        const typingEl = document.getElementById(typingId);
        if (typingEl) {
            const errorPhrase = deedeePersonality.getPhrase('errors');
            const errorMessage = `${errorPhrase}<br><br>❌ Error: ${err.message}<br><small>Check your API settings.</small>`;
            typingEl.parentElement.innerHTML = errorMessage;
            
            // Speak error phrase if TTS is enabled
            if (window.deedeeTTS && window.deedeeTTS.isSupported()) {
                window.deedeeTTS.speakPhrase('errors').catch(() => {});
            }
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
            
        case 'google':
            url = `https://generativelanguage.googleapis.com/v1beta/models/${model}:generateContent?key=${apiKey}`;
            headers = { 'Content-Type': 'application/json' };
            body = {
                contents: [{
                    parts: [{ text: prompt }]
                }]
            };
            break;
            
        case 'xai':
            url = 'https://api.x.ai/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json'
            };
            body = {
                model: model || 'grok-beta',
                messages: [{ role: 'user', content: prompt }],
                max_tokens: 2048
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
    } else if (provider === 'google') {
        return data.candidates?.[0]?.content?.parts?.[0]?.text || 'No response';
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

function appendTerminalOutput(text, cssClass = 'terminal-line') {
    if (!terminalOutput) return;
    const line = document.createElement('div');
    line.className = cssClass;
    line.textContent = text;
    terminalOutput.appendChild(line);
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

// Terminal helper functions
window.copyTerminalOutput = function() {
    if (!terminalOutput) return;
    const text = terminalOutput.innerText;
    navigator.clipboard.writeText(text).then(() => {
        showNotification('Terminal output copied!', 'success');
    }).catch(() => {
        showNotification('Failed to copy', 'error');
    });
};

window.clearTerminal = function() {
    if (terminalOutput) {
        terminalOutput.innerHTML = '';
        showNotification('Terminal cleared', 'info');
    }
};

window.popoutDocs = function() {
    // Open a popout docs window
    ipcRenderer.send('open-docs-window');
};

// REPL / Console functionality
const replOutput = document.getElementById('replOutput');
const replInput = document.getElementById('replInput');
const replPort = document.getElementById('replPort');
let replHistory = [];
let replHistoryIndex = -1;

// Append output to REPL console
function appendReplOutput(text, type = 'output') {
    if (!replOutput) return;
    const line = document.createElement('div');
    line.className = `repl-line ${type}`;
    line.textContent = text;
    replOutput.appendChild(line);
    replOutput.scrollTop = replOutput.scrollHeight;
}

// Format value for display
function formatValue(value) {
    if (value === undefined) return 'undefined';
    if (value === null) return 'null';
    if (typeof value === 'function') return value.toString();
    if (typeof value === 'object') {
        try {
            return JSON.stringify(value, null, 2);
        } catch (e) {
            return String(value);
        }
    }
    return String(value);
}

// Execute REPL command
function executeReplCommand(cmd) {
    cmd = cmd.trim();
    if (!cmd) return;
    
    // Add to history
    replHistory.push(cmd);
    replHistoryIndex = replHistory.length;
    
    // Show the command
    appendReplOutput(`> ${cmd}`, 'command');
    
    // Built-in commands
    if (cmd === 'help') {
        appendReplOutput(`REPL Commands:
  help        - Show this help
  clear       - Clear console
  status      - Show app status
  debug       - Toggle debug mode
  files       - List open files
  cwd         - Show current directory
  env         - Show environment info
  
JavaScript expressions are evaluated directly.
Use 'await' for async operations.`, 'info');
        return;
    }
    
    if (cmd === 'clear') {
        replOutput.innerHTML = '';
        return;
    }
    
    if (cmd === 'status') {
        appendReplOutput(`AIDE Status:
  Current Dir: ${currentDir}
  Open Files: ${openFiles.size}
  Current File: ${currentFile || 'none'}
  Monaco: ${monacoEditor ? 'loaded' : 'not loaded'}
  Features: ${JSON.stringify(features, null, 2)}`, 'info');
        return;
    }
    
    if (cmd === 'debug') {
        window.AIDE_DEBUG = !window.AIDE_DEBUG;
        appendReplOutput(`Debug mode: ${window.AIDE_DEBUG ? 'ON' : 'OFF'}`, 'info');
        return;
    }
    
    if (cmd === 'files') {
        const files = Array.from(openFiles.keys()).map(f => path.basename(f));
        appendReplOutput(`Open files: ${files.length ? files.join(', ') : 'none'}`, 'info');
        return;
    }
    
    if (cmd === 'cwd') {
        appendReplOutput(currentDir, 'info');
        return;
    }
    
    if (cmd === 'env') {
        appendReplOutput(`Node: ${process.version}
Electron: ${process.versions.electron}
Chrome: ${process.versions.chrome}
Platform: ${process.platform} ${process.arch}`, 'info');
        return;
    }
    
    // Evaluate JavaScript
    try {
        // Create a context with useful globals
        const context = {
            fs, path, os,
            currentDir, currentFile, openFiles,
            monacoEditor, features,
            log, showNotification,
            $: (sel) => document.querySelector(sel),
            $$: (sel) => document.querySelectorAll(sel)
        };
        
        // Wrap in async function to support await
        const isAsync = cmd.includes('await');
        let code = cmd;
        
        if (isAsync) {
            code = `(async () => { return ${cmd}; })()`;
        }
        
        // Evaluate with context
        const fn = new Function(...Object.keys(context), `return ${code}`);
        const result = fn(...Object.values(context));
        
        if (result instanceof Promise) {
            result.then(val => {
                appendReplOutput(formatValue(val), 'result');
            }).catch(err => {
                appendReplOutput(`Error: ${err.message}`, 'error');
            });
        } else {
            appendReplOutput(formatValue(result), 'result');
        }
    } catch (e) {
        appendReplOutput(`Error: ${e.message}`, 'error');
    }
}

// REPL input handler
if (replInput) {
    replInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();
            const cmd = replInput.value;
            replInput.value = '';
            executeReplCommand(cmd);
        } else if (e.key === 'ArrowUp') {
            e.preventDefault();
            if (replHistoryIndex > 0) {
                replHistoryIndex--;
                replInput.value = replHistory[replHistoryIndex] || '';
            }
        } else if (e.key === 'ArrowDown') {
            e.preventDefault();
            if (replHistoryIndex < replHistory.length - 1) {
                replHistoryIndex++;
                replInput.value = replHistory[replHistoryIndex] || '';
            } else {
                replHistoryIndex = replHistory.length;
                replInput.value = '';
            }
        }
    });
    
    // Track REPL usage for gamification
    replInput.addEventListener('focus', () => {
        ipcRenderer.send('record-feature-usage', 'terminal');
    });
}

window.copyReplOutput = function() {
    if (!replOutput) return;
    const text = replOutput.innerText;
    navigator.clipboard.writeText(text).then(() => {
        showNotification('Console output copied!', 'success');
    }).catch(() => {
        showNotification('Failed to copy', 'error');
    });
};

window.clearRepl = function() {
    if (replOutput) {
        replOutput.innerHTML = '';
        showNotification('Console cleared', 'info');
    }
};

// Update REPL port display
function updateReplPort(port) {
    if (replPort && port) {
        replPort.textContent = `:${port}`;
        replPort.title = `Running on port ${port}`;
    }
}

// Show debug info in REPL by default
if (replOutput) {
    window.AIDE_DEBUG = true;
    
    // Show startup debug info
    setTimeout(() => {
        appendReplOutput('═══════════════════════════════════════════', 'info');
        appendReplOutput('  AIDE Debug Console', 'info');
        appendReplOutput('═══════════════════════════════════════════', 'info');
        appendReplOutput(`  Node: ${process.version}`, 'output');
        appendReplOutput(`  Electron: ${process.versions.electron}`, 'output');
        appendReplOutput(`  Chrome: ${process.versions.chrome}`, 'output');
        appendReplOutput(`  Platform: ${process.platform} ${process.arch}`, 'output');
        appendReplOutput(`  CWD: ${currentDir}`, 'output');
        appendReplOutput(`  Monaco: ${monacoEditor ? '✓ loaded' : '✗ not loaded'}`, monacoEditor ? 'result' : 'error');
        appendReplOutput('───────────────────────────────────────────', 'info');
        appendReplOutput('  Type "help" for commands', 'info');
        appendReplOutput('═══════════════════════════════════════════', 'info');
    }, 500);
    
    // Intercept console.log/error/warn to show in debug console
    const originalLog = console.log;
    const originalError = console.error;
    const originalWarn = console.warn;
    
    console.log = function(...args) {
        originalLog.apply(console, args);
        if (window.AIDE_DEBUG && replOutput) {
            appendReplOutput(`[LOG] ${args.map(a => typeof a === 'object' ? JSON.stringify(a) : String(a)).join(' ')}`, 'output');
        }
    };
    
    console.error = function(...args) {
        originalError.apply(console, args);
        if (window.AIDE_DEBUG && replOutput) {
            appendReplOutput(`[ERR] ${args.map(a => typeof a === 'object' ? JSON.stringify(a) : String(a)).join(' ')}`, 'error');
        }
    };
    
    console.warn = function(...args) {
        originalWarn.apply(console, args);
        if (window.AIDE_DEBUG && replOutput) {
            appendReplOutput(`[WARN] ${args.map(a => typeof a === 'object' ? JSON.stringify(a) : String(a)).join(' ')}`, 'info');
        }
    };
}

function buildRunCommand(filePath) {
    const selection = runConfig.interpreter || 'auto';
    const fileArg = `"${filePath}"`;
    
    if (selection === 'custom') {
        const customCmd = (runConfig.customCommand || '').trim();
        if (!customCmd) return null;
        if (customCmd.includes('{file}')) {
            return customCmd.replace('{file}', fileArg);
        }
        return `${customCmd} ${fileArg}`;
    }
    
    if (selection === 'auto') {
        const ext = path.extname(filePath).toLowerCase();
        const template = autoInterpreterMap[ext];
        if (!template) return null;
        return template.replace('{file}', fileArg);
    }
    
    const template = interpreterTemplates[selection];
    if (!template) return null;
    return template.replace('{file}', fileArg);
}

function runCurrentFile() {
    if (!currentFile) {
        showNotification('Open a file to run it.', 'error');
        return;
    }
    
    const filePath = currentFile;
    const content = monacoEditor ? monacoEditor.getValue() : openFiles.get(filePath) || '';
    
    try {
        fs.writeFileSync(filePath, content, 'utf8');
        openFiles.set(filePath, content);
        clearTabDirty(filePath);
    } catch (err) {
        showNotification(`Save failed: ${err.message}`, 'error');
        return;
    }
    
    const command = buildRunCommand(filePath);
    if (!command) {
        showNotification('No interpreter configured for this file type. Configure one in Settings → Run & Interpreter.', 'error');
        return;
    }
    
    appendTerminalOutput(`$ ${command}`);
    
    const child = spawn(command, {
        cwd: path.dirname(filePath),
        shell: true,
        env: process.env
    });
    
    child.stdout.on('data', (data) => {
        const text = data.toString();
        if (text.trim()) appendTerminalOutput(text.trimEnd());
    });
    
    child.stderr.on('data', (data) => {
        const text = data.toString();
        if (text.trim()) appendTerminalOutput(text.trimEnd(), 'terminal-line err');
    });
    
    child.on('close', (code) => {
        appendTerminalOutput(`Process exited with code ${code}`);
    });
    
    child.on('error', (err) => {
        appendTerminalOutput(`Run error: ${err.message}`, 'terminal-line err');
        showNotification(`Run error: ${err.message}`, 'error');
    });
}

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
    shell.openExternal(baseUrl);
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
// NOTE: Click-off-to-hide is handled by blur events in main.js
// The main process detects when window loses focus and hides based on eyeMode
// Here we only handle keyboard shortcuts and explicit close button clicks

document.addEventListener('keydown', (e) => {
    const mode = window.eyeMode || 'single';
    // Escape only closes in single mode (no glow)
    if (e.key === 'Escape' && mode === 'single') {
        ipcRenderer.send('close-ide');
    }
    // Ctrl+B toggles sidebar
    if (e.ctrlKey && e.key === 'b') {
        e.preventDefault();
        toggleSidebarCollapse();
    }
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
        action: () => log('info', 'Theme toggle disabled - dark mode only'),
        feedback: { color: 'rgba(251, 191, 36, 0.8)', log: 'Dark mode only!' }
    },
    openExplorer: {
        phrases: ['dee dee files', 'show files', 'open explorer', 'dee dee explorer', 'show explorer'],
        action: () => { if (sidebar.classList.contains('collapsed')) document.getElementById('sidebarToggle').click(); },
        feedback: { color: 'rgba(139, 92, 246, 0.8)', log: 'Opening explorer!' }
    },
    openTodo: {
        phrases: ['dee dee todo', 'show todo', 'open todo', 'dee dee tasks', 'show tasks'],
        action: () => { switchSidebarTab('todo'); },
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

// Helper function to process voice commands (pattern matching or LLM)
async function processVoiceCommand(transcript, confidence) {
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
                
                // Speak command feedback with TTS if available
                if (window.deedeeTTS && window.deedeeTTS.isSupported()) {
                    const feedbackPhrase = deedeePersonality.getPhrase('helping');
                    window.deedeeTTS.speak(feedbackPhrase || cmd.feedback.log, { volume: 0.6 }).catch(() => {});
                }
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

    // If no command matched but confidence is high, try LLM if enabled
    if (!handled && confidence > 0.7) {
        log('debug', `High confidence but no pattern match: "${transcript}"`);
        
        // Try LLM interpretation if:
        // 1. Pro+ user (has included credits), OR
        // 2. API key is configured, OR
        // 3. Using Ollama (local, no key needed)
        const apiSettings = JSON.parse(localStorage.getItem('aide-api-settings') || '{}');
        if (userTier === 'proplus' || apiSettings.apiKey || apiSettings.provider === 'ollama') {
            try {
                await processVoiceCommandWithLLM(transcript, confidence);
                handled = true;
            } catch (error) {
                log('warn', `LLM processing failed: ${error.message}`);
            }
        } else {
            log('info', '💡 Tip: Configure an API key in settings or upgrade to Pro+ for natural language voice commands');
        }
    }
}

// === LLM VOICE COMMAND INTEGRATION ===

/**
 * Process voice command using LLM for natural language understanding
 */
async function processVoiceCommandWithLLM(transcript, confidence) {
    log('info', `🧠 Processing with LLM: "${transcript}"`);
    
    // Show processing indicator
    showVoiceFeedback('processing', 'Understanding command...', 2000);
    
    try {
        const interpretation = await interpretVoiceCommandWithLLM(transcript, getVoiceContext());
        
        if (interpretation.action === 'error') {
            showVoiceFeedback('error', interpretation.message, 3000);
            return;
        }
        
        // Execute the interpreted action
        await executeLLMAction(interpretation);
        
        // Show success feedback
        showVoiceFeedback('success', `Executed: ${interpretation.action}`, 1500);
        
    } catch (error) {
        log('error', `LLM voice command error: ${error.message}`);
        showVoiceFeedback('error', 'Failed to process command', 3000);
    }
}

/**
 * Get current app context for LLM
 */
function getVoiceContext() {
    return {
        currentDir: currentDir || os.homedir(),
        openFiles: Array.from(openFiles.keys()),
        activeFile: currentFile,
        sidebarVisible: !sidebar.classList.contains('collapsed'),
        currentTab: document.querySelector('.tab.active')?.textContent || null,
        fileCount: openFiles.size
    };
}

/**
 * Send voice transcript to LLM for intent understanding
 */
async function interpretVoiceCommandWithLLM(transcript, context) {
    // Pro+ users use included API credits - no key needed
    if (userTier === 'proplus') {
        log('info', '🎤 Using Pro+ AI backend for voice commands');
    } else {
        const apiSettings = JSON.parse(localStorage.getItem('aide-api-settings') || '{}');
        
        // Check if API key is configured (unless Ollama)
        if (!apiSettings.apiKey && apiSettings.provider !== 'ollama') {
            log('warn', 'No API key configured for voice commands');
            return { action: 'error', message: 'Please configure your API key in settings or upgrade to Pro+' };
        }
    }
    
    // Build system prompt for LLM
    const systemPrompt = `You are Dee Dee, an AI assistant for AIDE (AI Development Environment).
Your job is to interpret voice commands and return structured actions.

Available actions:
- wake: Wake/show the AIDE window
- hide: Hide the AIDE window
- newFile: Create a new file
- saveFile: Save the current file
- openFile: Open a file (requires filename parameter)
- openExplorer: Show/hide file explorer sidebar
- openTodo: Show TODO list
- focusChat: Focus the chat input
- focusTerminal: Focus the terminal
- runCode: Run the current file
- clearChat: Clear chat messages
- explain: Explain something (requires query parameter)
- searchFiles: Search for files (requires query parameter)
- chat: Send a message to chat (requires message parameter)

Current context:
- Current directory: ${context.currentDir || 'unknown'}
- Open files: ${context.openFiles?.length || 0}
- Active file: ${context.activeFile || 'none'}
- Sidebar visible: ${context.sidebarVisible || false}

Return ONLY a valid JSON object with this exact structure:
{
    "action": "action_name",
    "parameters": {},
    "confidence": 0.0-1.0,
    "reasoning": "brief explanation"
}

Do not include any other text, only the JSON.`;

    const userPrompt = `User said: "${transcript}"

What action should I take? Return only valid JSON.`;

    try {
        // Use Pro+ backend if available, otherwise use user's API settings
        const apiSettings = userTier === 'proplus' ? {} : JSON.parse(localStorage.getItem('aide-api-settings') || '{}');
        const response = await callLLMForVoice(apiSettings, systemPrompt, userPrompt);
        
        // Extract JSON from response (handle cases where LLM adds extra text)
        const jsonMatch = response.match(/\{[\s\S]*\}/);
        if (!jsonMatch) {
            throw new Error('No JSON found in LLM response');
        }
        
        return JSON.parse(jsonMatch[0]);
    } catch (error) {
        log('error', `LLM voice command error: ${error.message}`);
        return { action: 'error', message: error.message };
    }
}

/**
 * Call LLM API for voice commands (uses existing callAI infrastructure)
 */
async function callLLMForVoice(apiSettings, systemPrompt, userPrompt) {
    // Pro+ users use included backend API
    if (userTier === 'proplus') {
        const licenseKey = localStorage.getItem('aide-license-key');
        if (!licenseKey) {
            throw new Error('Pro+ license key not found');
        }
        
        // Combine system prompt and user prompt for Pro+ API
        const combinedPrompt = `${systemPrompt}\n\n${userPrompt}`;
        
        const res = await fetch('https://api.aide.dev/v1/chat', {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${licenseKey}`
            },
            body: JSON.stringify({ 
                prompt: combinedPrompt,
                model: apiSettings.model || 'gpt-4' // Use model from settings if available
            })
        });
        
        if (!res.ok) {
            const errorText = await res.text();
            throw new Error(`Pro+ API error: ${res.status} - ${errorText.slice(0, 200)}`);
        }
        
        const data = await res.json();
        return data.response || '{}';
    }
    
    // Regular users use their own API key
    const { provider, apiKey, model, customEndpoint } = apiSettings;
    
    // Use existing callAI function but with system prompt
    // For providers that support system prompts, we'll modify the call
    let url, headers, body;
    
    switch (provider) {
        case 'openai':
            url = 'https://api.openai.com/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json'
            };
            body = {
                model: model || 'gpt-4',
                messages: [
                    { role: 'system', content: systemPrompt },
                    { role: 'user', content: userPrompt }
                ],
                temperature: 0.3,
                max_tokens: 200
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
                model: model || 'claude-3-haiku-20240307',
                system: systemPrompt,
                messages: [{ role: 'user', content: userPrompt }],
                max_tokens: 200,
                temperature: 0.3
            };
            break;
            
        case 'google':
            url = `https://generativelanguage.googleapis.com/v1beta/models/${model || 'gemini-pro'}:generateContent?key=${apiKey}`;
            headers = { 'Content-Type': 'application/json' };
            body = {
                contents: [{
                    parts: [{ text: `${systemPrompt}\n\n${userPrompt}` }]
                }],
                generationConfig: {
                    maxOutputTokens: 200,
                    temperature: 0.3
                }
            };
            break;
            
        case 'xai':
            url = 'https://api.x.ai/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json'
            };
            body = {
                model: model || 'grok-beta',
                messages: [
                    { role: 'system', content: systemPrompt },
                    { role: 'user', content: userPrompt }
                ],
                temperature: 0.3,
                max_tokens: 200
            };
            break;
            
        case 'ollama':
            url = 'http://localhost:11434/api/chat';
            headers = { 'Content-Type': 'application/json' };
            body = {
                model: model || 'llama3',
                messages: [
                    { role: 'system', content: systemPrompt },
                    { role: 'user', content: userPrompt }
                ],
                options: {
                    temperature: 0.3,
                    num_predict: 200
                },
                stream: false
            };
            break;
            
        case 'openrouter':
            url = 'https://openrouter.ai/api/v1/chat/completions';
            headers = { 
                'Authorization': `Bearer ${apiKey}`,
                'Content-Type': 'application/json',
                'HTTP-Referer': window.location.origin,
                'X-Title': 'AIDE'
            };
            body = {
                model: model || 'openai/gpt-4',
                messages: [
                    { role: 'system', content: systemPrompt },
                    { role: 'user', content: userPrompt }
                ],
                temperature: 0.3,
                max_tokens: 200
            };
            break;
            
        case 'custom':
            url = customEndpoint || 'http://localhost:8080/v1/chat/completions';
            headers = { 
                'Content-Type': 'application/json',
                ...(apiKey ? { 'Authorization': `Bearer ${apiKey}` } : {})
            };
            body = {
                model: model || 'default',
                messages: [
                    { role: 'system', content: systemPrompt },
                    { role: 'user', content: userPrompt }
                ],
                temperature: 0.3,
                max_tokens: 200
            };
            break;
            
        default:
            throw new Error(`Unsupported provider: ${provider}`);
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
        return data.content?.[0]?.text || '{}';
    } else if (provider === 'google') {
        return data.candidates?.[0]?.content?.parts?.[0]?.text || '{}';
    } else if (provider === 'ollama') {
        return data.message?.content || '{}';
    } else {
        return data.choices?.[0]?.message?.content || '{}';
    }
}

/**
 * Execute action returned by LLM
 */
async function executeLLMAction(interpretation) {
    const { action, parameters, confidence, reasoning } = interpretation;
    
    log('info', `🎯 Executing LLM action: ${action} (confidence: ${confidence?.toFixed(2) || 'N/A'})`);
    if (reasoning) {
        log('debug', `Reasoning: ${reasoning}`);
    }
    
    // Only execute if confidence is high enough
    if (confidence !== undefined && confidence < 0.5) {
        log('warn', `Low confidence (${confidence.toFixed(2)}), asking for confirmation`);
        showVoiceFeedback('error', `Low confidence (${confidence.toFixed(2)})`, 2000);
        return;
    }
    
    switch (action) {
        case 'wake':
            ipcRenderer.send('voice-wake');
            break;
            
        case 'hide':
            ipcRenderer.send('voice-hide');
            break;
            
        case 'newFile':
            if (typeof newTab === 'function') {
                newTab();
            }
            break;
            
        case 'saveFile':
            document.dispatchEvent(new KeyboardEvent('keydown', {
                key: 's',
                ctrlKey: true
            }));
            break;
            
        case 'openFile':
            if (parameters?.filename) {
                const filePath = await findFileByName(parameters.filename);
                if (filePath) {
                    openFile(filePath);
                } else {
                    log('warn', `File not found: ${parameters.filename}`);
                    showVoiceFeedback('error', `File not found: ${parameters.filename}`, 3000);
                }
            }
            break;
            
        case 'openExplorer':
            if (sidebar.classList.contains('collapsed')) {
                document.getElementById('sidebarToggle')?.click();
            }
            break;
            
        case 'openTodo':
            if (typeof switchSidebarTab === 'function') {
                switchSidebarTab('todo');
            }
            break;
            
        case 'focusChat':
            if (chatInput) chatInput.focus();
            break;
            
        case 'focusTerminal':
            document.getElementById('terminalInput')?.focus();
            break;
            
        case 'runCode':
            if (runBtn) runBtn.click();
            break;
            
        case 'clearChat':
            const chatMessages = document.getElementById('chatMessages');
            if (chatMessages) chatMessages.innerHTML = '';
            break;
            
        case 'explain':
            if (parameters?.query && chatInput) {
                chatInput.value = parameters.query;
                if (typeof sendMessage === 'function') {
                    sendMessage();
                }
            }
            break;
            
        case 'searchFiles':
            if (parameters?.query) {
                searchFilesByName(parameters.query);
            }
            break;
            
        case 'chat':
            if (parameters?.message && chatInput) {
                chatInput.value = parameters.message;
                if (typeof sendMessage === 'function') {
                    sendMessage();
                }
            }
            break;
            
        default:
            log('warn', `Unknown LLM action: ${action}`);
            // Fallback: send to chat as a question
            if (chatInput) {
                chatInput.value = `What does "${action}" mean?`;
                if (typeof sendMessage === 'function') {
                    sendMessage();
                }
            }
    }
}

/**
 * Find file by name (searches current directory)
 */
async function findFileByName(filename) {
    try {
        const files = fs.readdirSync(currentDir || os.homedir());
        // Try exact match first
        let match = files.find(f => f.toLowerCase() === filename.toLowerCase());
        if (match) {
            return path.join(currentDir || os.homedir(), match);
        }
        // Try partial match
        match = files.find(f => f.toLowerCase().includes(filename.toLowerCase()));
        if (match) {
            return path.join(currentDir || os.homedir(), match);
        }
        return null;
    } catch (error) {
        log('error', `Error finding file: ${error.message}`);
        return null;
    }
}

/**
 * Search files by name
 */
function searchFilesByName(query) {
    try {
        const files = fs.readdirSync(currentDir || os.homedir());
        const matches = files.filter(f => 
            f.toLowerCase().includes(query.toLowerCase())
        );
        
        if (matches.length === 0) {
            showVoiceFeedback('error', `No files found matching "${query}"`, 3000);
            return;
        }
        
        if (matches.length === 1) {
            // Auto-open if single match
            openFile(path.join(currentDir || os.homedir(), matches[0]));
            showVoiceFeedback('success', `Opened: ${matches[0]}`, 2000);
        } else {
            // Show list of matches
            log('info', `Found ${matches.length} files: ${matches.join(', ')}`);
            showVoiceFeedback('info', `Found ${matches.length} files matching "${query}"`, 3000);
            // Could open a file picker here
        }
    } catch (error) {
        log('error', `Error searching files: ${error.message}`);
        showVoiceFeedback('error', 'Error searching files', 3000);
    }
}

let recognition = null;
let isListening = false;
let audioContext = null;
let analyser = null;
let microphone = null;
let voiceActivityTimeout = null;
let voiceActivityThreshold = 0.01; // Adjust based on environment
let voiceListeningEnabled = JSON.parse(localStorage.getItem('aide-voice-listening') || 'false');
let voicePreferenceSet = localStorage.getItem('aide-voice-pref-set') === 'true';
let pendingVoiceEnable = false;
let voiceActivationInitialized = false;

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
window.toggleFavorite = toggleFavorite;

// Check if directory is favorited
function isFavorite(dirPath) {
    return favoriteDirectories.has(dirPath);
}
window.isFavorite = isFavorite;

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
window.addDuplicateSidebar = addDuplicateSidebar;

// Duplicate todo sidebar removed - Todo is now in left sidebar tabs
window.addDuplicateTodoSidebar = function() {
    log('info', 'Todo is now in the left sidebar tabs');
};

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
window.deleteTodoForSidebar = deleteTodoForSidebar;
window.toggleTodoForSidebar = toggleTodoForSidebar;
window.addTodoForSidebar = addTodoForSidebar;
window.showTodoSaveLoadMenu = showTodoSaveLoadMenu;
window.createNewItem = createNewItem;
window.showRecentItems = showRecentItems;
window.deleteCurrentItem = deleteCurrentItem;
window.clearTodoSearch = clearTodoSearch;

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
        <div class="sidebar-header duplicate-sidebar-header">
            <span><i class="fas fa-columns"></i> Panel ${sidebarInstances.length}</span>
            <div class="duplicate-sidebar-controls">
                <button class="sidebar-control-btn" onclick="toggleDuplicateSidebar('${sidebarId}')" title="Collapse/Expand">
                    <i class="fas fa-chevron-left" id="collapse-icon-${sidebarId}"></i>
                </button>
                <button class="sidebar-control-btn sidebar-close-btn" onclick="closeDuplicateSidebar('${sidebarId}')" title="Close">
                    <i class="fas fa-times"></i>
                </button>
            </div>
        </div>

        <div class="duplicate-sidebar-content" id="content-${sidebarId}">
            <div class="sidebar-tabs">
                <button class="sidebar-tab active" data-tab="files-${sidebarId}" onclick="switchDuplicateSidebarTab('files', '${sidebarId}')" title="Files">
                    <i class="fas fa-folder"></i>
                </button>
                <button class="sidebar-tab" data-tab="git-${sidebarId}" onclick="switchDuplicateSidebarTab('git', '${sidebarId}')" title="Git">
                    <i class="fab fa-git-alt"></i>
                </button>
                <button class="sidebar-tab" data-tab="todo-${sidebarId}" onclick="switchDuplicateSidebarTab('todo', '${sidebarId}')" title="Todo">
                    <i class="fas fa-tasks"></i>
                </button>
            </div>

            <!-- Files Tab -->
            <div class="sidebar-tab-content active" id="filesTab-${sidebarId}">
                <div class="sidebar-path" id="currentPath-${sidebarId}">~</div>
                
                <div class="file-search-container">
                    <input type="text" id="fileSearchInput-${sidebarId}" placeholder="Filter files..." autocomplete="off">
                    <i class="fas fa-search file-search-icon"></i>
                </div>

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
            
            <!-- Git Tab -->
            <div class="sidebar-tab-content" id="gitTab-${sidebarId}">
                <div class="git-header">
                    <span class="git-branch" id="gitBranch-${sidebarId}"><i class="fas fa-code-branch"></i> main</span>
                    <div class="git-actions">
                        <button onclick="gitRefreshForSidebar('${sidebarId}')" title="Refresh"><i class="fas fa-sync-alt"></i></button>
                        <button onclick="gitPullForSidebar('${sidebarId}')" title="Pull"><i class="fas fa-arrow-down"></i></button>
                        <button onclick="gitPushForSidebar('${sidebarId}')" title="Push"><i class="fas fa-arrow-up"></i></button>
                    </div>
                </div>
                
                <div class="git-section">
                    <div class="git-section-header" onclick="toggleGitSectionForSidebar('staged', '${sidebarId}')">
                        <i class="fas fa-chevron-down"></i>
                        <span>Staged Changes</span>
                        <span class="git-count" id="stagedCount-${sidebarId}">0</span>
                    </div>
                    <div class="git-file-list" id="stagedFiles-${sidebarId}"></div>
                </div>
                
                <div class="git-section">
                    <div class="git-section-header" onclick="toggleGitSectionForSidebar('changes', '${sidebarId}')">
                        <i class="fas fa-chevron-down"></i>
                        <span>Changes</span>
                        <span class="git-count" id="changesCount-${sidebarId}">0</span>
                    </div>
                    <div class="git-file-list" id="changedFiles-${sidebarId}"></div>
                </div>
                
                <div class="git-commit-box">
                    <input type="text" id="commitMessage-${sidebarId}" placeholder="Commit message...">
                    <button onclick="gitCommitForSidebar('${sidebarId}')" id="commitBtn-${sidebarId}"><i class="fas fa-check"></i> Commit</button>
                </div>
            </div>
            
            <!-- Todo Tab -->
            <div class="sidebar-tab-content" id="todoTab-${sidebarId}">
                <div class="todo-input-row">
                    <input type="text" id="todoInput-${sidebarId}" placeholder="Add task..." autocomplete="off">
                    <button onclick="addTodoForSidebar('${sidebarId}')"><i class="fas fa-plus"></i></button>
                </div>
                <div class="todo-list" id="todoList-${sidebarId}">
                    <!-- Todos rendered here -->
                </div>
                <div class="todo-footer">
                    <span class="todo-stats" id="todoStats-${sidebarId}">0 tasks</span>
                    <button onclick="clearCompletedTodosForSidebar('${sidebarId}')" title="Clear completed"><i class="fas fa-trash"></i> Clear Done</button>
                </div>
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

// Toggle collapse/expand duplicate sidebar
function toggleDuplicateSidebar(sidebarId) {
    const sidebar = document.getElementById(sidebarId);
    const content = document.getElementById(`content-${sidebarId}`);
    const icon = document.getElementById(`collapse-icon-${sidebarId}`);
    
    if (sidebar.classList.contains('collapsed')) {
        sidebar.classList.remove('collapsed');
        content.style.display = '';
        icon.className = 'fas fa-chevron-left';
    } else {
        sidebar.classList.add('collapsed');
        content.style.display = 'none';
        icon.className = 'fas fa-chevron-right';
    }
}
window.toggleDuplicateSidebar = toggleDuplicateSidebar;

// Close and remove duplicate sidebar
function closeDuplicateSidebar(sidebarId) {
    const sidebar = document.getElementById(sidebarId);
    if (sidebar) {
        sidebar.remove();
        
        // Remove from instances array
        const index = sidebarInstances.indexOf(sidebarId);
        if (index > -1) {
            sidebarInstances.splice(index, 1);
        }
        
        // Clean up window variables
        delete window[`currentDir_${sidebarId}`];
        delete window[`fileTree_${sidebarId}`];
        delete window[`todos_${sidebarId}`];
        
        log('info', `Closed sidebar: ${sidebarId}`);
    }
}
window.closeDuplicateSidebar = closeDuplicateSidebar;

// Switch tabs in duplicate sidebar
function switchDuplicateSidebarTab(tabName, sidebarId) {
    const sidebar = document.getElementById(sidebarId);
    if (!sidebar) return;
    
    // Deactivate all tabs and content
    sidebar.querySelectorAll('.sidebar-tab').forEach(t => t.classList.remove('active'));
    sidebar.querySelectorAll('.sidebar-tab-content').forEach(c => c.classList.remove('active'));
    
    // Activate selected tab and content
    const tab = sidebar.querySelector(`.sidebar-tab[data-tab="${tabName}-${sidebarId}"]`);
    const content = document.getElementById(`${tabName}Tab-${sidebarId}`);
    
    if (tab) tab.classList.add('active');
    if (content) content.classList.add('active');
    
    // Load data for specific tabs
    if (tabName === 'git') {
        gitRefreshForSidebar(sidebarId);
    } else if (tabName === 'todo') {
        renderTodosForSidebar(sidebarId);
    }
}
window.switchDuplicateSidebarTab = switchDuplicateSidebarTab;

// Git functions for duplicate sidebars
function gitRefreshForSidebar(sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    
    // Check if it's a git repo
    try {
        execSync('git rev-parse --is-inside-work-tree', { cwd: dir, encoding: 'utf8', stdio: 'pipe' });
    } catch {
        document.getElementById(`gitBranch-${sidebarId}`).innerHTML = '<i class="fas fa-code-branch"></i> Not a Git repo';
        document.getElementById(`stagedFiles-${sidebarId}`).innerHTML = '<div class="git-empty">Not a Git repository</div>';
        document.getElementById(`changedFiles-${sidebarId}`).innerHTML = '';
        document.getElementById(`stagedCount-${sidebarId}`).textContent = '0';
        document.getElementById(`changesCount-${sidebarId}`).textContent = '0';
        return;
    }
    
    try {
        const branch = execSync('git branch --show-current', { cwd: dir, encoding: 'utf8', stdio: 'pipe' }).trim();
        document.getElementById(`gitBranch-${sidebarId}`).innerHTML = `<i class="fas fa-code-branch"></i> ${branch || 'HEAD'}`;
        
        const status = execSync('git status --porcelain', { cwd: dir, encoding: 'utf8', stdio: 'pipe' });
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
        
        renderGitFilesForSidebar(`stagedFiles-${sidebarId}`, staged, true, sidebarId);
        renderGitFilesForSidebar(`changedFiles-${sidebarId}`, changes, false, sidebarId);
        document.getElementById(`stagedCount-${sidebarId}`).textContent = staged.length;
        document.getElementById(`changesCount-${sidebarId}`).textContent = changes.length;
        
    } catch (e) {
        console.error('Git error for sidebar:', e);
    }
}
window.gitRefreshForSidebar = gitRefreshForSidebar;

function renderGitFilesForSidebar(containerId, files, isStaged, sidebarId) {
    const container = document.getElementById(containerId);
    if (!container) return;
    
    if (files.length === 0) {
        container.innerHTML = '<div class="git-empty">No changes</div>';
        return;
    }
    
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    
    container.innerHTML = files.map(f => {
        const iconClass = getGitStatusIcon(f.status);
        const action = isStaged 
            ? `<button onclick="gitUnstageForSidebar('${f.file}', '${sidebarId}')" title="Unstage"><i class="fas fa-minus"></i></button>`
            : `<button onclick="gitStageForSidebar('${f.file}', '${sidebarId}')" title="Stage"><i class="fas fa-plus"></i></button>
               <button onclick="gitDiscardForSidebar('${f.file}', '${sidebarId}')" title="Discard"><i class="fas fa-undo"></i></button>`;
        
        return `
            <div class="git-file" ondblclick="openFile('${path.join(dir, f.file).replace(/'/g, "\\'")}')">
                <span class="git-file-icon ${iconClass}"><i class="fas fa-circle"></i></span>
                <span class="git-file-name">${f.file}</span>
                <div class="git-file-actions">${action}</div>
            </div>
        `;
    }).join('');
}

function gitStageForSidebar(file, sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        execSync(`git add "${file}"`, { cwd: dir });
        gitRefreshForSidebar(sidebarId);
        log('info', `Staged: ${file}`);
    } catch (e) {
        log('error', `Failed to stage: ${e.message}`);
    }
}
window.gitStageForSidebar = gitStageForSidebar;

function gitUnstageForSidebar(file, sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        execSync(`git reset HEAD "${file}"`, { cwd: dir });
        gitRefreshForSidebar(sidebarId);
        log('info', `Unstaged: ${file}`);
    } catch (e) {
        log('error', `Failed to unstage: ${e.message}`);
    }
}
window.gitUnstageForSidebar = gitUnstageForSidebar;

function gitDiscardForSidebar(file, sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        execSync(`git checkout -- "${file}"`, { cwd: dir });
        gitRefreshForSidebar(sidebarId);
        log('info', `Discarded: ${file}`);
    } catch (e) {
        log('error', `Failed to discard: ${e.message}`);
    }
}
window.gitDiscardForSidebar = gitDiscardForSidebar;

function gitCommitForSidebar(sidebarId) {
    const msg = document.getElementById(`commitMessage-${sidebarId}`).value.trim();
    if (!msg) {
        log('warn', 'Please enter a commit message');
        return;
    }
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        execSync(`git commit -m "${msg.replace(/"/g, '\\"')}"`, { cwd: dir });
        document.getElementById(`commitMessage-${sidebarId}`).value = '';
        gitRefreshForSidebar(sidebarId);
        log('info', `Committed: ${msg}`);
    } catch (e) {
        log('error', `Commit failed: ${e.message}`);
    }
}
window.gitCommitForSidebar = gitCommitForSidebar;

function gitPullForSidebar(sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        log('info', 'Pulling...');
        const result = execSync('git pull', { cwd: dir, encoding: 'utf8' });
        gitRefreshForSidebar(sidebarId);
        log('info', result.trim() || 'Pull complete');
    } catch (e) {
        log('error', `Pull failed: ${e.message}`);
    }
}
window.gitPullForSidebar = gitPullForSidebar;

function gitPushForSidebar(sidebarId) {
    const dir = window[`currentDir_${sidebarId}`] || currentDir;
    try {
        log('info', 'Pushing...');
        const result = execSync('git push', { cwd: dir, encoding: 'utf8' });
        gitRefreshForSidebar(sidebarId);
        log('info', result.trim() || 'Push complete');
    } catch (e) {
        log('error', `Push failed: ${e.message}`);
    }
}
window.gitPushForSidebar = gitPushForSidebar;

function toggleGitSectionForSidebar(section, sidebarId) {
    const el = document.querySelector(`#${section}Files-${sidebarId}`)?.closest('.git-section');
    if (el) el.classList.toggle('collapsed');
}
window.toggleGitSectionForSidebar = toggleGitSectionForSidebar;

// Todo functions for duplicate sidebars
function addTodoForSidebar(sidebarId) {
    const input = document.getElementById(`todoInput-${sidebarId}`);
    const text = input.value.trim();
    if (!text) return;
    
    if (!window[`todos_${sidebarId}`]) {
        window[`todos_${sidebarId}`] = [];
    }
    
    window[`todos_${sidebarId}`].push({
        id: Date.now(),
        text: text,
        done: false
    });
    
    input.value = '';
    saveTodosForSidebarInstance(sidebarId);
    renderTodosForSidebar(sidebarId);
}
window.addTodoForSidebar = addTodoForSidebar;

function toggleTodoForSidebar(todoId, sidebarId) {
    const todoList = window[`todos_${sidebarId}`] || [];
    const todo = todoList.find(t => t.id === todoId);
    if (todo) {
        todo.done = !todo.done;
        saveTodosForSidebarInstance(sidebarId);
        renderTodosForSidebar(sidebarId);
    }
}
window.toggleTodoForSidebar = toggleTodoForSidebar;

function deleteTodoForSidebar(todoId, sidebarId) {
    if (!window[`todos_${sidebarId}`]) return;
    window[`todos_${sidebarId}`] = window[`todos_${sidebarId}`].filter(t => t.id !== todoId);
    saveTodosForSidebarInstance(sidebarId);
    renderTodosForSidebar(sidebarId);
}
window.deleteTodoForSidebar = deleteTodoForSidebar;

function clearCompletedTodosForSidebar(sidebarId) {
    if (!window[`todos_${sidebarId}`]) return;
    window[`todos_${sidebarId}`] = window[`todos_${sidebarId}`].filter(t => !t.done);
    saveTodosForSidebarInstance(sidebarId);
    renderTodosForSidebar(sidebarId);
}
window.clearCompletedTodosForSidebar = clearCompletedTodosForSidebar;

function renderTodosForSidebar(sidebarId) {
    const todoList = window[`todos_${sidebarId}`] || [];
    const container = document.getElementById(`todoList-${sidebarId}`);
    const stats = document.getElementById(`todoStats-${sidebarId}`);
    
    if (!container) return;
    
    if (todoList.length === 0) {
        container.innerHTML = '<div class="todo-empty">No tasks yet</div>';
    } else {
        container.innerHTML = todoList.map(todo => `
            <div class="todo-item ${todo.done ? 'done' : ''}" data-id="${todo.id}">
                <input type="checkbox" ${todo.done ? 'checked' : ''} onchange="toggleTodoForSidebar(${todo.id}, '${sidebarId}')">
                <span class="todo-text">${escapeHtml(todo.text)}</span>
                <button class="todo-delete" onclick="deleteTodoForSidebar(${todo.id}, '${sidebarId}')"><i class="fas fa-times"></i></button>
            </div>
        `).join('');
    }
    
    if (stats) {
        const doneCount = todoList.filter(t => t.done).length;
        stats.textContent = `${doneCount}/${todoList.length} done`;
    }
}

function saveTodosForSidebarInstance(sidebarId) {
    const todoList = window[`todos_${sidebarId}`] || [];
    localStorage.setItem(`aide-todos-${sidebarId}`, JSON.stringify(todoList));
}

function loadTodosForSidebarInstance(sidebarId) {
    try {
        const saved = localStorage.getItem(`aide-todos-${sidebarId}`);
        window[`todos_${sidebarId}`] = saved ? JSON.parse(saved) : [];
    } catch {
        window[`todos_${sidebarId}`] = [];
    }
}

function initDuplicateSidebar(sidebarId) {
    // Create unique variables for this sidebar instance
    window[`currentDir_${sidebarId}`] = os.homedir();
    window[`fileTree_${sidebarId}`] = document.getElementById(`fileTree-${sidebarId}`);

    // Load initial directory
    loadDirectoryForSidebar(os.homedir(), sidebarId);

    // Update favorites for this sidebar
    updateFavoritesBarForSidebar(sidebarId);
    
    // Initialize todos for this sidebar
    loadTodosForSidebarInstance(sidebarId);
    
    // Set up file search for this sidebar
    const searchInput = document.getElementById(`fileSearchInput-${sidebarId}`);
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            const searchTerm = e.target.value.toLowerCase().trim();
            const fileItems = document.querySelectorAll(`#fileTree-${sidebarId} .file-item`);
            
            fileItems.forEach(item => {
                const fileName = item.textContent.toLowerCase();
                if (searchTerm === '' || fileName.includes(searchTerm)) {
                    item.style.display = '';
                } else {
                    item.style.display = 'none';
                }
            });
        });
        
        searchInput.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                searchInput.value = '';
                searchInput.dispatchEvent(new Event('input'));
                searchInput.blur();
            }
        });
    }
    
    // Set up todo input enter key
    const todoInput = document.getElementById(`todoInput-${sidebarId}`);
    if (todoInput) {
        todoInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                addTodoForSidebar(sidebarId);
            }
        });
    }
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
    // Right sidebar removed - switch to todo tab instead
    switchSidebarTab('todo');
}
window.toggleRightSidebar = toggleRightSidebar;

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

function saveVoiceListeningPreference() {
    localStorage.setItem('aide-voice-listening', JSON.stringify(voiceListeningEnabled));
    localStorage.setItem('aide-voice-pref-set', voicePreferenceSet ? 'true' : 'false');
}

function syncVoiceListeningUI() {
    if (!mascotMouth) return;
    
    // Mouth shows listening state: glowing = listening, not glowing = not listening
    const isListening = voiceListeningEnabled && features.isPro;
    
    mascotMouth.textContent = '‿'; // Always show smile
    mascotMouth.classList.toggle('listening', isListening);
    mascotMouth.classList.toggle('glowing', isListening);
    
    if (!features.isPro) {
        mascotMouth.title = 'Voice control (Pro feature) - click to upgrade';
    } else if (isListening) {
        mascotMouth.title = 'Listening... click to stop';
    } else {
        mascotMouth.title = 'Not listening - click to start';
    }
}

function ensureVoiceReady() {
    // Free users can use wake/hide commands
    if (micPermissionGranted) {
        initVoiceActivation();
    } else {
        pendingVoiceEnable = true;
        showMicrophonePermissionDialog();
    }
}

function setVoiceListening(enabled, source = 'manual') {
    // Free users can enable wake/hide commands, Pro users get all commands
    // Only show upgrade modal for Pro-specific features, not basic voice
    if (enabled && !features.isPro && source === 'manual') {
        // Allow enabling for free users (wake/hide commands)
        // Pro features will be checked when commands are executed
    }

    if (voiceListeningEnabled === enabled) {
        if (source !== 'sync' && !voicePreferenceSet) {
            voicePreferenceSet = true;
            saveVoiceListeningPreference();
        }
        syncVoiceListeningUI();
        return;
    }

    voiceListeningEnabled = enabled;
    if (source !== 'sync') {
        voicePreferenceSet = true;
    }
    saveVoiceListeningPreference();
    syncVoiceListeningUI();

    if (enabled) {
        ensureVoiceReady();
    } else {
        pendingVoiceEnable = false;
        stopListening();
    }
}

function toggleVoiceListening() {
    setVoiceListening(!voiceListeningEnabled, 'mouth');
}

function showVoicePreferencePrompt() {
    if (document.getElementById('voice-pref-prompt')) return;

    const prompt = document.createElement('div');
    prompt.id = 'voice-pref-prompt';
    prompt.innerHTML = `
        <div style="
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.75);
            backdrop-filter: blur(5px);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 10000;
        ">
            <div style="
                background: var(--bg-panel, #161b22);
                border: 1px solid var(--border, #30363d);
                border-radius: 12px;
                padding: 24px;
                max-width: 420px;
                box-shadow: 0 20px 40px rgba(0,0,0,0.3);
                text-align: center;
            ">
                <div style="font-size: 40px; margin-bottom: 12px;">🎙️</div>
                <h3 style="margin: 0 0 8px 0;">Enable Voice Listening?</h3>
                <p style="margin: 0 0 16px 0; color: var(--text-muted, #9ca3af); line-height: 1.5;">
                    Dee Dee can listen for commands like "Hey Dee Dee" to help you code faster.
                    You can toggle this anytime by clicking Dee Dee's mouth.
                </p>
                <div style="display: flex; gap: 12px; justify-content: center; flex-wrap: wrap;">
                    <button id="voicePrefEnable" style="
                        background: #10b981;
                        color: white;
                        border: none;
                        padding: 10px 18px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                        font-weight: 600;
                    ">Enable Listening</button>
                    <button id="voicePrefLater" style="
                        background: transparent;
                        color: var(--text-muted, #9ca3af);
                        border: 1px solid var(--border, #30363d);
                        padding: 10px 18px;
                        border-radius: 6px;
                        cursor: pointer;
                        font-size: 14px;
                    ">Not Now</button>
                </div>
            </div>
        </div>
    `;

    document.body.appendChild(prompt);

    document.getElementById('voicePrefEnable').onclick = () => {
        prompt.remove();
        voicePreferenceSet = true;
        saveVoiceListeningPreference();
        setVoiceListening(true, 'prompt');
    };

    document.getElementById('voicePrefLater').onclick = () => {
        prompt.remove();
        voicePreferenceSet = true;
        setVoiceListening(false, 'prompt');
    };
}

function maybePromptVoicePreference() {
    if (!features.isPro) {
        if (voiceListeningEnabled) {
            voiceListeningEnabled = false;
            saveVoiceListeningPreference();
            stopListening();
        }
        syncVoiceListeningUI();
        return;
    }

    if (voiceListeningEnabled) {
        ensureVoiceReady();
    }

    if (!voicePreferenceSet) {
        showVoicePreferencePrompt();
    } else {
        syncVoiceListeningUI();
    }
}

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
        localStorage.setItem('aide-mic-permission-skipped', Date.now().toString());
        pendingVoiceEnable = false;
        setVoiceListening(false, 'permission-skip');
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

        if (pendingVoiceEnable || voiceListeningEnabled) {
            pendingVoiceEnable = false;
            initVoiceActivation();
        }

    } catch (error) {
        log('error', `❌ Microphone permission denied: ${error.message}`);
        micPermissionGranted = false;
        pendingVoiceEnable = false;
        setVoiceListening(false, 'sync');

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
    if (!voiceListeningEnabled || !features.isPro) {
        return;
    }

    if (voiceActivationInitialized) {
        startListening();
        return;
    }

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
    voiceActivationInitialized = true;
    
    // Request microphone permission and start listening if enabled
    requestMicrophonePermission().then(granted => {
        if (granted && voiceListeningEnabled) {
            startListening();
        }
    });
    
    recognition.onresult = (event) => {
        for (let i = event.resultIndex; i < event.results.length; i++) {
            const result = event.results[i];
            const transcript = result[0].transcript.toLowerCase().trim();
            const confidence = result[0].confidence;

            // Skip low confidence results unless it's a wake word
            const wakeWordCheck = isWakeWord(transcript);
            if (confidence < 0.3 && !wakeWordCheck.detected) {
                continue;
            }
            
            // If wake word detected, trigger wake action immediately
            if (wakeWordCheck.detected) {
                log('info', `🎤 Wake word detected: "${wakeWordCheck.word}" (confidence: ${wakeWordCheck.confidence.toFixed(2)})`);
                ipcRenderer.send('voice-wake');
                
                // Extract command after wake word and process it
                const commandText = transcript.replace(wakeWordCheck.word, '').trim();
                if (commandText && voiceListeningEnabled) {
                    log('info', `🎤 Processing command after wake word: "${commandText}"`);
                    // Process command in same utterance
                    processVoiceCommand(commandText, confidence);
                }
                continue; // Don't process as regular command again
            }

            log('debug', `Voice result: "${transcript}" (confidence: ${confidence.toFixed(2)})`);

            // Process command (pattern matching or LLM)
            if (voiceListeningEnabled) {
                processVoiceCommand(transcript, confidence);
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
    // Free users can use wake/hide commands, Pro users get all commands
    if (!voiceListeningEnabled) return;
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
    syncVoiceListeningUI();
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
            const sidebarEl = document.getElementById('sidebar');
            const todoTabActive = document.querySelector('.sidebar-tab[data-tab="todo"]')?.classList.contains('active');

            if (commandName === 'openExplorer' && sidebarEl && !sidebarEl.classList.contains('collapsed')) {
                context.relevance = 0.4;
                context.reason = 'explorer already open';
            } else if (commandName === 'openTodo' && todoTabActive) {
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
                               type === 'processing' ? 'rgba(251, 191, 36, 0.9)' :
                               type === 'info' ? 'rgba(59, 130, 246, 0.9)' :
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
const animationStyle = document.createElement('style');
animationStyle.textContent = `
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
document.head.appendChild(animationStyle);

setTimeout(() => {
    loadVoicePrivacySettings();
    syncVoiceListeningUI();
}, 500);

// === FEATURE FLAGS & PRO/FREE MODE ===
async function initFeatures() {
    features = await ipcRenderer.invoke('get-features');
    
    // Update tier from features
    userTier = features.tier || 'free';
    localStorage.setItem('aide-tier', userTier);
    
    const terminalPanel = document.getElementById('terminalPanel');
    
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
    } else if (features.isPro) {
        // PRO: All features enabled
        console.log('⭐ AIDE Pro - All features unlocked!');
        document.body.classList.add('pro-mode');
    } else {
        // FREE: Limited features
        console.log('🆓 AIDE Free - Sidebar tabs include Files, Git, and Todo!');
        document.body.classList.add('free-mode');
        
        // Disable drag-drop rearrange in free mode
        document.querySelectorAll('.quad-panel .quad-header > span').forEach(el => {
            el.setAttribute('draggable', false);
            el.style.cursor = 'default';
        });
    }

    maybePromptVoicePreference();
    
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
    shell.openExternal(urls[type] || urls.onetime);
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

// === GAMIFICATION & STATS ===

async function updateGamificationStats(providedStats) {
    try {
        const stats = providedStats || await ipcRenderer.invoke('get-gamification-stats');
        if (!stats) return;

        // Update Settings Stats
        const levelEl = document.getElementById('statsLevel');
        const streakEl = document.getElementById('statsStreak');
        const xpEl = document.getElementById('statsXp');
        const accessoriesEl = document.getElementById('statsAccessories');
        
        if (levelEl) levelEl.innerText = stats.level;
        if (streakEl) streakEl.innerText = stats.streak;
        if (xpEl) xpEl.innerText = stats.xp;
        
        // Achievements/Accessories
        if (accessoriesEl) {
            accessoriesEl.innerHTML = '';
            
            const allAccessories = [
                { id: 'antenna', icon: 'fa-wifi', title: 'Antenna (Default)', secret: false },
                { id: 'flower', icon: 'fa-leaf', title: 'Flower (Lvl 5)', secret: false },
                { id: 'headphones', icon: 'fa-headphones', title: 'Headphones (Lvl 10)', secret: false },
                { id: 'chain', icon: 'fa-link', title: 'Gold Chain (Lvl 25)', secret: false },
                { id: 'crown', icon: 'fa-crown', title: 'Crown (Lvl 50)', secret: false },
                // Secret items
                { id: 'propeller', icon: 'fa-fan', title: '???', secret: true },
                { id: 'sunglasses', icon: 'fa-glasses', title: '???', secret: true },
                { id: 'visor', icon: 'fa-vr-cardboard', title: '???', secret: true }
            ];
            
            allAccessories.forEach(acc => {
                const unlocked = stats.accessories.includes(acc.id);
                const div = document.createElement('div');
                
                if (unlocked) {
                    div.className = 'achievement-badge unlocked';
                    div.title = acc.secret ? acc.title.replace('???', 'Secret Unlocked!') : acc.title; // Reveal title if secret
                    // Restore real titles for unlocked secrets
                    if(acc.id === 'propeller') div.title = 'Propeller Hat (First Share)';
                    if(acc.id === 'sunglasses') div.title = 'Sunglasses (Social Butterfly)';
                    if(acc.id === 'visor') div.title = 'Cyber Visor (Night Owl)';
                } else {
                    div.className = 'achievement-badge' + (acc.secret ? ' secret-locked' : '');
                    div.title = acc.secret ? 'Secret Achievement' : acc.title;
                }
                
                div.innerHTML = '<i class="fas ' + (unlocked || !acc.secret ? acc.icon : 'fa-question') + '"></i>';
                accessoriesEl.appendChild(div);
            });
        }

        // Update Welcome Screen Badge
        const welcomeBadge = document.getElementById('welcomeLevelBadge');
        const welcomeLevel = document.getElementById('welcomeLevelNum');
        const welcomeXpText = document.getElementById('welcomeXpText');
        const welcomeXpBar = document.getElementById('welcomeXpBar');

        if (welcomeBadge) {
            welcomeBadge.style.display = 'flex';
            if (welcomeLevel) welcomeLevel.innerText = stats.level;
            if (welcomeXpText) welcomeXpText.innerText = stats.xp + ' / ' + stats.nextLevelXp + ' XP';
            
            if (welcomeXpBar) {
                const percent = Math.min(100, Math.round((stats.xp / stats.nextLevelXp) * 100));
                welcomeXpBar.style.width = percent + '%';
            }
        }

    } catch (e) {
        console.error('Failed to update gamification stats:', e);
    }
}

// Call on load
setTimeout(() => updateGamificationStats(), 1000);

// Listen for updates (re-fetch when something happens)
ipcRenderer.on('update-gamification', (event, stats) => {
    updateGamificationStats(stats);
});

// Listen for unlock notifications
ipcRenderer.on('unlock-result', (event, result) => {
    if (result.success && result.unlockedItem) {
        showUnlockNotification(result.unlockedItem);
    }
});

// Show unlock notification
function showUnlockNotification(item) {
    const notification = document.createElement('div');
    notification.className = 'unlock-notification';
    notification.innerHTML = `
        <div class="unlock-icon">${item.icon || '🎁'}</div>
        <div class="unlock-text">
            <div class="unlock-title">NEW DRIP UNLOCKED!</div>
            <div class="unlock-name">${item.name}</div>
            <div class="unlock-rarity ${item.rarity || 'common'}">${(item.rarity || 'common').toUpperCase()}</div>
        </div>
    `;
    document.body.appendChild(notification);
    
    // Animate in
    setTimeout(() => notification.classList.add('show'), 10);
    
    // Remove after 4 seconds
    setTimeout(() => {
        notification.classList.remove('show');
        setTimeout(() => notification.remove(), 500);
    }, 4000);
    
    log('info', `🎉 Unlocked: ${item.name} (${item.rarity})`);
}

// ===== GAMIFICATION TRACKING SETUP =====

// Konami code listener
document.addEventListener('keydown', (e) => {
    konamiSequence.push(e.key);
    if (konamiSequence.length > KONAMI_CODE.length) {
        konamiSequence.shift();
    }
    
    if (konamiSequence.join(',') === KONAMI_CODE.join(',')) {
        log('info', '🎮 KONAMI CODE ENTERED!');
        ipcRenderer.send('konami-code');
        konamiSequence = [];
    }
    
    // Track undos for Regret achievement (Ctrl/Cmd + Z)
    if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
        ipcRenderer.send('track-undo');
    }
});

// Track scroll for Nessie unlock
let lastScrollTop = 0;
document.addEventListener('scroll', (e) => {
    const scrollDelta = Math.abs(document.documentElement.scrollTop - lastScrollTop);
    if (scrollDelta > 0) {
        scrollLineBuffer += Math.ceil(scrollDelta / 20); // Approximate lines
        lastScrollTop = document.documentElement.scrollTop;
        
        // Send batch updates every 100 lines
        if (scrollLineBuffer >= 100) {
            ipcRenderer.send('track-scroll', scrollLineBuffer);
            scrollLineBuffer = 0;
        }
    }
}, { passive: true });

// Track cursor moves for Bigfoot (throttled)
let cursorMoveThrottle = 0;
document.addEventListener('mousemove', () => {
    cursorMoveThrottle++;
    if (cursorMoveThrottle >= 50) { // Batch 50 moves
        ipcRenderer.send('track-cursor-move');
        cursorMoveThrottle = 0;
    }
}, { passive: true });

// Periodic time-based checks (every 5 minutes)
setInterval(() => {
    ipcRenderer.send('check-time-unlocks');
}, 5 * 60 * 1000);

// Initial time check
setTimeout(() => {
    ipcRenderer.send('check-time-unlocks');
}, 5000);

// Track open files count for Kraken
function updateOpenFileCount() {
    const count = openFiles.size;
    if (count >= 50) { // Start checking at 50
        ipcRenderer.send('track-open-files', count);
    }
}

// Add tab listener for settings to refresh stats when opened
if (document.getElementById('settingsBtn')) {
    document.getElementById('settingsBtn').addEventListener('click', () => {
        setTimeout(() => updateGamificationStats(), 100);
    });
}

// ============================================
// FILE SEARCH/FILTER
// Filter files in the explorer as user types
// ============================================
const fileSearchInput = document.getElementById('fileSearchInput');
if (fileSearchInput) {
    fileSearchInput.addEventListener('input', (e) => {
        const searchTerm = e.target.value.toLowerCase().trim();
        const fileItems = document.querySelectorAll('#fileTree .file-item');
        
        fileItems.forEach(item => {
            const fileName = item.textContent.toLowerCase();
            if (searchTerm === '' || fileName.includes(searchTerm)) {
                item.style.display = '';
            } else {
                item.style.display = 'none';
            }
        });
    });
    
    // Clear search on Escape
    fileSearchInput.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            fileSearchInput.value = '';
            fileSearchInput.dispatchEvent(new Event('input'));
            fileSearchInput.blur();
        }
    });
}

// ============================================
// EXTERNAL DROP HANDLER
// Handle content dropped on the mascot from other apps
// ============================================
ipcRenderer.on('external-drop', (event, data) => {
    console.log('Received external drop:', data);
    
    if (data.files && data.files.length > 0) {
        // Files were dropped - open the first one
        const filePath = data.files[0];
        if (filePath) {
            console.log('Opening dropped file:', filePath);
            window.openFile(filePath);
        }
    } else if (data.text && data.text.trim()) {
        // Text was dropped - insert into current editor
        const text = data.text.trim();
        console.log('Inserting dropped text:', text.substring(0, 50) + '...');
        
        if (window.monacoEditor) {
            // Insert at cursor position
            const selection = window.monacoEditor.getSelection();
            const range = new monaco.Range(
                selection.startLineNumber,
                selection.startColumn,
                selection.endLineNumber,
                selection.endColumn
            );
            
            window.monacoEditor.executeEdits('external-drop', [{
                range: range,
                text: text,
                forceMoveMarkers: true
            }]);
            
            // Focus the editor
            window.monacoEditor.focus();
        } else {
            // No editor open - create a new file with the content
            console.log('No editor open - creating new file with dropped content');
            // Could trigger new file creation here
        }
    }
});

