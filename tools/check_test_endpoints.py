#!/usr/bin/env python3
"""No test binds a fixed machine-wide endpoint.

ROADMAP G0.3: the MCP HTTP tests bound port 31073 and the internal-server
tests bound /tmp/desktop-goose-mcp.sock, so two CadGooseTests processes
collided (36-41 of 80 MCP tests failed when two ran at once, measured
2026-10-17), the failure read like an MCP bug, and the socket tests unlinked
a running CadGoose's own socket. The server now binds g_config.ai.mcpPort 0
and reports the kernel's choice through MCP_HTTPPort(); the socket path is
set per process through MCP_SetInternalSocketPath().

This keeps it that way under tests/: no htons(<literal>), no mcpPort set to a
fixed non-zero port, no absolute socket-path literal ending in .sock.
ALLOWED is a ratchet (a stale entry is red); the floor refuses a blind scan.
"""

import pathlib
import re
import subprocess
import sys

MIN_FILES = 100  # 130 test sources on 2026-10-17

ALLOWED = {
    "tests/common/command_socket_stub.cpp": "the stub only returns a path from "
    "CommandSocket_GetPath(); nothing binds or unlinks it",
}

SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".m", ".mm"}

RULES = [
    (re.compile(r"\bhtons\s*\(\s*\d"), "a literal port number"),
    (re.compile(r"\bmcpPort\s*=\s*[1-9]"), "a fixed MCP port (use 0 and MCP_HTTPPort())"),
    (re.compile(r'"/[^"\n]*\.sock"'), "a fixed socket path (derive one per process)"),
]


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    files = sorted(
        p for p in subprocess.run(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "tests"],
            cwd=root, capture_output=True, text=True, check=True, stdin=subprocess.DEVNULL,
        ).stdout.split()
        if pathlib.PurePath(p).suffix in SUFFIXES
    )
    if len(files) < MIN_FILES:
        print(f"✗ [test_endpoints] read {len(files)} test sources, floor {MIN_FILES}: "
              "the scan lost its subject", file=sys.stderr)
        return 1
    hits = {}
    for rel in files:
        text = strip_comments((root / rel).read_text(encoding="utf-8", errors="replace"))
        for lineno, line in enumerate(text.splitlines(), 1):
            for rule, why in RULES:
                if rule.search(line):
                    hits.setdefault(rel, []).append(f"{lineno} ({why})")
    bad = 0
    for rel, found in sorted(hits.items()):
        if rel in ALLOWED:
            continue
        bad += 1
        print(f"✗ [test_endpoints] {rel}: {', '.join(found)}: a fixed endpoint collides "
              "with every other test process and with a running app", file=sys.stderr)
    for rel in sorted(ALLOWED):
        if rel not in hits:
            bad += 1
            print(f"✗ [test_endpoints] stale allowlist entry {rel}; delete it", file=sys.stderr)
    if bad:
        return 1
    print(f"✓ [test_endpoints] {len(files)} test sources, no fixed port or socket path")
    return 0


if __name__ == "__main__":
    sys.exit(main())
