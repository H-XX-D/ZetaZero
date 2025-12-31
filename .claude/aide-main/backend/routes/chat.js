/**
 * Chat API Route Handler
 * POST /v1/chat
 * 
 * Handles Pro+ user chat requests with LLM providers
 */

import { callLLMProvider } from '../services/llm.js';
import { trackUsage } from '../services/usage.js';
import { validateLicense } from '../services/license.js';

export async function chatHandler(req, res, next) {
    try {
        const { prompt, model } = req.body;
        const licenseKey = req.licenseKey; // Set by authMiddleware
        
        // Validate request
        if (!prompt || typeof prompt !== 'string') {
            return res.status(400).json({ 
                error: 'Invalid request',
                message: 'Prompt is required and must be a string'
            });
        }

        // Get license info
        const licenseInfo = await validateLicense(licenseKey);
        if (!licenseInfo) {
            return res.status(401).json({ 
                error: 'Invalid license',
                message: 'License key is invalid or expired'
            });
        }

        // Check if user has credits available
        if (licenseInfo.credits <= 0 && licenseInfo.tier !== 'proplus') {
            return res.status(403).json({ 
                error: 'Insufficient credits',
                message: 'No credits available. Please upgrade to Pro+ or add credits.'
            });
        }

        // Determine which LLM provider to use
        const provider = licenseInfo.preferredProvider || process.env.DEFAULT_LLM_PROVIDER || 'openai';
        const modelToUse = model || licenseInfo.preferredModel || process.env.DEFAULT_MODEL || 'gpt-4';

        // Call LLM provider
        const startTime = Date.now();
        let response;
        let tokensUsed = 0;
        let cost = 0;

        try {
            const llmResult = await callLLMProvider(provider, modelToUse, prompt);
            response = llmResult.response;
            tokensUsed = llmResult.tokensUsed || 0;
            cost = llmResult.cost || 0;
        } catch (error) {
            console.error('LLM Provider Error:', error);
            return res.status(500).json({ 
                error: 'LLM provider error',
                message: error.message || 'Failed to get response from LLM provider'
            });
        }

        const duration = Date.now() - startTime;

        // Track usage (deduct credits for non-Pro+ users)
        if (licenseInfo.tier !== 'proplus') {
            await trackUsage(licenseKey, {
                tokensUsed,
                cost,
                duration,
                model: modelToUse,
                provider
            });
        } else {
            // Track usage for Pro+ (for analytics, but don't deduct credits)
            await trackUsage(licenseKey, {
                tokensUsed,
                cost,
                duration,
                model: modelToUse,
                provider,
                tier: 'proplus'
            });
        }

        // Return response
        res.json({
            response: response,
            usage: {
                tokens: tokensUsed,
                cost: cost,
                duration: duration
            },
            model: modelToUse,
            provider: provider
        });

    } catch (error) {
        console.error('Chat Handler Error:', error);
        next(error);
    }
}

