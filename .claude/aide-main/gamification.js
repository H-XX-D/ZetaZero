const fs = require('fs');
const path = require('path');
const { app } = require('electron');

class GamificationManager {
    constructor() {
        this.userDataPath = app.getPath('userData');
        this.statsPath = path.join(this.userDataPath, 'user-stats.json');
        
        // Default State
        this.data = {
            xp: 0,
            level: 1,
            streak: 0,
            lastLogin: null,
            achievements: [],
            unlockedAccessories: [],
            equippedAccessories: ['antenna'], // What user has chosen to wear
            // Seasonal tracking
            halloweenMinutes: 0,
            halloweenYear: null,
            // December tracking
            decemberOpens: 0,
            decemberYear: null,
            decemberDays: [], // Track unique days opened
            // December shares (for Rudolph)
            decemberShares: 0,
            decemberSharesYear: null,
            // File saves tracking
            fileSaves: 0,
            // App starts tracking
            appStarts: 0,
            // Share count
            shareCount: 0,
            // Feature usage tracking for crown
            usedTerminal: false,
            usedEditor: false,
            usedGit: false,
            usedExplorer: false
        };
        
        // Accessory metadata - ONLY revealed when unlocked (server-side truth)
        this.accessoryMeta = {
            // ===== STARTER (Easy) =====
            'antenna': { name: 'Antenna', icon: '📡', rarity: 'common' },
            'flower': { name: 'Flower', icon: '🌸', rarity: 'common' },
            'headphones': { name: 'Neon Headphones', icon: '🎧', rarity: 'common' },
            'propeller': { name: 'Propeller Hat', icon: '🎡', rarity: 'common' },
            'bowtie': { name: 'Bow Tie', icon: '🎀', rarity: 'common' },
            'mustache': { name: 'Fancy Mustache', icon: '🥸', rarity: 'common' },
            'halo': { name: 'Angel Halo', icon: '😇', rarity: 'common' },
            'horns': { name: 'Devil Horns', icon: '😈', rarity: 'common' },
            
            // ===== UNCOMMON (Medium) =====
            'sunglasses': { name: 'Sunglasses', icon: '🕶️', rarity: 'uncommon' },
            'visor': { name: 'Cyber Visor', icon: '🥽', rarity: 'uncommon' },
            'chain': { name: 'Gold Chain', icon: '⛓️', rarity: 'uncommon' },
            'tophat': { name: 'Top Hat', icon: '🎩', rarity: 'uncommon' },
            'wizard': { name: 'Wizard Hat', icon: '🧙', rarity: 'uncommon' },
            'chef': { name: 'Chef Hat', icon: '👨‍🍳', rarity: 'uncommon' },
            'hardhat': { name: 'Hard Hat', icon: '👷', rarity: 'uncommon' },
            'cowboy': { name: 'Cowboy Hat', icon: '🤠', rarity: 'uncommon' },
            'beret': { name: 'Artist Beret', icon: '🎨', rarity: 'uncommon' },
            'monocle': { name: 'Monocle', icon: '🧐', rarity: 'uncommon' },
            'scarf': { name: 'Cozy Scarf', icon: '🧣', rarity: 'uncommon' },
            'cape': { name: 'Hero Cape', icon: '🦸', rarity: 'uncommon' },
            'wings': { name: 'Butterfly Wings', icon: '🦋', rarity: 'uncommon' },
            'crown': { name: 'The Crown', icon: '👑', rarity: 'uncommon' },
            
            // ===== RARE (Hard) =====
            'dchain': { name: 'Diamond D Chain', icon: '💎', rarity: 'rare' },
            'hypno': { name: 'Hypno Eyes', icon: '🌀', rarity: 'rare' },
            'chatterbox': { name: 'Chatterbox', icon: '💬', rarity: 'rare' },
            'konami': { name: 'Konami Code', icon: '🎮', rarity: 'rare' },
            'destroyer': { name: 'Destroyer', icon: '💀', rarity: 'rare' },
            'regret': { name: 'Regret', icon: '😢', rarity: 'rare' },
            'ninja': { name: 'Ninja Mask', icon: '🥷', rarity: 'rare' },
            'astronaut': { name: 'Space Helmet', icon: '👨‍🚀', rarity: 'rare' },
            'robot': { name: 'Robot Mode', icon: '🤖', rarity: 'rare' },
            'alien': { name: 'Alien Antennae', icon: '👽', rarity: 'rare' },
            'phoenix': { name: 'Phoenix Flames', icon: '🔥', rarity: 'rare' },
            'ice': { name: 'Ice Crystal', icon: '❄️', rarity: 'rare' },
            'thunder': { name: 'Thunder Crown', icon: '⚡', rarity: 'rare' },
            'rainbow': { name: 'Rainbow Trail', icon: '🌈', rarity: 'rare' },
            'samurai': { name: 'Samurai Helmet', icon: '⛩️', rarity: 'rare' },
            'viking': { name: 'Viking Helmet', icon: '🪓', rarity: 'rare' },
            'spartan': { name: 'Spartan Helm', icon: '🛡️', rarity: 'rare' },
            'centurion': { name: 'Centurion', icon: '⚔️', rarity: 'rare' },
            
            // ===== SEASONAL =====
            'santa': { name: 'Santa Dee Dee', icon: '🎅', rarity: 'seasonal' },
            'rudolph': { name: 'Rudolph Nose', icon: '🦌', rarity: 'seasonal' },
            'elf': { name: 'Elf Ears', icon: '🧝', rarity: 'seasonal' },
            'snowman': { name: 'Snowman', icon: '⛄', rarity: 'seasonal' },
            'gingerbread': { name: 'Gingerbread', icon: '🍪', rarity: 'seasonal' },
            'cupid': { name: 'Cupid', icon: '💘', rarity: 'seasonal' },
            'heartbreak': { name: 'Heartbreak', icon: '💔', rarity: 'seasonal' },
            'leprechaun': { name: 'Leprechaun', icon: '🍀', rarity: 'seasonal' },
            'pot_of_gold': { name: 'Pot of Gold', icon: '🪙', rarity: 'seasonal' },
            'bunny': { name: 'Easter Bunny', icon: '🐰', rarity: 'seasonal' },
            'egg': { name: 'Easter Egg', icon: '🥚', rarity: 'seasonal' },
            'unclesam': { name: 'Uncle Sam', icon: '🇺🇸', rarity: 'seasonal' },
            'fireworks': { name: 'Fireworks', icon: '🎆', rarity: 'seasonal' },
            'newyear': { name: 'New Year', icon: '🎉', rarity: 'seasonal' },
            'hellraiser': { name: 'Pinhead', icon: '📍', rarity: 'seasonal' },
            'vampire': { name: 'Vampire', icon: '🧛', rarity: 'seasonal' },
            'ghost': { name: 'Ghost', icon: '👻', rarity: 'seasonal' },
            'witch': { name: 'Witch Hat', icon: '🧙‍♀️', rarity: 'seasonal' },
            'pumpkin': { name: 'Pumpkin Head', icon: '🎃', rarity: 'seasonal' },
            'skeleton': { name: 'Skeleton', icon: '💀', rarity: 'seasonal' },
            'turkey': { name: 'Turkey Hat', icon: '🦃', rarity: 'seasonal' },
            'pilgrim': { name: 'Pilgrim', icon: '🧑‍🌾', rarity: 'seasonal' },
            
            // ===== BEHAVIOR-BASED =====
            'earlybird': { name: 'Early Bird', icon: '🐦', rarity: 'behavior' },
            'nightowl': { name: 'Night Owl', icon: '🦉', rarity: 'behavior' },
            'marathon': { name: 'Marathon Coder', icon: '🏃', rarity: 'behavior' },
            'zen': { name: 'Zen Master', icon: '🧘', rarity: 'behavior' },
            'speedster': { name: 'Speed Demon', icon: '💨', rarity: 'behavior' },
            'perfectionist': { name: 'Perfectionist', icon: '✨', rarity: 'behavior' },
            'bugslayer': { name: 'Bug Slayer', icon: '🐛', rarity: 'behavior' },
            'coffeelover': { name: 'Coffee Addict', icon: '☕', rarity: 'behavior' },
            'moonlighter': { name: 'Moonlighter', icon: '🌙', rarity: 'behavior' },
            'weekendwarrior': { name: 'Weekend Warrior', icon: '📅', rarity: 'behavior' },
            'streaker': { name: '30 Day Streak', icon: '🔥', rarity: 'behavior' },
            'centurion_streak': { name: '100 Day Streak', icon: '💯', rarity: 'behavior' },
            'yearlong': { name: '365 Day Streak', icon: '📆', rarity: 'behavior' },
            
            // ===== MILESTONE =====
            'first_commit': { name: 'First Commit', icon: '🎯', rarity: 'milestone' },
            'hundred_commits': { name: '100 Commits', icon: '💪', rarity: 'milestone' },
            'thousand_commits': { name: '1K Commits', icon: '🏆', rarity: 'milestone' },
            'first_pr': { name: 'First PR', icon: '🔀', rarity: 'milestone' },
            'first_star': { name: 'First Star', icon: '⭐', rarity: 'milestone' },
            'og': { name: 'OG Badge', icon: '🏅', rarity: 'milestone' },
            'beta_tester': { name: 'Beta Tester', icon: '🧪', rarity: 'milestone' },
            'founding_member': { name: 'Founding Member', icon: '🏛️', rarity: 'milestone' },
            'million_lines': { name: 'Million Lines', icon: '📜', rarity: 'milestone' },
            'polyglot': { name: 'Polyglot (10 langs)', icon: '🗣️', rarity: 'milestone' },
            
            // ===== EASTER EGGS =====
            'matrix': { name: 'Red Pill', icon: '💊', rarity: 'easter_egg' },
            'retro': { name: 'Retro 8-bit', icon: '👾', rarity: 'easter_egg' },
            'glitch': { name: 'Glitch Mode', icon: '📺', rarity: 'easter_egg' },
            'pirate': { name: 'Pirate', icon: '🏴‍☠️', rarity: 'easter_egg' },
            'jester': { name: 'Jester', icon: '🃏', rarity: 'easter_egg' },
            'disco': { name: 'Disco Ball', icon: '🪩', rarity: 'easter_egg' },
            'invisible': { name: 'Invisible', icon: '👁️‍🗨️', rarity: 'easter_egg' },
            'golden': { name: 'Solid Gold', icon: '🥇', rarity: 'easter_egg' },
            'diamond': { name: 'Diamond Encrusted', icon: '💠', rarity: 'easter_egg' },
            'hologram': { name: 'Hologram', icon: '🔮', rarity: 'easter_egg' },
            'neon': { name: 'Neon Glow', icon: '💡', rarity: 'easter_egg' },
            'birthday': { name: 'Birthday Party', icon: '🎂', rarity: 'easter_egg' },
            'pizza': { name: 'Pizza Hat', icon: '🍕', rarity: 'easter_egg' },
            'taco': { name: 'Taco Tuesday', icon: '🌮', rarity: 'easter_egg' },
            'sushi': { name: 'Sushi Roll', icon: '🍣', rarity: 'easter_egg' },
            
            // ===== PRIDE & AWARENESS =====
            'pride': { name: 'Pride', icon: '🏳️‍🌈', rarity: 'pride' },
            'trans': { name: 'Trans Pride', icon: '🏳️‍⚧️', rarity: 'pride' },
            'bi': { name: 'Bi Pride', icon: '💜', rarity: 'pride' },
            'autism': { name: 'Autism Awareness', icon: '♾️', rarity: 'pride' },
            'mental_health': { name: 'Mental Health', icon: '💚', rarity: 'pride' },
            'blm': { name: 'BLM Fist', icon: '✊', rarity: 'pride' },
            'ukraine': { name: 'Ukraine Support', icon: '🇺🇦', rarity: 'pride' },
            
            // ===== LEGENDARY (Very Hard) =====
            'void': { name: 'Void Walker', icon: '🕳️', rarity: 'legendary' },
            'cosmic': { name: 'Cosmic Entity', icon: '🌌', rarity: 'legendary' },
            'quantum': { name: 'Quantum State', icon: '⚛️', rarity: 'legendary' },
            'time_lord': { name: 'Time Lord', icon: '⏰', rarity: 'legendary' },
            'infinity': { name: 'Infinity', icon: '♾️', rarity: 'legendary' },
            'singularity': { name: 'Singularity', icon: '⚫', rarity: 'legendary' },
            'omega': { name: 'Omega', icon: 'Ω', rarity: 'legendary' },
            'alpha': { name: 'Alpha', icon: 'α', rarity: 'legendary' },
            
            // ===== MYTHIC (Extremely Hard / ARG) =====
            't800': { name: 'T-800', icon: '🦾', rarity: 'mythic' },           // Find "I'll be back" in 50 files
            'area51': { name: 'Area 51', icon: '🛸', rarity: 'mythic' },       // Code at 3:33 AM on Sep 20
            'cthulhu': { name: 'Cthulhu', icon: '🐙', rarity: 'mythic' },      // Type "Ph'nglui mglw'nafh"
            'illuminati': { name: 'Illuminati', icon: '👁️', rarity: 'mythic' }, // Find the hidden triangle
            'bigfoot': { name: 'Bigfoot', icon: '🦶', rarity: 'mythic' },      // Walk 10000 cursor movements
            'nessie': { name: 'Nessie', icon: '🦕', rarity: 'mythic' },        // Scroll 10000 lines
            'yeti': { name: 'Yeti', icon: '🏔️', rarity: 'mythic' },           // Code in December at midnight
            'kraken': { name: 'Kraken', icon: '🦑', rarity: 'mythic' },        // Open 100 files at once
            'phoenix_rise': { name: 'Phoenix Rising', icon: '🔥', rarity: 'mythic' }, // Delete & restore project
            'jason': { name: 'Jason Dee Dee', icon: '🏒', rarity: 'mythic' },  // Newsletter ARG clue
            
            // ===== IMPOSSIBLE (Only Dev Can Have) =====
            'deedee_god': { name: '???', icon: '✴️', rarity: 'impossible' },   // Only hendrixx has this
            'developer': { name: 'The Creator', icon: '🧬', rarity: 'impossible' },
            'one_of_one': { name: '1 of 1', icon: '1️⃣', rarity: 'impossible' }
        };
        
        // Total count of secrets (client only sees this number, not what they are)
        this.totalAccessories = Object.keys(this.accessoryMeta).length;
        
        this.loadStats();
        
        // Track session time for Halloween
        this.sessionStart = Date.now();
    }

