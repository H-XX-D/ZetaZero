/**
 * Usage Tracking Service
 * Tracks API usage and credits for licenses
 */

// In-memory usage store (replace with database in production)
const usageStore = new Map();

/**
 * Track API usage
 * @param {string} licenseKey - License key
 * @param {object} usage - Usage data
 */
export async function trackUsage(licenseKey, usage) {
    const timestamp = new Date().toISOString();
    
    // Get or create usage record
    if (!usageStore.has(licenseKey)) {
        usageStore.set(licenseKey, {
            totalRequests: 0,
            totalTokens: 0,
            totalCost: 0,
            requests: []
        });
    }

    const record = usageStore.get(licenseKey);
    record.totalRequests++;
    record.totalTokens += usage.tokensUsed || 0;
    record.totalCost += usage.cost || 0;
    record.requests.push({
        ...usage,
        timestamp
    });

    // Keep only last 1000 requests
    if (record.requests.length > 1000) {
        record.requests = record.requests.slice(-1000);
    }

    // TODO: In production, save to database
    // const db = await getDatabase();
    // await db.usage.insertOne({ licenseKey, ...usage, timestamp });
    
    // TODO: Deduct credits for non-Pro+ users
    // if (usage.tier !== 'proplus') {
    //     await deductCredits(licenseKey, usage.cost);
    // }
}

/**
 * Get usage statistics
 */
export async function getUsage(licenseKey) {
    return usageStore.get(licenseKey) || {
        totalRequests: 0,
        totalTokens: 0,
        totalCost: 0,
        requests: []
    };
}

/**
 * Get all usage (for admin)
 */
export function getAllUsage() {
    return Array.from(usageStore.entries()).map(([key, usage]) => ({
        licenseKey: key.substring(0, 8) + '...',
        ...usage
    }));
}

