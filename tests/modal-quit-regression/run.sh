#!/usr/bin/env bash
#
# Quit-during-modal regression gate -- issue #904.
#
#   tests/modal-quit-regression/run.sh --binary build/qelectrotech
#   tests/modal-quit-regression/run.sh --binary build/qelectrotech \
#                                      --project read-only.elmt
#
# --project also picks the editor under test: QElectroTech chooses it from the
# extension, so a .qet exercises QETDiagramEditor and a read-only .elmt
# exercises QETElementEditor (which opens a message box of its own).
#
# WHAT IT GUARDS
#
# QETDiagramEditor::openAndAddProject() shows BackupDialog as a stack object
# parented to the editor and exec()s it; QET::QetMessageBox does the same for
# every message box. exec() runs a NESTED event loop. If the editor is closed
# during that loop, WA_DeleteOnClose becomes a deleteLater() that the nested
# loop processes: ~QWidget() deletes the editor's children, the stack-allocated
# dialog among them, and the process aborts ("free(): invalid size").
# QETMainWindow::refuseCloseWhileModal() refuses such a close.
#
# HOW IT TRIGGERS THE BUG WITHOUT A MOUSE
#
# On Linux the menu bar belongs to the window the modal blocks, and Qt ignores
# window-manager close requests for a blocked window, so there is nothing to
# click. The reported route is macOS, where File > Quit moves to the
# application menu and stays usable. What that route does is call close()
# programmatically while the dialog's loop is spinning, and that is what this
# test does, through gdb:
#
#   1. break on QDialog::exec(), i.e. the moment the first dialog is shown;
#   2. let it run for a moment, then interrupt it -- the main thread is now
#      inside the dialog's nested loop, which is the only place the bug lives
#      (a deleteLater() posted before exec() started is not processed by it);
#   3. let it run on to the start of the loop's next QEventLoop::processEvents()
#      on the main thread, and call QETApp::quitQET() there, which closes
#      every editor. Not where the interrupt lands: that can be inside
#      malloc(), whose lock a call into the program then waits on for ever
#      (1 run in 20 timed out that way, on master too). Between two
#      events the loop holds nothing, and QElectroTech's event loop
#      watchdog wakes it every 50 ms, so the stop comes at once;
#   4. let it run again: the unfixed build aborts within a second, the fixed
#      one keeps running until the test interrupts it.
#
# Nothing here matches a window title or a window id. A title is a locale:
# tests/ipc-regression once shipped a pass that could not fail because the
# dialog it filtered by name was translated differently in Docker.
#
# It runs on the offscreen platform, so no X server or window manager is
# needed -- only gdb with Python.
#
# RESULT
#   exit 0   PASS   quitQET() ran inside the modal loop and the process survived
#   exit 1   FAIL   the process died of a signal after quitQET()
#   exit 2   ERROR  the scenario never happened (no dialog, gdb problem, ...)
#   exit 77  SKIP   cannot run here: no gdb, a gdb without Python, or a
#                   stripped binary. 77 is CTest's SKIP_RETURN_CODE, so a
#                   build this test cannot drive is reported as skipped
#                   rather than failed.
#
set -uo pipefail

BINARY=""
PROJECT=""
SETTLE=2      # seconds inside the dialog loop before interrupting
OBSERVE=5     # seconds to wait for a crash after quitQET()

while [ $# -gt 0 ]; do
    case "$1" in
        --binary)  BINARY="$2"; shift 2 ;;
        --project) PROJECT="$2"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

[ -n "$BINARY" ] || { echo "usage: $0 --binary <qet> [--project <file.qet>]" >&2; exit 2; }
[ -x "$BINARY" ] || { echo "not executable: $BINARY" >&2; exit 2; }
BINARY="$(readlink -f "$BINARY")"

if [ -z "$PROJECT" ]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    PROJECT="$(ls "$SCRIPT_DIR"/../../examples/*.qet 2>/dev/null | head -1)"
fi
[ -f "$PROJECT" ] || { echo "no project found; pass --project" >&2; exit 2; }

skip() { echo "SKIP: $1"; exit 77; }

command -v gdb >/dev/null || skip "gdb is not installed"
gdb -batch -ex 'python import gdb' >/dev/null 2>&1 \
    || skip "this gdb has no Python support"

# The scenario is driven by calling QETApp::instance() and QETApp::quitQET()
# through gdb, so their symbols have to survive into the binary. A stripped
# release build cannot be driven at all -- that is a property of the build,
# not a failure of the code under test, so report it as skipped.
#
# Read nm's output once into a variable rather than piping it into grep -q:
# grep -q exits at the first match, nm dies of SIGPIPE, and under `set -o
# pipefail` the pipeline reports failure even though the symbol was found --
# which would skip this test on every build that can actually run it.
if command -v nm >/dev/null; then
    SYMBOLS="$(nm -C "$BINARY" 2>/dev/null || true)"
    for sym in "QETApp::instance()" "QETApp::quitQET()"; do
        case "$SYMBOLS" in
            *"$sym"*) ;;
            *) skip "binary has no symbol for $sym (stripped build?)" ;;
        esac
    done
fi

SANDBOX="$(mktemp -d /tmp/qet-modal-quit.XXXXXX)"

