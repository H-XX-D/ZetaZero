/**
 * AIDE Backend API Server
 * 
 * Handles Pro+ user API requests with included credits
 * Endpoint: https://api.aide.dev/v1/chat
 */

import express from 'express';
import cors from 'cors';
import helmet from 'helmet';
import rateLimit from 'express-rate-limit';
import dotenv from 'dotenv';
import { chatHandler } from './routes/chat.js';
import { healthHandler } from './routes/health.js';
import { licenseHandler } from './routes/license.js';
import { errorHandler } from './middleware/errorHandler.js';
import { authMiddleware } from './middleware/auth.js';

dotenv.config();

const app = express();
const PORT = process.env.PORT || 3000;

// Security middleware
app.use(helmet());
app.use(cors({
    origin: process.env.ALLOWED_ORIGINS?.split(',') || ['https://aide.dev', 'http://localhost:3000'],
    credentials: true
}));
app.use(express.json({ limit: '10mb' }));

// Rate limiting
const apiLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // Limit each IP to 100 requests per windowMs
    message: 'Too many requests from this IP, please try again later.',
    standardHeaders: true,
    legacyHeaders: false,
});

app.use('/v1/', apiLimiter);

// Routes
app.get('/health', healthHandler);
app.get('/v1/health', healthHandler);
app.post('/v1/license/validate', licenseHandler);
app.post('/v1/chat', authMiddleware, chatHandler);

// Error handling
app.use(errorHandler);

// 404 handler
app.use((req, res) => {
    res.status(404).json({ error: 'Not found' });
});

// Start server
const server = app.listen(PORT, '0.0.0.0', () => {
    console.log(`🚀 AIDE API Server running on port ${PORT}`);
    console.log(`📡 Environment: ${process.env.NODE_ENV || 'development'}`);
    console.log(`🔐 License validation: ${process.env.LICENSE_VALIDATION || 'enabled'}`);
});

// Export for Vercel/serverless
export default app;

// For Railway/Docker, keep server running
if (process.env.RAILWAY_ENVIRONMENT || process.env.DOCKER) {
    // Server already started above
}

