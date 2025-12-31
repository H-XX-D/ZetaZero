#!/usr/bin/env node

/**
 * Soundbite Downloader Script
 * Downloads audio clips from legal sources and organizes them for Dee Dee
 * 
 * NOTE: Most movie/TV quotes are copyrighted. This script helps you:
 * 1. Download from royalty-free sources
 * 2. Organize files you've legally obtained
 * 3. Create your own recordings/impressions
 */

const fs = require('fs');
const path = require('path');
const https = require('https');
const http = require('http');

const AUDIO_DIR = path.join(__dirname, '..', 'assets', 'audio', 'soundbites');
const MAPPING_FILE = path.join(__dirname, '..', 'src', 'renderer', 'deedee-tts.js');

// Ensure directory exists
if (!fs.existsSync(AUDIO_DIR)) {
    fs.mkdirSync(AUDIO_DIR, { recursive: true });
}

/**
 * Download a file from URL
 */
function downloadFile(url, filepath) {
    return new Promise((resolve, reject) => {
        const protocol = url.startsWith('https') ? https : http;
        const file = fs.createWriteStream(filepath);
        
        protocol.get(url, (response) => {
            if (response.statusCode === 301 || response.statusCode === 302) {
                // Handle redirects
                return downloadFile(response.headers.location, filepath)
                    .then(resolve)
                    .catch(reject);
            }
            
            if (response.statusCode !== 200) {
                file.close();
                fs.unlinkSync(filepath);
                reject(new Error(`Failed to download: ${response.statusCode}`));
                return;
            }
            
            response.pipe(file);
            
            file.on('finish', () => {
                file.close();
                resolve();
            });
        }).on('error', (err) => {
            file.close();
            if (fs.existsSync(filepath)) {
                fs.unlinkSync(filepath);
            }
            reject(err);
        });
    });
}

/**
 * Legal sources for sound effects (royalty-free)
 */
const LEGAL_SOURCES = {
    freesound: {
        baseUrl: 'https://freesound.org',
        requiresApiKey: true,
        note: 'Requires free account and API key'
    },
    pixabay: {
        baseUrl: 'https://pixabay.com/sound-effects',
        requiresApiKey: false,
        note: 'Free downloads, attribution required'
    },
    zapsplat: {
        baseUrl: 'https://www.zapsplat.com',
        requiresApiKey: false,
        note: 'Free account required, attribution required'
    }
};

/**
 * Manual download guide - prints instructions for downloading from various sources
 */
function printDownloadGuide() {
    console.log(`
╔══════════════════════════════════════════════════════════════╗
║         Dee Dee Soundbite Download Guide                     ║
╚══════════════════════════════════════════════════════════════╝

⚠️  IMPORTANT LEGAL NOTE:
   Most movie/TV quotes are copyrighted. You have these options:

   1. CREATE YOUR OWN IMPRESSIONS
      - Record yourself doing impressions of characters
      - Use voice changers/effects to match the style
      - This is legal for personal use

   2. USE ROYALTY-FREE ALTERNATIVES
      - Freesound.org (requires free account)
      - Pixabay.com (free, attribution required)
      - Zapsplat.com (free account, attribution required)

   3. OBTAIN PROPER LICENSES
      - Contact copyright holders
      - Use licensed sound libraries
      - Purchase from stock audio sites

📁 Your audio files should go in:
   ${AUDIO_DIR}

📝 File naming format:
   [source]_[character]_[phrase].mp3
   Example: terminator_ill_be_back.mp3

🔧 After downloading files manually:
   1. Place them in: ${AUDIO_DIR}
   2. Run: node scripts/download-soundbites.js --update-mapping
   3. This will auto-update the audioFileMap in deedee-tts.js

📋 Current mappings needed:
`);
    
    // Read current mappings from deedee-tts.js
    try {
        const ttsContent = fs.readFileSync(MAPPING_FILE, 'utf8');
        const mappingMatch = ttsContent.match(/audioFileMap:\s*\{([^}]+)\}/s);
        if (mappingMatch) {
            const mappings = mappingMatch[1];
            const fileMatches = mappings.matchAll(/"([^"]+)":\s*"([^"]+)"/g);
            console.log('   Phrases that need audio files:');
            for (const match of fileMatches) {
                const phrase = match[1];
                const filename = match[2];
                const filepath = path.join(AUDIO_DIR, filename);
                const exists = fs.existsSync(filepath);
                console.log(`   ${exists ? '✅' : '❌'} "${phrase}" → ${filename}`);
            }
        }
    } catch (e) {
        console.log('   Could not read current mappings');
    }
    
    console.log(`
🌐 Useful Resources:
   - Freesound: https://freesound.org (search for "movie quote", "character voice")
   - Pixabay: https://pixabay.com/sound-effects/search/movie%20quote/
   - YouTube Audio Library: https://www.youtube.com/audiolibrary
   - Internet Archive: https://archive.org/details/audio (public domain)

💡 Tips:
   - Search for "impression" or "parody" versions (often legal)
   - Use voice synthesis tools to create your own versions
   - Check Creative Commons licenses on audio sites
   - Consider using TTS with character voices as fallback
`);
}

