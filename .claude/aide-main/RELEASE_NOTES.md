# AIDE v1.1.0 - Monaco Editor Fix & Provider Support

## 🐛 Bug Fixes
- **Fixed Monaco Editor Loading**: Improved Monaco editor initialization with better error handling and retry logic
- **Fixed AMD Loader Conflicts**: Resolved Node.js require conflicts preventing Monaco from loading in Electron
- **Improved Editor Reliability**: Added multiple fallback mechanisms to ensure editor loads after installation

## ✨ New Features
- **Google Gemini Support**: Added Google Gemini as a separate provider option
- **xAI Grok Support**: Added xAI Grok as a separate provider option
- **Enhanced API Provider Setup**: Improved setup instructions for all providers
- **Better Error Handling**: More detailed error messages for API connection issues

## 📚 Documentation
- **Business API Setup Guide**: Added comprehensive guide for linking business API accounts
- **Provider Architecture**: Added architecture documentation for API provider/reseller system
- **Hosting Recommendations**: Added detailed hosting guide for API backend and website

## 🔧 Technical Improvements
- **Backend Server Updates**: Updated server.js to listen on 0.0.0.0 for Railway compatibility
- **Railway Configuration**: Added railway.json for optimized Railway deployments
- **Monaco Loading**: Enhanced Monaco editor loading with polling and retry mechanisms
- **API Provider Models**: Expanded model lists for Google and xAI providers

## 📦 Infrastructure
- **Railway Support**: Full Railway deployment support with database and Redis
- **Provider API Keys**: Support for managing multiple provider API keys
- **Usage Tracking**: Foundation for usage tracking and billing system

---

# AIDE v1.0.0 - Initial Release

## 🎉 First Public Release!

AIDE (AI Development Environment) is a floating mini-IDE with a "Grammarly-style" dock behavior. Features Dee Dee, a silver pill-shaped mascot that lives at the edge of your screen.

## ✨ Features

### Core Features (All Tiers)
- 📝 **Monaco Editor** - VS Code's powerful editor engine
- 💬 **AI Chat** - Bring your own API key (OpenAI, Anthropic, etc.)
- 💻 **Terminal** - Full terminal with popout --help documentation
- 📁 **File Explorer** - Navigate and manage your files
- 🤖 **Dee Dee Mascot** - Docks to any screen edge (left, right, top, bottom)
- 🎨 **Beautiful Dark Theme** - Easy on the eyes
- 📋 **Git Integration** - Built-in Git panel for version control

### Pro Features ($5/month)
- 🎤 **Voice Activation** - Say "Hey Dee Dee" to summon
- ✅ **TODO List** - Built-in task management
- 🐛 **Debug Console** - Both Terminal AND Debug Console
- ↔️ **Drag & Drop** - Rearrange panels to your liking

### Pro+ Features ($15/month)
- 🔓 **Unlimited Sidebars** - Add as many as you need
- 💰 **$15 API Credit** - Included monthly
- 🤖 **AI CLI Access** - AI runs terminal commands
- 🎨 **Exclusive Drip** - Premium Dee Dee accessories

## 🚀 What's New

- Initial release of AIDE
- Cross-platform support (macOS, Windows, Linux)
- Dee Dee mascot with drip system
- Voice activation (Pro)
- Full IDE features with Monaco editor
- GitHub Pages website ready

## 📦 Downloads

### macOS
- **Installer**: `AIDE-1.0.0-arm64.dmg` (114 MB)
- Compatible with Apple Silicon (M1/M2/M3)

### Windows
- **Installer**: `AIDE Setup 1.0.0.exe` (94 MB)
- **Portable**: `AIDE 1.0.0.exe` (94 MB)
- Compatible with Windows 10/11 (x64)

### Linux
- **AppImage**: `AIDE-1.0.0.AppImage` (125 MB) - Universal, works on all distros
- **Debian Package**: `aide_1.0.0_amd64.deb` (87 MB) - For Ubuntu/Debian

## 🎮 Quick Start

1. **Download** the installer for your platform
2. **Install** (or run portable version)
3. **Dee Dee appears** at the screen edge
4. **Double-click** Dee Dee to open the IDE
5. **Configure** your AI API key in Settings

## 🐛 Known Issues

- File explorer may need a refresh on first load
- Eye click behavior cycling through states works but may need refinement
- Some features require Pro/Pro+ subscription

## 📝 System Requirements

- **macOS**: 11.0+ (Apple Silicon)
- **Windows**: Windows 10/11 (64-bit)
- **Linux**: Most modern distributions (AppImage) or Debian/Ubuntu (.deb)

## 🙏 Thank You

Thank you for trying AIDE! We're excited to see what you build with Dee Dee.

## 🔗 Links

- **Website**: https://h-xx-d.github.io/aide
- **GitHub**: https://github.com/H-XX-D/aide
- **Issues**: https://github.com/H-XX-D/aide/issues

---

**Built with ❤️ by ORKASTRATOR**

