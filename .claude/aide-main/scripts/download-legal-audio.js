#!/usr/bin/env node

/**
 * Download audio clips from legal sources
 * - Archive.org (public domain)
 * - Freesound (Creative Commons, requires API key)
 * - Other royalty-free sources
 */

const fs = require('fs');
const path = require('path');
const https = require('https');
const http = require('http');
const { URL } = require('url');

const AUDIO_DIR = path.join(__dirname, '..', 'assets', 'audio', 'soundbites');

// Ensure directory exists
if (!fs.existsSync(AUDIO_DIR)) {
    fs.mkdirSync(AUDIO_DIR, { recursive: true });
}

/**
 * Download file from URL
 */
function downloadFile(url, filepath) {
    return new Promise((resolve, reject) => {
        console.log(`📥 Downloading: ${url}`);
        const protocol = url.startsWith('https') ? https : http;
        const file = fs.createWriteStream(filepath);
        
        const request = protocol.get(url, (response) => {
            // Handle redirects
            if (response.statusCode === 301 || response.statusCode === 302) {
                file.close();
                fs.unlinkSync(filepath);
                return downloadFile(response.headers.location, filepath)
                    .then(resolve)
                    .catch(reject);
            }
            
            if (response.statusCode !== 200) {
                file.close();
                if (fs.existsSync(filepath)) {
                    fs.unlinkSync(filepath);
                }
                reject(new Error(`HTTP ${response.statusCode}: ${response.statusMessage}`));
                return;
            }
            
            const totalSize = parseInt(response.headers['content-length'], 10);
            let downloadedSize = 0;
            
            response.on('data', (chunk) => {
                downloadedSize += chunk.length;
                if (totalSize) {
                    const percent = ((downloadedSize / totalSize) * 100).toFixed(1);
                    process.stdout.write(`\r   Progress: ${percent}%`);
                }
            });
            
            response.pipe(file);
            
            file.on('finish', () => {
                file.close();
                console.log(`\n✅ Saved: ${path.basename(filepath)}`);
                resolve();
            });
        });
        
        request.on('error', (err) => {
            file.close();
            if (fs.existsSync(filepath)) {
                fs.unlinkSync(filepath);
            }
            reject(err);
        });
        
        request.setTimeout(30000, () => {
            request.destroy();
            file.close();
            if (fs.existsSync(filepath)) {
                fs.unlinkSync(filepath);
            }
            reject(new Error('Download timeout'));
        });
    });
}

/**
 * Search Archive.org for public domain audio
 */
async function searchArchiveOrg(query) {
    const searchUrl = `https://archive.org/advancedsearch.php?q=${encodeURIComponent(query)}&fl=identifier,title,mediatype&output=json`;
    
    return new Promise((resolve, reject) => {
        https.get(searchUrl, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    const json = JSON.parse(data);
                    resolve(json.response?.docs || []);
                } catch (e) {
                    resolve([]); // Return empty if parse fails
                }
            });
        }).on('error', () => resolve([]));
    });
}

/**
 * Get download URL from Archive.org item
 */
async function getArchiveDownloadUrl(identifier) {
    // Archive.org items have files accessible via:
    // https://archive.org/download/{identifier}/{filename}
    // We need to get the file list first
    const metadataUrl = `https://archive.org/metadata/${identifier}`;
    
    return new Promise((resolve, reject) => {
        https.get(metadataUrl, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    const json = JSON.parse(data);
                    // Find audio files
                    const audioFiles = (json.files || []).filter(f => 
                        f.format && (f.format.includes('MP3') || f.format.includes('VBR MP3') || f.format.includes('Ogg Vorbis'))
                    );
                    if (audioFiles.length > 0) {
                        const file = audioFiles[0];
                        resolve(`https://archive.org/download/${identifier}/${file.name}`);
                    } else {
                        resolve(null);
                    }
                } catch (e) {
                    resolve(null);
                }
            });
        }).on('error', () => resolve(null));
    });
}

/**
 * Search Freesound API
 */
async function searchFreesound(query, apiKey) {
    if (!apiKey) {
        return [];
    }
    
    const url = `https://freesound.org/apiv2/search/text/?query=${encodeURIComponent(query)}&token=${apiKey}&fields=id,name,previews,license&page_size=5`;
    
    return new Promise((resolve) => {
        https.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    const json = JSON.parse(data);
                    resolve(json.results || []);
                } catch (e) {
                    resolve([]);
                }
            });
        }).on('error', () => resolve([]));
    });
}

/**
 * Download from Freesound
 */
async function downloadFreesound(soundId, apiKey, filepath) {
    const url = `https://freesound.org/apiv2/sounds/${soundId}/download/?token=${apiKey}`;
    return downloadFile(url, filepath);
}

/**
 * Try to find and download audio for a phrase
 */