    loadStats() {
        try {
            if (fs.existsSync(this.statsPath)) {
                const fileData = fs.readFileSync(this.statsPath, 'utf8');
                const parsed = JSON.parse(fileData);
                this.data = { ...this.data, ...parsed };
            }
        } catch (e) {
            console.error('Failed to load gamification stats:', e);
        }
    }

    saveStats() {
        try {
            fs.writeFileSync(this.statsPath, JSON.stringify(this.data, null, 2));
        } catch (e) {
            console.error('Failed to save gamification stats:', e);
        }
    }

    // --- XP & Leveling ---

    getRequiredXP(level) {
        return Math.floor(500 * Math.pow(level, 1.5));
    }

    addXP(amount) {
        this.data.xp += amount;
        const oldLevel = this.data.level;
        let leveledUp = false;
        let newUnlock = null;
        
        while (this.data.xp >= this.getRequiredXP(this.data.level)) {
            this.data.level++;
            leveledUp = true;
        }

        if (leveledUp) {
            newUnlock = this.handleLevelUp(this.data.level);
        }

        this.saveStats();
        return {
            xp: this.data.xp,
            level: this.data.level,
            leveledUp,
            newUnlock
        };
    }

    // Track file saves - flower unlocks at 3 saves
    trackFileSave() {
        this.data.fileSaves++;
        this.saveStats();
        
        if (this.data.fileSaves >= 3) {
            return this.unlockAchievement('saver');
        }
        return { success: false, progress: this.data.fileSaves, required: 3 };
    }
    
