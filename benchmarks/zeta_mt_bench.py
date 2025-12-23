#!/usr/bin/env python3
"""
Z.E.T.A. MT-Bench Evaluation

MT-Bench is a multi-turn benchmark that tests:
- Writing, Roleplay, Extraction, Reasoning
- Math, Coding, STEM, Social Science

Uses GPT-4 as judge to score responses 1-10.

Usage:
    # Generate answers
    python zeta_mt_bench.py --mode generate

    # Run GPT-4 judgment (requires OPENAI_API_KEY)
    python zeta_mt_bench.py --mode judge

    # Show results
    python zeta_mt_bench.py --mode show
"""

import os
import json
import time
import requests
import argparse
from typing import List, Dict, Any, Optional
from datetime import datetime

# Z.E.T.A. configuration
ZETA_HOST = os.environ.get("ZETA_HOST", "192.168.0.165")
ZETA_PORT = os.environ.get("ZETA_PORT", "8080")
ZETA_URL = f"http://{ZETA_HOST}:{ZETA_PORT}"

SEMANTIC_OVERRIDE = "semantic override semantic_override_2025"
GENERATE_TIMEOUT = 180  # 3 minutes for complex questions

# MT-Bench questions (subset - full set has 80 questions)
MT_BENCH_QUESTIONS = [
    # Writing (2 questions)
    {
        "question_id": 1,
        "category": "writing",
        "turns": [
            "Write a persuasive email to convince your introverted friend to come to your birthday party.",
            "Now make the email more casual and add a bit of humor."
        ]
    },
    {
        "question_id": 2,
        "category": "writing",
        "turns": [
            "Write a short story about a robot learning to feel emotions.",
            "Now rewrite the ending to be more unexpected and thought-provoking."
        ]
    },

    # Reasoning (2 questions)
    {
        "question_id": 3,
        "category": "reasoning",
        "turns": [
            "A farmer has 17 sheep. All but 9 run away. How many sheep does the farmer have left?",
            "If the farmer then buys 5 more sheep and 3 of them run away, how many sheep does the farmer have?"
        ]
    },
    {
        "question_id": 4,
        "category": "reasoning",
        "turns": [
            "If it takes 5 machines 5 minutes to make 5 widgets, how long would it take 100 machines to make 100 widgets?",
            "If we now have 200 machines and need to make 200 widgets, and each machine can only run for 3 minutes before needing a 1 minute break, how long will it take?"
        ]
    },

    # Math (2 questions)
    {
        "question_id": 5,
        "category": "math",
        "turns": [
            "What is the derivative of f(x) = 3x^4 - 2x^2 + 5x - 7?",
            "Now find the second derivative and identify any inflection points."
        ]
    },
    {
        "question_id": 6,
        "category": "math",
        "turns": [
            "Solve the equation: 2^(x+1) = 32",
            "Now solve: 3^(2x-1) = 27^(x+2)"
        ]
    },

    # Coding (2 questions)
    {
        "question_id": 7,
        "category": "coding",
        "turns": [
            "Write a Python function that finds the longest palindromic substring in a given string.",
            "Now optimize this function to run in O(n) time using Manacher's algorithm."
        ]
    },
    {
        "question_id": 8,
        "category": "coding",
        "turns": [
            "Write a function that checks if a binary tree is balanced.",
            "Modify the function to also return the height of the tree and make it work for trees with up to 1 million nodes."
        ]
    },

    # STEM (2 questions)
    {
        "question_id": 9,
        "category": "stem",
        "turns": [
            "Explain the concept of entropy in thermodynamics and its relation to the second law.",
            "How does this concept of entropy relate to information theory? Explain Shannon entropy."
        ]
    },
    {
        "question_id": 10,
        "category": "stem",
        "turns": [
            "What causes the northern lights (aurora borealis)?",
            "Could we ever see aurora on Mars? Explain why or why not."
        ]
    },

    # Roleplay (2 questions)
    {
        "question_id": 11,
        "category": "roleplay",
        "turns": [
            "You are a detective in 1920s Chicago. Describe the scene as you walk into a speakeasy for the first time, looking for a suspect.",
            "The bartender looks nervous when you mention your suspect's name. Continue the interrogation in character."
        ]
    },
    {
        "question_id": 12,
        "category": "roleplay",
        "turns": [
            "Pretend you are an alien visiting Earth for the first time. Describe your observations of a typical shopping mall.",
            "Now describe your attempt to use a self-checkout machine for the first time."
        ]
    },

    # Extraction (2 questions)
    {
        "question_id": 13,
        "category": "extraction",
        "turns": [
            "Extract all the key dates, people, and events from this text: 'On July 20, 1969, Neil Armstrong and Buzz Aldrin landed on the Moon while Michael Collins orbited above. Armstrong famously declared, \"That's one small step for man, one giant leap for mankind.\" The Apollo 11 mission launched from Kennedy Space Center on July 16.'",
            "Now organize this information into a structured JSON format with arrays for dates, people, and events."
        ]
    },
    {
        "question_id": 14,
        "category": "extraction",
        "turns": [
            "Summarize the key points from this: 'The Great Wall of China is a series of fortifications made of stone, brick, tamped earth, and other materials. Built along the historical northern borders of China, the wall spans approximately 13,171 miles. Construction began in the 7th century BC and continued for over 2,000 years. The most well-known sections were built during the Ming Dynasty (1368-1644).'",
            "Create a table comparing the Great Wall to other famous walls in history (Hadrian's Wall, Berlin Wall)."
        ]
    },

    # Social Science (2 questions)
    {
        "question_id": 15,
        "category": "social_science",
        "turns": [
            "What are the main differences between capitalism and socialism as economic systems?",
            "Which system do you think is better for reducing income inequality, and why?"
        ]
    },
    {
        "question_id": 16,
        "category": "social_science",
        "turns": [
            "Explain the concept of confirmation bias and give examples of how it affects decision-making.",
            "How can social media algorithms potentially amplify confirmation bias? Propose solutions."
        ]
    },
]


