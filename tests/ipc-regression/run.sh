#!/usr/bin/env bash
#
# IPC open-forwarding regression gate -- upstream PR #868.
#
#   tests/ipc-regression/run.sh --binary build-fast/qelectrotech
#
# WHAT IT GUARDS
#
# With QET already running, opening a .qet from a file manager forwards the
# path to the running instance over SingleApplication's local socket. Before
# #868, QETApp::receiveMessage() called openFiles() directly -- inside the
# socket's readyRead handler. openFiles() loads the project (seconds, on a
# large one) and puts up a modal BackupDialog whose exec() runs a NESTED event
# loop while the socket handler is still on the stack. During that nested loop
# the secondary exits, the connection closes, the QLocalSocket is deleted, and
# when the dialog is dismissed and the stack unwinds QMetaObject::activate()
# carries on emitting on the freed sender. Segfault.
#
# #868 defers openFiles() with a zero-timer so the socket stack unwinds first.
# The comment there is long on purpose; this test is the other half of making
# sure nobody "simplifies" it away.
#
# THE STEP EVERYONE MISSES
#
# Leaving the backup dialog OPEN never crashes -- the stack never unwinds.
# The bug was twice reported unreproducible for exactly this reason. DISMISSING
# the dialog is the load-bearing step, not opening the project. That is why
# this test needs a window manager and synthetic input rather than being a
# headless CLI check.
#
# WHY A COPIED BINARY (the isolation that makes this safe to run)
#
# SingleApplication hashes the socket name from app/org name, version, and --
# on Linux, unless ExcludeAppPath is set -- applicationFilePath()
# (SingleApplication/singleapplication_p.cpp:156-171). QET constructs it as
# `SingleApplication app(argc, argv, true)` (sources/main.cpp:160), so the
# path IS in the hash.
#
# That matters twice over:
#   * Qt's local sockets live in the abstract namespace, which is scoped to the
#     network namespace, so a container sharing the host's network namespace
#     and running the same binary path will silently capture native launches.
#   * Copying the binary to a unique path therefore gives this test its OWN
#     socket. It cannot talk to, or be hijacked by, a QElectroTech you already
#     have open -- and two runs of this test cannot collide with each other.
#
# Do not "optimise" the copy into a symlink: applicationFilePath() resolves
# through /proc/self/exe, so a symlink lands back on the real path and the
# isolation is silently lost.
#
set -uo pipefail

BINARY=""
PROJECT=""
KEEP_OPEN=0

while [ $# -gt 0 ]; do
    case "$1" in
        --binary)  BINARY="$2"; shift 2 ;;
        --project) PROJECT="$2"; shift 2 ;;
        --keep-open) KEEP_OPEN=1; shift ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

[ -n "$BINARY" ] || { echo "usage: $0 --binary <qet> [--project <file.qet>]" >&2; exit 2; }
[ -x "$BINARY" ] || { echo "not executable: $BINARY" >&2; exit 2; }
BINARY="$(readlink -f "$BINARY")"

# A big project is required, not incidental: the load has to take long enough
# that the modal is up while the socket handler is still on the stack.
# industrial.qet is 2.6 MB and loads in ~4 s.
if [ -z "$PROJECT" ]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    for cand in "$(dirname "$BINARY")/../examples/industrial.qet" \
                "$SCRIPT_DIR/../../examples/industrial.qet"; do
        [ -f "$cand" ] && { PROJECT="$(readlink -f "$cand")"; break; }
    done
fi
[ -f "$PROJECT" ] || { echo "no project found; pass --project" >&2; exit 2; }

for tool in Xvfb openbox xdotool; do
    command -v "$tool" >/dev/null || { echo "missing required tool: $tool" >&2; exit 2; }
done


SANDBOX="$(mktemp -d /tmp/qet-ipc-regression.XXXXXX)"
LOG_DIR="$SANDBOX/logs"; mkdir -p "$LOG_DIR"
PRIMARY_PID=""; SECONDARY_PID=""; XVFB_PID=""; OPENBOX_PID=""

cleanup() {
    if [ "$KEEP_OPEN" = "1" ]; then
        echo "--keep-open: DISPLAY=$DISPLAY sandbox=$SANDBOX (nothing killed)"
        return
    fi
    for pid in "$SECONDARY_PID" "$PRIMARY_PID" "$OPENBOX_PID" "$XVFB_PID"; do
        [ -n "$pid" ] && kill "$pid" 2>/dev/null
    done
    sleep 0.3
    for pid in "$SECONDARY_PID" "$PRIMARY_PID" "$OPENBOX_PID" "$XVFB_PID"; do
        [ -n "$pid" ] && kill -9 "$pid" 2>/dev/null
    done
    [ "${KEEP_LOGS:-0}" = "1" ] || rm -rf "$SANDBOX"
}
trap cleanup EXIT

# Unique binary path == private SingleApplication socket. See header.
TEST_BINARY="$SANDBOX/qelectrotech-ipctest"
cp "$BINARY" "$TEST_BINARY" || { echo "could not copy binary" >&2; exit 2; }