/**
 * Update the audioFileMap in deedee-tts.js with files that exist
 */
function updateMapping() {
    console.log('📝 Updating audio file mappings...\n');
    
    try {
        let ttsContent = fs.readFileSync(MAPPING_FILE, 'utf8');
        const files = fs.readdirSync(AUDIO_DIR).filter(f => 
            f.match(/\.(mp3|wav|ogg|m4a)$/i)
        );
        
        console.log(`Found ${files.length} audio files:`);
        files.forEach(file => {
            console.log(`  ✅ ${file}`);
        });
        
        // Extract current mappings
        const mappingMatch = ttsContent.match(/(audioFileMap:\s*\{)([^}]+)(\})/s);
        if (mappingMatch) {
            const existingMappings = mappingMatch[2];
            const newMappings = [];
            
            // Parse existing mappings
            const existing = {};
            const regex = /"([^"]+)":\s*"([^"]+)"/g;
            let match;
            while ((match = regex.exec(existingMappings)) !== null) {
                existing[match[1]] = match[2];
            }
            
            // Check which files exist
            for (const [phrase, filename] of Object.entries(existing)) {
                const filepath = path.join(AUDIO_DIR, filename);
                if (fs.existsSync(filepath)) {
                    newMappings.push(`        "${phrase}": "${filename}"`);
                } else {
                    newMappings.push(`        "${phrase}": "${filename}", // ⚠️ File missing`);
                }
            }
            
            // Add any new files found
            for (const file of files) {
                const found = Object.values(existing).includes(file);
                if (!found) {
                    // Try to guess phrase from filename
                    const phrase = file.replace(/\.(mp3|wav|ogg|m4a)$/i, '')
                        .replace(/_/g, ' ')
                        .replace(/\b\w/g, l => l.toUpperCase());
                    newMappings.push(`        "${phrase.toLowerCase()}": "${file}", // ✨ New file`);
                }
            }
            
            const newMappingBlock = newMappings.join(',\n');
            ttsContent = ttsContent.replace(
                /(audioFileMap:\s*\{)([^}]+)(\})/s,
                `$1\n${newMappingBlock}\n    `
            );
            
            fs.writeFileSync(MAPPING_FILE, ttsContent);
            console.log('\n✅ Updated deedee-tts.js with current audio files!');
        }
    } catch (e) {
        console.error('❌ Error updating mappings:', e.message);
    }
}

/**
 * Create a template file listing all needed phrases
 */
function createDownloadList() {
    const listFile = path.join(AUDIO_DIR, 'DOWNLOAD_LIST.md');
    try {
        const ttsContent = fs.readFileSync(MAPPING_FILE, 'utf8');
        const mappingMatch = ttsContent.match(/audioFileMap:\s*\{([^}]+)\}/s);
        
        let content = `# Soundbite Download List\n\n`;
        content += `This file lists all phrases that need audio files.\n\n`;
        content += `## Instructions\n\n`;
        content += `1. Download or create audio files for each phrase below\n`;
        content += `2. Name them exactly as shown in the "Filename" column\n`;
        content += `3. Place them in this directory: \`${AUDIO_DIR}\`\n`;
        content += `4. Run: \`node scripts/download-soundbites.js --update-mapping\`\n\n`;
        content += `## Phrases Needed\n\n`;
        content += `| Phrase | Filename | Status |\n`;
        content += `|--------|----------|--------|\n`;
        
        if (mappingMatch) {
            const mappings = mappingMatch[1];
            const fileMatches = mappings.matchAll(/"([^"]+)":\s*"([^"]+)"/g);
            for (const match of fileMatches) {
                const phrase = match[1];
                const filename = match[2];
                const filepath = path.join(AUDIO_DIR, filename);
                const exists = fs.existsSync(filepath);
                content += `| "${phrase}" | \`${filename}\` | ${exists ? '✅ Exists' : '❌ Missing'} |\n`;
            }
        }
        
        fs.writeFileSync(listFile, content);
        console.log(`✅ Created download list: ${listFile}`);
    } catch (e) {
        console.error('❌ Error creating download list:', e.message);
    }
}

// CLI
const args = process.argv.slice(2);

if (args.includes('--help') || args.includes('-h')) {
    printDownloadGuide();
} else if (args.includes('--update-mapping') || args.includes('-u')) {
    updateMapping();
} else if (args.includes('--create-list') || args.includes('-l')) {
    createDownloadList();
} else {
    printDownloadGuide();
    console.log('\n💡 Run with --update-mapping to sync files with mappings');
    console.log('💡 Run with --create-list to generate a download checklist');
}
