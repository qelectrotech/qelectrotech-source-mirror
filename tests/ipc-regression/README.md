# IPC open-forwarding regression test

Guards the use-after-free fixed in PR #868.

With QElectroTech already running, opening a `.qet` from a file manager forwards
the path to the running instance over SingleApplication's local socket. Before
#868, `QETApp::receiveMessage()` called `openFiles()` directly — inside the
socket's `readyRead` handler. Loading the project raises a modal backup prompt
whose `exec()` runs a nested event loop while that handler is still on the
stack. The second instance then exits, its socket is deleted, and when the
prompt is dismissed and the stack unwinds, `QMetaObject::activate()` carries on
emitting on freed memory.

## Running it

```bash
tests/ipc-regression/run.sh --binary build/qelectrotech
```

Needs `Xvfb`, `openbox` and `xdotool`. It allocates its own display, runs
entirely in a sandbox, and cleans up after itself.

Exit codes: `0` survived, `1` crashed, `2` inconclusive or unusable input.

## Requirements that are not obvious

**Qt 6.** An unfixed Qt 5 build survives this scenario every time — measured,
not assumed. The script refuses to run on a Qt 5 binary rather than report a
pass that cannot fail. Reproduced on Qt 6.10.2.

**A Debug build.** Whether a use-after-free faults depends on what the allocator
does with the freed block. The same unfixed commit crashes 3 times in 3 built
Debug, and survives every attempt built `-O3 -DNDEBUG`.

**A large project.** The load has to take long enough that the prompt is up
while the socket handler is still on the stack. `examples/industrial.qet`
(2.6 MB, ~4 s) is used by default.

## Why it repeats

The crash is a race: the second instance's disconnect has to be processed while
the nested event loop is on the stack. Single attempts reproduced it 2 times in
3, so one attempt would let a reintroduced bug through about a third of the
time. Three attempts is roughly 96%; `QET_IPC_ATTEMPTS` changes it. Any single
crash fails the run, and survival must be unanimous.

## Validation

| build | result |
|---|---|
| `ceda1e082` (before #868) | 3/3 crashed, exit 139 |
| `199444b6` (after #868) | 3/3 survived |

If this test is changed, re-validate it against a build without the fix. A gate
nobody has watched go red is not evidence.

## Things that made earlier versions of this test pass a crashing build

Recorded because each one looked exactly like success.

**Answering the wrong dialog.** A progress dialog appears several seconds before
the backup prompt and both stay up. Dismissing the progress dialog satisfies a
naive "a modal appeared, dismiss it" check while never touching the crash path.

**Filtering that dialog out by title.** That is a locale filter: it worked in
French and silently stopped working in English. The script now dismisses every
dialog and identifies none, and treats a run that saw fewer than two distinct
dialogs as inconclusive rather than passing.

**Sharing a home directory between attempts.** A crashed attempt leaves a backup
file, so the next one opens with the restore prompt instead of the backup
prompt — a path that never unwinds the socket stack. Each attempt now gets a
fresh `HOME`.
