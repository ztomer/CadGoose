#!/usr/bin/env bash
# test_run_trail_test.sh — drive tools/profiling/run_trail_test.sh on fakes and
# check the two things it got wrong:
#
#   1. It waited for /tmp/desktop-goose-mcp.sock, a socket the app binds only
#      with MCP on; the trail test talks to the COMMAND socket. The script must
#      wait for the path the binary under test names (`CadGoose --socket-path`).
#   2. Under `set -e`, a failing build or a failing trail test ended the script
#      before step 7, leaving every app it had hidden hidden. The restore must
#      run on every exit, and the script must exit with the test's own status.
#
# Every external command the script reaches is a fake on PATH (osascript logs
# instead of hiding anything; pkill kills nothing), and CG_PROFILE_ROOT points
# it at a scratch tree whose build/ holds a fake app and a fake test binary.
#
#   bash tests/profiling/test_run_trail_test.sh
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$REPO/tools/profiling/run_trail_test.sh"
FAILURES=0

make_fakes() {   # $1 = scratch dir
    local dir="$1"
    mkdir -p "$dir/bin" "$dir/root/build" "$dir/tmp"
    cat > "$dir/bin/osascript" <<'FAKE'
#!/usr/bin/env bash
# Records what the script asked the desktop to do; changes nothing.
script="$*"
case "$script" in
    *"return appList"*) echo "FakeAppOne, FakeAppTwo" ;;
    *"set visible of p to false"*) echo "hide-all" >> "$FAKE_LOG" ;;
    *"to set visible to true"*)
        app="${script#*tell application \"}"; app="${app%%\"*}"
        echo "restore $app" >> "$FAKE_LOG" ;;
esac
FAKE
    cat > "$dir/bin/clang" <<'FAKE'
#!/usr/bin/env bash
# The Screen Recording probe: "compile" to a binary that reports success.
out=""
while [ $# -gt 0 ]; do
    if [ "$1" = "-o" ]; then out="$2"; shift; fi
    shift
done
printf '#!/bin/sh\nexit 0\n' > "$out"
chmod +x "$out"
FAKE
    printf '#!/bin/sh\nexit 0\n' > "$dir/bin/cmake"
    printf '#!/bin/sh\nexit "${FAKE_MAKE_RC:-0}"\n' > "$dir/bin/make"
    printf '#!/bin/sh\nexit 0\n' > "$dir/bin/pkill"
    cat > "$dir/root/build/CadGoose" <<'FAKE'
#!/usr/bin/env bash
# The app: names its command socket, or binds it and idles.
if [ "${1:-}" = "--socket-path" ]; then echo "$FAKE_SOCK"; exit 0; fi
exec python3 -c 'import socket,sys,time
s=socket.socket(socket.AF_UNIX); s.bind(sys.argv[1]); s.listen(1); time.sleep(60)' "$FAKE_SOCK"
FAKE
    printf '#!/bin/sh\nexit "${FAKE_TRAIL_RC:-0}"\n' > "$dir/root/build/trail_detection_test"
    chmod +x "$dir/bin/"* "$dir/root/build/CadGoose" "$dir/root/build/trail_detection_test"
}

# run_case NAME MAKE_RC TRAIL_RC EXPECT_EXIT
run_case() {
    local name="$1" make_rc="$2" trail_rc="$3" expect="$4"
    local dir rc=0
    dir="$(mktemp -d "${TMPDIR:-/tmp}/cgtt.XXXXXX")"
    make_fakes "$dir"
    : > "$dir/log"
    env -i HOME="$HOME" PATH="$dir/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
        TERM_PROGRAM=ghostty SHELL=/usr/bin/true TMPDIR="$dir/tmp" \
        CG_PROFILE_ROOT="$dir/root" FAKE_LOG="$dir/log" FAKE_SOCK="$dir/cmd.sock" \
        FAKE_MAKE_RC="$make_rc" FAKE_TRAIL_RC="$trail_rc" \
        bash "$SCRIPT" > "$dir/out" 2>&1 < /dev/null || rc=$?
    pkill -f "$dir/cmd.sock" 2>/dev/null || true

    local problems=()
    [ "$rc" -eq "$expect" ] || problems+=("exit $rc, want $expect")
    grep -qx "restore FakeAppOne" "$dir/log" || problems+=("FakeAppOne never restored")
    grep -qx "restore FakeAppTwo" "$dir/log" || problems+=("FakeAppTwo never restored")
    if [ "${#problems[@]}" -eq 0 ]; then
        echo "✓ $name"
    else
        echo "✗ $name: ${problems[*]}"
        sed 's/^/    | /' "$dir/out" | tail -n 15
        FAILURES=$((FAILURES + 1))
    fi
    rm -rf "$dir"
}

# The relaunch into Ghostty must go through `goh launch-app`, never a bare `open`: an app
# started by `open` carries this shell's environment (a ship's GOH_DIR and GOH_SHIP_ITEM
# reached every app it started; GOH.CLEANOPEN). A fake goh records its argv; a fake `open`
# on PATH records any bare use and fails.
run_relaunch_case() {
    local dir rc=0
    dir="$(mktemp -d "${TMPDIR:-/tmp}/cgtt.XXXXXX")"
    make_fakes "$dir"
    : > "$dir/log"
    mkdir -p "$dir/goh/bin"
    printf '#!/bin/sh\necho "goh $*" >> "$FAKE_LOG"\n' > "$dir/goh/bin/goh"
    printf '#!/bin/sh\necho "BARE-OPEN $*" >> "$FAKE_LOG"\nexit 1\n' > "$dir/bin/open"
    chmod +x "$dir/goh/bin/goh" "$dir/bin/open"
    env -i HOME="$HOME" PATH="$dir/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
        TERM_PROGRAM=Apple_Terminal SHELL=/usr/bin/true TMPDIR="$dir/tmp" GOH_DIR="$dir/goh" \
        CG_PROFILE_ROOT="$dir/root" FAKE_LOG="$dir/log" FAKE_SOCK="$dir/cmd.sock" \
        bash "$SCRIPT" > "$dir/out" 2>&1 < /dev/null || rc=$?

    local problems=()
    [ "$rc" -eq 0 ] || problems+=("exit $rc, want 0")
    grep -q "^BARE-OPEN" "$dir/log" && problems+=("a bare open launched Ghostty")
    grep -qx "goh launch-app --new Ghostty -- -e $dir/root/tools/profiling/run_trail_test.sh" "$dir/log" \
        || problems+=("goh launch-app was not asked for a new Ghostty running this script")
    if [ "${#problems[@]}" -eq 0 ]; then
        echo "✓ the Ghostty relaunch goes through goh launch-app"
    else
        echo "✗ the Ghostty relaunch: ${problems[*]}"
        sed 's/^/    | /' "$dir/log" | tail -n 15
        FAILURES=$((FAILURES + 1))
    fi
    rm -rf "$dir"
}

run_case "clean run restores the desktop and exits 0" 0 0 0
run_case "a trail (test exit 10) still restores, and exits 10" 0 10 10
run_case "a failed build still restores, and exits 97" 1 0 97
run_relaunch_case

if [ "$FAILURES" -ne 0 ]; then
    echo "✗ run_trail_test.sh: cases failed: $FAILURES"
    exit 1
fi
echo "✓ run_trail_test.sh: waits on the binary's socket, restores the desktop on every exit"
