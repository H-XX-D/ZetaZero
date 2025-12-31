/**
 * LLM Provider Service
 * Handles calls to various LLM providers (OpenAI, Anthropic, etc.)
 */

import OpenAI from 'openai';
import Anthropic from '@anthropic-ai/sdk';

// Initialize providers
const openai = process.env.OPENAI_API_KEY ? new OpenAI({
    apiKey: process.env.OPENAI_API_KEY
}) : null;

const anthropic = process.env.ANTHROPIC_API_KEY ? new Anthropic({
    apiKey: process.env.ANTHROPIC_API_KEY
}) : null;

/**
 * Call LLM provider
 * @param {string} provider - 'openai', 'anthropic', etc.
 * @param {string} model - Model name
 * @param {string} prompt - User prompt
 * @returns {Promise<object>} Response with text, tokens, cost
 */
export async function callLLMProvider(provider, model, prompt) {
    switch (provider.toLowerCase()) {
        case 'openai':
            return await callOpenAI(model, prompt);
        case 'anthropic':
        case 'claude':
            return await callAnthropic(model, prompt);
        default:
            throw new Error(`Unsupported provider: ${provider}`);
    }
}

/**
 * Call OpenAI API
 */
async function callOpenAI(model, prompt) {
    if (!openai) {
        throw new Error('OpenAI API key not configured');
    }

    try {
        const response = await openai.chat.completions.create({
            model: model || 'gpt-4',
            messages: [
                { role: 'user', content: prompt }
            ],
            temperature: 0.7,
            max_tokens: 2000
        });

        const content = response.choices[0]?.message?.content || '';
        const tokensUsed = response.usage?.total_tokens || 0;
        
        // Estimate cost (rough estimates, adjust based on actual pricing)
        const cost = estimateOpenAICost(model, tokensUsed);

        return {
            response: content,
            tokensUsed,
            cost
        };
    } catch (error) {
        console.error('OpenAI API Error:', error);
        throw new Error(`OpenAI API error: ${error.message}`);
    }
}

/**
 * Call Anthropic Claude API
 */
async function callAnthropic(model, prompt) {
    if (!anthropic) {
        throw new Error('Anthropic API key not configured');
    }

    try {
        const response = await anthropic.messages.create({
            model: model || 'claude-3-opus-20240229',
            max_tokens: 2000,
            messages: [
                { role: 'user', content: prompt }
            ]
        });

        const content = response.content[0]?.text || '';
        const tokensUsed = (response.usage?.input_tokens || 0) + (response.usage?.output_tokens || 0);
        
        // Estimate cost
        const cost = estimateAnthropicCost(model, tokensUsed);

        return {
            response: content,
            tokensUsed,
            cost
        };
    } catch (error) {
        console.error('Anthropic API Error:', error);
        throw new Error(`Anthropic API error: ${error.message}`);
    }
}

/**
 * Estimate OpenAI cost (per 1M tokens)
 */
function estimateOpenAICost(model, tokens) {
    const pricing = {
        'gpt-4': { input: 30, output: 60 },
        'gpt-4-turbo': { input: 10, output: 30 },
        'gpt-4o': { input: 5, output: 15 },
        'gpt-4o-mini': { input: 0.15, output: 0.6 },
        'gpt-3.5-turbo': { input: 0.5, output: 1.5 }
    };

    const modelPricing = pricing[model] || pricing['gpt-4'];
    // Rough estimate: assume 70% input, 30% output
    const estimatedCost = (tokens * 0.7 * modelPricing.input + tokens * 0.3 * modelPricing.output) / 1000000;
    return estimatedCost;
}

/**
 * Estimate Anthropic cost (per 1M tokens)
 */
function estimateAnthropicCost(model, tokens) {
    const pricing = {
        'claude-3-opus-20240229': { input: 15, output: 75 },
        'claude-3-sonnet-20240229': { input: 3, output: 15 },
        'claude-3-haiku-20240307': { input: 0.25, output: 1.25 },
        'claude-3-5-sonnet-20241022': { input: 3, output: 15 }
    };

    const modelPricing = pricing[model] || pricing['claude-3-haiku-20240307'];
    const estimatedCost = (tokens * 0.7 * modelPricing.input + tokens * 0.3 * modelPricing.output) / 1000000;
    return estimatedCost;
}

