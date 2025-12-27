#!/usr/bin/env python3
"""
Z.E.T.A. LiteLLM Proxy Server
=============================
Bridges Z.E.T.A. C++ server to cloud LLM providers via LiteLLM.

Usage:
    python scripts/litellm_proxy.py [--port 8081] [--config zeta.conf]

Environment Variables:
    OPENAI_API_KEY      - For OpenAI models
    ANTHROPIC_API_KEY   - For Claude models
    LITELLM_MODEL       - Default model (e.g., "gpt-4o", "claude-3-sonnet")
"""

import os
import sys
import json
import argparse
import logging
from http.server import HTTPServer, BaseHTTPRequestHandler
from typing import Optional
import traceback

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='[LITELLM] %(asctime)s - %(levelname)s - %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

# Try to import litellm
try:
    import litellm
    from litellm import completion
    LITELLM_AVAILABLE = True
except ImportError:
    logger.warning("LiteLLM not installed. Run: pip install litellm")
    LITELLM_AVAILABLE = False


class LiteLLMConfig:
    """Configuration for the LiteLLM proxy."""

    def __init__(self):
        self.default_model = os.getenv("LITELLM_MODEL", "gpt-4o-mini")
        self.fallback_model = os.getenv("LITELLM_FALLBACK", "gpt-3.5-turbo")
        self.timeout = int(os.getenv("LITELLM_TIMEOUT", "120"))
        self.max_retries = int(os.getenv("LITELLM_RETRIES", "2"))

        # Model aliases for Z.E.T.A. routing
        self.model_map = {
            "14B": os.getenv("LITELLM_14B_MODEL", "claude-3-5-sonnet-20241022"),
            "7B": os.getenv("LITELLM_7B_MODEL", "gpt-4o-mini"),
            "reasoning": os.getenv("LITELLM_REASONING_MODEL", "claude-3-5-sonnet-20241022"),
            "code": os.getenv("LITELLM_CODE_MODEL", "gpt-4o"),
            "fast": os.getenv("LITELLM_FAST_MODEL", "gpt-3.5-turbo"),
        }

    def get_model(self, requested: str) -> str:
        """Resolve model alias to actual model name."""
        return self.model_map.get(requested, requested)


config = LiteLLMConfig()


class LiteLLMHandler(BaseHTTPRequestHandler):
    """HTTP request handler for LiteLLM proxy."""

    def log_message(self, format, *args):
        """Override to use our logger."""
        logger.info("%s - %s", self.address_string(), format % args)

    def send_json_response(self, data: dict, status: int = 200):
        """Send a JSON response."""
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def send_error_response(self, message: str, status: int = 500):
        """Send an error response."""
        self.send_json_response({"error": message, "success": False}, status)

    def do_GET(self):
        """Handle GET requests."""
        if self.path == "/health":
            self.send_json_response({
                "status": "ok",
                "litellm_available": LITELLM_AVAILABLE,
                "default_model": config.default_model,
            })
        elif self.path == "/models":
            self.send_json_response({
                "models": config.model_map,
                "default": config.default_model,
            })
        else:
            self.send_error_response("Not found", 404)

    def do_POST(self):
        """Handle POST requests."""
        if self.path == "/generate":
            self.handle_generate()
        elif self.path == "/chat":
            self.handle_chat()
        else:
            self.send_error_response("Not found", 404)

    def handle_generate(self):
        """Handle /generate endpoint - matches Z.E.T.A. server format."""
        if not LITELLM_AVAILABLE:
            self.send_error_response("LiteLLM not installed", 503)
            return

        try:
            # Parse request body
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode()
            request = json.loads(body)

            prompt = request.get("prompt", "")
            max_tokens = request.get("max_tokens", 512)
            temperature = request.get("temperature", 0.7)
            model = request.get("model", config.default_model)

            # Resolve model alias
            resolved_model = config.get_model(model)

            logger.info(f"Generating with {resolved_model} (max_tokens={max_tokens}, temp={temperature})")

            # Build messages for chat completion
            messages = [{"role": "user", "content": prompt}]

            # Add system prompt if provided
            if "system" in request:
                messages.insert(0, {"role": "system", "content": request["system"]})

            # Call LiteLLM
            response = completion(
                model=resolved_model,
                messages=messages,
                max_tokens=max_tokens,
                temperature=temperature,
                timeout=config.timeout,
            )

            # Extract response text
            output = response.choices[0].message.content

            # Return in Z.E.T.A. format
            self.send_json_response({
                "output": output,
                "model": resolved_model,
                "usage": {
                    "prompt_tokens": response.usage.prompt_tokens,
                    "completion_tokens": response.usage.completion_tokens,
                    "total_tokens": response.usage.total_tokens,
                },
                "success": True,
            })

        except json.JSONDecodeError as e:
            self.send_error_response(f"Invalid JSON: {e}", 400)
        except Exception as e:
            logger.error(f"Generation error: {e}")
            logger.error(traceback.format_exc())
            self.send_error_response(str(e), 500)

    def handle_chat(self):
        """Handle /chat endpoint - full chat completion format."""
        if not LITELLM_AVAILABLE:
            self.send_error_response("LiteLLM not installed", 503)
            return

        try:
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode()
            request = json.loads(body)

            messages = request.get("messages", [])
            max_tokens = request.get("max_tokens", 512)
            temperature = request.get("temperature", 0.7)
            model = request.get("model", config.default_model)

            resolved_model = config.get_model(model)

            logger.info(f"Chat with {resolved_model} ({len(messages)} messages)")

            response = completion(
                model=resolved_model,
                messages=messages,
                max_tokens=max_tokens,
                temperature=temperature,
                timeout=config.timeout,
            )

            # Return OpenAI-compatible format
            self.send_json_response({
                "id": response.id,
                "object": "chat.completion",
                "model": resolved_model,
                "choices": [{
                    "index": 0,
                    "message": {
                        "role": "assistant",
                        "content": response.choices[0].message.content,
                    },
                    "finish_reason": response.choices[0].finish_reason,
                }],
                "usage": {
                    "prompt_tokens": response.usage.prompt_tokens,
                    "completion_tokens": response.usage.completion_tokens,
                    "total_tokens": response.usage.total_tokens,
                },
            })

        except json.JSONDecodeError as e:
            self.send_error_response(f"Invalid JSON: {e}", 400)
        except Exception as e:
            logger.error(f"Chat error: {e}")
            logger.error(traceback.format_exc())
            self.send_error_response(str(e), 500)


