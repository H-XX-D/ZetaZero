/**
 * Global Error Handler Middleware
 */

export function errorHandler(err, req, res, next) {
    console.error('API Error:', err);

    // Don't leak error details in production
    const isDevelopment = process.env.NODE_ENV === 'development';

    res.status(err.status || 500).json({
        error: err.message || 'Internal server error',
        ...(isDevelopment && { stack: err.stack })
    });
}

