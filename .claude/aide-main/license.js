/**
 * AIDE License & Feature Flag System
 * 
 * Free:
 * - Editor (tabbed)
 * - Bring your own API key
 * - File Explorer OR TODO List (one at a time)
 * - Terminal OR Debug Console (one at a time)
 * - Dee Dee mascot & docking
 * 
 * Pro ($5/mo):
 * - All voice commands (save, new file, theme, etc.)
 * - Both Terminal AND Debug Console
 * - Customizable keyboard shortcuts
 * - Still bring your own API key
 * 
 * Pro+ ($15/mo):
 * - Everything in Pro
 * - Multiple sidebars open at once
 * - Drag sidebars to either side (left/right)
 * - Drag & drop panel rearrangement
 * - $10 API credit included (no key needed)
 * - All flagship models:
 *   • o3 Pro, GPT-5 Pro, Codex
 *   • Claude Opus 4.5
 *   • Grok 4, Gemini 3 Pro
 * - AI CLI Access (AI can run terminal commands)
 * - Unused credit rolls over (up to $20 max)
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

// License file location
const LICENSE_PATH = path.join(require('electron').app.getPath('userData'), 'aide-license.key');

// Simple license validation (in production, use proper cryptographic verification)
const LICENSE_SECRET = 'AIDE-DEE-DEE-2024'; // In production, use proper key validation

class LicenseManager {
    constructor() {
        this.isPro = false;
        this.tier = 'free'; // 'free', 'pro', or 'proplus'
        this.licenseData = null;
        this.budgetUsed = 0;         // $ used this month (Pro+ only)
        this.includedCredit = 10;    // $10 included with Pro+ subscription
        this.userBudget = 0;         // Additional budget user sets (pay what you use)
        this.rolloverBalance = 0;    // Unused credit rolls over (up to $20 max)
    }

    // Check if pro license exists and is valid
    checkLicense() {
        try {
            if (fs.existsSync(LICENSE_PATH)) {
                const licenseKey = fs.readFileSync(LICENSE_PATH, 'utf8').trim();
                this.isPro = this.validateLicenseKey(licenseKey);
                if (this.isPro) {
                    this.licenseData = { key: licenseKey, valid: true };
                }
            }
        } catch (err) {
            console.log('License check error:', err.message);
            this.isPro = false;
        }
        return this.isPro;
    }

    // Validate license key format
    validateLicenseKey(key) {
        if (!key || key.length < 10) return false;
        
        // Check for Pro+ key: AIDE-PLUS-XXXX-XXXX-XXXX
        const proPlusPattern = /^AIDE-PLUS-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$/;
        if (proPlusPattern.test(key)) {
            const baseKey = key.substring(0, key.lastIndexOf('-'));
            const checksum = key.substring(key.lastIndexOf('-') + 1);
            if (checksum === this.generateChecksum(baseKey)) {
                this.tier = 'proplus';
                return true;
            }
        }
        
        // Check for Pro key: AIDE-PRO-XXXX-XXXX-XXXX
        const proPattern = /^AIDE-PRO-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$/;
        if (proPattern.test(key)) {
            const baseKey = key.substring(0, key.lastIndexOf('-'));
            const checksum = key.substring(key.lastIndexOf('-') + 1);
            if (checksum === this.generateChecksum(baseKey)) {
                this.tier = 'pro';
                return true;
            }
        }
        
        return false;
    }

    // Generate checksum for license key
    generateChecksum(baseKey) {
        const hash = crypto.createHash('md5')
            .update(baseKey + LICENSE_SECRET)
            .digest('hex')
            .toUpperCase();
        return hash.substring(0, 4);
    }

    // Activate a license key
    activateLicense(key) {
        if (this.validateLicenseKey(key)) {
            fs.writeFileSync(LICENSE_PATH, key, 'utf8');
            this.isPro = true;
            this.licenseData = { key, valid: true };
            return { success: true, message: 'Pro license activated! Restart AIDE to enable all features.' };
        }
        return { success: false, message: 'Invalid license key. Please check and try again.' };
    }

    // Deactivate license
    deactivateLicense() {
        try {
            if (fs.existsSync(LICENSE_PATH)) {
                fs.unlinkSync(LICENSE_PATH);
            }
            this.isPro = false;
            this.licenseData = null;
            return { success: true, message: 'License deactivated.' };
        } catch (err) {
            return { success: false, message: err.message };
        }
    }

    // Get feature availability
    getFeatures() {
        const isProPlus = this.tier === 'proplus';
        const isPro = this.isPro || isProPlus;
        
        return {
            // Free features (always available)
            editor: true,
            aiChat: true,
            fileExplorer: true,
            mascot: true,
            docking: true,
            todoList: true,            // Free: but only one sidebar at a time
            debugConsole: true,        // Free: but only one bottom panel at a time
            terminal: true,            // Free: but only one bottom panel at a time
            
            // Pro features ($5/mo) - remove basic friction
            voiceCommands: isPro,         // All voice commands (free gets wake/hide only)
            bothPanels: isPro,            // Both terminal AND debug open together
            keyboardShortcuts: isPro,     // Customizable keyboard shortcuts
            maxSidebars: isPro ? (isProPlus ? 99 : 2) : 1,  // Free=1, Pro=2, Pro+=unlimited
            
            // Pro+ features ($10/mo) - full customization + API
            unlimitedSidebars: isProPlus, // Unlimited sidebars
            dragSidebars: isProPlus,      // Drag sidebars anywhere
            dragDropPanels: isProPlus,    // Drag & drop panel rearrangement
            
            // Pro+ features ($10/mo) - includes $10 credit + set your own budget
            premiumApi: isProPlus,        // API access included
            aiTerminalAccess: isProPlus,  // AI CLI - AI can execute terminal commands
            includedCredit: isProPlus ? this.includedCredit : 0,
            userBudget: isProPlus ? this.userBudget : 0,
            rollover: isProPlus ? this.rolloverBalance : 0,
            totalBudget: isProPlus ? this.includedCredit + this.userBudget + this.rolloverBalance : 0,
            budgetUsed: this.budgetUsed,
            budgetRemaining: isProPlus ? this.includedCredit + this.userBudget + this.rolloverBalance - this.budgetUsed : 0,
            
            // License status
            isPro: isPro,
            isProPlus: isProPlus,
            tier: this.tier || 'free'
        };
    }

    // Generate a valid pro license key (for testing/admin)
    static generateProKey() {
        const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
        let key = 'AIDE-PRO-';
        for (let i = 0; i < 4; i++) {
            key += chars.charAt(Math.floor(Math.random() * chars.length));
        }
        key += '-';
        for (let i = 0; i < 4; i++) {
            key += chars.charAt(Math.floor(Math.random() * chars.length));
        }
        
        // Add checksum
        const hash = crypto.createHash('md5')
            .update(key + LICENSE_SECRET)
            .digest('hex')
            .toUpperCase();
        key += '-' + hash.substring(0, 4);
        
        return key;
    }
}

module.exports = LicenseManager;

