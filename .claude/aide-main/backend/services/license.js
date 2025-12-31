/**
 * License Validation Service
 * 
 * In production, this would connect to a database
 * For now, uses environment variables or a simple in-memory store
 */

// In-memory license store (replace with database in production)
const licenses = new Map();

// Initialize from environment variables (for testing)
if (process.env.TEST_LICENSE_KEY) {
    licenses.set(process.env.TEST_LICENSE_KEY, {
        tier: 'proplus',
        credits: 1000,
        expiresAt: null, // Never expires for Pro+
        preferredProvider: 'openai',
        preferredModel: 'gpt-4',
        features: ['voice', 'ai-chat', 'unlimited-sidebars']
    });
}

/**
 * Validate license key
 * @param {string} licenseKey - The license key to validate
 * @returns {Promise<object|null>} License info or null if invalid
 */
export async function validateLicense(licenseKey) {
    // In production, query database
    // For now, check in-memory store or environment
    
    if (!licenseKey) {
        return null;
    }

    // Check in-memory store
    if (licenses.has(licenseKey)) {
        const license = licenses.get(licenseKey);
        
        // Check expiration
        if (license.expiresAt && new Date(license.expiresAt) < new Date()) {
            return null;
        }
        
        return license;
    }

    // TODO: Query database in production
    // const db = await getDatabase();
    // const license = await db.licenses.findOne({ key: licenseKey });
    
    // For development, you can add test licenses here
    // Or use environment variables
    
    return null;
}

/**
 * Add a license (for testing/admin)
 */
export function addLicense(key, info) {
    licenses.set(key, info);
}

/**
 * Get all licenses (for admin)
 */
export function getAllLicenses() {
    return Array.from(licenses.entries()).map(([key, info]) => ({
        key: key.substring(0, 8) + '...', // Mask key
        ...info
    }));
}

