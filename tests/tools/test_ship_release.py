#!/usr/bin/env python3
"""Tests for CadGoose ship release kit and version tag resolution."""
# goh-unregistered-ok: runs directly under GOH_CI_STEPS and CI workflow via python3

from __future__ import annotations

import os
import re
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
GATESRC = REPO_ROOT / ".gatesrc"
SHIP_RELEASE = REPO_ROOT / "scripts" / "ship_release.sh"
PUSH_RELEASE = REPO_ROOT / "scripts" / "push_release.sh"
GOH_DEFAULT = Path.home() / "Projects" / "gates_of_heck"
GOH_DIR = Path(os.environ.get("GOH_DIR", str(GOH_DEFAULT)))


def _run(
    cmd: list[str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        stdin=subprocess.DEVNULL,
        check=check,
    )


def _git(cwd: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return _run(["git", *args], cwd=cwd, check=check)


def _parse_gatesrc(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, val = line.split("=", 1)
        val = val.strip()
        if len(val) >= 2 and val[0] == val[-1] and val[0] in {"'", '"'}:
            val = val[1:-1]
        values[key.strip()] = val
    return values


def _write_executable(path: Path, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def _init_fixture_repo(
    tmp: Path,
    changelog_body: str,
    *,
    tags: tuple[str, ...] = ("v0.5.3", "v1.83"),
) -> tuple[Path, Path, Path, Path]:
    bare = tmp / "origin.git"
    _run(["git", "init", "--bare", "--initial-branch=main", str(bare)], cwd=tmp)

    repo = tmp / "repo"
    _run(["git", "init", "--initial-branch=main", str(repo)], cwd=tmp)
    _git(repo, "config", "user.name", "Test Goose")
    _git(repo, "config", "user.email", "test@cadgoose.invalid")
    _git(repo, "remote", "add", "origin", str(bare))

    (repo / "scripts").mkdir(parents=True)
    (repo / "docs").mkdir(parents=True)

    target_ship = repo / "scripts" / "ship_release.sh"
    target_ship.write_text(SHIP_RELEASE.read_text(encoding="utf-8"), encoding="utf-8")
    target_ship.chmod(stat.S_IRWXU)

    push_log = tmp / "push_release.log"
    stub_push = repo / "scripts" / "push_release.sh"
    _write_executable(
        stub_push,
        f'#!/usr/bin/env bash\nprintf "%s\\n" "$*" >> "{push_log}"\n',
    )

    gh_stub = tmp / "gh_stub.sh"
    _write_executable(
        gh_stub,
        "#!/usr/bin/env bash\n"
        'if [ "${1:-}" = "release" ] && [ "${2:-}" = "view" ]; then\n'
        '  if [ "${GH_RELEASE_EXISTS:-0}" = "1" ]; then\n'
        "    exit 0\n"
        "  fi\n"
        "  exit 1\n"
        "fi\n"
        "exit 0\n",
    )

    changelog = repo / "docs" / "CHANGELOG.md"
    changelog.write_text(
        "# Changelog\n\n## v0.5.3 — Old\n- old note.\n", encoding="utf-8"
    )
    _git(repo, "add", ".")
    _git(repo, "commit", "-m", "chore: initial release", "--no-verify")
    if "v0.5.3" in tags:
        _git(repo, "tag", "v0.5.3")

    for extra_tag in tags:
        if extra_tag == "v0.5.3":
            continue
        changelog.write_text(
            f"# Changelog\n\n## {extra_tag} — Previous\n- previous note.\n\n"
            "## v0.5.3 — Old\n- old note.\n",
            encoding="utf-8",
        )
        _git(repo, "add", "docs/CHANGELOG.md")
        _git(repo, "commit", "-m", f"chore: release {extra_tag}", "--no-verify")
        _git(repo, "tag", extra_tag)

    changelog.write_text(changelog_body, encoding="utf-8")
    _git(repo, "add", "docs/CHANGELOG.md")
    _git(repo, "commit", "-m", "feat: pending unreleased work", "--no-verify")
    _git(repo, "push", "-u", "origin", "main", "--tags")
    return repo, bare, gh_stub, push_log


def _base_env(gh_stub: Path, push_stub: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["GOH_DIR"] = str(GOH_DIR)
    env["GOH_GH"] = str(gh_stub)
    env["CADGOOSE_PUSH_RELEASE"] = str(push_stub)
    env["CADGOOSE_REMOTE"] = "origin"
    env.pop("GOH_SHIP_SUMMARY", None)
    return env


class ShipReleaseContractTest(unittest.TestCase):
    """Static wiring checks in .gatesrc and scripts/."""

    def test_gatesrc_declares_ship_kit(self) -> None:
        cfg = _parse_gatesrc(GATESRC)
        self.assertEqual(
            cfg.get("GOH_SHIP_RELEASE"), "scripts/ship_release.sh {version}"
        )
        self.assertEqual(
            cfg.get("GOH_SHIP_PREFLIGHT"),
            "scripts/ship_release.sh --check {version}",
        )
        self.assertEqual(cfg.get("GOH_SHIP_CHANGELOG"), "kit")
        self.assertEqual(cfg.get("GOH_SHIP_RELEASE_PUSHES"), "1")
        self.assertTrue(SHIP_RELEASE.is_file(), f"missing {SHIP_RELEASE}")
        self.assertTrue(
            os.access(SHIP_RELEASE, os.X_OK), f"{SHIP_RELEASE} must be executable"
        )

    def test_no_gh_run_watch_in_release_scripts(self) -> None:
        for script in (SHIP_RELEASE, PUSH_RELEASE):
            text = script.read_text(encoding="utf-8")
            self.assertNotRegex(text, re.compile(r"gh\s+run\s+watch|--watch"))


class ShipReleaseBehaviorTest(unittest.TestCase):
    """Behavioral preflight, version-order, and live-mode tests in temp repos."""

    DATED_UNRELEASED = (
        "# Changelog\n\n"
        "## November 14, 2026 — Pending fix\n"
        "- Wire release kit.\n\n"
        "## v1.83 — October 19, 2026 — Previous\n"
        "- Prior change.\n"
    )

    KEEP_A_CHANGELOG_UNRELEASED = (
        "# Changelog\n\n"
        "## Unreleased\n\n"
        "- Wire release kit.\n\n"
        "## v1.83 — October 19, 2026 — Previous\n"
        "- Prior change.\n"
    )

    def test_preflight_passes_on_1_84_with_dated_unreleased(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            repo, _, gh_stub, push_log = _init_fixture_repo(
                Path(td), self.DATED_UNRELEASED
            )
            env = _base_env(gh_stub, repo / "scripts" / "push_release.sh")
            before_head = _git(repo, "rev-parse", "HEAD").stdout.strip()

            res = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(res.returncode, 0, res.stdout + res.stderr)
            self.assertIn("v1.83", res.stdout + res.stderr)
            self.assertEqual(
                _git(repo, "rev-parse", "HEAD").stdout.strip(), before_head
            )
            self.assertFalse(
                push_log.exists(), "--check must not invoke push_release.sh"
            )

    def test_preflight_rejects_stale_and_non_advancing_versions(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            repo, _, gh_stub, _ = _init_fixture_repo(Path(td), self.DATED_UNRELEASED)
            env = _base_env(gh_stub, repo / "scripts" / "push_release.sh")
            for bad_version in ("0.5.4", "0.6.0", "0.6", "1.83", "1.82", "1.84.0"):
                with self.subTest(bad_version=bad_version):
                    res = _run(
                        ["bash", "scripts/ship_release.sh", "--check", bad_version],
                        cwd=repo,
                        env=env,
                        check=False,
                    )
                    self.assertNotEqual(
                        res.returncode, 0, f"{bad_version} should be refused"
                    )
                    combined = res.stdout + res.stderr
                    self.assertTrue(
                        "v1.83" in combined or "X.Y" in combined,
                        f"expected v1.83 or X.Y message, got: {combined}",
                    )

    def test_preflight_rejects_dirty_branch_tag_release_and_empty_log(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            repo, _, gh_stub, _ = _init_fixture_repo(Path(td), self.DATED_UNRELEASED)
            env = _base_env(gh_stub, repo / "scripts" / "push_release.sh")

            changelog_path = repo / "docs" / "CHANGELOG.md"
            changelog_path.write_text(
                self.DATED_UNRELEASED + "- dirty\n", encoding="utf-8"
            )
            dirty = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertNotEqual(dirty.returncode, 0)
            _git(repo, "checkout", "--", "docs/CHANGELOG.md")

            _git(repo, "checkout", "-b", "feature/test")
            side = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertNotEqual(side.returncode, 0)
            _git(repo, "checkout", "main")

            _git(repo, "tag", "v1.84")
            tagged = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertNotEqual(tagged.returncode, 0)
            _git(repo, "tag", "-d", "v1.84")

            env_rel = env.copy()
            env_rel["GH_RELEASE_EXISTS"] = "1"
            released = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env_rel,
                check=False,
            )
            self.assertNotEqual(released.returncode, 0)

            changelog_path.write_text(
                "# Changelog\n\n"
                "## v1.83 — October 19, 2026 — Previous\n"
                "- Prior change.\n",
                encoding="utf-8",
            )
            _git(repo, "add", "docs/CHANGELOG.md")
            _git(repo, "commit", "-m", "docs: clear unreleased entries", "--no-verify")
            empty_cl = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertNotEqual(empty_cl.returncode, 0)

    def test_live_run_cuts_dated_log_pushes_and_calls_push_release(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            repo, bare, gh_stub, push_log = _init_fixture_repo(
                Path(td), self.DATED_UNRELEASED
            )
            env = _base_env(gh_stub, repo / "scripts" / "push_release.sh")
            env["GOH_SHIP_SUMMARY"] = "release kit"

            res = _run(
                ["bash", "scripts/ship_release.sh", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(res.returncode, 0, res.stdout + res.stderr)

            changelog_text = (repo / "docs" / "CHANGELOG.md").read_text(
                encoding="utf-8"
            )
            self.assertRegex(
                changelog_text,
                re.compile(r"^## v1\.84 — .* — release kit$", re.MULTILINE),
            )
            self.assertEqual(push_log.read_text(encoding="utf-8").strip(), "v1.84")

            local_head = _git(repo, "rev-parse", "HEAD").stdout.strip()
            remote_head = _git(bare, "rev-parse", "refs/heads/main").stdout.strip()
            self.assertEqual(local_head, remote_head)

            chk = _run(
                [
                    "python3",
                    str(GOH_DIR / "tools" / "release-kit" / "changelog_cut.py"),
                    "check",
                    "--version",
                    "1.84",
                    "--file",
                    "docs/CHANGELOG.md",
                ],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(chk.returncode, 0, chk.stdout + chk.stderr)

            # Resumed run when ## v1.84 is already cut must not add a commit
            resumed = _run(
                ["bash", "scripts/ship_release.sh", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(resumed.returncode, 0, resumed.stdout + resumed.stderr)
            self.assertEqual(_git(repo, "rev-parse", "HEAD").stdout.strip(), local_head)

    def test_live_run_cuts_keep_a_changelog_unreleased_heading(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            repo, _, gh_stub, push_log = _init_fixture_repo(
                Path(td), self.KEEP_A_CHANGELOG_UNRELEASED
            )
            env = _base_env(gh_stub, repo / "scripts" / "push_release.sh")

            chk = _run(
                ["bash", "scripts/ship_release.sh", "--check", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(chk.returncode, 0, chk.stdout + chk.stderr)

            res = _run(
                ["bash", "scripts/ship_release.sh", "1.84"],
                cwd=repo,
                env=env,
                check=False,
            )
            self.assertEqual(res.returncode, 0, res.stdout + res.stderr)
            changelog_text = (repo / "docs" / "CHANGELOG.md").read_text(
                encoding="utf-8"
            )
            self.assertIn("## Unreleased\n\n## v1.84 — ", changelog_text)
            self.assertEqual(push_log.read_text(encoding="utf-8").strip(), "v1.84")


if __name__ == "__main__":
    unittest.main()
