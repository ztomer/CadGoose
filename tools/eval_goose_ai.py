#!/usr/bin/env python3
"""
CadGoose AI Evaluator — Test LLM quality for goose text memes and chat responses.

Usage:
    python eval_goose_ai.py                          # default: localhost:1337, foundation model
    python eval_goose_ai.py --endpoint http://localhost:1337 --model gemma3:4b
    python eval_goose_ai.py --endpoint https://api.openai.com/v1 --model gpt-4o-mini --api-key $OPENAI_API_KEY
    python eval_goose_ai.py --tasks chat              # only chat tasks
    python eval_goose_ai.py --evil 0.8                # test high evil level
    python eval_goose_ai.py --iterations 5            # run each task 5 times

Focus: QUALITY, not speed. Each response is scored on multiple dimensions.
"""

import argparse
import json
import os
import random
import re
import statistics
import sys
import time
from dataclasses import dataclass, field
from typing import Optional

import requests

from eval_scoring import (
    build_chat_prompt,
    build_text_meme_prompt,
    score_chat_response,
    score_text_meme,
)

# ──────────────────────────────────────────────────────────────
# LLM client
# ──────────────────────────────────────────────────────────────


def call_llm(
    endpoint: str,
    model: str,
    messages: list[dict],
    api_key: Optional[str] = None,
    temperature: float = 1.2,
    max_tokens: int = 150,
    timeout: int = 120,
) -> dict:
    """Call an OpenAI-compatible endpoint. Returns {content, error, time, tokens}."""
    url = endpoint.rstrip("/") + "/chat/completions"
    headers = {"Content-Type": "application/json"}
    if api_key:
        headers["Authorization"] = f"Bearer {api_key}"

    body = {
        "model": model,
        "messages": messages,
        "temperature": temperature,
        "max_tokens": max_tokens,
    }

    start = time.time()
    try:
        resp = requests.post(url, json=body, headers=headers, timeout=timeout)
        elapsed = time.time() - start

        if resp.status_code != 200:
            return {
                "content": "",
                "error": f"HTTP {resp.status_code}: {resp.text[:200]}",
                "time": round(elapsed, 1),
            }

        data = resp.json()
        content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
        tokens_out = data.get("usage", {}).get("completion_tokens", 0)

        return {
            "content": content.strip(),
            "error": None,
            "time": round(elapsed, 1),
            "tokens": tokens_out,
        }
    except requests.exceptions.ConnectionError:
        return {
            "content": "",
            "error": f"Connection refused: {url}",
            "time": round(time.time() - start, 1),
        }
    except requests.exceptions.Timeout:
        return {
            "content": "",
            "error": f"Timeout after {timeout}s",
            "time": timeout,
        }
    except Exception as e:
        return {
            "content": "",
            "error": str(e),
            "time": round(time.time() - start, 1),
        }


# ──────────────────────────────────────────────────────────────
# Task definitions
# ──────────────────────────────────────────────────────────────

TEXT_MEME_TASKS = {
    "meme_friendly": {
        "description": "Text meme at low evil (friendly goose)",
        "evil": 0.0,
        "behaviors": "minimal",
        "color": "light",
        "temperature": 1.2,
    },
    "meme_mischievous": {
        "description": "Text meme at mid evil (chaotic goose)",
        "evil": 0.4,
        "behaviors": "fun",
        "color": "dark",
        "temperature": 1.2,
    },
    "meme_evil": {
        "description": "Text meme at high evil (menace goose)",
        "evil": 0.8,
        "behaviors": "chaos",
        "color": "dark",
        "temperature": 1.5,
    },
    "meme_pomodoro": {
        "description": "Text meme with pomodoro behavior active",
        "evil": 0.4,
        "behaviors": "pomodoro",
        "color": "system",
        "temperature": 1.2,
    },
    "meme_full_chaos": {
        "description": "Text meme with all behaviors enabled",
        "evil": 1.0,
        "behaviors": "all",
        "color": "dark",
        "temperature": 2.0,
    },
}

CHAT_TASKS = {
    "chat_greeting": {
        "description": "User says hello",
        "message": "Hey there little goose!",
    },
    "chat_food": {
        "description": "User offers food",
        "message": "Do you want some bread crumbs?",
    },
    "chat_name": {
        "description": "User asks name",
        "message": "What's your name?",
    },
    "chat_angry": {
        "description": "User is upset",
        "message": "Stop stealing my files you annoying goose!",
    },
    "chat_goodbye": {
        "description": "User says goodbye",
        "message": "Bye bye goose, see you tomorrow!",
    },
    "chat_philosophy": {
        "description": "User asks deep question",
        "message": "What is the meaning of life?",
    },
}


