#!/usr/bin/env bash
#
# Quit-during-modal regression gate -- issue #904.
#
#   tests/modal-quit-regression/run.sh --binary build/qelectrotech
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
#   3. call QETApp::quitQET(), which closes every editor;
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
#   exit 0  PASS   quitQET() ran inside the modal loop and the process survived
#   exit 1  FAIL   the process died of a signal after quitQET()
#   exit 2  ERROR  the scenario never happened (no dialog, gdb problem, ...)
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

command -v gdb >/dev/null || { echo "missing required tool: gdb" >&2; exit 2; }
gdb -batch -ex 'python import gdb' >/dev/null 2>&1 \
    || { echo "gdb has no Python support" >&2; exit 2; }

SANDBOX="$(mktemp -d /tmp/qet-modal-quit.XXXXXX)"
cleanup() { [ "${KEEP_LOGS:-0}" = "1" ] || rm -rf "$SANDBOX"; }
trap cleanup EXIT

# A unique binary path gives this run its own SingleApplication socket, so it
# can neither be captured by nor capture a QElectroTech already running. A
# symlink would not do: applicationFilePath() resolves it back to the real path.
TEST_BINARY="$SANDBOX/qelectrotech-modalquit"
cp "$BINARY" "$TEST_BINARY" || { echo "could not copy binary" >&2; exit 2; }

cp "$PROJECT" "$SANDBOX/project.qet" || { echo "could not copy project" >&2; exit 2; }

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
printf "QET_TEST: dialog exec() entered\n"
python qet_interrupt_later($SETTLE)
continue
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

gdb -batch -x "$SANDBOX/scenario.gdb" --args "$TEST_BINARY" "$SANDBOX/project.qet" \
    > "$SANDBOX/gdb.log" 2>&1 &
GDB_PID=$!
( sleep 120; kill -9 "$GDB_PID" 2>/dev/null ) &
WATCHDOG_PID=$!
wait "$GDB_PID"
kill "$WATCHDOG_PID" 2>/dev/null

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
