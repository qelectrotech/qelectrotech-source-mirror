# Quit-during-modal regression test

Guards the abort reported in #904.

`QETDiagramEditor::openAndAddProject()` shows `BackupDialog` as a stack object
parented to the editor and `exec()`s it, and `QET::QetMessageBox` does the same
for every message box. `exec()` runs a nested event loop. Closing the editor
during that loop turns `WA_DeleteOnClose` into a `deleteLater()` that the nested
loop processes, so `~QWidget()` deletes the stack-allocated dialog and the
process aborts. `QETMainWindow::refuseCloseWhileModal()` refuses such a close.

## Running it

```bash
tests/modal-quit-regression/run.sh --binary build/qelectrotech
```

Needs `gdb` with Python. It runs on the offscreen platform, so no X server or
window manager is required. Takes about ten seconds.

It is also registered with CTest on Linux (`ctest -R modal_quit_regression`).

Exit codes: `0` survived, `1` crashed, `2` the scenario did not happen,
`77` it could not run here.

That last one matters for a release build. The scenario is driven by calling
`QETApp::instance()` and `QETApp::quitQET()` through gdb, so those symbols have
to survive into the binary: against a **stripped** build there is nothing to
call, and the same applies without gdb or with a gdb built without Python. All
three report 77, which is CTest's `SKIP_RETURN_CODE`, so a build this test
cannot drive is skipped rather than failed.

## How it works, and why this way

On Linux there is nothing to click: the menu bar belongs to the window the
dialog blocks, and Qt ignores window-manager close requests for a blocked
window. The reported route is macOS, where File > Quit lives in the application
menu and stays usable during a modal — and what it does is call `close()` while
the dialog's loop is running. The test does the same thing through gdb:

1. break on `QDialog::exec()`;
2. let the dialog's loop run, then interrupt it, so the main thread is inside
   the nested loop — the only place the bug exists, because a `deleteLater()`
   posted *before* `exec()` started is not processed by that loop;
3. call `QETApp::quitQET()`, which closes every editor;
4. let it run: an unfixed build aborts within a second.

It matches no window titles and no window ids. Titles are translated, and
`tests/ipc-regression` once shipped a pass that could not fail because of that.

## Covering the element editor too

`--project` decides which editor is under test, because QElectroTech picks the
editor from the file extension. Pass a `.qet` and the run exercises
`QETDiagramEditor`; pass a **read-only `.elmt`** and it exercises
`QETElementEditor`, which shows a "file is read-only" message box on open and so
reaches the same nested loop by a different route:

```bash
chmod -w some.elmt
tests/modal-quit-regression/run.sh --binary build/qelectrotech --project some.elmt
```

That distinction is why the file name is preserved when it is copied into the
sandbox. Renaming a `.elmt` to `project.qet` would make the run silently test
the diagram editor again, and still report PASS.

Checked both ways, in both editors, before being committed: without the fix the
diagram-editor run aborts with signal 6 under `~QETDiagramEditor()` and the
element-editor run with `double free or corruption` under
`~QETElementEditor()`; with the fix both survive.
