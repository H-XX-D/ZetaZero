/**
 * Health Check Route
 * GET /health or GET /v1/health
 */

export function healthHandler(req, res) {
    res.json({
        status: 'ok',
        timestamp: new Date().toISOString(),
        uptime: process.uptime(),
        environment: process.env.NODE_ENV || 'development',
        version: process.env.npm_package_version || '1.0.0'
    });
}