    // Check file content for secret unlocks
    checkFileContent(content) {
        if (!content) return { success: false };
        
        // D Chain: 350+ letter "d"s in a file
        const dCount = (content.toLowerCase().match(/d/g) || []).length;
        if (dCount >= 350) {
            return this.unlockAchievement('dchain');
        }
        
        // Destroyer: file has 1000+ deleted lines in session (tracked separately)
        
        return { success: false, dCount };
    }
    
    // Track lines deleted for Destroyer unlock
    trackLinesDeleted(count) {
        if (!this.data.sessionLinesDeleted) this.data.sessionLinesDeleted = 0;
        this.data.sessionLinesDeleted += count;
        
        if (this.data.sessionLinesDeleted >= 1000) {
            return this.unlockAchievement('destroyer');
        }
        return { success: false, deleted: this.data.sessionLinesDeleted };
    }
    
    // Track undos for Regret unlock
    trackUndo() {
        if (!this.data.sessionUndos) this.data.sessionUndos = 0;
        this.data.sessionUndos++;
        
        if (this.data.sessionUndos >= 100) {
            return this.unlockAchievement('regret');
        }
        return { success: false, undos: this.data.sessionUndos };
    }
    
    // Track app starts - headphones unlock at 3 starts
    trackAppStart() {
        this.data.appStarts++;
        this.saveStats();
        
        if (this.data.appStarts >= 3) {
            return this.unlockAchievement('regular');
        }
        return { success: false, progress: this.data.appStarts, required: 3 };
    }
    
