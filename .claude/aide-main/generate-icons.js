/**
 * Icon Generator for AIDE
 * Run: node generate-icons.js
 * 
 * Creates icon.png from the SVG for dock/tray
 * Requires: npm install sharp (optional, falls back to manual)
 */

const fs = require('fs');
const path = require('path');

// Simple PNG icon as base64 (Dee Dee's face - silver pill with dark eyes and smile)
// This is a 512x512 PNG encoded as base64
const ICON_PNG_BASE64 = `iVBORw0KGgoAAAANSUhEUgAAAgAAAAIACAYAAAD0eNT6AAAACXBIWXMAAAsTAAALEwEAmpwYAAAK
T2lDQ1BQaG90b3Nob3AgSUNDIHByb2ZpbGUAAHjanVNnVFPpFj333vRCS4iAlEtvUhUIIFJCi4AU
kSYqIQkQSoghodkVUcERRUUEG8igiAOOjoCMFVEsDIoK2AfkIaKOg6OIisr74Xuja9a89+bN/rXX
Pues852zzwfACAyWSDNRNYAMqUIeEeCDx8TG4eQuQIEKJHAAEAizZCFz/SMBAPh+PDwrIsAHvgAB
eNMLCADATZvAMByH/w/qQplcAYCEAcB0kThLCIAUAEB6jkKmAEBGAYCdmCZTAKAEAGDLY2LjAFAt
AGAnf+bTAICd+Jl7AQBblCEVAaCRACATZYhEAGg7AKzPVopFAFgwABRmS8Q5ANgtADBJV2ZIALC3
AMDOEAuyAAgMADBRiIUpAAR7AGDIIyN4AISZABRG8lc88SuuEOcqAAB4mbI8uSQ5RYFbCC1xB1dX
Lh4ozkkXKxQ2YQJhmkAuwnmZGTKBNA/g88wAAKCRFRHgg/P9eM4Ors7ONo62Voices8/Pnvt/idf
PzO+f3/++f/5x8/Pnvv/f/5/////Pn/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5////
/P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v
/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5
/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5////
/P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////P/v/f/5/////wAA
AABJRU5ErkJggg==`;

// Create a proper PNG icon programmatically
function createIconPNG() {
    const assetsDir = path.join(__dirname, 'assets');
    
    // Ensure assets directory exists
    if (!fs.existsSync(assetsDir)) {
        fs.mkdirSync(assetsDir, { recursive: true });
    }
    
    // Try to use sharp if available
    try {
        const sharp = require('sharp');
        const svgPath = path.join(assetsDir, 'icon.svg');
        
        if (fs.existsSync(svgPath)) {
            // Convert SVG to PNG at multiple sizes
            const sizes = [16, 32, 64, 128, 256, 512, 1024];
            
            sizes.forEach(size => {
                sharp(svgPath)
                    .resize(size, size)
                    .png()
                    .toFile(path.join(assetsDir, `icon-${size}.png`))
                    .then(() => console.log(`Created icon-${size}.png`))
                    .catch(err => console.error(`Error creating icon-${size}.png:`, err));
            });
            
            // Main icon
            sharp(svgPath)
                .resize(512, 512)
                .png()
                .toFile(path.join(assetsDir, 'icon.png'))
                .then(() => console.log('Created icon.png'))
                .catch(err => console.error('Error creating icon.png:', err));
                
            return true;
        }
    } catch (e) {
        console.log('Sharp not available, using fallback method');
    }
    
    console.log('\n📌 To create icons manually:');
    console.log('1. Open assets/icon.svg in a browser');
    console.log('2. Screenshot at 512x512');
    console.log('3. Save as assets/icon.png');
    console.log('\nOr install sharp: npm install sharp');
    console.log('Then run: node generate-icons.js\n');
    
    return false;
}

// Create tray icon (smaller, simpler)
function createTrayIcon() {
    const assetsDir = path.join(__dirname, 'assets');
    const trayIconPath = path.join(assetsDir, 'tray-icon.png');
    
    // Simple 22x22 tray icon SVG
    const traySvg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 22 22">
  <rect x="3" y="6" width="16" height="10" rx="5" ry="5" fill="#c8c8c8" stroke="#888" stroke-width="1"/>
  <circle cx="8" cy="11" r="2" fill="#333"/>
  <circle cx="14" cy="11" r="2" fill="#333"/>
  <path d="M 9 14 Q 11 16 13 14" stroke="#555" stroke-width="1" fill="none" stroke-linecap="round"/>
</svg>`;
    
    fs.writeFileSync(path.join(assetsDir, 'tray-icon.svg'), traySvg);
    console.log('Created tray-icon.svg');
    
    return true;
}

// Run
console.log('🎨 AIDE Icon Generator\n');
createIconPNG();
createTrayIcon();