# SingleApplication keeps a System V shared memory block, keyed from a file in
# the temporary folder: 16 bits of the key vary. This run ends QElectroTech
# with gdb's kill, which leaves its block behind; blocks left so pile up, and
# once a new run's key falls on one, SingleApplication cannot start ("Unable to
# attach to shared memory block") -- 1 run in 40 with 976 of them. So the
# blocks this run leaves, attached to no process, are removed at the end, and
# a run that meets someone else's is tried again (below).
orphan_blocks() {
    command -v ipcs >/dev/null || return 0
    ipcs -m 2>/dev/null | awk -v me="$(id -un)" 'NR > 3 && $3 == me && $6 == 0 { print $2 }'
}
BLOCKS_BEFORE=" $(orphan_blocks | tr '\n' ' ') "
cleanup() {
    if command -v ipcrm >/dev/null; then
        for id in $(orphan_blocks); do
            case "$BLOCKS_BEFORE" in *" $id "*) ;; *) ipcrm -m "$id" 2>/dev/null ;; esac
        done
    fi
    [ "${KEEP_LOGS:-0}" = "1" ] || rm -rf "$SANDBOX"
}
trap cleanup EXIT

# A unique binary path gives this run its own SingleApplication socket, so it
# can neither be captured by nor capture a QElectroTech already running. A
# symlink would not do: applicationFilePath() resolves it back to the real path.

# Keep the original file name. QElectroTech decides what to open from the
# extension, so copying a .elmt to "project.qet" would quietly turn an
# element-editor run into a failed project load -- and the scenario would
# still "work", against the wrong window.
SANDBOX_PROJECT="$SANDBOX/$(basename "$PROJECT")"
cp "$PROJECT" "$SANDBOX_PROJECT" || { echo "could not copy input file" >&2; exit 2; }

export HOME="$SANDBOX/home"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_HOME="$HOME/.local/share"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_DATA_HOME"
export QT_QPA_PLATFORM=offscreen

cat > "$SANDBOX/scenario.gdb" <<EOF
set debuginfod enabled off
set pagination off
set confirm off
set breakpoint pending on
handle SIGINT stop print nopass
python
import subprocess
def qet_interrupt_later(seconds):
    pid = gdb.selected_inferior().pid
    subprocess.Popen(["sh", "-c", "sleep %d; kill -INT %d" % (seconds, pid)])
end
break QDialog::exec
run
delete
if \$_isvoid(\$_exitcode)
printf "QET_TEST: dialog exec() entered\n"
else
printf "QET_TEST: exited before any dialog, code %d\n", \$_exitcode
end
python qet_interrupt_later($SETTLE)
continue
tbreak QEventLoop::processEvents thread 1
continue
printf "QET_TEST: at the dialog's event loop\n"
thread 1
set \$app = ((void* (*)(void))'QETApp::instance()')()
call ((void (*)(void*))'QETApp::quitQET()')(\$app)
printf "QET_TEST: quitQET() returned\n"
python qet_interrupt_later($OBSERVE)
continue
printf "QET_TEST: final signal %d\n", \$_siginfo.si_signo
bt 20
kill
EOF

# Bound the run with timeout(1) rather than a backgrounded watchdog subshell.
# A "( sleep N; kill ) &" watchdog runs sleep as a child of the subshell, so
# killing the subshell orphans the sleep -- and the orphan keeps this script's
# stdout open. Read through a pipe, as CTest does, that makes every run last
# the full watchdog period no matter how fast gdb finished: ten seconds of
# work reported as two minutes, a stone's throw from the CTest timeout.
GDB_TIMEOUT=120
# Each try under a binary path of its own: a new SingleApplication key. Tried
# again only when QElectroTech stopped before any dialog because
# SingleApplication met a block it could not use -- the scenario never ran,
# the code under test played no part.
for attempt in 1 2 3; do
    TEST_BINARY="$SANDBOX/qelectrotech-modalquit-$attempt"
    cp "$BINARY" "$TEST_BINARY" || { echo "could not copy binary" >&2; exit 2; }
    if command -v timeout >/dev/null; then
        timeout --signal=KILL "$GDB_TIMEOUT" \
            gdb -batch -x "$SANDBOX/scenario.gdb" --args "$TEST_BINARY" "$SANDBOX_PROJECT" \
            > "$SANDBOX/gdb.log" 2>&1
    else
        gdb -batch -x "$SANDBOX/scenario.gdb" --args "$TEST_BINARY" "$SANDBOX_PROJECT" \
            > "$SANDBOX/gdb.log" 2>&1
    fi
    if grep -q "QET_TEST: exited before any dialog" "$SANDBOX/gdb.log" \
            && grep -q "SingleApplication: Unable to" "$SANDBOX/gdb.log"; then
        echo "note: try $attempt: SingleApplication could not set up its shared memory (a block left by another run); trying again"
        cp "$SANDBOX/gdb.log" "$SANDBOX/gdb-try$attempt.log"
        continue
    fi
    break
done

log="$SANDBOX/gdb.log"
if ! grep -q "QET_TEST: dialog exec() entered" "$log"; then
    echo "ERROR: no dialog was ever shown -- the scenario did not happen"
    KEEP_LOGS=1; echo "log kept: $log"; exit 2
fi
if ! grep -q "QET_TEST: quitQET() returned" "$log"; then
    echo "ERROR: quitQET() was not called inside the dialog loop"
    KEEP_LOGS=1; echo "log kept: $log"; exit 2
fi

sig="$(sed -n 's/^QET_TEST: final signal \([0-9]*\)$/\1/p' "$log" | tail -1)"
case "$sig" in
    2)
        echo "PASS: closing the editor during a modal dialog was refused; the process survived"
        exit 0 ;;
    "")
        echo "ERROR: could not tell how the run ended"
        KEEP_LOGS=1; echo "log kept: $log"; exit 2 ;;
    *)
        echo "FAIL: the process died of signal $sig after quitQET() ran inside the dialog loop (issue #904)"
        grep -m1 -E "free\(\)|double free|corrupted" "$log" | sed 's/^/  /'
        sed -n '/^QET_TEST: final signal/,$p' "$log" | grep -E '^#[0-9]+ ' | sed 's/^/  /'
        KEEP_LOGS=1; echo "log kept: $log"; exit 1 ;;
esac