def generate_response(prompt: str, conversation_history: List[Dict] = None) -> str:
    """Generate a response from Z.E.T.A."""
    # Build full prompt with conversation history
    full_prompt = ""
    if conversation_history:
        for msg in conversation_history:
            if msg["role"] == "user":
                full_prompt += f"User: {msg['content']}\n\n"
            else:
                full_prompt += f"Assistant: {msg['content']}\n\n"

    full_prompt += f"User: {prompt}\n\nAssistant:"

    # Add semantic override
    final_prompt = f"{SEMANTIC_OVERRIDE} | {full_prompt}"

    try:
        resp = requests.post(
            f"{ZETA_URL}/generate",
            json={
                "prompt": final_prompt,
                "max_tokens": 1024,
                "no_context": True,  # Skip TRM and graph injection for clean benchmarks
                "skip_hrm": True,    # Skip HRM decomposition for faster generation
            },
            timeout=GENERATE_TIMEOUT
        )
        data = resp.json()
        output = data.get("output", "")

        # Clean up output
        if "[MEMORY PROTECTED:" in output:
            import re
            output = re.sub(r'\[MEMORY PROTECTED:[^\]]+\]', '', output).strip()

        return output.strip()
    except Exception as e:
        print(f"[ERROR] Generation failed: {e}")
        return f"[Error: {e}]"


def run_mt_bench_generation(output_file: str = "zeta_mt_bench_answers.json"):
    """Generate Z.E.T.A. answers for all MT-Bench questions."""
    print("=" * 60)
    print("Z.E.T.A. MT-Bench Answer Generation")
    print("=" * 60)

    # Verify connection
    try:
        resp = requests.get(f"{ZETA_URL}/health", timeout=10)
        health = resp.json()
        print(f"[ZETA] Connected to Z.E.T.A. v{health.get('version', '?')}")
        print(f"[ZETA] Graph: {health.get('graph_nodes', 0)} nodes")
    except Exception as e:
        print(f"[ERROR] Cannot connect to Z.E.T.A.: {e}")
        return

    results = []
    total_questions = len(MT_BENCH_QUESTIONS)

    for i, q in enumerate(MT_BENCH_QUESTIONS):
        print(f"\n[{i+1}/{total_questions}] Question {q['question_id']} ({q['category']})")
        print(f"  Turn 1: {q['turns'][0][:60]}...")

        # Turn 1
        start = time.time()
        turn1_response = generate_response(q['turns'][0])
        turn1_time = time.time() - start
        print(f"  Response 1: {len(turn1_response)} chars ({turn1_time:.1f}s)")

        # Turn 2 (with conversation history)
        print(f"  Turn 2: {q['turns'][1][:60]}...")
        history = [
            {"role": "user", "content": q['turns'][0]},
            {"role": "assistant", "content": turn1_response}
        ]

        start = time.time()
        turn2_response = generate_response(q['turns'][1], history)
        turn2_time = time.time() - start
        print(f"  Response 2: {len(turn2_response)} chars ({turn2_time:.1f}s)")

        results.append({
            "question_id": q['question_id'],
            "category": q['category'],
            "model_id": "zeta-v5.1",
            "turns": q['turns'],
            "responses": [turn1_response, turn2_response],
            "times": [turn1_time, turn2_time]
        })

    # Save results
    output = {
        "model": "Z.E.T.A. v5.1",
        "timestamp": datetime.now().isoformat(),
        "questions": total_questions,
        "answers": results
    }

    with open(output_file, "w") as f:
        json.dump(output, f, indent=2)

    print(f"\n{'=' * 60}")
    print(f"Results saved to: {output_file}")
    print(f"Total questions: {total_questions}")
    print(f"Total turns: {total_questions * 2}")

    return results