# Work on a COPY of the project, inside the sandbox.
#
# Answering "yes" to the backup prompt is the whole point of this test, and
# QElectroTech writes that backup next to the project it opened. Pointed at
# examples/ directly, a three-attempt run drops three dated .qet files into the
# source tree and leaves them there. Copying first keeps the repository clean
# and means the teardown takes the backups with it.
SANDBOX_PROJECT="$SANDBOX/$(basename "$PROJECT")"
cp "$PROJECT" "$SANDBOX_PROJECT" || { echo "could not copy project" >&2; exit 2; }
PROJECT="$SANDBOX_PROJECT"

# Isolated HOME so the run cannot inherit or leave behind real settings.
export HOME="$SANDBOX/home"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_HOME="$HOME/.local/share"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_DATA_HOME"

# Xvfb -displayfd atomically picks a free display; hardcoding :99 races.
Xvfb -displayfd 9 -screen 0 1600x1000x24 >"$LOG_DIR/xvfb.log" 2>&1 9>"$SANDBOX/dispnum" &
XVFB_PID=$!
for _ in $(seq 1 50); do
    DISP_NUM="$(cat "$SANDBOX/dispnum" 2>/dev/null | tr -d '[:space:]')"
    [ -n "$DISP_NUM" ] && break
    sleep 0.1
done
[ -n "${DISP_NUM:-}" ] || { echo "FAIL: Xvfb never reported a display" >&2; exit 1; }
export DISPLAY=":$DISP_NUM"

# openbox is mandatory -- without a WM, xdotool cannot activate windows
# ("your windowmanager claims not to support _NET_ACTIVE_WINDOW").
openbox >"$LOG_DIR/openbox.log" 2>&1 &
OPENBOX_PID=$!
sleep 1

# Only count real windows: Qt/QPA leaves a HIDDEN 1x1 helper window whose
# title matches the main window exactly, so name-only matching is a coin flip.
real_window() {
    local geo w h
    geo="$(xdotool getwindowgeometry --shell "$1" 2>/dev/null)" || return 1
    w="$(echo "$geo" | sed -n 's/^WIDTH=//p')"
    h="$(echo "$geo" | sed -n 's/^HEIGHT=//p')"
    [ -n "$w" ] && [ -n "$h" ] && [ "$w" -gt 50 ] && [ "$h" -gt 50 ]
}

main_window() {
    local w
    for w in $(xdotool search --onlyvisible --name "QElectroTech" 2>/dev/null); do
        real_window "$w" && { echo "$w"; return 0; }
    done
    return 1
}

# Any real window that is not the main window. NOT filtered by title -- see
# below for why that matters more than it looks.
modal_window() {
    local main="$1" w
    for w in $(xdotool search --onlyvisible --name "." 2>/dev/null); do
        [ "$w" = "$main" ] && continue
        real_window "$w" && { echo "$w"; return 0; }
    done
    return 1
}

# Every non-main real window, newest last (X window ids increase).
modal_windows() {
    local main="$1" w out=""
    for w in $(xdotool search --onlyvisible --name "." 2>/dev/null | sort -n); do
        [ "$w" = "$main" ] && continue
        real_window "$w" && out="$out $w"
    done
    echo "$out"
}

# WHY DIALOGS ARE NOT IDENTIFIED BY TITLE
#
# Opening a project raises a progress dialog several seconds BEFORE the backup
# prompt, and both stay up. Answering the progress dialog looks exactly like a
# successful run: a modal was found, a key was sent, the app survived. PASS,
# against a build that crashes reliably.
#
# The first version of this test filtered the progress dialog out by title.
# That worked locally ("Merci de patienter") and silently broke in Docker,
# where the same dialog is "Thank you for your patience" -- a green that could
# not go red, which is the exact failure this test exists to prevent. A title
# list is a locale list, and there is no reason to believe anyone will keep it
# in sync with QET's translations.
#
# So: dismiss every dialog that appears, repeatedly, and identify nothing.
# The progress dialog ignores Return harmlessly; the backup prompt takes it.
# The run is only meaningful if at least TWO distinct dialogs were seen (the
# progress dialog plus the prompt) -- one means the prompt never appeared and
# the crash path was never exercised, which is INCONCLUSIVE, not a pass.

ATTEMPTS="${QET_IPC_ATTEMPTS:-3}"

echo "── QElectroTech IPC open-forwarding regression (PR #868) ──"
echo "binary   : $BINARY"
echo "project  : $(basename "$PROJECT")"
echo "display  : $DISPLAY"
echo "attempts : $ATTEMPTS"
echo

# WHY THIS REPEATS
#
# The crash is a race: it needs the secondary's disconnect to be processed
# while the modal's nested event loop is still on the stack. Measured on an
# unfixed Qt 6 build of master, a single attempt reproduced it 2 times in 3.
# One attempt is therefore not a gate -- it would wave a reintroduced
# use-after-free through about a third of the time.
#
# Attempts are independent, so N runs miss it with probability (1/3)^N:
# 3 attempts ~= 96% detection, 5 ~= 99.6%. Any single crash fails the run;
# survival must be unanimous.
#
# A fixed build passes every attempt, so the repetition costs nothing but
# time on a green build.

