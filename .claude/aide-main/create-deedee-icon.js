const sharp = require('sharp');
const path = require('path');
const fs = require('fs');

const assetsDir = path.join(__dirname, 'assets');

// Create Dee Dee's head as SVG - matching the actual mascot exactly
// Mascot: 70x55px pill shape, eyes 14px, mouth between eyes
const deeDeeSvg = `
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <!-- Transparent background -->
  
  <defs>
    <!-- Exact gradient from mascot.html -->
    <linearGradient id="headGradient" x1="0%" y1="0%" x2="0%" y2="100%">
      <stop offset="0%" style="stop-color:#f0f0f0"/>
      <stop offset="30%" style="stop-color:#c8c8c8"/>
      <stop offset="70%" style="stop-color:#a8a8a8"/>
      <stop offset="100%" style="stop-color:#d0d0d0"/>
    </linearGradient>
    <!-- Inner highlights like the mascot -->
    <linearGradient id="innerHighlight" x1="0%" y1="0%" x2="0%" y2="100%">
      <stop offset="0%" style="stop-color:rgba(255,255,255,0.6)"/>
      <stop offset="100%" style="stop-color:rgba(0,0,0,0.1)"/>
    </linearGradient>
    <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="6" stdDeviation="8" flood-opacity="0.4"/>
    </filter>
  </defs>
  
  <!-- Main head - pill shape filling the icon better -->
  <rect x="20" y="100" width="472" height="340" rx="140" ry="140" 
        fill="url(#headGradient)" 
        stroke="#888" stroke-width="8"
        filter="url(#shadow)"/>
  
  <!-- Inner highlight overlay -->
  <rect x="35" y="115" width="442" height="150" rx="120" ry="70" 
        fill="rgba(255,255,255,0.15)"/>
  
  <!-- Left eye - pushed to edge -->
  <circle cx="115" cy="270" r="34" fill="#333"/>
  
  <!-- Mouth - curved smile between eyes, raised up -->
  <path d="M 210 240 Q 256 280 302 240" 
        stroke="#555" stroke-width="12" fill="none" stroke-linecap="round"/>
  
  <!-- Right eye - pushed to edge -->
  <circle cx="397" cy="270" r="34" fill="#333"/>
</svg>
`;

async function createIcon() {
    try {
        // Create main icon (512x512)
        await sharp(Buffer.from(deeDeeSvg))
            .resize(512, 512)
            .png()
            .toFile(path.join(assetsDir, 'icon.png'));
        
        console.log('✅ Created icon.png (512x512) with transparent background');
        
        // Create 1024x1024 for macOS
        await sharp(Buffer.from(deeDeeSvg))
            .resize(1024, 1024)
            .png()
            .toFile(path.join(assetsDir, 'icon-1024.png'));
        
        console.log('✅ Created icon-1024.png (1024x1024)');
        
        // Create tray icon (18x18 for macOS menu bar)
        await sharp(Buffer.from(deeDeeSvg))
            .resize(18, 18)
            .png()
            .toFile(path.join(assetsDir, 'tray-icon.png'));
        
        console.log('✅ Created tray-icon.png (18x18)');
        
        // Create @2x version for retina
        await sharp(Buffer.from(deeDeeSvg))
            .resize(36, 36)
            .png()
            .toFile(path.join(assetsDir, 'tray-icon@2x.png'));
        
        console.log('✅ Created tray-icon@2x.png (36x36)');
        
        // Create dock icon sizes
        const sizes = [16, 32, 64, 128, 256, 512, 1024];
        for (const size of sizes) {
            await sharp(Buffer.from(deeDeeSvg))
                .resize(size, size)
                .png()
                .toFile(path.join(assetsDir, `icon-${size}.png`));
        }
        
        console.log('✅ Created all icon sizes for .icns generation');
        console.log('\nTo create .icns for macOS, run:');
        console.log('  mkdir -p assets/icon.iconset');
        console.log('  cp assets/icon-16.png assets/icon.iconset/icon_16x16.png');
        console.log('  cp assets/icon-32.png assets/icon.iconset/icon_16x16@2x.png');
        console.log('  cp assets/icon-32.png assets/icon.iconset/icon_32x32.png');
        console.log('  cp assets/icon-64.png assets/icon.iconset/icon_32x32@2x.png');
        console.log('  cp assets/icon-128.png assets/icon.iconset/icon_128x128.png');
        console.log('  cp assets/icon-256.png assets/icon.iconset/icon_128x128@2x.png');
        console.log('  cp assets/icon-256.png assets/icon.iconset/icon_256x256.png');
        console.log('  cp assets/icon-512.png assets/icon.iconset/icon_256x256@2x.png');
        console.log('  cp assets/icon-512.png assets/icon.iconset/icon_512x512.png');
        console.log('  cp assets/icon-1024.png assets/icon.iconset/icon_512x512@2x.png');
        console.log('  iconutil -c icns assets/icon.iconset -o assets/icon.icns');
        
    } catch (err) {
        console.error('Error creating icons:', err);
    }
}

createIcon();

