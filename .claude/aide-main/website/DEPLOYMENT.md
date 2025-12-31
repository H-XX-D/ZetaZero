# GitHub Pages Deployment Guide

## ✅ Pre-Deployment Checklist

- [x] All links use relative paths (no absolute `/` paths except external)
- [x] `.nojekyll` file created to disable Jekyll processing
- [x] `404.html` created for fallback routing
- [x] All external links have `rel="noopener noreferrer"`
- [x] Cross-browser compatibility fixes applied (webkit prefixes)
- [x] Accessibility improvements (aria-labels, proper labels)
- [x] Viewport meta tags added
- [x] HTML lang attributes set

## 📁 File Structure

```
website/
├── .nojekyll          # Disables Jekyll processing
├── 404.html          # Fallback router for GitHub Pages
├── index.html        # Main landing page
├── print.html        # 3D print generator
├── vercel.json       # Vercel config (optional, doesn't affect GitHub Pages)
└── assets/
    ├── deedee.png
    └── icon.png
```

## 🚀 Deployment Steps

### Option 1: Deploy from `website/` folder (Recommended)

1. **Push to GitHub:**
   ```bash
   git add website/
   git commit -m "Prepare website for GitHub Pages deployment"
   git push
   ```

2. **Configure GitHub Pages:**
   - Go to repository Settings → Pages
   - Source: Deploy from a branch
   - Branch: `main` (or your default branch)
   - Folder: `/website`
   - Click Save

3. **Access your site:**
   - URL will be: `https://[username].github.io/[repo-name]/`
   - Example: `https://orkastrator.github.io/aide/`

### Option 2: Deploy from root (if you move files)

If you want to deploy from root instead:

1. Move all files from `website/` to root
2. Configure GitHub Pages:
   - Source: Deploy from a branch
   - Branch: `main`
   - Folder: `/ (root)`

## 🔧 Important Notes

### Base Path Handling

If your repository name is not `aide`, you may need to update paths:

- Current: All paths are relative (e.g., `assets/deedee.png`, `print.html`)
- These work automatically with GitHub Pages base paths
- The `404.html` handles `/print` → `print.html` redirects

### Testing Locally

Before deploying, test locally:

```bash
cd website
python3 -m http.server 8000
# or
npx serve .
```

Then visit `http://localhost:8000`

### Download Links

Download links currently point to:
- `https://github.com/orkastrator/aide/releases/latest`

Make sure you have releases set up in your GitHub repository, or update these links to point to your actual download location.

## ✅ Verification

After deployment, verify:

- [ ] Homepage loads correctly
- [ ] Navigation links work (#features, #drip)
- [ ] Print page loads (`/print.html` or `/print`)
- [ ] All images load (assets/deedee.png, assets/icon.png)
- [ ] External links open in new tabs
- [ ] Download buttons link to GitHub releases
- [ ] Mobile responsive design works
- [ ] 404 page redirects work

## 🐛 Troubleshooting

### Images not loading
- Check that `assets/` folder is in the same directory as `index.html`
- Verify image paths are relative (not starting with `/`)

### 404 errors
- Ensure `.nojekyll` file exists
- Check that `404.html` is in the root of the deployed folder
- Verify GitHub Pages is enabled in repository settings

### Styles not applying
- Check browser console for CSS errors
- Verify all CSS is inline or paths are correct
- Clear browser cache

## 📝 Next Steps

After successful deployment:

1. Update README.md with website URL
2. Add website badge to repository
3. Set up custom domain (optional)
4. Configure GitHub Actions for automatic deployment (optional)

