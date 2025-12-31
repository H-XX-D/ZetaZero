#!/usr/bin/env node

/**
 * Quick script to launch Dee Dee mascot window for screenshots
 * Usage: node launch-dee-dee.js
 */

const { app, BrowserWindow } = require('electron');
const path = require('path');

let mascotWindow = null;

function createMascotWindow() {
    mascotWindow = new BrowserWindow({
        width: 200,
        height: 200,
        frame: false,
        transparent: true,
        alwaysOnTop: true,
        skipTaskbar: true,
        resizable: false,
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true
        }
    });

    mascotWindow.loadFile('mascot.html');
    
    mascotWindow.on('closed', () => {
        mascotWindow = null;
    });
    
    // Position in center of screen for screenshot
    const { screen } = require('electron');
    const primaryDisplay = screen.getPrimaryDisplay();
    const { width, height } = primaryDisplay.workAreaSize;
    
    mascotWindow.setPosition(
        Math.floor((width - 200) / 2),
        Math.floor((height - 200) / 2)
    );
    
    console.log('Dee Dee launched! Window is ready for screenshot.');
    console.log('Press Ctrl+C to close.');
}

app.whenReady().then(() => {
    createMascotWindow();
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    if (mascotWindow === null) {
        createMascotWindow();
    }
});