async function findAndDownloadAudio(phrase, filename) {
    const filepath = path.join(AUDIO_DIR, filename);
    
    // Skip if already exists
    if (fs.existsSync(filepath)) {
        console.log(`⏭️  Already exists: ${filename}`);
        return true;
    }
    
    console.log(`\n🔍 Searching for: "${phrase}"`);
    
    // Try Archive.org
    console.log('   Checking Archive.org...');
    const archiveResults = await searchArchiveOrg(phrase);
    if (archiveResults.length > 0) {
        console.log(`   Found ${archiveResults.length} results on Archive.org`);
        // Try first result
        const identifier = archiveResults[0].identifier;
        const downloadUrl = await getArchiveDownloadUrl(identifier);
        if (downloadUrl) {
            try {
                await downloadFile(downloadUrl, filepath);
                return true;
            } catch (e) {
                console.log(`   ❌ Download failed: ${e.message}`);
            }
        }
    }
    
    // Try Freesound if API key is available
    const freesoundKey = process.env.FREESOUND_API_KEY;
    if (freesoundKey) {
        console.log('   Checking Freesound...');
        const freesoundResults = await searchFreesound(phrase, freesoundKey);
        if (freesoundResults.length > 0) {
            console.log(`   Found ${freesoundResults.length} results on Freesound`);
            // Find CC0 or CC-BY licensed sounds
            const ccSound = freesoundResults.find(s => 
                s.license === 'http://creativecommons.org/publicdomain/zero/1.0/' ||
                s.license === 'http://creativecommons.org/licenses/by/3.0/'
            );
            if (ccSound && ccSound.previews && ccSound.previews['preview-hq-mp3']) {
                try {
                    await downloadFile(ccSound.previews['preview-hq-mp3'], filepath);
                    console.log(`   ✅ Downloaded from Freesound (${ccSound.name})`);
                    return true;
                } catch (e) {
                    console.log(`   ❌ Download failed: ${e.message}`);
                }
            }
        }
    } else {
        console.log('   ⚠️  Freesound API key not set (set FREESOUND_API_KEY env var)');
    }
    
    console.log(`   ❌ No legal source found for "${phrase}"`);
    return false;
}

/**
 * Main download function
 */
async function main() {
    console.log(`
╔══════════════════════════════════════════════════════════════╗
║     Legal Audio Downloader                                   ║
╚══════════════════════════════════════════════════════════════╝

⚠️  NOTE: Most movie/TV quotes are copyrighted and won't be
   available as free downloads. This script searches legal sources:
   - Archive.org (public domain)
   - Freesound (Creative Commons, requires API key)

📋 To use Freesound API:
   1. Sign up at https://freesound.org
   2. Get API key from https://freesound.org/apiv2/apply/
   3. Set: export FREESOUND_API_KEY=your_key_here

🔍 Starting search...
`);

    // Read phrases from deedee-tts.js
    const ttsFile = path.join(__dirname, '..', 'src', 'renderer', 'deedee-tts.js');
    const ttsContent = fs.readFileSync(ttsFile, 'utf8');
    const mappingMatch = ttsContent.match(/audioFileMap:\s*\{([^}]+)\}/s);
    
    if (!mappingMatch) {
        console.log('❌ Could not find audioFileMap in deedee-tts.js');
        return;
    }
    
    const mappings = [];
    const regex = /"([^"]+)":\s*"([^"]+)"/g;
    let match;
    while ((match = regex.exec(mappingMatch[1])) !== null) {
        mappings.push({
            phrase: match[1],
            filename: match[2]
        });
    }
    
    console.log(`Found ${mappings.length} phrases to search for\n`);
    
    let downloaded = 0;
    let skipped = 0;
    let failed = 0;
    
    // Download in batches to avoid rate limiting
    for (let i = 0; i < mappings.length; i++) {
        const { phrase, filename } = mappings[i];
        console.log(`\n[${i + 1}/${mappings.length}]`);
        
        const success = await findAndDownloadAudio(phrase, filename);
        if (success) {
            downloaded++;
        } else if (fs.existsSync(path.join(AUDIO_DIR, filename))) {
            skipped++;
        } else {
            failed++;
        }
        
        // Rate limiting - wait between requests
        if (i < mappings.length - 1) {
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
    }
    
    console.log(`
╔══════════════════════════════════════════════════════════════╗
║                    Download Summary                          ║
╚══════════════════════════════════════════════════════════════╝
   ✅ Downloaded: ${downloaded}
   ⏭️  Skipped (already exists): ${skipped}
   ❌ Not found: ${failed}

💡 Most movie quotes are copyrighted and won't be available.
   Consider:
   - Creating your own recordings/impressions
   - Using TTS (already configured as fallback)
   - Obtaining proper licenses for copyrighted material
`);
}

if (require.main === module) {
    main().catch(console.error);
}

module.exports = { findAndDownloadAudio, downloadFile };
