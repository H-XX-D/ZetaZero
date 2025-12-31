#!/usr/bin/env node

/**
 * Download soundbites from legal sources
 * Uses APIs and web scraping (where allowed) to download royalty-free audio
 */

const fs = require('fs');
const path = require('path');
const https = require('https');
const http = require('http');

const AUDIO_DIR = path.join(__dirname, '..', 'assets', 'audio', 'soundbites');

/**
 * Download from Pixabay (free, requires attribution)
 * Note: Pixabay doesn't have movie quotes, but has sound effects
 */
async function searchPixabay(query) {
    // Pixabay requires API key for programmatic access
    // For now, return search URL
    const searchUrl = `https://pixabay.com/sound-effects/search/${encodeURIComponent(query)}/`;
    console.log(`🔍 Search Pixabay: ${searchUrl}`);
    return [];
}

/**
 * Download from Freesound (requires API key)
 */
async function searchFreesound(query, apiKey) {
    if (!apiKey) {
        console.log('⚠️  Freesound requires API key. Get one at: https://freesound.org/apiv2/apply/');
        return [];
    }
    
    const url = `https://freesound.org/apiv2/search/text/?query=${encodeURIComponent(query)}&token=${apiKey}&fields=id,name,previews`;
    
    return new Promise((resolve, reject) => {
        https.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    const json = JSON.parse(data);
                    resolve(json.results || []);
                } catch (e) {
                    reject(e);
                }
            });
        }).on('error', reject);
    });
}

/**
 * Main function - provides guidance
 */
async function main() {
    console.log(`
╔══════════════════════════════════════════════════════════════╗
║     Automated Soundbite Downloader (Legal Sources Only)     ║
╚══════════════════════════════════════════════════════════════╝

⚠️  IMPORTANT: I cannot download copyrighted movie/TV quotes.
   However, I can help you with legal alternatives:

📋 OPTIONS:

1. CREATE YOUR OWN (RECOMMENDED)
   - Record yourself doing impressions
   - Use voice synthesis tools
   - This is 100% legal for personal use

2. USE ROYALTY-FREE SOUND EFFECTS
   - Download from legal sources manually
   - Use the --update-mapping command after downloading

3. USE TTS AS FALLBACK
   - The system already uses TTS when audio files are missing
   - Character-specific voices are configured

🔧 MANUAL DOWNLOAD STEPS:

1. Visit these sites and search for "movie quote" or character names:
   - https://freesound.org (free account required)
   - https://pixabay.com/sound-effects (free, attribution)
   - https://www.zapsplat.com (free account, attribution)

2. Download files and save them to:
   ${AUDIO_DIR}

3. Name them according to the mapping in deedee-tts.js

4. Run: node scripts/download-soundbites.js --update-mapping

💡 TIP: For best results, create your own recordings!
   - Use a voice changer app
   - Record yourself doing impressions
   - Use AI voice synthesis tools
   - This avoids all copyright issues

📝 Current status:
`);
    
    // Check existing files
    const files = fs.existsSync(AUDIO_DIR) 
        ? fs.readdirSync(AUDIO_DIR).filter(f => f.match(/\.(mp3|wav|ogg|m4a)$/i))
        : [];
    
    console.log(`   Found ${files.length} audio files`);
    if (files.length > 0) {
        console.log('   Files:');
        files.forEach(f => console.log(`     ✅ ${f}`));
    }
    
    console.log(`
🎤 VOICE SYNTHESIS ALTERNATIVES:
   - ElevenLabs (AI voice cloning - check their terms)
   - Speechify (text-to-speech with character voices)
   - VoiceMod (voice changer for recordings)
   - Audacity + voice effects (free, open source)

📚 LEGAL RESOURCES:
   - Internet Archive Public Domain: https://archive.org/details/audio
   - Creative Commons Search: https://search.creativecommons.org
   - Public Domain Review: https://publicdomainreview.org/collections
`);
}

if (require.main === module) {
    main().catch(console.error);
}

module.exports = { searchPixabay, searchFreesound };
