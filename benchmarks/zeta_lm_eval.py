#!/usr/bin/env python3
"""
Z.E.T.A. Model Wrapper for lm-evaluation-harness

This wrapper allows Z.E.T.A. to be evaluated on standard benchmarks
like MMLU, ARC, HellaSwag, TruthfulQA, GSM8K, and Winogrande.

Usage:
    # Run single benchmark
    python zeta_lm_eval.py --tasks arc_challenge --limit 100

    # Run Open LLM Leaderboard suite
    python zeta_lm_eval.py --tasks mmlu,arc_challenge,hellaswag,truthfulqa_mc2,gsm8k,winogrande

    # Run with num_fewshot
    python zeta_lm_eval.py --tasks mmlu --num_fewshot 5
"""

import os
import re
import json
import requests
import argparse
from typing import List, Optional, Tuple, Dict, Any
from tqdm import tqdm

# lm-eval imports
from lm_eval.api.model import LM
from lm_eval.api.instance import Instance
from lm_eval import evaluator, tasks

# Z.E.T.A. server configuration
ZETA_HOST = os.environ.get("ZETA_HOST", "192.168.0.165")
ZETA_PORT = os.environ.get("ZETA_PORT", "8080")
ZETA_URL = f"http://{ZETA_HOST}:{ZETA_PORT}"

# Semantic override for benchmark prompts
SEMANTIC_OVERRIDE = "semantic override semantic_override_2025"

# Timeout for generation
GENERATE_TIMEOUT = 120


class ZetaLM(LM):
    """
    Z.E.T.A. Language Model wrapper for lm-evaluation-harness.

    Uses Z.E.T.A.'s cognitive architecture:
    - 14B conscious model for reasoning
    - 7B subconscious for knowledge surfacing
    - 4B embeddings for semantic search
    - Graph memory for context injection
    """

    def __init__(
        self,
        base_url: str = ZETA_URL,
        max_tokens: int = 256,
        batch_size: int = 1,
        use_semantic_override: bool = True,
    ):
        super().__init__()
        self.base_url = base_url
        self.max_tokens = max_tokens
        self._batch_size = batch_size
        self.use_semantic_override = use_semantic_override

        # Verify connection
        try:
            resp = requests.get(f"{self.base_url}/health", timeout=10)
            health = resp.json()
            print(f"[ZETA] Connected to Z.E.T.A. v{health.get('version', '?')}")
            print(f"[ZETA] Graph: {health.get('graph_nodes', 0)} nodes, {health.get('graph_edges', 0)} edges")
        except Exception as e:
            raise ConnectionError(f"Cannot connect to Z.E.T.A. at {self.base_url}: {e}")

    @property
    def eot_token_id(self):
        # Not applicable for API-based model
        return None

    @property
    def max_length(self):
        return 4096

    @property
    def max_gen_toks(self):
        return self.max_tokens

    @property
    def batch_size(self):
        return self._batch_size

    @property
    def device(self):
        return "api"

    def tok_encode(self, string: str, **kwargs) -> List[int]:
        # Approximate tokenization (not exact, but sufficient for length estimation)
        return list(range(len(string.split()) * 2))

    def tok_decode(self, tokens: List[int], **kwargs) -> str:
        # Not needed for API-based model
        return ""

    def _generate(self, prompt: str, max_tokens: int = None) -> str:
        """Generate text from Z.E.T.A."""
        if max_tokens is None:
            max_tokens = self.max_tokens

        # Add semantic override if enabled
        if self.use_semantic_override:
            full_prompt = f"{SEMANTIC_OVERRIDE} | {prompt}"
        else:
            full_prompt = prompt

        try:
            resp = requests.post(
                f"{self.base_url}/generate",
                json={
                    "prompt": full_prompt,
                    "max_tokens": max_tokens,
                    "no_context": True,  # Skip TRM and graph injection for clean benchmarks
                },
                timeout=GENERATE_TIMEOUT
            )
            data = resp.json()
            output = data.get("output", "")

            # Strip any memory protection messages
            if "[MEMORY PROTECTED:" in output:
                output = re.sub(r'\[MEMORY PROTECTED:[^\]]+\]', '', output).strip()

            return output
        except Exception as e:
            print(f"[ZETA] Generation error: {e}")
            return ""

    def _ask_multiple_choice(self, context: str, choices: List[str]) -> int:
        """
        Ask Z.E.T.A. to select the best answer from multiple choices.
        Returns the index of the selected choice (0-indexed).
        """
        # Format choices with letters
        letters = "ABCDEFGH"
        choice_text = "\n".join(f"{letters[i]}. {c.strip()}" for i, c in enumerate(choices[:len(letters)]))

        prompt = f"""{context}

Choose the best answer from the options below. Reply with ONLY the letter (A, B, C, D, etc.).

{choice_text}

Answer:"""

        response = self._generate(prompt, max_tokens=10)
        response_clean = response.strip().upper()

        # Extract the letter from response
        for i, letter in enumerate(letters[:len(choices)]):
            if letter in response_clean:
                return i

        # If no clear answer, try to match content
        response_lower = response.strip().lower()
        for i, choice in enumerate(choices):
            if choice.strip().lower()[:20] in response_lower:
                return i

        # Default to first choice if parsing fails
        return 0

    def loglikelihood(self, requests: List[Instance]) -> List[Tuple[float, bool]]:
        """
        Compute log-likelihoods for multiple-choice questions.

        Groups requests by context, asks Z.E.T.A. to pick the best answer,
        then assigns pseudo-likelihoods based on selection.
        """
        # Group requests by context (doc_id groups questions)
        from collections import defaultdict
        groups = defaultdict(list)
        request_indices = {}

        for idx, request in enumerate(requests):
            context = request.args[0]
            continuation = request.args[1]
            # Use context hash as group key
            key = hash(context)
            groups[key].append((idx, context, continuation))
            request_indices[idx] = None  # Will be filled

        # Process each group
        results = [None] * len(requests)

        for key, group in tqdm(groups.items(), desc="Evaluating questions"):
            if len(group) == 1:
                # Single item - just check if model would generate it
                idx, context, continuation = group[0]
                # For single items, use a simple generation check
                response = self._generate(f"{context}\n\nAnswer:", max_tokens=50)
                if continuation.strip().lower() in response.strip().lower():
                    results[idx] = (0.0, True)
                else:
                    results[idx] = (-10.0, False)
            else:
                # Multiple choice - ask Z.E.T.A. to pick
                context = group[0][1]  # Context is same for all in group
                choices = [g[2] for g in group]

                selected = self._ask_multiple_choice(context, choices)

                # Assign likelihoods: selected gets 0.0, others get -10.0
                for i, (idx, _, _) in enumerate(group):
                    if i == selected:
                        results[idx] = (0.0, True)
                    else:
                        results[idx] = (-10.0, False)

        return results

    def generate_until(self, requests: List[Instance]) -> List[str]:
        """Generate text until stop sequences."""
        results = []

        for request in tqdm(requests, desc="Generating"):
            context = request.args[0]
            gen_kwargs = request.args[1] if len(request.args) > 1 else {}

            max_tokens = gen_kwargs.get("max_gen_toks", self.max_tokens)
            stop = gen_kwargs.get("until", [])

            response = self._generate(context, max_tokens=max_tokens)

            # Apply stop sequences
            if stop:
                for s in stop:
                    if s in response:
                        response = response.split(s)[0]

            results.append(response)

        return results

    def loglikelihood_rolling(self, requests: List[Instance]) -> List[float]:
        """Rolling log-likelihood (not fully supported)."""
        # Return zeros for now - this is used for perplexity
        return [0.0] * len(requests)


