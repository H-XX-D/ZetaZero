#!/bin/bash
cd "$(dirname "$0")"

echo "🚀 Deploying AIDE Backend API..."

# Check if Railway CLI is installed
if ! command -v railway &> /dev/null; then
    echo "❌ Railway CLI not found. Install it first:"
    echo "   curl -fsSL https://railway.app/install.sh | sh"
    exit 1
fi

# Link if not already linked
if [ ! -f .railway/config.json ]; then
    echo "📎 Linking to Railway project..."
    railway link
fi

# Set variables (update with your keys)
echo "🔐 Setting environment variables..."
railway variables set NODE_ENV=production
railway variables set PORT=3000

# Deploy
echo "🚀 Deploying..."
railway up

echo "✅ Deployment complete!"
echo "📊 View logs: railway logs"
echo "🌐 Open dashboard: railway open"