    // Track shares - antenna at 1, chain at 12
    trackShare() {
        this.data.shareCount++;
        this.saveStats();
        
        if (this.data.shareCount === 1) {
            this.unlockAchievement('first_share');
        }
        if (this.data.shareCount >= 12) {
            this.unlockAchievement('sharer');
        }
        
        // Check for crown (needs 10 shares + all features)
        this.checkCrown();
        
        return { success: true, count: this.data.shareCount };
    }
    
    // Track feature usage for crown
    trackFeatureUsage(feature) {
        if (feature === 'terminal') this.data.usedTerminal = true;
        if (feature === 'editor') this.data.usedEditor = true;
        if (feature === 'git') this.data.usedGit = true;
        if (feature === 'explorer') this.data.usedExplorer = true;
        this.saveStats();
        this.checkCrown();
    }
    
    // Crown: used terminal, editor, git, explorer + 10 shares
    checkCrown() {
        if (this.data.usedTerminal && 
            this.data.usedEditor && 
            this.data.usedGit && 
            this.data.usedExplorer && 
            this.data.shareCount >= 10) {
            return this.unlockAchievement('power_user');
        }
        return { success: false };
    }
    
    // ===== MYTHIC UNLOCK CHECKS =====
    
    // T-800: Find "I'll be back" in 50 different files
    checkT800(content) {
        if (!content) return { success: false };
        if (content.toLowerCase().includes("i'll be back") || content.toLowerCase().includes("ill be back")) {
            if (!this.data.t800Files) this.data.t800Files = [];
            const hash = this.simpleHash(content.substring(0, 100));
            if (!this.data.t800Files.includes(hash)) {
                this.data.t800Files.push(hash);
                this.saveStats();
                if (this.data.t800Files.length >= 50) {
                    return this.unlockAchievement('t800');
                }
            }
            return { success: false, progress: this.data.t800Files.length, required: 50 };
        }
        return { success: false };
    }
    
    // Area 51: Code at exactly 3:33 AM on September 20th
    checkArea51() {
        const now = new Date();
        const month = now.getMonth(); // September = 8
        const day = now.getDate();
        const hour = now.getHours();
        const minute = now.getMinutes();
        
        if (month === 8 && day === 20 && hour === 3 && minute === 33) {
            return this.unlockAchievement('area51');
        }
        return { success: false };
    }
    
    // Cthulhu: Type the forbidden words
    checkCthulhu(content) {
        if (!content) return { success: false };
        const forbidden = ["ph'nglui", "mglw'nafh", "cthulhu", "r'lyeh", "wgah'nagl", "fhtagn"];
        const lowerContent = content.toLowerCase();
        const found = forbidden.filter(word => lowerContent.includes(word));
        if (found.length >= 3) {
            return this.unlockAchievement('cthulhu');
        }
        return { success: false, found: found.length };
    }
    
    // Kraken: Have 100 files open at once
    checkKraken(openFileCount) {
        if (openFileCount >= 100) {
            return this.unlockAchievement('kraken');
        }
        return { success: false, count: openFileCount };
    }
    
    // Bigfoot: Move cursor 10,000 times in a session
    trackCursorMove() {
        if (!this.data.sessionCursorMoves) this.data.sessionCursorMoves = 0;
        this.data.sessionCursorMoves++;
        if (this.data.sessionCursorMoves >= 10000) {
            return this.unlockAchievement('bigfoot');
        }
        return { success: false, moves: this.data.sessionCursorMoves };
    }
    
