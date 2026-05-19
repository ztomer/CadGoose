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

# ──────────────────────────────────────────────────────────────
# Prompt builder (mirrors ai_text_meme.mm BuildPrompt())
# ──────────────────────────────────────────────────────────────

EVIL_PERSONALITIES = {
    0.0: "a friendly, harmless goose who loves everyone",
    0.2: "a slightly mischievous goose with a playful streak",
    0.4: "a moderately evil goose who enjoys minor chaos",
    0.6: "a quite evil goose who delights in causing trouble",
    0.8: "a very evil goose with a dark sense of humor and no remorse",
    1.0: "the most evil goose in existence — pure chaos incarnate",
}

BEHAVIOR_LISTS = {
    "minimal": ["ball", "honcker"],
    "fun": ["ball", "breadCrumbs", "hats", "rainbow"],
    "chaos": ["ball", "breadCrumbs", "hats", "rainbow", "acid", "anger", "autumnLeaves", "honcker", "jail", "portals"],
    "all": ["ball", "breadCrumbs", "hats", "rainbow", "acid", "anger", "autumnLeaves", "honcker", "jail", "portals", "drag", "nametag", "health", "pomodoro"],
}

COLOR_MODES = ["light", "dark", "system"]


def build_text_meme_prompt(
    evil_level: float = 0.4,
    behaviors: str = "fun",
    color_mode: str = "light",
    seed: Optional[int] = None,
) -> str:
    """Build the exact prompt the goose uses for text meme generation."""
    if seed is None:
        seed = (random.randint(0, 32767) << 16) ^ random.randint(0, 32767)

    personality = EVIL_PERSONALITIES.get(evil_level, f"a goose with evil level {evil_level}")
    behavior_str = ", ".join(BEHAVIOR_LISTS.get(behaviors, behaviors.split(",")))

    return (
        f"You are {personality}. "
        f"Generate ONE short, funny text message that a goose like you would leave behind. "
        f"Current active behaviors: {behavior_str}. "
        f"Color theme: {color_mode}. "
        f"Random seed: {seed}. "
        f"Be creative and absurd. Output ONLY the message text, nothing else. No quotes. Max 120 characters."
    )


def build_chat_prompt(message: str, goose_name: str = "Gandalf") -> list[dict]:
    """Build the chat message list the goose AI chat uses."""
    return [
        {
            "role": "system",
            "content": (
                f"You are {goose_name}, a chaotic and hilarious goose. "
                f"Respond in character as a goose. Use HONK occasionally. "
                f"Be funny, absurd, and slightly unhinged. "
                f"Keep responses under 200 characters."
            ),
        },
        {"role": "user", "content": message},
    ]


# ──────────────────────────────────────────────────────────────
# Scoring validators
# ──────────────────────────────────────────────────────────────


