# AIDE Documentation

## Quick Links

- **Getting Started**: See main [README.md](../README.md)
- **Building**: See [BUILDING.md](BUILDING.md)
- **Deployment**: See [DEPLOYMENT.md](DEPLOYMENT.md)
- **Release Notes**: See [RELEASE_NOTES.md](RELEASE_NOTES.md)

## Building

### Prerequisites
- Node.js 18+
- npm

### Build Commands

```bash
# Build renderer bundle
npm run build:renderer

# Build for your platform
npm run build:mac      # macOS
npm run build:win      # Windows
npm run build:linux    # Linux
npm run build:all      # All platforms
```

### Build Outputs

- **macOS**: `dist/AIDE-1.0.0-arm64.dmg`
- **Windows**: `dist/AIDE Setup 1.0.0.exe` (installer) + `dist/AIDE 1.0.0.exe` (portable)
- **Linux**: `dist/AIDE-1.0.0.AppImage` (universal) + `dist/aide_1.0.0_amd64.deb` (Debian/Ubuntu)

## Deployment

### GitHub Pages

The website is automatically deployed via GitHub Actions from the `/website` folder.

**Site URL**: https://h-xx-d.github.io/aide/

### GitHub Releases

Upload build files from `dist/` folder to GitHub Releases:
- Tag: `v1.0.0`
- Title: `AIDE v1.0.0 - Initial Release`
- Upload: All `.dmg`, `.exe`, `.AppImage`, `.deb` files

## Troubleshooting

### Build Issues

- **Windows**: May need Wine for cross-compilation on macOS
- **Linux**: Requires proper metadata (homepage, author email, maintainer)
- **macOS**: Code signing optional but recommended

### Pages Issues

- Check Actions tab for build/deployment errors
- Verify `/website` folder exists in `main` branch
- Wait 1-2 minutes after enabling Pages