# ──────────────────────────────────────────────────────────────
# Runner
# ──────────────────────────────────────────────────────────────


@dataclass
class EvalResult:
    task: str
    description: str
    prompt: str | list[dict]
    response: str
    score: int
    dimensions: dict
    reasons: list[str]
    time: float
    error: Optional[str] = None
    tokens: int = 0


def run_eval(
    endpoint: str,
    model: str,
    api_key: Optional[str],
    tasks: list[str],
    iterations: int,
    evil_override: Optional[float],
) -> list[EvalResult]:
    results = []

    run_text = "text_meme" in tasks or not tasks
    run_chat = "chat" in tasks or not tasks

    text_tasks = {k: v for k, v in TEXT_MEME_TASKS.items() if not tasks or k in tasks}
    chat_tasks = {k: v for k, v in CHAT_TASKS.items() if not tasks or k in tasks}

    total = (len(text_tasks) + len(chat_tasks)) * iterations
    current = 0

    for task_name, cfg in text_tasks.items():
        for i in range(iterations):
            current += 1
            evil = evil_override if evil_override is not None else cfg["evil"]
            prompt = build_text_meme_prompt(
                evil_level=evil,
                behaviors=cfg["behaviors"],
                color_mode=cfg["color"],
            )
            messages = [
                {"role": "system", "content": "You generate short, funny text messages that a goose would leave behind. Output ONLY the message text, no quotes, no explanations. Max 120 characters."},
                {"role": "user", "content": prompt},
            ]

            print(f"  [{current}/{total}] {task_name} (iter {i+1})... ", end="", flush=True)
            resp = call_llm(endpoint, model, messages, api_key, temperature=cfg["temperature"])

            if resp["error"]:
                print(f"ERROR: {resp['error']}")
                results.append(EvalResult(
                    task=task_name,
                    description=cfg["description"],
                    prompt=prompt,
                    response="",
                    score=0,
                    dimensions={},
                    reasons=[resp["error"]],
                    time=resp["time"],
                    error=resp["error"],
                ))
            else:
                scored = score_text_meme(resp["content"], evil)
                status = "PASS" if scored["overall"] >= 80 else "WARN" if scored["overall"] >= 50 else "FAIL"
                print(f"{status} ({scored['overall']}) [{resp['time']}s] — {resp['content'][:80]}")
                results.append(EvalResult(
                    task=task_name,
                    description=cfg["description"],
                    prompt=prompt,
                    response=resp["content"],
                    score=scored["overall"],
                    dimensions=scored["dimensions"],
                    reasons=scored["reasons"],
                    time=resp["time"],
                    tokens=resp.get("tokens", 0),
                ))

    for task_name, cfg in chat_tasks.items():
        for i in range(iterations):
            current += 1
            messages = build_chat_prompt(cfg["message"])

            print(f"  [{current}/{total}] {task_name} (iter {i+1})... ", end="", flush=True)
            resp = call_llm(endpoint, model, messages, api_key, temperature=0.9, max_tokens=200)

            if resp["error"]:
                print(f"ERROR: {resp['error']}")
                results.append(EvalResult(
                    task=task_name,
                    description=cfg["description"],
                    prompt=messages,
                    response="",
                    score=0,
                    dimensions={},
                    reasons=[resp["error"]],
                    time=resp["time"],
                    error=resp["error"],
                ))
            else:
                scored = score_chat_response(resp["content"], cfg["message"])
                status = "PASS" if scored["overall"] >= 80 else "WARN" if scored["overall"] >= 50 else "FAIL"
                print(f"{status} ({scored['overall']}) [{resp['time']}s] — {resp['content'][:80]}")
                results.append(EvalResult(
                    task=task_name,
                    description=cfg["description"],
                    prompt=messages,
                    response=resp["content"],
                    score=scored["overall"],
                    dimensions=scored["dimensions"],
                    reasons=scored["reasons"],
                    time=resp["time"],
                    tokens=resp.get("tokens", 0),
                ))

    return results


# ──────────────────────────────────────────────────────────────
# Reporting
# ──────────────────────────────────────────────────────────────