# Runs the scenario once. 0 = survived, 1 = crashed, 2 = inconclusive.
run_once() {
    local attempt="$1"
    local primary_pid="" secondary_pid="" main="" modal=""
    local log="$LOG_DIR/attempt-$attempt"
    mkdir -p "$log"

    # Each attempt gets a FRESH HOME. A crashed attempt leaves a backup file
    # behind, and the next run then opens with "Restore file" instead of --
    # or before -- "Create a backup copy?". Observed directly: attempt 2 of a
    # Docker run saw a restore prompt and never saw the backup prompt at all,
    # so it exercised a different path and proved nothing. Attempts have to be
    # independent or the repetition is not buying what it claims to.
    export HOME="$SANDBOX/home-$attempt"
    export XDG_CONFIG_HOME="$HOME/.config"
    export XDG_DATA_HOME="$HOME/.local/share"
    rm -rf "$HOME"
    mkdir -p "$XDG_CONFIG_HOME" "$XDG_DATA_HOME"

    "$TEST_BINARY" >"$log/primary.log" 2>&1 &
    primary_pid=$!

    for _ in $(seq 1 100); do
        main="$(main_window)" && [ -n "$main" ] && break
        kill -0 "$primary_pid" 2>/dev/null || { echo "  primary died during startup"; return 2; }
        sleep 0.2
    done
    [ -n "$main" ] || { kill "$primary_pid" 2>/dev/null; echo "  primary never showed a window"; return 2; }

    for _ in $(seq 1 10); do
        m="$(modal_window "$main")" || break
        xdotool windowactivate --sync "$m" 2>/dev/null
        xdotool key --clearmodifiers Escape 2>/dev/null
        sleep 0.3
    done

    "$TEST_BINARY" "$PROJECT" >"$log/secondary.log" 2>&1 &
    secondary_pid=$!

    local seen_ids="" distinct=0 crashed=0
    for _ in $(seq 1 200); do
        if ! kill -0 "$primary_pid" 2>/dev/null; then crashed=1; break; fi
        for w in $(modal_windows "$main"); do
            case " $seen_ids " in
                *" $w "*) ;;
                *)  seen_ids="$seen_ids $w"
                    distinct=$((distinct + 1))
                    echo "  dialog $distinct: '$(xdotool getwindowname "$w" 2>/dev/null)'" ;;
            esac
            xdotool windowactivate --sync "$w" 2>/dev/null
            xdotool key --clearmodifiers Return 2>/dev/null
        done
        [ "$distinct" -ge 2 ] && [ -z "$(modal_windows "$main")" ] && break
        sleep 0.2
    done

    if [ "$crashed" = "0" ] && [ "$distinct" -lt 2 ]; then
        kill "$primary_pid" "$secondary_pid" 2>/dev/null
        echo "  only $distinct dialog(s) seen -- the backup prompt never appeared,"
        echo "  so the crash path was not exercised"
        return 2
    fi

    sleep 2
    local result=0
    if ! kill -0 "$primary_pid" 2>/dev/null; then
        wait "$primary_pid" 2>/dev/null; local rc=$?
        echo "  CRASH: primary died after dismissal (exit $rc)"
        result=1
    elif grep -qiE "segmentation fault|SIGSEGV|AddressSanitizer" "$log/primary.log" 2>/dev/null; then
        echo "  CRASH: primary logged a crash signature"
        result=1
    else
        echo "  survived"
    fi

    kill "$primary_pid" "$secondary_pid" 2>/dev/null
    sleep 0.5
    kill -9 "$primary_pid" "$secondary_pid" 2>/dev/null
    return $result
}

crashes=0
inconclusive=0
for attempt in $(seq 1 "$ATTEMPTS"); do
    echo "attempt $attempt/$ATTEMPTS"
    run_once "$attempt"
    case $? in
        1) crashes=$((crashes + 1)) ;;
        2) inconclusive=$((inconclusive + 1)) ;;
    esac
done

echo
if [ "$crashes" -gt 0 ]; then
    echo "FAIL: primary crashed in $crashes of $ATTEMPTS attempts."
    echo "      This is the PR #868 use-after-free -- forwarded files are being"
    echo "      opened inside the SingleApplication socket handler again."
    echo "      Logs: $LOG_DIR (preserved)"
    KEEP_LOGS=1
    exit 1
fi

if [ "$inconclusive" -eq "$ATTEMPTS" ]; then
    echo "INCONCLUSIVE: every attempt failed to exercise the crash path."
    echo "      Nothing was proven. Check the binary forwards at all (needs"
    echo "      upstream #861) and that the project is large enough to raise"
    echo "      the backup prompt."
    exit 2
fi

echo "PASS: primary survived $((ATTEMPTS - inconclusive)) of $ATTEMPTS attempts, none crashed."
[ "$inconclusive" -gt 0 ] && echo "      ($inconclusive attempt(s) inconclusive.)"
exit 0
