"""Result record and reporting for eval_goose_ai.py (split out to keep both files under the line cap)."""

import json
import statistics
import time
from dataclasses import dataclass
from typing import Optional


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
        print(
            f"{task_name:<22} {desc:<30} {avg_score:>6.0f} {avg_time:>5.1f}s {status:>7}"
        )

    # ── Overall stats ──
    all_scores = [r.score for r in results]
    all_times = [r.time for r in results if r.time > 0]
    errors = [r for r in results if r.error]

    print(
        f"\n{'Overall':<22} {'':<30} {statistics.mean(all_scores):>6.0f} {statistics.mean(all_times):>5.1f}s"
    )
    print(f"\nTotal runs: {len(results)}")
    print(f"Errors: {len(errors)}")
    print(
        f"Pass rate: {sum(1 for s in all_scores if s >= 80)}/{len(all_scores)} ({100 * sum(1 for s in all_scores if s >= 80) / len(all_scores):.0f}%)"
    )

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