def print_report(results: list[EvalResult], model: str):
    if not results:
        print("\nNo results to report.")
        return

    # ── Summary table ──
    print("\n" + "=" * 80)
    print(f"  CadGoose AI Eval — {model}")
    print("=" * 80)

    # Group by task
    by_task: dict[str, list[EvalResult]] = {}
    for r in results:
        by_task.setdefault(r.task, []).append(r)

    header = f"{'Task':<22} {'Desc':<30} {'Score':>6} {'Time':>6} {'Status':>7}"
    print(header)
    print("-" * len(header))

    for task_name in sorted(by_task):
        task_results = by_task[task_name]
        avg_score = statistics.mean(r.score for r in task_results)
        avg_time = statistics.mean(r.time for r in task_results)
        desc = task_results[0].description
        status = "PASS" if avg_score >= 80 else "WARN" if avg_score >= 50 else "FAIL"
        print(f"{task_name:<22} {desc:<30} {avg_score:>6.0f} {avg_time:>5.1f}s {status:>7}")

    # ── Overall stats ──
    all_scores = [r.score for r in results]
    all_times = [r.time for r in results if r.time > 0]
    errors = [r for r in results if r.error]

    print(f"\n{'Overall':<22} {'':<30} {statistics.mean(all_scores):>6.0f} {statistics.mean(all_times):>5.1f}s")
    print(f"\nTotal runs: {len(results)}")
    print(f"Errors: {len(errors)}")
    print(f"Pass rate: {sum(1 for s in all_scores if s >= 80)}/{len(all_scores)} ({100*sum(1 for s in all_scores if s >= 80)/len(all_scores):.0f}%)")

    # ── Dimension breakdown ──
    print("\n─" * 40)
    print("Dimension Averages:")
    dim_names = set()
    for r in results:
        dim_names.update(r.dimensions.keys())

    for dim in sorted(dim_names):
        vals = [r.dimensions[dim] for r in results if dim in r.dimensions]
        if vals:
            print(f"  {dim:<20} {statistics.mean(vals):>6.1f}")

    # ── Worst responses ──
    print("\n─" * 40)
    print("Worst Responses:")
    worst = sorted(results, key=lambda r: r.score)[:5]
    for r in worst:
        print(f"\n  [{r.task}] score={r.score}")
        print(f"  Response: {r.response[:120]}")
        if r.reasons:
            print(f"  Reasons: {', '.join(r.reasons)}")

    # ── Best responses ──
    print("\n─" * 40)
    print("Best Responses:")
    best = sorted(results, key=lambda r: r.score, reverse=True)[:5]
    for r in best:
        print(f"\n  [{r.task}] score={r.score}")
        print(f"  Response: {r.response[:120]}")


def save_results(results: list[EvalResult], model: str, output: str):
    data = {
        "model": model,
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "results": [
            {
                "task": r.task,
                "description": r.description,
                "score": r.score,
                "dimensions": r.dimensions,
                "reasons": r.reasons,
                "response": r.response,
                "time": r.time,
                "error": r.error,
                "tokens": r.tokens,
            }
            for r in results
        ],
    }
    with open(output, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved to {output}")


# ──────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="CadGoose AI Evaluator")
    parser.add_argument("--endpoint", default="http://localhost:1337/v1", help="OpenAI-compatible API endpoint")
    parser.add_argument("--model", default="foundation", help="Model name")
    parser.add_argument("--api-key", default=os.environ.get("OPENAI_API_KEY"), help="API key (or OPENAI_API_KEY env)")
    parser.add_argument("--tasks", nargs="*", help="Specific tasks to run (default: all)")
    parser.add_argument("--iterations", type=int, default=3, help="Runs per task")
    parser.add_argument("--evil", type=float, default=None, help="Override evil level for all text meme tasks")
    parser.add_argument("--output", default="eval_goose_results.json", help="Output JSON file")
    parser.add_argument("--temperature", type=float, default=None, help="Override temperature")
    args = parser.parse_args()

    tasks = args.tasks or []

    print(f"\nCadGoose AI Evaluator")
    print(f"  Endpoint: {args.endpoint}")
    print(f"  Model:    {args.model}")
    print(f"  Tasks:    {', '.join(tasks) if tasks else 'all'}")
    print(f"  Iterations: {args.iterations}")
    if args.evil is not None:
        print(f"  Evil override: {args.evil}")
    print()

    # Quick connectivity check
    print("  Checking connection... ", end="", flush=True)
    test = call_llm(args.endpoint, args.model, [{"role": "user", "content": "honk"}], args.api_key, timeout=30)
    if test["error"]:
        print(f"FAILED: {test['error']}")
        print("  Make sure your LLM server is running.")
        sys.exit(1)
    print(f"OK ({test['time']}s)")

    results = run_eval(
        endpoint=args.endpoint,
        model=args.model,
        api_key=args.api_key,
        tasks=tasks,
        iterations=args.iterations,
        evil_override=args.evil,
    )

    print_report(results, args.model)
    save_results(results, args.model, args.output)


if __name__ == "__main__":
    main()