    // Nessie: Scroll 10,000 lines total
    trackScroll(lines) {
        if (!this.data.totalScrollLines) this.data.totalScrollLines = 0;
        this.data.totalScrollLines += lines;
        this.saveStats();
        if (this.data.totalScrollLines >= 10000) {
            return this.unlockAchievement('nessie');
        }
        return { success: false, scrolled: this.data.totalScrollLines };
    }
    
    // Yeti: Code at midnight in December
    checkYeti() {
        const now = new Date();
        const month = now.getMonth(); // December = 11
        const hour = now.getHours();
        if (month === 11 && hour === 0) {
            return this.unlockAchievement('yeti');
        }
        return { success: false };
    }
    
    // Marathon: Code for 8 hours straight
    checkMarathon() {
        const sessionHours = (Date.now() - this.sessionStart) / (1000 * 60 * 60);
        if (sessionHours >= 8) {
            return this.unlockAchievement('marathon');
        }
        return { success: false, hours: sessionHours.toFixed(1) };
    }
    
    // Early Bird: Code between 5-6 AM
    checkEarlyBird() {
        const hour = new Date().getHours();
        if (hour >= 5 && hour < 6) {
            return this.unlockAchievement('earlybird');
        }
        return { success: false };
    }
    
    // Weekend Warrior: Code on both Saturday and Sunday
    checkWeekendWarrior() {
        const now = new Date();
        const day = now.getDay(); // 0 = Sunday, 6 = Saturday
        const weekKey = `${now.getFullYear()}-W${Math.ceil(now.getDate() / 7)}`;
        
        if (!this.data.weekendDays) this.data.weekendDays = {};
        if (!this.data.weekendDays[weekKey]) this.data.weekendDays[weekKey] = [];
        
        if ((day === 0 || day === 6) && !this.data.weekendDays[weekKey].includes(day)) {
            this.data.weekendDays[weekKey].push(day);
            this.saveStats();
            
            if (this.data.weekendDays[weekKey].length >= 2) {
                return this.unlockAchievement('weekendwarrior');
            }
        }
        return { success: false };
    }
    
    // Streak milestones
    checkStreakMilestones() {
        if (this.data.streak >= 30) this.unlockAchievement('streaker');
        if (this.data.streak >= 100) this.unlockAchievement('centurion_streak');
        if (this.data.streak >= 365) this.unlockAchievement('yearlong');
    }
    
