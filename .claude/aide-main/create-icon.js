const sharp = require('sharp');
const path = require('path');
const fs = require('fs');

const assetsDir = path.join(__dirname, 'assets');
const svgPath = path.join(assetsDir, 'icon.svg');
const pngPath = path.join(assetsDir, 'icon.png');

async function createIcon() {
    try {
        // Read SVG and convert to PNG
        await sharp(svgPath)
            .resize(512, 512)
            .png()
            .toFile(pngPath);
        
        console.log('✅ Created icon.png (512x512)');
        
        // Also create tray icon (smaller)
        await sharp(svgPath)
            .resize(22, 22)
            .png()
            .toFile(path.join(assetsDir, 'tray-icon.png'));
        
        console.log('✅ Created tray-icon.png (22x22)');
        
        // Create @2x version for retina
        await sharp(svgPath)
            .resize(44, 44)
            .png()
            .toFile(path.join(assetsDir, 'tray-icon@2x.png'));
        
        console.log('✅ Created tray-icon@2x.png (44x44)');
        
    } catch (err) {
        console.error('Error creating icons:', err);
    }
}

createIcon();