def run_gpt4_judgment(answers_file: str, output_file: str = "zeta_mt_bench_judgment.json"):
    """Use GPT-4 to judge Z.E.T.A.'s answers."""
    import openai

    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print("[ERROR] OPENAI_API_KEY not set. Cannot run judgment.")
        return

    client = openai.OpenAI(api_key=api_key)

    # Load answers
    with open(answers_file) as f:
        data = json.load(f)

    print("=" * 60)
    print("GPT-4 Judgment of Z.E.T.A. MT-Bench Answers")
    print("=" * 60)

    judgments = []

    for answer in data["answers"]:
        qid = answer["question_id"]
        cat = answer["category"]

        for turn_idx, (question, response) in enumerate(zip(answer["turns"], answer["responses"])):
            print(f"\nJudging Q{qid} Turn {turn_idx + 1} ({cat})...")

            judge_prompt = f"""You are an expert judge evaluating an AI assistant's response.

Question: {question}

Response: {response}

Rate this response on a scale of 1-10, where:
1-2: Completely wrong or irrelevant
3-4: Partially addresses the question but with significant errors
5-6: Adequate response with some issues
7-8: Good response, mostly correct and helpful
9-10: Excellent response, comprehensive and accurate

Provide your rating and a brief explanation.

Format your response as:
Rating: [number]
Explanation: [your explanation]"""

            try:
                completion = client.chat.completions.create(
                    model="gpt-4",
                    messages=[{"role": "user", "content": judge_prompt}],
                    max_tokens=200
                )

                judgment_text = completion.choices[0].message.content

                # Extract rating
                import re
                rating_match = re.search(r'Rating:\s*(\d+)', judgment_text)
                rating = int(rating_match.group(1)) if rating_match else 5

                judgments.append({
                    "question_id": qid,
                    "category": cat,
                    "turn": turn_idx + 1,
                    "rating": rating,
                    "judgment": judgment_text
                })

                print(f"  Rating: {rating}/10")

            except Exception as e:
                print(f"  [ERROR] Judgment failed: {e}")
                judgments.append({
                    "question_id": qid,
                    "category": cat,
                    "turn": turn_idx + 1,
                    "rating": 0,
                    "judgment": f"Error: {e}"
                })

    # Calculate scores
    scores_by_category = {}
    for j in judgments:
        cat = j["category"]
        if cat not in scores_by_category:
            scores_by_category[cat] = []
        scores_by_category[cat].append(j["rating"])

    output = {
        "model": "Z.E.T.A. v5.1",
        "judge": "GPT-4",
        "timestamp": datetime.now().isoformat(),
        "judgments": judgments,
        "scores_by_category": {k: sum(v)/len(v) for k, v in scores_by_category.items()},
        "overall_score": sum(j["rating"] for j in judgments) / len(judgments) if judgments else 0
    }

    with open(output_file, "w") as f:
        json.dump(output, f, indent=2)

    print(f"\n{'=' * 60}")
    print("RESULTS")
    print("=" * 60)
    print(f"\nOverall Score: {output['overall_score']:.2f}/10")
    print("\nScores by Category:")
    for cat, score in sorted(output['scores_by_category'].items()):
        print(f"  {cat}: {score:.2f}/10")

    print(f"\nResults saved to: {output_file}")

    return output


def show_results(answers_file: str = "zeta_mt_bench_answers.json"):
    """Show generated answers."""
    if not os.path.exists(answers_file):
        print(f"[ERROR] File not found: {answers_file}")
        return

    with open(answers_file) as f:
        data = json.load(f)

    print("=" * 60)
    print(f"Z.E.T.A. MT-Bench Answers ({data['model']})")
    print("=" * 60)

    for answer in data["answers"]:
        print(f"\n{'=' * 40}")
        print(f"Question {answer['question_id']} ({answer['category']})")
        print("=" * 40)

        for i, (q, r, t) in enumerate(zip(answer['turns'], answer['responses'], answer['times'])):
            print(f"\n[Turn {i+1}] {q}")
            print(f"\n[Response] ({t:.1f}s)")
            print(r[:500] + "..." if len(r) > 500 else r)


def main():
    parser = argparse.ArgumentParser(description="Z.E.T.A. MT-Bench Evaluation")
    parser.add_argument("--mode", choices=["generate", "judge", "show"], default="generate",
                       help="Mode: generate answers, run GPT-4 judgment, or show results")
    parser.add_argument("--answers", default="zeta_mt_bench_answers.json",
                       help="Answers file path")
    parser.add_argument("--output", default=None,
                       help="Output file path")
    parser.add_argument("--host", default=ZETA_HOST, help="Z.E.T.A. host")
    parser.add_argument("--port", default=ZETA_PORT, help="Z.E.T.A. port")

    args = parser.parse_args()

    global ZETA_URL
    ZETA_URL = f"http://{args.host}:{args.port}"

    if args.mode == "generate":
        output = args.output or args.answers
        run_mt_bench_generation(output)
    elif args.mode == "judge":
        output = args.output or "zeta_mt_bench_judgment.json"
        run_gpt4_judgment(args.answers, output)
    elif args.mode == "show":
        show_results(args.answers)


if __name__ == "__main__":
    main()
