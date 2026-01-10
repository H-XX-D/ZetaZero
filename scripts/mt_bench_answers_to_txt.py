#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from typing import Any


def _safe_get(obj: dict[str, Any], key: str, default: Any) -> Any:
    val = obj.get(key, default)
    return default if val is None else val


def render_answers_txt(data: dict[str, Any]) -> str:
    model = _safe_get(data, "model", "")
    timestamp = _safe_get(data, "timestamp", "")
    total_questions = _safe_get(data, "questions", None)
    answers = _safe_get(data, "answers", [])

    lines: list[str] = []
    lines.append(f"MODEL: {model}")
    if timestamp:
        lines.append(f"TIMESTAMP: {timestamp}")
    if total_questions is not None:
        lines.append(f"QUESTIONS: {total_questions}")
    lines.append(f"ANSWERS: {len(answers)}")
    lines.append("" )

    for a in answers:
        qid = _safe_get(a, "question_id", "")
        category = _safe_get(a, "category", "")
        model_id = _safe_get(a, "model_id", "")
        turns = _safe_get(a, "turns", [])
        responses = _safe_get(a, "responses", [])
        times = _safe_get(a, "times", [])

        lines.append("=" * 80)
        lines.append(f"QUESTION_ID: {qid}")
        if category:
            lines.append(f"CATEGORY: {category}")
        if model_id:
            lines.append(f"MODEL_ID: {model_id}")
        lines.append("" )

        max_turns = max(len(turns), len(responses))
        for i in range(max_turns):
            user_text = turns[i] if i < len(turns) else ""
            assistant_text = responses[i] if i < len(responses) else ""

            lines.append(f"TURN {i+1} USER:")
            lines.append(user_text.rstrip("\n"))
            lines.append("" )
            lines.append(f"TURN {i+1} ASSISTANT:")
            lines.append(assistant_text.rstrip("\n"))
            if i < len(times):
                lines.append("" )
                lines.append(f"TURN {i+1} TIME_SEC: {times[i]}")
            lines.append("" )

        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert Zeta MT-Bench answers JSON to readable .txt transcript")
    parser.add_argument("input", type=Path, help="Path to zeta_mt_bench*_answers.json")
    parser.add_argument("-o", "--output", type=Path, default=None, help="Output .txt path (default: alongside input)")
    args = parser.parse_args()

    input_path: Path = args.input
    if args.output is None:
        output_path = input_path.with_suffix(".txt")
    else:
        output_path = args.output

    data = json.loads(input_path.read_text(encoding="utf-8"))
    txt = render_answers_txt(data)
    output_path.write_text(txt, encoding="utf-8")
    print(str(output_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
