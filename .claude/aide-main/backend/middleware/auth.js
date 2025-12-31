/**
 * Authentication Middleware
 * Validates license key from Authorization header
 */

import { validateLicense } from '../services/license.js';

export async function authMiddleware(req, res, next) {
    try {
        // Get license key from Authorization header
        const authHeader = req.headers.authorization;
        
        if (!authHeader || !authHeader.startsWith('Bearer ')) {
            return res.status(401).json({ 
                error: 'Unauthorized',
                message: 'Missing or invalid Authorization header. Use: Bearer <license-key>'
            });
        }

        const licenseKey = authHeader.substring(7); // Remove 'Bearer ' prefix

        // Validate license
        const licenseInfo = await validateLicense(licenseKey);
        
        if (!licenseInfo) {
            return res.status(401).json({ 
                error: 'Unauthorized',
                message: 'Invalid or expired license key'
            });
        }

        // Attach license info to request
        req.licenseKey = licenseKey;
        req.licenseInfo = licenseInfo;

        next();
    } catch (error) {
        console.error('Auth Middleware Error:', error);
        res.status(500).json({ 
            error: 'Internal server error',
            message: error.message 
        });
    }
}

