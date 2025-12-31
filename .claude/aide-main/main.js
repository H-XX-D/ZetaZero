/**
 * AIDE - AI Development Environment
 * ==================================
 * Silver mascot with antenna on head, mouth points to screen edge
 * Rotates based on dock position
 * 
 * Copyright (C) 2024 ORKASTRATOR
 * Licensed under GPL-2.0
 */

const { app, BrowserWindow, Tray, Menu, screen, ipcMain, systemPreferences, nativeImage, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const LicenseManager = require('./license');
const GamificationManager = require('./gamification');

let mascotWindow = null;
let mainWindow = null;
let edgeZoneWindow = null; // Invisible hover zone at screen edge
let tray = null;
let licenseManager = null;
let gamificationManager = null;
let popoutEditors = []; // Pop-out editor windows

// State
let state = 'hidden';  // 'hidden', 'peeking', 'open', 'fully-hidden'
let currentEdge = 'right';  // 'right', 'left', 'bottom', 'top'
let isQuitting = false;  // True when actually quitting (Cmd+Q or dock quit)
let longPressTimer = null;  // Timer for long-press hide detection

// Remember window size and position
let savedWindowBounds = null;

// Dee Dee's nickname
const NICKNAME = 'Dee Dee';
const WAKE_PHRASES = ['aye dee dee', 'hey dee dee', 'a d d', 'aide', 'ay dee dee'];

// Sizes
const ANTENNA_SIZE = 18;
const MASCOT_WIDTH = 90;
const MASCOT_HEIGHT = 120;
const SLIDE_AMOUNT = 75;

const IDE_WIDTH = 1400;
const IDE_HEIGHT = 900;

function getScreenBounds() {
    const display = screen.getPrimaryDisplay();
    const bounds = display.workArea;  // Use workArea which includes x,y position
    console.log('Screen bounds:', bounds);
    return bounds;
}

function getMascotHTML() {
    // Dee Dee: Silver HORIZONTAL pill with eye-mouth-eye
    // His head is ALWAYS horizontal (wide pill shape)
    // Long side (chin) against screen edge
    // Antenna always on TOP of his head (relative to his face orientation)
    // For RIGHT edge: he's rotated so his chin is against right, antenna points LEFT
    
    let mouthChar = '›';
    let hideX = 0, hideY = 0;
    
    // Mouth points toward edge he's hiding on
    switch (currentEdge) {
        case 'right':
            mouthChar = '›';
            hideX = 70;  // Hide off right
            break;
        case 'left':
            mouthChar = '‹';
            hideX = -70; // Hide off left
            break;
        case 'top':
            mouthChar = '∧';
            hideY = -70; // Hide off top
            break;
        case 'bottom':
            mouthChar = '∨';
            hideY = 70;  // Hide off bottom
            break;
    }
    
    return `<!DOCTYPE html>
<html>
<head>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body { 
    background: transparent !important; 
    overflow: visible;
    width: 100%;
    height: 100%;
}

.container {
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    background: transparent !important;
}

.mascot-wrapper {
    transform: translate(${hideX}px, ${hideY}px);
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.mascot-wrapper:hover,
.mascot-wrapper.peeking {
    transform: translate(0, 0);
}

/* Mascot - HORIZONTAL pill, antenna on top */
.mascot {
    display: flex;
    flex-direction: column;
    align-items: center;
    cursor: pointer;
}

.mascot:hover { filter: brightness(1.1); }

/* Purple glowing antenna on TOP */
.antenna {
    display: flex;
    flex-direction: column;
    align-items: center;
    margin-bottom: -3px;
}

.antenna-ball {
    width: 22px;
    height: 22px;
    background: radial-gradient(circle at 30% 30%, #e0b0ff, #9333ea, #6b21a8);
    border-radius: 50%;
    box-shadow: 0 0 18px #a855f7, 0 0 35px rgba(168, 85, 247, 0.7);
    animation: pulse 2s ease-in-out infinite;
}

.antenna-stem {
    width: 4px;
    height: 12px;
    background: linear-gradient(to bottom, #9333ea, #888);
    border-radius: 2px;
}

@keyframes pulse {
    0%, 100% { box-shadow: 0 0 15px #a855f7, 0 0 30px rgba(168, 85, 247, 0.5); }
    50% { box-shadow: 0 0 25px #a855f7, 0 0 50px rgba(168, 85, 247, 0.8); }
}

/* Silver HORIZONTAL pill body - BIGGER */
.body {
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: center;
    gap: 8px;
    background: linear-gradient(180deg, #f5f5f5 0%, #d0d0d0 25%, #b0b0b0 75%, #c8c8c8 100%);
    border: 3px solid #999;
    border-radius: 28px;
    padding: 10px 18px;
    box-shadow: 
        0 4px 15px rgba(0,0,0,0.4), 
        inset 0 3px 5px rgba(255,255,255,0.6),
        inset 0 -2px 4px rgba(0,0,0,0.1);
}

.body:hover {
    background: linear-gradient(180deg, #fff 0%, #e0e0e0 25%, #c0c0c0 75%, #d8d8d8 100%);
}

/* Eyes - BIGGER */
.eye {
    width: 16px;
    height: 16px;
    background: radial-gradient(circle at 30% 30%, #fff, #e8e8e8);
    border: 3px solid #333;
    border-radius: 50%;
    position: relative;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.15);
}

.eye::after {
    content: '';
    position: absolute;
    width: 7px;
    height: 7px;
    background: #1a1a2e;
    border-radius: 50%;
    top: 3px;
    left: 3px;
}

/* Mouth */
.mouth {
    font-size: 18px;
    font-weight: bold;
    color: #444;
    line-height: 1;
}

/* Blink on hover */
@keyframes blink {
    0%, 90%, 100% { transform: scaleY(1); }
    95% { transform: scaleY(0.1); }
}
.mascot:hover .eye { animation: blink 2s infinite; }
</style>
</head>
<body>
<div class="container">
    <div class="mascot-wrapper" id="wrapper">
        <div class="mascot" id="mascot">
            <div class="antenna">
                <div class="antenna-ball"></div>
                <div class="antenna-stem"></div>
            </div>
            <div class="body">
                <div class="eye"></div>
                <div class="mouth">${mouthChar}</div>
                <div class="eye"></div>
            </div>
        </div>
    </div>
</div>
<script>
const { ipcRenderer } = require('electron');
const wrapper = document.getElementById('wrapper');
const mascot = document.getElementById('mascot');

// Click to open IDE
mascot.addEventListener('click', () => ipcRenderer.send('mascot-clicked'));

// Hover to peek
document.body.addEventListener('mouseenter', () => wrapper.classList.add('peeking'));
document.body.addEventListener('mouseleave', () => wrapper.classList.remove('peeking'));
</script>
</body>
</html>`;
}

function getMascotPosition(hidden = true) {
    const bounds = getScreenBounds();
    
    // Classic AI head - bigger with room for antenna
    const W = 90;
    const H = 120; // Taller for antenna + drip
    
    const screenW = bounds.width;
    const screenH = bounds.height;
    
    // Grammarly style: when hidden, only 15px visible at edge
    // When peeking, full mascot visible
    const visibleWhenHidden = 15;
    const hiddenOffset = W - visibleWhenHidden;
    
    switch (currentEdge) {
        case 'right':
            return { 
                x: hidden ? screenW - visibleWhenHidden : screenW - W, 
                y: Math.round((screenH - H) / 2), 
                width: W, 
                height: H 
            };
        case 'left':
            return { 
                x: hidden ? -hiddenOffset : 0, 
                y: Math.round((screenH - H) / 2), 
                width: W, 
                height: H 
            };
        case 'top':
            return { 
                x: Math.round((screenW - W) / 2), 
                y: hidden ? -hiddenOffset : 0, 
                width: W, 
                height: H 
            };
        case 'bottom':
            return { 
                x: Math.round((screenW - W) / 2), 
                y: hidden ? screenH - visibleWhenHidden : screenH - H, 
                width: W, 
                height: H 
            };
        default:
            return { 
                x: hidden ? screenW - visibleWhenHidden : screenW - W, 
                y: Math.round((screenH - H) / 2), 
                width: W, 
                height: H 
            };
    }
}

// Slide mascot in/out
function slideMascot(show) {
    if (!mascotWindow) return;
    
    const pos = getMascotPosition(!show); // hidden = !show
    mascotWindow.setBounds({ x: pos.x, y: pos.y, width: pos.width, height: pos.height }, true);
}

function createMascotWindow() {
    const pos = getMascotPosition(true); // Start hidden
    
    console.log('Creating mascot window at:', pos);
    
    mascotWindow = new BrowserWindow({
        width: pos.width,
        height: pos.height,
        x: pos.x,
        y: pos.y,
        frame: false,
        transparent: true, // Transparent for dynamic drip background
        alwaysOnTop: true,
        skipTaskbar: true,
        resizable: false,
        hasShadow: false, // No shadow - mascot-bg has its own
        focusable: false, // Don't steal focus from other apps
        roundedCorners: true,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    // Keep mascot floating above all windows on all workspaces
    mascotWindow.setAlwaysOnTop(true, 'floating');
    mascotWindow.setVisibleOnAllWorkspaces(true, { visibleOnFullScreen: true });
    
    // Load from file
    mascotWindow.loadFile('mascot.html');
    
    // Send edge info once loaded
    mascotWindow.webContents.on('did-finish-load', () => {
        mascotWindow.webContents.send('set-edge', currentEdge);
        // Update accessories based on saved stats
        if (gamificationManager) {
            const classes = gamificationManager.getAccessoryClasses();
            mascotWindow.webContents.send('update-accessories', classes);
        }
    });
    
    // Allow click-through on transparent areas but capture mouse on the mascot itself
    // This helps with hover detection even when not focused
    mascotWindow.setIgnoreMouseEvents(false);
    
    mascotWindow.on('closed', () => {
        mascotWindow = null;
    });
}

// Create invisible edge zone for hover detection when mascot is fully hidden
function createEdgeZone() {
    if (edgeZoneWindow && !edgeZoneWindow.isDestroyed()) {
        return; // Already exists
    }
    
    const { x, y, width, height } = getScreenBounds();
    
    // Thin strip at the edge - extends to absolute screen edge (1px wide/tall for maximum coverage)
    let zoneX, zoneY, zoneW, zoneH;
    switch (currentEdge) {
        case 'right':
            zoneX = x + width - 1;  // Extend to absolute right edge
            zoneY = y;
            zoneW = 1;  // Minimal width but covers entire edge
            zoneH = height;
            break;
        case 'left':
            zoneX = x;  // Start at absolute left edge
            zoneY = y;
            zoneW = 1;
            zoneH = height;
            break;
        case 'top':
            zoneX = x;
            zoneY = y;  // Start at absolute top edge
            zoneW = width;
            zoneH = 1;
            break;
        case 'bottom':
            zoneX = x;
            zoneY = y + height - 1;  // Extend to absolute bottom edge
            zoneW = width;
            zoneH = 1;
            break;
    }
    
    edgeZoneWindow = new BrowserWindow({
        x: zoneX,
        y: zoneY,
        width: zoneW,
        height: zoneH,
        frame: false,
        transparent: true,
        alwaysOnTop: true,
        skipTaskbar: true,
        resizable: false,
        focusable: false,
        hasShadow: false,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    edgeZoneWindow.setAlwaysOnTop(true, 'screen-saver'); // Highest level
    edgeZoneWindow.setVisibleOnAllWorkspaces(true, { visibleOnFullScreen: true });
    edgeZoneWindow.setIgnoreMouseEvents(false);
    
    // Simple HTML that detects mouse enter
    edgeZoneWindow.loadURL(`data:text/html,
        <html>
        <head><style>
            * { margin: 0; padding: 0; }
            body { background: transparent; width: 100vw; height: 100vh; }
        </style></head>
        <body>
        <script>
            const { ipcRenderer } = require('electron');
            document.body.addEventListener('mouseenter', () => {
                ipcRenderer.send('edge-hover');
            });
        </script>
        </body>
        </html>
    `);
    
    edgeZoneWindow.on('closed', () => {
        edgeZoneWindow = null;
    });
}

function showEdgeZone() {
    createEdgeZone();
    if (edgeZoneWindow) {
        edgeZoneWindow.show();
    }
}

function hideEdgeZone() {
    if (edgeZoneWindow) {
        edgeZoneWindow.hide();
    }
}

function createMainWindow() {
    const { width, height } = getScreenBounds();
    
    let x, y;
    switch (currentEdge) {
        case 'right':
            x = width - IDE_WIDTH - 20;
            y = (height - IDE_HEIGHT) / 2;
            break;
        case 'left':
            x = 20;
            y = (height - IDE_HEIGHT) / 2;
            break;
        case 'bottom':
            x = (width - IDE_WIDTH) / 2;
            y = height - IDE_HEIGHT - 20;
            break;
        case 'top':
            x = (width - IDE_WIDTH) / 2;
            y = 20;
            break;
        default:
            x = width - IDE_WIDTH - 20;
            y = (height - IDE_HEIGHT) / 2;
    }
    
    mainWindow = new BrowserWindow({
        width: IDE_WIDTH,
        height: IDE_HEIGHT,
        x: Math.round(x),
        y: Math.round(y),
        frame: false,
        show: false,
        resizable: true,
        hasShadow: true,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    mainWindow.loadFile('index.html');
    
    // Intercept close to hide instead of quit (unless actually quitting)
    mainWindow.on('close', (e) => {
        if (!isQuitting) {
            e.preventDefault();
            closeIDE();
        }
    });
    
    mainWindow.on('closed', () => {
        mainWindow = null;
    });
    
    // Hide behavior based on closeBehavior setting (eye glow states)
    // 'single' (no glow) = single click off hides AIDE and Dee Dee, other app comes to front
    // 'single' (no glow) = single click off hides immediately  
    // 'double' (one eye glowing) = long-press off hides (stay away for 5 seconds)
    // 'minimize' (both eyes glowing) = only minimize button hides
    
    mainWindow.on('blur', () => {
        if (state !== 'open') return;
        
        console.log('Blur event, closeBehavior:', closeBehavior);
        
        if (closeBehavior === 'single') {
            // NO GLOW: Single click off - hide immediately
            console.log('Single click mode - hiding');
            closeIDEFully();
        } else if (closeBehavior === 'double') {
            // ONE EYE: Long-press off required
            // User can work in other apps - only hides after 5 seconds of no interaction with AIDE
            // Clicking back on AIDE resets the timer
            console.log('Long-press mode - stay away for 5 seconds to hide...');
            
            if (longPressTimer) clearTimeout(longPressTimer);
            
            longPressTimer = setTimeout(() => {
                console.log('Long-press detected - hiding');
                longPressTimer = null;
                closeIDEFully();
            }, 5000);
        }
        // 'minimize' (both eyes) = do nothing on blur
    });
    
    mainWindow.on('focus', () => {
        // User clicked back - cancel the long-press timer
        if (longPressTimer) {
            console.log('Focus regained - cancelling long-press timer');
            clearTimeout(longPressTimer);
            longPressTimer = null;
        }
    });
}

function showMascot() {
    console.log('showMascot called, mascotWindow exists:', !!mascotWindow);
    
    // Hide edge zone since mascot is now visible
    hideEdgeZone();
    
    if (!mascotWindow || mascotWindow.isDestroyed()) {
        console.log('Creating new mascot window');
        createMascotWindow();
    }
    
    // Make sure mascot is always on top so it's visible after clicking off
    mascotWindow.setAlwaysOnTop(true, 'floating');
    mascotWindow.show();
    
    // Small delay to ensure window is ready, then slide into hidden position
    setTimeout(() => {
        slideMascot(false);
        // Bring to front after a moment so user can see it
        mascotWindow.moveTop();
    }, 100);
    
    state = 'hidden';
    console.log('Mascot should be visible now');
}

function hideMascot() {
    if (mascotWindow) {
        mascotWindow.hide();
    }
}

let voiceChatWindow = null;

function openVoiceChat() {
    // Open small voice chat popup near the mascot
    if (voiceChatWindow && !voiceChatWindow.isDestroyed()) {
        voiceChatWindow.focus();
        return;
    }

    const { x, y, width, height } = getScreenBounds();

    // Position near mascot based on edge
    let chatX, chatY;
    switch (currentEdge) {
        case 'right':
            chatX = x + width - 420;
            chatY = y + Math.floor(height / 2) - 200;
            break;
        case 'left':
            chatX = x + 80;
            chatY = y + Math.floor(height / 2) - 200;
            break;
        case 'top':
            chatX = x + Math.floor(width / 2) - 180;
            chatY = y + 80;
            break;
        case 'bottom':
            chatX = x + Math.floor(width / 2) - 180;
            chatY = y + height - 480;
            break;
    }

    voiceChatWindow = new BrowserWindow({
        width: 360,
        height: 400,
        x: chatX,
        y: chatY,
        frame: false,
        transparent: false,
        backgroundColor: '#0d1117',
        alwaysOnTop: true,
        skipTaskbar: true,
        resizable: true,
        hasShadow: true,
        roundedCorners: true,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    voiceChatWindow.setAlwaysOnTop(true, 'floating');

    // Check if voice-chat.html exists, otherwise show placeholder
    const fs = require('fs');
    const path = require('path');
    const voiceChatPath = path.join(__dirname, 'voice-chat.html');

    if (fs.existsSync(voiceChatPath)) {
        voiceChatWindow.loadFile('voice-chat.html');
    } else {
        // Show placeholder message for now
        voiceChatWindow.loadURL(`data:text/html,
            <html>
            <head>
                <style>
                    body {
                        background: #0d1117;
                        color: #e6edf3;
                        font-family: -apple-system, BlinkMacSystemFont, sans-serif;
                        padding: 20px;
                        text-align: center;
                        display: flex;
                        flex-direction: column;
                        justify-content: center;
                        height: 100vh;
                        margin: 0;
                    }
                    .mascot {
                        font-size: 48px;
                        margin-bottom: 20px;
                    }
                    h2 {
                        color: #6366f1;
                        margin-bottom: 10px;
                    }
                    p {
                        color: #8b949e;
                        line-height: 1.5;
                    }
                </style>
            </head>
            <body>
                <div class="mascot">🤖</div>
                <h2>Voice Chat Coming Soon!</h2>
                <p>Dee Dee's voice chat feature is under development.<br>Double-click the mascot to open the full IDE instead.</p>
            </body>
            </html>
        `);
    }

    voiceChatWindow.on('closed', () => {
        voiceChatWindow = null;
    });

    voiceChatWindow.on('blur', () => {
        // Close voice chat on click off (unless pinned)
        if (voiceChatWindow && !voiceChatWindow.isDestroyed()) {
            voiceChatWindow.close();
        }
    });
}

function openIDE() {
    hideMascot();
    
    // Close voice chat if open
    if (voiceChatWindow && !voiceChatWindow.isDestroyed()) {
        voiceChatWindow.close();
    }
    
    if (!mainWindow) {
        createMainWindow();
    }
    
    // Restore saved bounds if available
    if (savedWindowBounds) {
        console.log('Restoring window bounds:', savedWindowBounds);
        mainWindow.setBounds(savedWindowBounds);
    }
    
    mainWindow.show();
    mainWindow.focus();
    state = 'open';
}

function closeIDE() {
    // Close IDE but show mascot (used by minimize, close button, etc)
    // Dock icon always stays visible!
    console.log('closeIDE called - hiding AIDE, showing Dee Dee (dock stays)');
    if (mainWindow) {
        savedWindowBounds = mainWindow.getBounds();
        console.log('Saved window bounds:', savedWindowBounds);
        mainWindow.hide();
    }
    
    // Make sure dock icon stays visible on macOS
    if (process.platform === 'darwin') {
        app.dock.show();
    }
    
    // Show mascot after a moment
    setTimeout(() => {
        showMascot();
    }, 150);
    
    state = 'hidden';
}

function closeIDEFully() {
    // Close IDE AND hide mascot completely (no glow / double-click mode)
    // BUT keep dock icon visible!
    console.log('closeIDEFully called - hiding AIDE AND Dee Dee (dock stays)');
    if (mainWindow) {
        savedWindowBounds = mainWindow.getBounds();
        console.log('Saved window bounds:', savedWindowBounds);
        mainWindow.hide();
    }
    
    // Hide mascot too - user clicked off completely
    hideMascot();
    state = 'fully-hidden';
    
    // Make sure dock icon stays visible on macOS
    if (process.platform === 'darwin') {
        app.dock.show();
    }
    
    // Show invisible edge zone so user can hover to bring Dee Dee back
    showEdgeZone();
}

function createTray() {
    // Try to find an icon
    const iconPath = path.join(__dirname, 'assets', 'icon.png');
    
    if (!fs.existsSync(iconPath)) {
        console.log('No tray icon found, skipping tray');
        return;
    }
    
    try {
        // Load and resize icon for menu bar (16x16 on macOS)
        let trayImage = nativeImage.createFromPath(iconPath);
        
        // Resize to proper menu bar size
        if (process.platform === 'darwin') {
            trayImage = trayImage.resize({ width: 18, height: 18 });
            trayImage.setTemplateImage(true); // Makes it work with dark/light menu bar
        } else {
            trayImage = trayImage.resize({ width: 16, height: 16 });
        }
        
        tray = new Tray(trayImage);
    } catch (e) {
        console.log('Failed to create tray:', e.message);
        return;
    }
    
    const contextMenu = Menu.buildFromTemplate([
        { label: 'Open AIDE', click: () => openIDE() },
        { label: 'Show Dee Dee', click: () => showMascot() },
        { type: 'separator' },
        { 
            label: 'Dock Position',
            submenu: [
                { label: 'Right', type: 'radio', checked: currentEdge === 'right', click: () => setEdge('right') },
                { label: 'Left', type: 'radio', checked: currentEdge === 'left', click: () => setEdge('left') },
                { label: 'Bottom', type: 'radio', checked: currentEdge === 'bottom', click: () => setEdge('bottom') },
                { label: 'Top', type: 'radio', checked: currentEdge === 'top', click: () => setEdge('top') }
            ]
        },
        { type: 'separator' },
        { label: 'Reset Position', click: () => resetPosition() },
        { label: 'Restart AIDE', click: () => restartApp() },
        { type: 'separator' },
        { label: 'Quit', click: () => app.quit() }
    ]);
    
    tray.setToolTip('AIDE - Dee Dee');
    tray.setContextMenu(contextMenu);
    tray.on('click', () => openIDE());
}

function createAppMenu() {
    const isMac = process.platform === 'darwin';
    
    const template = [
        // App menu (macOS only)
        ...(isMac ? [{
            label: 'AIDE',
            submenu: [
                { label: 'About AIDE', role: 'about' },
                { type: 'separator' },
                { label: 'Show Dee Dee', accelerator: 'CmdOrCtrl+Shift+D', click: () => showMascot() },
                { label: 'Open IDE', accelerator: 'CmdOrCtrl+Shift+O', click: () => openIDE() },
                { type: 'separator' },
                { label: 'Preferences...', accelerator: 'CmdOrCtrl+,', click: () => {
                    if (mainWindow) mainWindow.webContents.send('open-settings');
                }},
                { type: 'separator' },
                { role: 'services' },
                { type: 'separator' },
                { role: 'hide' },
                { role: 'hideOthers' },
                { role: 'unhide' },
                { type: 'separator' },
                { role: 'quit' }
            ]
        }] : []),
        
        // File menu
        {
            label: 'File',
            submenu: [
                { label: 'New File', accelerator: 'CmdOrCtrl+N', click: () => {
                    if (mainWindow) mainWindow.webContents.send('new-file');
                }},
                { label: 'Open...', accelerator: 'CmdOrCtrl+O', click: () => {
                    if (mainWindow) mainWindow.webContents.send('open-file-dialog');
                }},
                { type: 'separator' },
                { label: 'Save', accelerator: 'CmdOrCtrl+S', click: () => {
                    if (mainWindow) mainWindow.webContents.send('save-file');
                    // Gamification: Add XP for saving files + track saves
                    if (gamificationManager) {
                        gamificationManager.addXP(10);
                        gamificationManager.trackFileSave();
                        updateMascotAccessories();
                    }
                }},
                { label: 'Save As...', accelerator: 'CmdOrCtrl+Shift+S', click: () => {
                    if (mainWindow) mainWindow.webContents.send('save-file-as');
                }},
                { type: 'separator' },
                ...(isMac ? [] : [{ role: 'quit' }])
            ]
        },
        
        // Edit menu
        {
            label: 'Edit',
            submenu: [
                { role: 'undo' },
                { role: 'redo' },
                { type: 'separator' },
                { role: 'cut' },
                { role: 'copy' },
                { role: 'paste' },
                { role: 'selectAll' },
                { type: 'separator' },
                { label: 'Find', accelerator: 'CmdOrCtrl+F', click: () => {
                    if (mainWindow) mainWindow.webContents.send('find');
                }},
                { label: 'Replace', accelerator: 'CmdOrCtrl+H', click: () => {
                    if (mainWindow) mainWindow.webContents.send('replace');
                }}
            ]
        },
        
        // View menu
        {
            label: 'View',
            submenu: [
                { label: 'Toggle Sidebar', accelerator: 'CmdOrCtrl+B', click: () => {
                    if (mainWindow) mainWindow.webContents.send('toggle-sidebar');
                }},
                { label: 'Toggle TODO', accelerator: 'CmdOrCtrl+Shift+T', click: () => {
                    if (mainWindow) mainWindow.webContents.send('toggle-todo');
                }},
                { type: 'separator' },
                { label: 'Toggle Terminal', accelerator: 'CmdOrCtrl+`', click: () => {
                    if (mainWindow) mainWindow.webContents.send('toggle-terminal');
                }},
                { type: 'separator' },
                { role: 'reload' },
                { role: 'forceReload' },
                { role: 'toggleDevTools' },
                { type: 'separator' },
                { role: 'resetZoom' },
                { role: 'zoomIn' },
                { role: 'zoomOut' },
                { type: 'separator' },
                { role: 'togglefullscreen' }
            ]
        },
        
        // Dee Dee menu
        {
            label: 'Dee Dee',
            submenu: [
                { label: 'Show Dee Dee', accelerator: 'CmdOrCtrl+Shift+D', click: () => showMascot() },
                { label: 'Hide Dee Dee', click: () => hideMascot() },
                { type: 'separator' },
                { 
                    label: 'Position',
                    submenu: [
                        { label: 'Right Edge', type: 'radio', checked: currentEdge === 'right', click: () => setEdge('right') },
                        { label: 'Left Edge', type: 'radio', checked: currentEdge === 'left', click: () => setEdge('left') },
                        { label: 'Top Edge', type: 'radio', checked: currentEdge === 'top', click: () => setEdge('top') },
                        { label: 'Bottom Edge', type: 'radio', checked: currentEdge === 'bottom', click: () => setEdge('bottom') }
                    ]
                },
                { type: 'separator' },
                { label: 'Reset Position', click: () => resetPosition() }
            ]
        },
        
        // Window menu
        {
            label: 'Window',
            submenu: [
                { role: 'minimize' },
                { role: 'zoom' },
                ...(isMac ? [
                    { type: 'separator' },
                    { role: 'front' },
                    { type: 'separator' },
                    { role: 'window' }
                ] : [
                    { role: 'close' }
                ])
            ]
        },
        
        // Help menu
        {
            label: 'Help',
            submenu: [
                { label: 'Voice Commands', click: () => {
                    if (mainWindow) mainWindow.webContents.send('show-voice-help');
                }},
                { label: 'Keyboard Shortcuts', accelerator: 'CmdOrCtrl+/', click: () => {
                    if (mainWindow) mainWindow.webContents.send('show-shortcuts');
                }},
                { type: 'separator' },
                { label: 'AIDE Website', click: () => {
                    require('electron').shell.openExternal('https://aide.dev');
                }},
                { label: 'Report Issue', click: () => {
                    require('electron').shell.openExternal('https://github.com/aide/aide/issues');
                }}
            ]
        }
    ];
    
    const menu = Menu.buildFromTemplate(template);
    Menu.setApplicationMenu(menu);
    console.log('App menu created');
}

function resetPosition() {
    // Reset mascot to default position
    currentEdge = 'right';
    savedWindowBounds = null;
    state = 'hidden';
    closeBehavior = 'single';
    
    if (mascotWindow && !mascotWindow.isDestroyed()) {
        mascotWindow.destroy();
        mascotWindow = null;
    }
    if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.destroy();
        mainWindow = null;
    }
    if (edgeZoneWindow && !edgeZoneWindow.isDestroyed()) {
        edgeZoneWindow.destroy();
        edgeZoneWindow = null;
    }
    
    showMascot();
    console.log('Position reset to defaults');
}

function restartApp() {
    app.relaunch();
    app.exit(0);
}

function setEdge(edge) {
    currentEdge = edge;
    if (mascotWindow) {
        mascotWindow.destroy();
        mascotWindow = null;
    }
    showMascot();
}

// Helper to update mascot accessories across windows
function updateMascotAccessories() {
    if (gamificationManager) {
        // Update Mascot Window (Full data: unlocked items, equipped, secret count)
        if (mascotWindow && !mascotWindow.isDestroyed()) {
            const mascotData = gamificationManager.getMascotData();
            mascotWindow.webContents.send('update-accessories', mascotData);
        }
        
        // Update Main Window (Stats UI)
        if (mainWindow && !mainWindow.isDestroyed()) {
            mainWindow.webContents.send('update-gamification', gamificationManager.getStats());
        }
    }
}

// Pin state
let isPinned = false;

// Close behavior: 'single', 'double', 'minimize'
let closeBehavior = 'single';

// IPC Handlers
ipcMain.on('mascot-peek', () => {
    state = 'peeking';
});

ipcMain.on('mascot-hide', () => {
    state = 'hidden';
});

// Single click on mascot = open AI voice chat popup (Pro only) or IDE (Free)
ipcMain.on('mascot-single-clicked', () => {
    const features = licenseManager ? licenseManager.getFeatures() : { isPro: false };
    if (features.isPro || features.isProPlus) {
        console.log('Single click (Pro) - opening AI voice chat');
        openVoiceChat();
    } else {
        console.log('Single click (Free) - opening IDE');
        openIDE();
    }
});

// Double click on mascot = open full IDE
ipcMain.on('mascot-double-clicked', () => {
    console.log('Double click - opening full IDE');
    openIDE();
});

// Legacy support
ipcMain.on('mascot-clicked', () => {
    openIDE();
});

// Grammarly-style slide in/out on hover
ipcMain.on('mascot-hover', (event, isHovering) => {
    slideMascot(isHovering);
});

// Edge hover - brings back mascot when fully hidden
ipcMain.on('edge-hover', () => {
    console.log('Edge hover detected, state:', state);
    if (state === 'fully-hidden') {
        // User hovered at edge - bring back Dee Dee
        console.log('Bringing Dee Dee back from edge hover');
        hideEdgeZone();
        showMascot();
    }
});

// Drag-to-show: When user drags something over the mascot, show the app
ipcMain.on('drag-over-mascot', (event, isDragging) => {
    console.log('Drag over mascot detected - showing app for drop');
    
    // Cancel any pending hide timer
    if (longPressTimer) {
        clearTimeout(longPressTimer);
        longPressTimer = null;
    }
    
    // Show the app so user can drop into it
    if (mainWindow && !mainWindow.isVisible()) {
        openIDE();
    } else if (mainWindow) {
        mainWindow.focus();
    }
});

// Handle drop on mascot - forward to the app
ipcMain.on('drop-on-mascot', (event, data) => {
    console.log('Drop on mascot:', data);
    
    // Make sure app is visible
    if (mainWindow && !mainWindow.isVisible()) {
        openIDE();
    }
    
    // Forward the dropped content to the renderer
    if (mainWindow && mainWindow.webContents) {
        setTimeout(() => {
            mainWindow.webContents.send('external-drop', data);
        }, 300); // Small delay to let window appear
    }
});

ipcMain.on('get-edge', (event) => {
    event.sender.send('apply-hide', currentEdge);
});

// Open 3D Print Generator
ipcMain.on('open-3d-print', (event, data) => {
    const { code } = data || {};
    const printWindow = new BrowserWindow({
        width: 1000,
        height: 700,
        title: '3D Print Dee Dee',
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    // Load with code parameter
    const filePath = path.join(__dirname, '3d-print', 'generator.html');
    if (code) {
        printWindow.loadFile(filePath, { query: { code } });
    } else {
        printWindow.loadFile(filePath);
    }
});

// Legacy handler for interest form
ipcMain.on('open-3d-print-interest', (event, data) => {
    const printWindow = new BrowserWindow({
        width: 1000,
        height: 700,
        title: '3D Print Dee Dee',
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    printWindow.loadFile('3d-print/generator.html');
});

// Pop-out editor window
ipcMain.on('popout-editor', (event, data) => {
    const { filePath, content, language } = data;
    const fileName = filePath ? path.basename(filePath) : 'untitled';
    
    const popoutWindow = new BrowserWindow({
        width: 800,
        height: 600,
        title: `${fileName} - AIDE Editor`,
        alwaysOnTop: true,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    popoutWindow.setAlwaysOnTop(true, 'floating');
    
    // Load editor HTML with the file content
    popoutWindow.loadURL(`data:text/html,${encodeURIComponent(`
<!DOCTYPE html>
<html>
<head>
    <title>${fileName}</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            background: #1e1e1e; 
            color: #d4d4d4;
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            height: 100vh;
            display: flex;
            flex-direction: column;
        }
        .titlebar {
            height: 32px;
            background: #252526;
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 0 12px;
            -webkit-app-region: drag;
            border-bottom: 1px solid #3c3c3c;
        }
        .titlebar-title {
            font-size: 12px;
            color: #ccc;
        }
        .titlebar-controls {
            display: flex;
            gap: 8px;
            -webkit-app-region: no-drag;
        }
        .titlebar-btn {
            background: none;
            border: none;
            color: #888;
            cursor: pointer;
            padding: 4px 8px;
            border-radius: 4px;
        }
        .titlebar-btn:hover { background: #3c3c3c; color: #fff; }
        .split-controls {
            display: flex;
            gap: 4px;
            margin-right: 12px;
            -webkit-app-region: no-drag;
        }
        .split-btn {
            background: #3c3c3c;
            border: none;
            color: #888;
            cursor: pointer;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 11px;
        }
        .split-btn:hover { background: #4c4c4c; color: #fff; }
        .editor-container {
            flex: 1;
            display: flex;
        }
        .editor-pane {
            flex: 1;
            position: relative;
        }
        .editor-pane + .editor-pane {
            border-left: 2px solid #3c3c3c;
        }
        #editor1, #editor2 { width: 100%; height: 100%; }
        .pin-active { color: #22c55e !important; }
    </style>
</head>
<body>
    <div class="titlebar">
        <span class="titlebar-title">${fileName}</span>
        <div style="display: flex; align-items: center;">
            <div class="split-controls">
                <button class="split-btn" onclick="toggleSplit()" title="Toggle Split View">⊞ Split</button>
            </div>
            <div class="titlebar-controls">
                <button class="titlebar-btn" id="pinBtn" onclick="togglePin()" title="Toggle Always on Top">📌</button>
                <button class="titlebar-btn" onclick="saveFile()" title="Save">💾</button>
            </div>
        </div>
    </div>
    <div class="editor-container">
        <div class="editor-pane" id="pane1">
            <div id="editor1"></div>
        </div>
        <div class="editor-pane" id="pane2" style="display: none;">
            <div id="editor2"></div>
        </div>
    </div>
    
    <script src="https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs/loader.min.js"></script>
    <script>
        const { ipcRenderer } = require('electron');
        let editor1, editor2;
        let isPinned = true;
        let isSplit = false;
        const filePath = ${JSON.stringify(filePath)};
        const initialContent = ${JSON.stringify(content)};
        const lang = ${JSON.stringify(language)};
        
        require.config({ paths: { vs: 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs' }});
        
        require(['vs/editor/editor.main'], function() {
            editor1 = monaco.editor.create(document.getElementById('editor1'), {
                value: initialContent,
                language: lang,
                theme: 'vs-dark',
                minimap: { enabled: false },
                fontSize: 13,
                automaticLayout: true
            });
            
            editor2 = monaco.editor.create(document.getElementById('editor2'), {
                value: initialContent,
                language: lang,
                theme: 'vs-dark',
                minimap: { enabled: false },
                fontSize: 13,
                automaticLayout: true
            });
        });
        
        function toggleSplit() {
            isSplit = !isSplit;
            document.getElementById('pane2').style.display = isSplit ? 'block' : 'none';
        }
        
        function togglePin() {
            isPinned = !isPinned;
            ipcRenderer.send('popout-set-pin', isPinned);
            document.getElementById('pinBtn').classList.toggle('pin-active', isPinned);
        }
        
        function saveFile() {
            if (filePath && editor1) {
                ipcRenderer.send('popout-save', { filePath, content: editor1.getValue() });
            }
        }
        
        // Keyboard shortcut
        document.addEventListener('keydown', (e) => {
            if ((e.metaKey || e.ctrlKey) && e.key === 's') {
                e.preventDefault();
                saveFile();
            }
        });
    </script>
</body>
</html>
    `)}`);
    
    popoutEditors.push(popoutWindow);
    
    popoutWindow.on('closed', () => {
        popoutEditors = popoutEditors.filter(w => w !== popoutWindow);
    });
});

// Pop-out editor pin toggle
ipcMain.on('popout-set-pin', (event, isPinned) => {
    const win = BrowserWindow.fromWebContents(event.sender);
    if (win) {
        win.setAlwaysOnTop(isPinned, 'floating');
    }
});

// Pop-out editor save
ipcMain.on('popout-save', (event, data) => {
    const { filePath, content } = data;
    if (filePath) {
        fs.writeFileSync(filePath, content, 'utf8');
        console.log('Popout saved:', filePath);
        // Notify main window to refresh
        if (mainWindow && !mainWindow.isDestroyed()) {
            mainWindow.webContents.send('file-updated-externally', filePath);
        }
    }
});

// Toggle voice listening
ipcMain.on('toggle-listening', (event, isListening) => {
    console.log('Listening toggled:', isListening);
    // TODO: Connect to actual voice system
});

// Hide mascot from context menu
ipcMain.on('hide-mascot', () => {
    if (mascotWindow && !mascotWindow.isDestroyed()) {
        mascotWindow.hide();
    }
});

// Show native context menu for mascot
ipcMain.on('show-mascot-menu', (event, data) => {
    const { Menu } = require('electron');
    const { isListening, unlockedItems, equippedIds, secretCount } = data;
    
    const unlockedCount = unlockedItems ? unlockedItems.length : 0;
    const totalCount = unlockedCount + (secretCount || 0);
    
    // Build wardrobe submenu
    const wardrobeItems = [];
    if (unlockedItems && unlockedItems.length > 0) {
        for (const item of unlockedItems) {
            const isEquipped = equippedIds && equippedIds.includes(item.id);
            wardrobeItems.push({
                label: `${item.icon} ${item.name}`,
                type: 'checkbox',
                checked: isEquipped,
                click: () => {
                    if (mascotWindow && !mascotWindow.isDestroyed()) {
                        mascotWindow.webContents.send('toggle-accessory', item.id);
                    }
                }
            });
        }
    }
    
    if (secretCount > 0) {
        wardrobeItems.push({ type: 'separator' });
        wardrobeItems.push({
            label: `🔮 ${secretCount} more to discover...`,
            enabled: false
        });
    }
    
    const menuTemplate = [
        {
            label: isListening ? '🎤 Listening...' : '🔇 Not Listening',
            click: () => {
                if (mascotWindow && !mascotWindow.isDestroyed()) {
                    mascotWindow.webContents.send('toggle-listening-from-menu');
                }
            }
        },
        { type: 'separator' },
        {
            label: `👕 Wardrobe (${unlockedCount}/${totalCount})`,
            submenu: wardrobeItems.length > 0 ? wardrobeItems : [{ label: 'No items yet', enabled: false }]
        },
        { type: 'separator' },
        {
            label: '🖨️ 3D Print Dee Dee',
            click: () => {
                const printWindow = new BrowserWindow({
                    width: 1000,
                    height: 700,
                    title: '3D Print Dee Dee',
                    webPreferences: {
                        nodeIntegration: true,
                        contextIsolation: false
                    }
                });
                printWindow.loadFile('3d-print/generator.html');
            }
        },
        {
            label: '🔑 Get Print Code',
            click: () => {
                if (gamificationManager) {
                    const code = gamificationManager.generateUnlockCode();
                    const { clipboard } = require('electron');
                    clipboard.writeText(code);
                    require('electron').dialog.showMessageBox({
                        type: 'info',
                        title: '🔑 Your Print Code',
                        message: 'Code copied to clipboard!',
                        detail: `${code}\n\nUse this code at deedee.dev/print`,
                        buttons: ['OK']
                    });
                }
            }
        },
        { type: 'separator' },
        {
            label: '⚙️ Settings',
            click: () => {
                if (mainWindow && !mainWindow.isDestroyed()) {
                    mainWindow.show();
                    mainWindow.webContents.send('open-settings');
                }
            }
        },
        {
            label: '✕ Hide Dee Dee',
            click: () => {
                if (mascotWindow && !mascotWindow.isDestroyed()) {
                    mascotWindow.hide();
                }
            }
        }
    ];
    
    const menu = Menu.buildFromTemplate(menuTemplate);
    menu.popup({ window: mascotWindow });
});

// Save equipped accessories
ipcMain.on('equipped-accessories', (event, equipped) => {
    if (gamificationManager) {
        gamificationManager.setEquipped(equipped);
    }
});

// ===== GAMIFICATION TRACKING =====

// Track share
ipcMain.on('track-share', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.trackShare();
        // Also check December shares
        gamificationManager.trackDecemberShare();
        event.reply('unlock-result', result);
        updateMascotAccessories();
    }
});

// Track file content for secret unlocks
ipcMain.on('check-file-content', (event, content) => {
    if (gamificationManager) {
        // Check D Chain (350+ d's)
        let result = gamificationManager.checkFileContent(content);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check T-800 ("I'll be back")
        result = gamificationManager.checkT800(content);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Cthulhu (forbidden words)
        result = gamificationManager.checkCthulhu(content);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Track lines deleted
ipcMain.on('track-lines-deleted', (event, count) => {
    if (gamificationManager) {
        const result = gamificationManager.trackLinesDeleted(count);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Track undos
ipcMain.on('track-undo', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.trackUndo();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Track cursor moves (for Bigfoot)
ipcMain.on('track-cursor-move', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.trackCursorMove();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Track scroll (for Nessie)
ipcMain.on('track-scroll', (event, lines) => {
    if (gamificationManager) {
        const result = gamificationManager.trackScroll(lines);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Track open file count (for Kraken)
ipcMain.on('track-open-files', (event, count) => {
    if (gamificationManager) {
        const result = gamificationManager.checkKraken(count);
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Periodic time-based checks
ipcMain.on('check-time-unlocks', (event) => {
    if (gamificationManager) {
        // Check Area 51 (3:33 AM Sep 20)
        let result = gamificationManager.checkArea51();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Yeti (midnight December)
        result = gamificationManager.checkYeti();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Night Owl (midnight-4am)
        result = gamificationManager.checkNightOwl();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Early Bird (5-6am)
        result = gamificationManager.checkEarlyBird();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Marathon (8 hours)
        result = gamificationManager.checkMarathon();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Weekend Warrior
        result = gamificationManager.checkWeekendWarrior();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check Halloween
        result = gamificationManager.checkHalloween();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
            return;
        }
        
        // Check December (Santa)
        result = gamificationManager.checkDecember();
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Konami code unlock
ipcMain.on('konami-code', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.unlockAchievement('konami');
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Hypno eyes (triple click both eyes)
ipcMain.on('hypno-eyes', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.unlockAchievement('hypno');
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Chatterbox (5x click mouth)
ipcMain.on('chatterbox', (event) => {
    if (gamificationManager) {
        const result = gamificationManager.unlockAchievement('chatterbox');
        if (result.success) {
            event.reply('unlock-result', result);
            updateMascotAccessories();
        }
    }
});

// Get wardrobe data
ipcMain.handle('get-wardrobe', async () => {
    if (gamificationManager) {
        return gamificationManager.getWardrobeData();
    }
    return [];
});

// Get mascot data (for renderer)
ipcMain.handle('get-mascot-data', async () => {
    if (gamificationManager) {
        return gamificationManager.getMascotData();
    }
    return { unlocked: [], equipped: [], locked: [], secretCount: 0, totalCount: 0 };
});

// Generate print code for website
ipcMain.on('get-print-code', (event) => {
    if (gamificationManager) {
        const code = gamificationManager.generateUnlockCode();
        // Show dialog with the code
        const { dialog, clipboard } = require('electron');
        clipboard.writeText(code);
        dialog.showMessageBox({
            type: 'info',
            title: '🔑 Your Print Code',
            message: 'Code copied to clipboard!',
            detail: `${code}\n\nUse this code at deedee.dev/print to download your unlocked STL files.\n\nCode expires in 30 days.`,
            buttons: ['OK']
        });
    }
});

ipcMain.on('close-ide', () => {
    // Close button (X) just hides, doesn't quit
    if (!isPinned) {
        closeIDE();
    }
});

// Actual quit - from dock or Cmd+Q
ipcMain.on('quit-app', () => {
    app.quit();
});

ipcMain.on('minimize-ide', () => {
    // Minimize always hides both AIDE and Dee Dee (even in "both eyes" mode)
    console.log('Minimize clicked - hiding AIDE and Dee Dee');
    closeIDEFully();
});

ipcMain.on('hide-to-dock', () => {
    // Hide app to dock/sidebar - window minimizes but doesn't close
    console.log('Hide to dock - minimizing window');
    if (mainWindow) {
        mainWindow.minimize();
    }
});

ipcMain.on('toggle-pin', (event, pinned) => {
    isPinned = pinned;
    if (mainWindow) {
        mainWindow.setAlwaysOnTop(pinned);
    }
});

ipcMain.on('set-close-behavior', (event, behavior) => {
    closeBehavior = behavior;
    console.log('=== CLOSE BEHAVIOR CHANGED ===');
    console.log('New behavior:', behavior);
    console.log('closeBehavior variable is now:', closeBehavior);
    
    // In 'double' mode (one eye) or 'minimize' mode (both eyes), window stays on top
    // Only 'single' mode (no glow) allows window to go behind others
    if (mainWindow) {
        const stayOnTop = (behavior === 'double' || behavior === 'minimize');
        mainWindow.setAlwaysOnTop(stayOnTop, stayOnTop ? 'floating' : undefined);
        console.log('Window always on top:', stayOnTop);
    }
});

// Voice wake - "Aye Dee Dee"
ipcMain.on('voice-wake', () => {
    console.log('🎤 Voice wake: Dee Dee summoned!');
    
    if (gamificationManager) {
        gamificationManager.addXP(15); // XP for using voice
        updateMascotAccessories();
    }
    
    openIDE();
});

// Voice hide - "Dee Dee Hide"
ipcMain.on('voice-hide', () => {
    console.log('🎤 Voice hide: Dee Dee hiding!');
    closeIDEFully();
});

// IPC: Get gamification info
ipcMain.handle('get-gamification-stats', () => {
    return gamificationManager ? gamificationManager.getStats() : null;
});

// Request microphone permission for voice activation
async function requestMicrophonePermission() {
    if (process.platform === 'darwin') {
        // macOS - request permission
        const status = systemPreferences.getMediaAccessStatus('microphone');
        console.log('Microphone permission status:', status);
        
        if (status !== 'granted') {
            const granted = await systemPreferences.askForMediaAccess('microphone');
            console.log('Microphone permission granted:', granted);
            return granted;
        }
        return true;
    } else if (process.platform === 'linux') {
        // Linux - usually handled by PulseAudio/PipeWire, no special request needed
        console.log('Linux: Microphone access via system audio');
        return true;
    } else if (process.platform === 'win32') {
        // Windows - handled by OS prompt when accessing mic
        console.log('Windows: Microphone permission will be requested on use');
        return true;
    }
    return true;
}

// App lifecycle
app.whenReady().then(async () => {
    // Set dock icon and menu on macOS
    if (process.platform === 'darwin') {
        // Make sure dock icon is visible
        app.dock.show();
        
        const iconPath = path.join(__dirname, 'assets', 'icon.png');
        if (fs.existsSync(iconPath)) {
            const icon = nativeImage.createFromPath(iconPath);
            app.dock.setIcon(icon);
            console.log('Dock icon set');
        }
        
        // Create dock menu
        const dockMenu = Menu.buildFromTemplate([
            { label: 'Open AIDE', click: () => openIDE() },
            { label: 'Show Dee Dee', click: () => showMascot() },
            { type: 'separator' },
            { label: 'New File', click: () => {
                openIDE();
                if (mainWindow) mainWindow.webContents.send('new-file');
            }},
            { type: 'separator' },
            { label: 'Quit', click: () => {
                isQuitting = true;
                app.quit();
            }}
        ]);
        app.dock.setMenu(dockMenu);
        console.log('Dock menu set');
    }
    
    // Initialize license manager
    licenseManager = new LicenseManager();
    licenseManager.checkLicense();
    
    // Initialize gamification manager
    gamificationManager = new GamificationManager();
    gamificationManager.checkDailyStreak(); // Check for streak
    gamificationManager.trackAppStart(); // Track app starts for headphones
    
    // Check for secret time-based unlocks every 5 minutes
    setInterval(() => {
        if (gamificationManager) {
            // Night owl check (12am-4am)
            const nightOwlResult = gamificationManager.checkNightOwl();
            if (nightOwlResult.success && nightOwlResult.unlockedItem) {
                console.log('🦉 SECRET UNLOCK: Night Owl - Cyber Visor!');
                updateMascotAccessories();
            }
            
            // Halloween check (Oct 31, 3+ hours)
            const halloweenResult = gamificationManager.checkHalloween();
            if (halloweenResult.success && halloweenResult.unlockedItem) {
                console.log('🎃 SECRET UNLOCK: Hellraiser - Pinhead mode!');
                updateMascotAccessories();
            } else if (halloweenResult.isHalloween && halloweenResult.progress) {
                console.log(`🎃 Halloween progress: ${halloweenResult.progress}/180 minutes`);
            }
        }
    }, 5 * 60 * 1000); // Every 5 minutes
    
    // Also check immediately on launch
    gamificationManager.checkNightOwl();
    gamificationManager.checkHalloween();
    
    console.log('AIDE License Status:', licenseManager.isPro ? 'PRO' : 'FREE');
    console.log('Features:', licenseManager.getFeatures());
    console.log('Gamification Level:', gamificationManager.getStats().level);
    
    // Request mic permission for "Aye Dee Dee" voice wake (Pro only)
    if (licenseManager.isPro) {
        await requestMicrophonePermission();
    }
    
    console.log('Creating app menu...');
    createAppMenu();
    console.log('Creating tray...');
    createTray();
    
    // Always reset state on startup so Dee Dee shows
    state = 'hidden';
    console.log('Showing mascot...');
    showMascot();
    
    // Force show after a delay to ensure window is ready
    setTimeout(() => {
        if (mascotWindow && !mascotWindow.isDestroyed()) {
            mascotWindow.show();
            mascotWindow.moveTop();
            console.log('Mascot force-shown');
        }
    }, 500);
    
    console.log('App ready complete!');
});

// IPC: Get license/features info
ipcMain.handle('get-features', () => {
    return licenseManager ? licenseManager.getFeatures() : { isPro: false };
});

ipcMain.handle('activate-license', (event, key) => {
    if (licenseManager) {
        return licenseManager.activateLicense(key);
    }
    return { success: false, message: 'License manager not ready' };
});

ipcMain.handle('get-pricing', () => {
    return {
        oneTime: { price: 20, currency: 'USD', label: '$20 one-time' },
        monthly: { price: 3, currency: 'USD', label: '$3/month' }
    };
});

// Export project as zip
ipcMain.handle('export-project', async (event, options) => {
    const { projectPath, includeGit, includeNodeModules, includeEnv } = options;
    const archiver = require('archiver');
    
    try {
        // Show save dialog
        const { filePath, canceled } = await dialog.showSaveDialog(mainWindow, {
            title: 'Export Project',
            defaultPath: path.join(app.getPath('downloads'), `${path.basename(projectPath)}.zip`),
            filters: [{ name: 'ZIP Archive', extensions: ['zip'] }]
        });
        
        if (canceled || !filePath) {
            return { success: false, cancelled: true };
        }
        
        // Create zip archive
        const output = fs.createWriteStream(filePath);
        const archive = archiver('zip', { zlib: { level: 9 } });
        
        return new Promise((resolve, reject) => {
            output.on('close', () => {
                resolve({ success: true, outputPath: filePath, size: archive.pointer() });
            });
            
            archive.on('error', (err) => {
                reject({ success: false, error: err.message });
            });
            
            archive.pipe(output);
            
            // Build ignore patterns
            const ignorePatterns = [];
            if (!includeGit) ignorePatterns.push('.git/**');
            if (!includeNodeModules) ignorePatterns.push('node_modules/**');
            if (!includeEnv) ignorePatterns.push('.env', '.env.*');
            
            // Add files with ignore patterns
            archive.glob('**/*', {
                cwd: projectPath,
                ignore: ignorePatterns,
                dot: true
            });
            
            archive.finalize();
        });
    } catch (err) {
        return { success: false, error: err.message };
    }
});

// Import project from zip
ipcMain.handle('import-project', async () => {
    const AdmZip = require('adm-zip');
    
    try {
        // Show open dialog
        const { filePaths, canceled } = await dialog.showOpenDialog(mainWindow, {
            title: 'Import Shared Project',
            filters: [{ name: 'ZIP Archive', extensions: ['zip'] }],
            properties: ['openFile']
        });
        
        if (canceled || filePaths.length === 0) {
            return { success: false, cancelled: true };
        }
        
        const zipPath = filePaths[0];
        const projectName = path.basename(zipPath, '.zip');
        
        // Ask where to extract
        const { filePath: destPath, canceled: destCanceled } = await dialog.showSaveDialog(mainWindow, {
            title: 'Choose Project Location',
            defaultPath: path.join(app.getPath('documents'), projectName),
            buttonLabel: 'Extract Here'
        });
        
        if (destCanceled || !destPath) {
            return { success: false, cancelled: true };
        }
        
        // Create directory and extract
        fs.mkdirSync(destPath, { recursive: true });
        
        const zip = new AdmZip(zipPath);
        zip.extractAllTo(destPath, true);
        
        return { success: true, projectPath: destPath };
    } catch (err) {
        return { success: false, error: err.message };
    }
});

// Prevent app from quitting when windows close - just hide
app.on('window-all-closed', (e) => {
    e.preventDefault();
});

// macOS: clicking dock icon brings back AIDE
app.on('activate', () => {
    if (state === 'fully-hidden') {
        showMascot();
    } else if (!mainWindow || !mainWindow.isVisible()) {
        openIDE();
    }
});

// Allow actual quit from dock menu or Cmd+Q
app.on('before-quit', () => {
    isQuitting = true;
});

// Single instance
const gotLock = app.requestSingleInstanceLock();
if (!gotLock) {
    app.quit();
} else {
    app.on('second-instance', () => {
        openIDE();
    });
}