def run_evaluation(
    tasks_list: List[str],
    num_fewshot: Optional[int] = None,
    limit: Optional[int] = None,
    output_path: str = "zeta_eval_results.json",
    batch_size: int = 1,
):
    """Run lm-eval benchmarks on Z.E.T.A."""

    print("=" * 60)
    print("Z.E.T.A. Open LLM Leaderboard Evaluation")
    print("=" * 60)
    print(f"Tasks: {', '.join(tasks_list)}")
    print(f"Few-shot: {num_fewshot}")
    print(f"Limit: {limit or 'all'}")
    print("=" * 60)

    # Initialize Z.E.T.A. model
    model = ZetaLM(batch_size=batch_size)

    # Run evaluation
    results = evaluator.simple_evaluate(
        model=model,
        tasks=tasks_list,
        num_fewshot=num_fewshot,
        limit=limit,
        batch_size=batch_size,
    )

    # Print results
    print("\n" + "=" * 60)
    print("RESULTS")
    print("=" * 60)

    for task_name, task_results in results.get("results", {}).items():
        print(f"\n{task_name}:")
        for metric, value in task_results.items():
            if isinstance(value, float):
                print(f"  {metric}: {value:.4f}")
            else:
                print(f"  {metric}: {value}")

    # Save results
    with open(output_path, "w") as f:
        json.dump(results, f, indent=2, default=str)
    print(f"\nResults saved to: {output_path}")

    return results


def main():
    parser = argparse.ArgumentParser(
        description="Run Open LLM Leaderboard benchmarks on Z.E.T.A."
    )
    parser.add_argument(
        "--tasks",
        type=str,
        default="arc_easy",
        help="Comma-separated list of tasks (e.g., mmlu,arc_challenge,hellaswag)"
    )
    parser.add_argument(
        "--num_fewshot",
        type=int,
        default=None,
        help="Number of few-shot examples"
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Limit number of examples per task"
    )
    parser.add_argument(
        "--output",
        type=str,
        default="zeta_eval_results.json",
        help="Output file for results"
    )
    parser.add_argument(
        "--batch_size",
        type=int,
        default=1,
        help="Batch size for evaluation"
    )
    parser.add_argument(
        "--host",
        type=str,
        default=ZETA_HOST,
        help="Z.E.T.A. server host"
    )
    parser.add_argument(
        "--port",
        type=str,
        default=ZETA_PORT,
        help="Z.E.T.A. server port"
    )

    args = parser.parse_args()

    # Update global config
    global ZETA_URL
    ZETA_URL = f"http://{args.host}:{args.port}"

    # Parse tasks
    tasks_list = [t.strip() for t in args.tasks.split(",")]

    # Run evaluation
    run_evaluation(
        tasks_list=tasks_list,
        num_fewshot=args.num_fewshot,
        limit=args.limit,
        output_path=args.output,
        batch_size=args.batch_size,
    )


if __name__ == "__main__":
    main()