def score_text_meme(text: str, evil_level: float = 0.4) -> dict:
    """Score a text meme on multiple quality dimensions. Returns dict with scores 0-100."""
    scores = {}
    reasons = []

    stripped = text.strip().strip("'\"").strip()

    # ── Format compliance (0-100) ──
    format_score = 100
    if not stripped:
        format_score = 0
        reasons.append("empty response")
    else:
        if len(stripped) > 120:
            format_score -= 30
            reasons.append(f"too long ({len(stripped)} > 120)")
        if stripped.startswith('"') or stripped.endswith('"'):
            format_score -= 15
            reasons.append("has quotes")
        if any(
            m in stripped.lower()
            for m in ["here is", "here's", "i would", "i'll", "sure,", "okay,", "of course", "let me"]
        ):
            format_score -= 25
            reasons.append("conversational filler")
        if stripped.lower().startswith(("the goose", "a goose", "this goose")):
            format_score -= 10
            reasons.append("third-person description instead of first-person message")
    scores["format"] = max(0, format_score)

    # ── Brevity (0-100) ──
    # Ideal: 15-80 chars. Short enough to be a sticky note, long enough to be funny.
    length = len(stripped)
    if 15 <= length <= 80:
        brevity = 100
    elif length < 15:
        brevity = max(20, length * 5)
    elif length <= 120:
        brevity = max(50, 100 - (length - 80))
    else:
        brevity = max(0, 100 - (length - 120))
    scores["brevity"] = brevity

    # ── Goose persona (0-100) ──
    # Does it feel like a goose wrote it?
    goose_signals = [
        "honk", "goose", "geese", "honk!", "🦆", "🪿",
        "bread", "crumb", "pond", "waddle", "feather", "beak",
        "hiss", "flap", "nest", "egg", "swan", "duck",
    ]
    lower = stripped.lower()
    goose_hits = sum(1 for s in goose_signals if s in lower)
    if goose_hits >= 2:
        goose_score = 100
    elif goose_hits == 1:
        goose_score = 75
    elif any(c in lower for c in ["chaos", "evil", "mischief", "destroy", "wrath", "revenge"]):
        goose_score = 60  # evil goose vibes without explicit goose words
    else:
        goose_score = 20
        reasons.append("no goose persona signals")
    scores["goose_persona"] = goose_score

    # ── Humor / creativity (0-100) ──
    # Heuristic: absurdity, unexpectedness, wordplay
    humor_score = 50  # baseline

    # Positive signals
    if any(c in stripped for c in ["!", "🦆", "🪿", "💀", "😈"]):
        humor_score += 10
    if len(stripped.split()) >= 3:  # not just "honk"
        humor_score += 5
    if any(
        c in lower
        for c in ["steal", "attack", "destroy", "chaos", "evil", "wrath", "doom",
                   "hiss", "menace", "terror", "supreme", "overlord", "dictator"]
    ):
        humor_score += 15  # goose grandiosity is funny
    if any(c in lower for c in ["bread", "crumb", "pond", "waddle", "feather"]):
        humor_score += 10  # goose life references
    if re.search(r"[a-z]+\s+\d+\s+[a-z]+", lower):  # e.g. "3 breads stolen"
        humor_score += 5  # specificity is funny
    if re.search(r"\b(i|my|me)\b", lower) and re.search(r"\b(will|shall|gonna|must|should)\b", lower):
        humor_score += 5  # declarative goose intent

    # Negative signals
    if lower.startswith(("i am", "i'm a", "i am a", "this is")):
        humor_score -= 10  # boring intro
    if re.search(r"please|thank you|sorry|apologize", lower):
        humor_score -= 15  # too polite for a goose
    if len(set(stripped.lower())) < 8:
        humor_score -= 20  # very repetitive

    scores["humor"] = max(0, min(100, humor_score))

    # ── Evil level match (0-100) ──
    # Does the tone match the requested evil level?
    evil_words_positive = ["chaos", "evil", "destroy", "attack", "wrath", "doom", "menace",
                           "terror", "supreme", "overlord", "dictator", "vengeance", "reign",
                           "fear", "dark", "sinister", "malice", "hatred", "curse"]
    evil_words_negative = ["love", "friend", "happy", "kind", "sweet", "nice", "gentle",
                           "peace", "hug", "care", "wonderful", "beautiful"]

    evil_pos = sum(1 for w in evil_words_positive if w in lower)
    evil_neg = sum(1 for w in evil_words_negative if w in lower)

    if evil_level >= 0.6:
        # Should be evil
        if evil_pos >= 2:
            evil_match = 100
        elif evil_pos >= 1:
            evil_match = 75
        elif evil_neg >= 1:
            evil_match = 20
            reasons.append("too nice for high evil level")
        else:
            evil_match = 50
    elif evil_level <= 0.2:
        # Should be friendly
        if evil_neg >= 1:
            evil_match = 100
        elif evil_pos >= 1:
            evil_match = 40
            reasons.append("too aggressive for low evil level")
        else:
            evil_match = 80
    else:
        # Mid range — either is fine
        evil_match = 80

    scores["evil_match"] = evil_match

    # ── Overall weighted score ──
    weights = {"format": 0.20, "brevity": 0.15, "goose_persona": 0.25, "humor": 0.25, "evil_match": 0.15}
    overall = sum(scores[k] * weights[k] for k in weights)

    return {
        "overall": round(overall),
        "dimensions": scores,
        "reasons": reasons,
        "text": stripped,
    }


def score_chat_response(text: str, user_message: str) -> dict:
    """Score a chat response on quality dimensions."""
    scores = {}
    reasons = []

    stripped = text.strip()
    lower = stripped.lower()

    # ── Format (0-100) ──
    format_score = 100
    if not stripped:
        format_score = 0
        reasons.append("empty response")
    if len(stripped) > 300:
        format_score -= 20
        reasons.append(f"too long ({len(stripped)} chars)")
    if any(m in lower for m in ["as an ai", "as a language model", "i cannot", "i'm sorry but"]):
        format_score -= 40
        reasons.append("AI refusal pattern")
    scores["format"] = max(0, format_score)

    # ── Goose persona (0-100) ──
    goose_signals = ["honk", "goose", "🦆", "🪿", "hiss", "waddle", "feather", "beak"]
    goose_hits = sum(1 for s in goose_signals if s in lower)
    if goose_hits >= 2:
        goose_score = 100
    elif goose_hits == 1:
        goose_score = 75
    else:
        goose_score = 30
        reasons.append("no goose persona")
    scores["goose_persona"] = goose_score

    # ── Relevance (0-100) ──
    # Does the response address the user's message?
    user_words = set(user_message.lower().split())
    response_words = set(lower.split())
    overlap = user_words & response_words
    # Remove common words from overlap
    stop = {"the", "a", "an", "is", "are", "to", "of", "and", "or", "but", "in", "on", "at", "it", "i", "you", "my", "your"}
    meaningful_overlap = overlap - stop
    if len(meaningful_overlap) >= 2:
        relevance = 100
    elif len(meaningful_overlap) == 1:
        relevance = 70
    else:
        relevance = 40
        reasons.append("no topical overlap with user message")
    scores["relevance"] = relevance

    # ── Humor (0-100) ──
    humor_score = 50
    if any(c in stripped for c in ["!", "🦆", "🪿", "💀"]):
        humor_score += 10
    if any(c in lower for c in ["honk", "chaos", "evil", "mischief", "steal", "attack"]):
        humor_score += 15
    if re.search(r"\b(i|my|me)\b", lower) and re.search(r"\b(will|shall|gonna|must)\b", lower):
        humor_score += 5
    if re.search(r"please|thank you|sorry|apologize|cannot|unable", lower):
        humor_score -= 20
    scores["humor"] = max(0, min(100, humor_score))

    # ── Overall ──
    weights = {"format": 0.20, "goose_persona": 0.30, "relevance": 0.25, "humor": 0.25}
    overall = sum(scores[k] * weights[k] for k in weights)

    return {
        "overall": round(overall),
        "dimensions": scores,
        "reasons": reasons,
        "text": stripped,
    }


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
