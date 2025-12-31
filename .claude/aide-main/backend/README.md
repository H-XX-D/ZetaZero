# AIDE Backend API Server

Backend API server for AIDE Pro+ users, providing included AI credits and license validation.

## 🚀 Quick Start

### 1. Install Dependencies

```bash
cd backend
npm install
```

### 2. Configure Environment

```bash
cp .env.example .env
# Edit .env with your API keys
```

### 3. Start Server

```bash
# Development (with auto-reload)
npm run dev

# Production
npm start
```

## 📡 API Endpoints

### Health Check
```
GET /health
GET /v1/health
```

### Chat (Pro+ Users)
```
POST /v1/chat
Authorization: Bearer <license-key>
Content-Type: application/json

{
  "prompt": "User's prompt here",
  "model": "gpt-4" // optional
}
```

**Response:**
```json
{
  "response": "AI response text",
  "usage": {
    "tokens": 150,
    "cost": 0.002,
    "duration": 1200
  },
  "model": "gpt-4",
  "provider": "openai"
}
```

### License Validation
```
POST /v1/license/validate
Content-Type: application/json

{
  "licenseKey": "license-key-here"
}
```

## 🔐 Authentication

All `/v1/chat` requests require a valid license key in the Authorization header:

```
Authorization: Bearer <license-key>
```

## 🗄️ Database Setup (Production)

For production, you'll need to:

1. **Set up a database** (MongoDB, PostgreSQL, etc.)
2. **Create license table:**
   ```sql
   CREATE TABLE licenses (
     key VARCHAR(255) PRIMARY KEY,
     tier VARCHAR(50),
     credits DECIMAL(10,2),
     expires_at TIMESTAMP,
     created_at TIMESTAMP DEFAULT NOW()
   );
   ```

3. **Create usage tracking table:**
   ```sql
   CREATE TABLE usage (
     id SERIAL PRIMARY KEY,
     license_key VARCHAR(255),
     tokens_used INTEGER,
     cost DECIMAL(10,6),
     model VARCHAR(100),
     provider VARCHAR(50),
     created_at TIMESTAMP DEFAULT NOW()
   );
   ```

4. **Update services:**
   - `services/license.js` - Connect to database
   - `services/usage.js` - Save usage to database

## 🔑 Environment Variables

| Variable | Description | Required |
|----------|-------------|----------|
| `PORT` | Server port | No (default: 3000) |
| `NODE_ENV` | Environment | No (default: development) |
| `OPENAI_API_KEY` | OpenAI API key | Yes (for OpenAI) |
| `ANTHROPIC_API_KEY` | Anthropic API key | Yes (for Anthropic) |
| `DEFAULT_LLM_PROVIDER` | Default provider | No (default: openai) |
| `DEFAULT_MODEL` | Default model | No (default: gpt-4) |
| `ALLOWED_ORIGINS` | CORS origins | No |

## 🧪 Testing

```bash
# Test health endpoint
curl http://localhost:3000/health

# Test chat endpoint (with test license)
curl -X POST http://localhost:3000/v1/chat \
  -H "Authorization: Bearer test-proplus-license-key-12345" \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Hello, world!", "model": "gpt-4"}'
```

## 📊 Usage Tracking

The backend tracks:
- Total requests per license
- Tokens used
- Cost per request
- Model and provider used
- Request duration

## 🚢 Deployment

### Using PM2

```bash
npm install -g pm2
pm2 start server.js --name aide-api
pm2 save
pm2 startup
```

### Using Docker

```dockerfile
FROM node:18-alpine
WORKDIR /app
COPY package*.json ./
RUN npm install --production
COPY . .
EXPOSE 3000
CMD ["node", "server.js"]
```

### Using Vercel/Netlify

The backend can be deployed as serverless functions. See deployment docs for details.

## 🔒 Security

- ✅ Helmet.js for security headers
- ✅ CORS protection
- ✅ Rate limiting (100 requests per 15 minutes)
- ✅ License key validation
- ✅ Input validation
- ✅ Error handling

## 📝 License Management

### Adding Test Licenses

Edit `services/license.js`:

```javascript
licenses.set('your-test-key', {
    tier: 'proplus',
    credits: 1000,
    expiresAt: null,
    preferredProvider: 'openai',
    preferredModel: 'gpt-4'
});
```

### Production License Management

In production, licenses should be:
1. Stored in a secure database
2. Validated cryptographically
3. Tied to user accounts
4. Managed through an admin panel

## 🐛 Troubleshooting

### "OpenAI API key not configured"
- Set `OPENAI_API_KEY` in `.env`

### "Invalid license key"
- Check license key format
- Verify license exists in database/store
- Check expiration date

### CORS errors
- Add your domain to `ALLOWED_ORIGINS` in `.env`

## 📚 Next Steps

1. Set up database connection
2. Implement proper license validation
3. Add admin dashboard
4. Set up monitoring/logging
5. Add more LLM providers
6. Implement credit system for non-Pro+ users

