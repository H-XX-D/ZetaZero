/**
 * License Validation Route
 * POST /v1/license/validate
 */

import { validateLicense } from '../services/license.js';

export async function licenseHandler(req, res) {
    try {
        const { licenseKey } = req.body;

        if (!licenseKey) {
            return res.status(400).json({ 
                error: 'License key required' 
            });
        }

        const licenseInfo = await validateLicense(licenseKey);

        if (!licenseInfo) {
            return res.status(401).json({ 
                valid: false,
                message: 'Invalid or expired license key'
            });
        }

        res.json({
            valid: true,
            tier: licenseInfo.tier,
            credits: licenseInfo.credits,
            expiresAt: licenseInfo.expiresAt,
            features: licenseInfo.features
        });

    } catch (error) {
        console.error('License Handler Error:', error);
        res.status(500).json({ 
            error: 'Internal server error',
            message: error.message 
        });
    }
}

