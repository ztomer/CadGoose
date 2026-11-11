"""--tasks selection in tools/eval_goose_ai.py, against a fake model (no server).

The docstring documents `--tasks chat` ("only chat tasks"); the runner once built
run_text/run_chat flags and never used them, so a group name matched no task key and
the run silently evaluated nothing.

    python3 tests/tools/test_eval_goose_ai.py

goh-unregistered-ok: a Python unittest run by its own CI step, not a CMake target
"""

import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(
    0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools")
)

import eval_goose_ai


class FakeModel:
    """Stands in for call_llm: answers every prompt, records how often it was asked."""

    def __init__(self):
        self.calls = 0

    def __call__(self, endpoint, model, messages, api_key=None, **kwargs):
        self.calls += 1
        return {
            "content": "HONK. Give me the bread.",
            "error": None,
            "time": 0.0,
            "tokens": 5,
        }


def run(tasks, iterations=1):
    fake = FakeModel()
    with (
        mock.patch.object(eval_goose_ai, "call_llm", fake),
        contextlib.redirect_stdout(io.StringIO()),
    ):
        results = eval_goose_ai.run_eval(
            endpoint="http://fake.invalid/v1",
            model="fake",
            api_key=None,
            tasks=tasks,
            iterations=iterations,
            evil_override=None,
        )
    return results, fake


class TaskSelection(unittest.TestCase):
    def test_chat_group_runs_every_chat_eval_and_no_meme(self):
        results, fake = run(["chat"])
        names = {r.task for r in results}
        self.assertEqual(names, set(eval_goose_ai.CHAT_TASKS))
        self.assertEqual(fake.calls, len(eval_goose_ai.CHAT_TASKS))

    def test_text_meme_group_runs_every_meme_eval_and_no_chat(self):
        results, _ = run(["text_meme"])
        self.assertEqual({r.task for r in results}, set(eval_goose_ai.TEXT_MEME_TASKS))

    def test_a_single_named_task_still_runs_alone(self):
        results, _ = run(["chat_name"])
        self.assertEqual([r.task for r in results], ["chat_name"])

    def test_no_tasks_runs_everything(self):
        results, _ = run([])
        self.assertEqual(
            {r.task for r in results},
            set(eval_goose_ai.TEXT_MEME_TASKS) | set(eval_goose_ai.CHAT_TASKS),
        )

    def test_unknown_task_is_refused_not_a_silent_empty_run(self):
        with self.assertRaises(SystemExit):
            run(["no_such_task"])

    def test_main_tasks_chat_runs_and_saves_chat_results(self):
        fake = FakeModel()
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "results.json")
            argv = [
                "eval_goose_ai.py",
                "--tasks",
                "chat",
                "--iterations",
                "1",
                "--output",
                out,
            ]
            with (
                mock.patch.object(eval_goose_ai, "call_llm", fake),
                mock.patch.object(sys, "argv", argv),
                contextlib.redirect_stdout(io.StringIO()),
            ):
                eval_goose_ai.main()
            with open(out, encoding="utf-8") as f:
                saved = json.load(f)
        self.assertGreaterEqual(
            fake.calls, 1 + len(eval_goose_ai.CHAT_TASKS)
        )  # +1 probe
        self.assertTrue(json.dumps(saved).count("chat_greeting"))


if __name__ == "__main__":
    unittest.main()