def load_config_from_file(config_path: str):
    """Load configuration from zeta.conf file."""
    if not os.path.exists(config_path):
        return

    logger.info(f"Loading config from {config_path}")

    with open(config_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            if "=" in line:
                key, value = line.split("=", 1)
                key = key.strip()
                value = value.strip().strip('"').strip("'")

                # Map zeta.conf keys to environment variables
                env_map = {
                    "LITELLM_MODEL": "LITELLM_MODEL",
                    "LITELLM_14B_MODEL": "LITELLM_14B_MODEL",
                    "LITELLM_7B_MODEL": "LITELLM_7B_MODEL",
                    "LITELLM_REASONING_MODEL": "LITELLM_REASONING_MODEL",
                    "LITELLM_CODE_MODEL": "LITELLM_CODE_MODEL",
                    "LITELLM_TIMEOUT": "LITELLM_TIMEOUT",
                }

                if key in env_map and not os.getenv(env_map[key]):
                    os.environ[env_map[key]] = value
                    logger.info(f"  Set {key}={value}")


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. LiteLLM Proxy Server")
    parser.add_argument("--port", type=int, default=8081, help="Port to listen on")
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--config", default="zeta.conf", help="Config file path")
    args = parser.parse_args()

    # Load config file if exists
    for config_path in [args.config, os.path.expanduser("~/ZetaZero/zeta.conf"), "/etc/zeta/zeta.conf"]:
        if os.path.exists(config_path):
            load_config_from_file(config_path)
            break

    # Reload config after loading file
    global config
    config = LiteLLMConfig()

    if not LITELLM_AVAILABLE:
        logger.error("LiteLLM is not installed. Install with: pip install litellm")
        logger.error("Proxy will start but return errors for all requests.")

    # Check for API keys
    has_keys = bool(os.getenv("OPENAI_API_KEY") or os.getenv("ANTHROPIC_API_KEY"))
    if not has_keys:
        logger.warning("No API keys found. Set OPENAI_API_KEY or ANTHROPIC_API_KEY")

    server = HTTPServer((args.host, args.port), LiteLLMHandler)

    logger.info("=" * 50)
    logger.info("Z.E.T.A. LiteLLM Proxy Server")
    logger.info("=" * 50)
    logger.info(f"Listening on http://{args.host}:{args.port}")
    logger.info(f"Default model: {config.default_model}")
    logger.info(f"Model map: {config.model_map}")
    logger.info("=" * 50)
    logger.info("Endpoints:")
    logger.info("  GET  /health  - Health check")
    logger.info("  GET  /models  - List configured models")
    logger.info("  POST /generate - Generate (Z.E.T.A. format)")
    logger.info("  POST /chat    - Chat completion (OpenAI format)")
    logger.info("=" * 50)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        server.shutdown()


if __name__ == "__main__":
    main()
