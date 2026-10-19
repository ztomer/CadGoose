# _cadgoose.sh — what every CadGoose profiling script needs to know about the
# app (sourced, never executed: no shebang by design; dialect declared below).
# shellcheck shell=bash
#
# These scripts lived in gates_of_heck from 2026-09 to 2026-10-19, canonized
# there while CadGoose and CadGoose2 carried byte-identical copies. CadGoose2
# is gone, so they are one repo's scripts again and live here; the generic
# trace ranker (analyze_trace.sh, xctrace_profile.py) stays in the house suite.

# The repository these scripts act on: this checkout, unless CG_PROFILE_ROOT
# names another (the seam tests/profiling uses to drive a script on fakes).
cg_root() {
    if [ -n "${CG_PROFILE_ROOT:-}" ]; then
        printf '%s\n' "$CG_PROFILE_ROOT"
        return 0
    fi
    (cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
}

# The command socket the given CadGoose binary binds, read from the binary
# itself (`CadGoose --socket-path` prints CommandSocket_GetPath()). A path
# typed here would be a second copy of the app's constant; one such copy
# waited for /tmp/desktop-goose-mcp.sock, a socket the app binds only when MCP
# is enabled, while the command socket is /tmp/desktop-goose.sock.
cg_socket_path() {
    "$1" --socket-path < /dev/null
}