    // Simple hash for tracking unique files
    simpleHash(str) {
        let hash = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash;
        }
        return hash.toString(16);
    }

    handleLevelUp(newLevel) {
        const rewards = {
            // Crown now unlocked by achievement, not level
        };

        if (rewards[newLevel]) {
            const accessory = rewards[newLevel];
            if (!this.data.unlockedAccessories.includes(accessory)) {
                this.data.unlockedAccessories.push(accessory);
                return { id: accessory, name: this.getAccessoryName(accessory), type: 'level_up' };
            }
        }
        return null;
    }

    // --- Achievements ---

    unlockAchievement(id) {
        if (!this.data.achievements.includes(id)) {
            this.data.achievements.push(id);
            let unlockedItem = null;
            
            // Achievement to accessory mapping
            const achievementRewards = {
                // Basic
                'night_owl': 'visor',
                'first_share': 'antenna',
                'saver': 'flower',
                'regular': 'headphones',
                'sharer': 'chain',
                'power_user': 'crown',
                'social_butterfly': 'sunglasses',
                
                // Easter eggs
                'dchain': 'dchain',
                'destroyer': 'destroyer',
                'regret': 'regret',
                'hypno': 'hypno',
                'chatterbox': 'chatterbox',
                'konami': 'konami',
                'matrix': 'matrix',
                'retro': 'retro',
                'glitch': 'glitch',
                'pirate': 'pirate',
                'jester': 'jester',
                'disco': 'disco',
                'golden': 'golden',
                'pizza': 'pizza',
                
                // Seasonal
                'hellraiser': 'hellraiser',
                'santa': 'santa',
                'rudolph': 'rudolph',
                'cupid': 'cupid',
                'leprechaun': 'leprechaun',
                'bunny': 'bunny',
                'unclesam': 'unclesam',
                'newyear': 'newyear',
                'vampire': 'vampire',
                'ghost': 'ghost',
                'witch': 'witch',
                'pumpkin': 'pumpkin',
                'elf': 'elf',
                'snowman': 'snowman',
                'turkey': 'turkey',
                
                // Behavior
                'earlybird': 'earlybird',
                'nightowl': 'nightowl',
                'marathon': 'marathon',
                'zen': 'zen',
                'speedster': 'speedster',
                'perfectionist': 'perfectionist',
                'bugslayer': 'bugslayer',
                'coffeelover': 'coffeelover',
                'moonlighter': 'moonlighter',
                'weekendwarrior': 'weekendwarrior',
                'streaker': 'streaker',
                'centurion_streak': 'centurion_streak',
                'yearlong': 'yearlong',
                
                // Milestone
                'first_commit': 'first_commit',
                'hundred_commits': 'hundred_commits',
                'thousand_commits': 'thousand_commits',
                'og': 'og',
                'beta_tester': 'beta_tester',
                'founding_member': 'founding_member',
                'polyglot': 'polyglot',
                'million_lines': 'million_lines',
                
                // Rare
                'ninja': 'ninja',
                'astronaut': 'astronaut',
                'robot': 'robot',
                'alien': 'alien',
                'phoenix': 'phoenix',
                'ice': 'ice',
                'thunder': 'thunder',
                'rainbow': 'rainbow',
                'samurai': 'samurai',
                'viking': 'viking',
                'spartan': 'spartan',
                'centurion': 'centurion',
                
                // Legendary
                'void': 'void',
                'cosmic': 'cosmic',
                'quantum': 'quantum',
                'time_lord': 'time_lord',
                'infinity': 'infinity',
                'singularity': 'singularity',
                'omega': 'omega',
                'alpha': 'alpha',
                
                // Mythic (ARG)
                't800': 't800',
                'area51': 'area51',
                'cthulhu': 'cthulhu',
                'illuminati': 'illuminati',
                'bigfoot': 'bigfoot',
                'nessie': 'nessie',
                'yeti': 'yeti',
                'kraken': 'kraken',
                'phoenix_rise': 'phoenix_rise',
                'jason': 'jason',
                
                // Pride
                'pride': 'pride',
                'trans': 'trans',
                'bi': 'bi',
                'autism': 'autism',
                'mental_health': 'mental_health',
                'blm': 'blm',
                'ukraine': 'ukraine',
                
                // Impossible (manual only)
                'deedee_god': 'deedee_god',
                'developer': 'developer',
                'one_of_one': 'one_of_one'
            };
            
            // Check if this achievement has a reward
            const reward = achievementRewards[id];
            if (reward && !this.data.unlockedAccessories.includes(reward)) {
                this.data.unlockedAccessories.push(reward);
                unlockedItem = reward;
            }
            
            this.saveStats();
            
            if (unlockedItem) {
                return {
                    success: true,
                    unlockedItem: {
                        id: unlockedItem,
                        name: this.getAccessoryName(unlockedItem),
                        icon: this.getAccessoryIcon(unlockedItem),
                        rarity: this.getAccessoryRarity(unlockedItem),
                        achievementId: id
                    }
                };
            }
            return { success: true };
        }
        return { success: false };
    }
    
    getAccessoryName(id) {
        return this.accessoryMeta[id]?.name || 'Unknown';
    }
    
    getAccessoryRarity(id) {
        return this.accessoryMeta[id]?.rarity || 'common';
    }
    
    getAccessoryIcon(id) {
        return this.accessoryMeta[id]?.icon || '❓';
    }
    
    // Get unlock hint (vague, for wardrobe display)
    getUnlockHint(id) {
        const hints = {
            // Easy
            'antenna': 'Share Dee Dee with a friend',
            'flower': 'Save a few files',
            'headphones': 'Keep coming back',
            'propeller': 'Reach level 5',
            'bowtie': 'Dress up your code',
            'mustache': 'Be fancy',
            'halo': 'Do something good',
            'horns': 'Do something bad',
            
            // Medium
            'sunglasses': 'Be social',
            'visor': 'Work in the dark',
            'chain': 'Share a lot',
            'crown': 'Master all the tools',
            'tophat': 'Be classy',
            'wizard': 'Cast 100 spells (commands)',
            'chef': 'Cook up something good',
            'hardhat': 'Build something big',
            'cowboy': 'Wrangle some code',
            'beret': 'Create art',
            'monocle': 'Inspect closely',
            'scarf': 'Stay warm in winter',
            'cape': 'Save the day',
            'wings': 'Transform your code',
            
            // Hard
            'dchain': 'D is for...',
            'hypno': 'Look into my eyes...',
            'chatterbox': 'Talk to me',
            'konami': '↑↑↓↓←→←→BA',
            'destroyer': 'Delete everything',
            'regret': 'Undo your mistakes',
            'ninja': 'Be stealthy',
            'astronaut': 'Reach for the stars',
            'robot': 'Become one with the machine',
            'alien': 'Make contact',
            'phoenix': 'Rise from the ashes',
            'ice': 'Stay cool',
            'thunder': 'Bring the storm',
            'rainbow': 'Find all the colors',
            'samurai': 'Honor the code',
            'viking': 'Raid and conquer',
            'spartan': 'This is coding!',
            'centurion': 'Lead the legion',
            
            // Seasonal
            'santa': 'Ho ho ho! (December)',
            'rudolph': 'Guide the way (December)',
            'elf': 'Help in the workshop',
            'snowman': 'Build in the cold',
            'cupid': 'Spread love (February)',
            'leprechaun': 'Find the gold (March)',
            'bunny': 'Hunt for eggs (Spring)',
            'unclesam': 'Celebrate freedom (July)',
            'hellraiser': 'Code on Halloween',
            'vampire': 'Drink code (October)',
            'ghost': 'Boo! (October)',
            'witch': 'Brew some code (October)',
            'pumpkin': 'Carve it out (October)',
            'newyear': 'New beginnings (January)',
            
            // Behavior
            'earlybird': 'Code before sunrise',
            'nightowl': 'Code after midnight',
            'marathon': 'Code for 8 hours straight',
            'zen': 'Meditate on your code',
            'speedster': 'Type really fast',
            'perfectionist': 'Zero errors',
            'bugslayer': 'Fix 100 bugs',
            'coffeelover': 'Code in the morning',
            'streaker': 'Keep the streak alive',
            'centurion_streak': 'Century of coding',
            'yearlong': 'A year of dedication',
            
            // Milestone
            'og': 'Be here from the start',
            'beta_tester': 'Test the waters',
            'founding_member': 'Be a founder',
            'polyglot': 'Learn many languages',
            
            // Easter eggs
            'matrix': 'Take the red pill',
            'retro': 'Go back in time',
            'glitch': 'Break reality',
            'pirate': 'Arr matey',
            'jester': 'Make them laugh',
            'disco': 'Dance the night away',
            'golden': 'Everything you touch',
            'birthday': 'Celebrate!',
            'pizza': 'Code fuel',
            
            // Legendary
            'void': 'Stare into the abyss',
            'cosmic': 'Transcend reality',
            'quantum': 'Exist in superposition',
            'time_lord': 'Master time itself',
            'infinity': 'Never stop',
            'singularity': 'Compress everything',
            'omega': 'The end',
            'alpha': 'The beginning',
            
            // Mythic (ARG clues)
            't800': 'I\'ll be back... (find it)',
            'area51': 'The truth is out there (3:33 AM)',
            'cthulhu': 'Ph\'nglui mglw\'nafh...',
            'illuminati': '△ See the hidden truth △',
            'bigfoot': 'Walk 10,000 steps',
            'nessie': 'Scroll the depths',
            'yeti': 'December midnight',
            'kraken': 'Release the kraken (100 files)',
            'phoenix_rise': 'From the ashes',
            'jason': '[NEWSLETTER CLUE]',
            
            // Impossible
            'deedee_god': '???',
            'developer': 'Be the creator',
            'one_of_one': 'There can only be one'
        };
        return hints[id] || 'Secret unlock condition';
    }
    
    // --- Secret Time-Based Checks ---
    
    checkNightOwl() {
        const hour = new Date().getHours();
        if (hour >= 0 && hour < 4) {
            return this.unlockAchievement('night_owl');
        }
        return { success: false };
    }
    
    // Halloween check - call this periodically during session
    checkHalloween() {
        const now = new Date();
        const month = now.getMonth(); // 0-indexed, October = 9
        const day = now.getDate();
        const year = now.getFullYear();
        
        // Only October 31st
        if (month === 9 && day === 31) {
            // Reset counter if new year
            if (this.data.halloweenYear !== year) {
                this.data.halloweenMinutes = 0;
                this.data.halloweenYear = year;
            }
            
            // Calculate session minutes
            const sessionMinutes = Math.floor((Date.now() - this.sessionStart) / 60000);
            
            // Add to total (capped to prevent abuse)
            const newTotal = Math.min(this.data.halloweenMinutes + sessionMinutes, 300);
            
            if (newTotal !== this.data.halloweenMinutes) {
                this.data.halloweenMinutes = newTotal;
                this.saveStats();
            }
            
            // 180 minutes = 3 hours
            if (this.data.halloweenMinutes >= 180) {
                return this.unlockAchievement('hellraiser');
            }
            
            return { 
                success: false, 
                progress: this.data.halloweenMinutes,
                required: 180,
                isHalloween: true
            };
        }
        
        return { success: false, isHalloween: false };
    }
    
    // December check - 5 app opens between Dec 1-24
    checkDecember() {
        const now = new Date();
        const month = now.getMonth(); // December = 11
        const day = now.getDate();
        const year = now.getFullYear();
        const dateKey = `${year}-12-${day}`;
        
        // Only December 1-24
        if (month === 11 && day >= 1 && day <= 24) {
            // Reset if new year
            if (this.data.decemberYear !== year) {
                this.data.decemberOpens = 0;
                this.data.decemberYear = year;
                this.data.decemberDays = [];
            }
            
            // Only count unique days (opening 5 times in one day doesn't count)
            if (!this.data.decemberDays.includes(dateKey)) {
                this.data.decemberDays.push(dateKey);
                this.data.decemberOpens++;
                this.saveStats();
                
                console.log(`🎄 December opens: ${this.data.decemberOpens}/5`);
            }
            
            // 5 unique days = Santa unlock
            if (this.data.decemberOpens >= 5) {
                return this.unlockAchievement('santa');
            }
            
            return {
                success: false,
                progress: this.data.decemberOpens,
                required: 5,
                isDecember: true
            };
        }
        
        return { success: false, isDecember: false };
    }
    
    // Track a share during December for Rudolph
    trackDecemberShare() {
        const now = new Date();
        const month = now.getMonth(); // December = 11
        const day = now.getDate();
        const year = now.getFullYear();
        
        // Only December 1-25 (including Christmas!)
        if (month === 11 && day >= 1 && day <= 25) {
            // Reset if new year
            if (this.data.decemberSharesYear !== year) {
                this.data.decemberShares = 0;
                this.data.decemberSharesYear = year;
            }
            
            this.data.decemberShares++;
            this.saveStats();
            
            console.log(`🦌 December shares: ${this.data.decemberShares}/5`);
            
            // 5 shares = Rudolph unlock
            if (this.data.decemberShares >= 5) {
                return this.unlockAchievement('rudolph');
            }
            
            return {
                success: false,
                progress: this.data.decemberShares,
                required: 5,
                isDecember: true
            };
        }
        
        return { success: false, isDecember: false };
    }

    // --- Daily Streak ---

    checkDailyStreak() {
        const now = new Date();
        const today = now.toDateString();
        
        if (this.data.lastLogin !== today) {
            if (this.data.lastLogin) {
                const lastDate = new Date(this.data.lastLogin);
                const diffTime = Math.abs(now - lastDate);
                const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24)); 
                
                if (diffDays === 1) {
                    this.data.streak++;
                } else {
                    this.data.streak = 1;
                }
            } else {
                this.data.streak = 1;
            }
            
            this.data.lastLogin = today;
            this.addXP(100);
            this.saveStats();
            return true;
        }
        return false;
    }

    // --- Unlock Code Generation ---
    
    generateUnlockCode() {
        const crypto = require('crypto');
        const SECRET = 'D33D33-SW4G-2024'; // Obfuscated in production
        
        // Payload: unlocked items + timestamp
        const payload = {
            u: this.data.unlockedAccessories,
            t: Date.now(),
            v: 1 // version
        };
        
        const payloadStr = JSON.stringify(payload);
        const payloadB64 = Buffer.from(payloadStr).toString('base64url');
        
        // Sign it
        const signature = crypto
            .createHmac('sha256', SECRET)
            .update(payloadStr)
            .digest('base64url')
            .substring(0, 12); // Short signature
        
        // Format: DEEDEE-{payload}-{sig}
        return `DEEDEE-${payloadB64}-${signature}`;
    }
    
    // Static method for decoding (used by website)
    static decodeUnlockCode(code) {
        const crypto = require('crypto');
        const SECRET = 'D33D33-SW4G-2024';
        
        try {
            const parts = code.split('-');
            if (parts.length !== 3 || parts[0] !== 'DEEDEE') {
                return { valid: false, error: 'Invalid code format' };
            }
            
            const payloadB64 = parts[1];
            const signature = parts[2];
            
            const payloadStr = Buffer.from(payloadB64, 'base64url').toString();
            
            // Verify signature
            const expectedSig = crypto
                .createHmac('sha256', SECRET)
                .update(payloadStr)
                .digest('base64url')
                .substring(0, 12);
            
            if (signature !== expectedSig) {
                return { valid: false, error: 'Invalid signature' };
            }
            
            const payload = JSON.parse(payloadStr);
            
            // Check timestamp (codes expire after 30 days)
            const age = Date.now() - payload.t;
            const maxAge = 30 * 24 * 60 * 60 * 1000;
            if (age > maxAge) {
                return { valid: false, error: 'Code expired' };
            }
            
            return { valid: true, unlocked: payload.u };
        } catch (e) {
            return { valid: false, error: 'Decode error' };
        }
    }

    // --- Public Getters ---

    getStats() {
        return {
            xp: this.data.xp,
            level: this.data.level,
            nextLevelXp: this.getRequiredXP(this.data.level),
            streak: this.data.streak,
            accessories: this.data.unlockedAccessories
        };
    }
    
    // Set what the user wants to wear
    setEquipped(equipped) {
        // Only allow equipping unlocked items
        this.data.equippedAccessories = equipped.filter(id => 
            this.data.unlockedAccessories.includes(id)
        );
        this.saveStats();
    }
    
    // Get data for the mascot window - ONLY reveals unlocked items
    getMascotData() {
        const unlockedItems = this.data.unlockedAccessories.map(id => ({
            id,
            name: this.accessoryMeta[id]?.name || 'Unknown',
            icon: this.accessoryMeta[id]?.icon || '❓',
            rarity: this.accessoryMeta[id]?.rarity || 'common'
        }));
        
        const secretCount = this.totalAccessories - this.data.unlockedAccessories.length;
        
        // Get locked items with hints (but not names)
        const lockedItems = Object.keys(this.accessoryMeta)
            .filter(id => !this.data.unlockedAccessories.includes(id))
            .map(id => ({
                id,
                hint: this.getUnlockHint(id),
                rarity: this.accessoryMeta[id]?.rarity || 'common'
            }));
        
        return {
            unlocked: unlockedItems,
            locked: lockedItems,
            equipped: this.data.equippedAccessories,
            secretCount: secretCount,
            totalCount: this.totalAccessories
        };
    }
    
    // Get all accessories for wardrobe display
    getWardrobeData() {
        return Object.entries(this.accessoryMeta).map(([id, meta]) => ({
            id,
            name: this.data.unlockedAccessories.includes(id) ? meta.name : '???',
            icon: this.data.unlockedAccessories.includes(id) ? meta.icon : '🔒',
            rarity: meta.rarity,
            unlocked: this.data.unlockedAccessories.includes(id),
            equipped: this.data.equippedAccessories.includes(id),
            hint: this.data.unlockedAccessories.includes(id) ? null : this.getUnlockHint(id)
        }));
    }
    
    // Get CSS classes based on EQUIPPED items (user choice)
    getAccessoryClasses() {
        const equipped = this.data.equippedAccessories;
        return equipped.map(id => `has-${id}`);
    }
}

module.exports = GamificationManager;
