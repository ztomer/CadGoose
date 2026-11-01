#!/usr/bin/env bash
# test_run_tests_linux_runtime.sh — on macOS, scripts/run_tests_linux.sh starts
# a docker that is down (colima) and stops it again; a docker that is up is
# left alone; a host with no runtime to start is red, never a skip. Driven on
# stub docker/colima through the CG_HOST_OS / DOCKER / COLIMA seams.
#
#   bash tests/profiling/test_run_tests_linux_runtime.sh
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$REPO/scripts/run_tests_linux.sh"
DIR="$(mktemp -d "${TMPDIR:-/tmp}/cadgoose-runtime-XXXXXX")"
trap 'rm -rf "$DIR"' EXIT
FAILURES=0

cat > "$DIR/docker" <<EOF
#!/usr/bin/env bash
echo "docker \$1" >> "$DIR/calls"
case "\$1" in
    info) [ -f "$DIR/up" ] ;;
    ps) exit 0 ;;
    *) exit 0 ;;
esac
EOF
cat > "$DIR/colima" <<EOF
#!/usr/bin/env bash
echo "colima \$1" >> "$DIR/calls"
case "\$1" in start) : > "$DIR/up" ;; stop) rm -f "$DIR/up" ;; esac
EOF
chmod +x "$DIR/docker" "$DIR/colima"

drive() {   # up (0/1), colima path; prints rc
    local rc=0
    : > "$DIR/calls"
    rm -f "$DIR/up"; [ "$1" = 1 ] && : > "$DIR/up"
    CG_HOST_OS=Darwin DOCKER="$DIR/docker" COLIMA="$2" GOH_CONTAINER_STATE="$DIR/state" \
        bash "$SCRIPT" > "$DIR/out" 2>&1 < /dev/null || rc=$?
    echo "$rc"
}
check() {   # name, condition-result
    if [ "$2" = 0 ]; then echo "✓ $1"; else echo "✗ $1"; cat "$DIR/calls" "$DIR/out"; FAILURES=$((FAILURES + 1)); fi
}

rc="$(drive 0 "$DIR/colima")"
ok=0; { grep -q '^colima start$' "$DIR/calls" && grep -q '^colima stop$' "$DIR/calls" && [ "$rc" = 0 ]; } || ok=1
check "a down docker is started, the suite runs, and it is stopped after" "$ok"

rc="$(drive 1 "$DIR/colima")"
ok=0; { ! grep -q '^colima' "$DIR/calls" && grep -q '^docker run$' "$DIR/calls" && [ "$rc" = 0 ]; } || ok=1
check "an up docker is used and nothing is started or stopped" "$ok"

rc="$(drive 0 "$DIR/no-such-colima")"
ok=0; { [ "$rc" != 0 ] && ! grep -q '^docker run$' "$DIR/calls"; } || ok=1
check "no runtime to start is red, never a skip" "$ok"

[ "$FAILURES" = 0 ]
