# qet-mcp — a Model Context Protocol server for QElectroTech projects

A small stdio MCP server that lets an AI assistant read and verify
QElectroTech projects: what is in a project, what an edit actually
changed, and what a whole corpus of projects contains.

It has **no third-party dependencies** — Python 3.9+ and the standard
library only. The MCP SDK is not required.

## Why

Verifying a change by screenshot is unreliable, and this tool exists
because that unreliability produced two wrong conclusions in one review
session:

- A drag of a multi-element selection *looked* like it had left the
  symbols behind and detached their labels. Diffing the saved file showed
  all four elements had moved by an identical `(0, -80)` and **no label
  had moved at all**. A bug report was one step away from being filed.
- An "Apply" button *looked* like it did nothing. It was disabled,
  because a required field was empty.

Both times the pixels misled and the model told the truth. So the tools
here read the model.

## Tools

| Tool | What it answers |
|---|---|
| `qet_project_info` | title, format version, folios with their uuids, element and conductor counts |
| `qet_elements` | placed elements: uuid, type, position, label, information bag |
| `qet_conductors` | conductors and their documentation fields; filter by attribute |
| `qet_items` | free texts, shapes, pictures, tables and symbol text fields, each with its uuid |
| `qet_diff` | **what an edit actually changed** — element moves, adds, removes, relabels; conductor changes; and folio fields, texts, shapes, images, symbol text fields and terminal strips |
| `qet_scan` | sweep a directory of projects, counting nodes carrying an attribute |
| `qet_element_info` | a `.elmt`: translated names, terminals, info fields, part counts |
| `qet_export` | run a headless export (pdf, png, svg, dxf, bom, cables, wires, wiring, nets, links, info) |
| `qet_edit` | **change a project** — place, move, rotate, label, wire, number, cross-reference, add text, shapes and images, restyle a symbol's text fields, delete; then diff the result |
| `qet_element_build` | **author a `.elmt`** — draw a new symbol, with terminals to wire it by |
| `qet_project_new` | **start from nothing** — an empty project with a title and folios |
| `qet_element_search` | **find a symbol** in a collection by name (any language), type or terminal count |
| `qet_check` | **design-rule checks** — duplicate labels, unlabelled masters, unnumbered conductors, empty folios |
| `qet_query` | **ask the project database** — read-only SQL over the views and tables |
| `qet_about` | **start here** — where QElectroTech keeps things, what is switched on, the stored scripts, the calls a script can make (from `qet-assistant.json`) |
| `qet_script_api` | **what a script can call** — every `qet.*` call of this build, and the header that makes a script a button |
| `qet_script_test` | **try a script** on a copy of a project: what it would change, what it logged, its errors |
| `qet_script_install` | **make a button** — store a script (and an SVG icon) where QElectroTech shows it in Projet > Scripts and the Scripts toolbar |
| `qet_script_list`, `qet_script_read`, `qet_script_remove` | the stored scripts: list, read one to change it, delete one |
| `qet_recording_list`, `qet_recording_read`, `qet_recording_check`, `qet_recording_remove` | **macro recordings** — what you did by hand, and whether a script does the same |

`qet_export` and `qet_edit` launch QElectroTech. Everything else parses the
file directly, which is faster, needs no display, and cannot be confused by
a dialog.

## Running it

```bash
# list the tools and exit
misc/qet-mcp/qet_mcp.py --list

# speak MCP on stdin/stdout
misc/qet-mcp/qet_mcp.py

# run one tool and exit, no MCP client needed
misc/qet-mcp/qet_mcp.py --call qet_project_info '{"path": "drawing.qet"}'
```

Register it with an MCP client, for example:

```json
{
  "mcpServers": {
    "qet": {
      "command": "python3",
      "args": ["/path/to/qelectrotech/misc/qet-mcp/qet_mcp.py"],
      "env": {
        "QET_MCP_WORKSPACE": "/home/you/drawings",
        "QET_BINARY": "/usr/bin/qelectrotech",
        "QET_ENABLE_SCRIPTING": "1"
      }
    }
  }
}
```

`QET_BINARY` is the QElectroTech the tools launch. Leave it out when
`qelectrotech` is on your `PATH`, or when the server is installed with
QElectroTech (as `<prefix>/share/qelectrotech/mcp/qet_mcp.py`, which also
finds the installed element collection).

## Installed with QElectroTech

QElectroTech's packages install the server next to the program, and from
there it finds that QElectroTech and its element collection by itself:

| Package | Server | Finds |
|---|---|---|
| `make install`, Linux distributions | `<prefix>/share/qelectrotech/mcp/qet_mcp.py` | `<prefix>/bin/qelectrotech`, `<prefix>/share/qelectrotech/elements` |
| Windows installer, MSI, portable folder | `<folder>\mcp\qet_mcp.py` (the "AI assistant (MCP)" component) | `<folder>\bin\QElectroTech.exe`, `<folder>\elements` |

So a client configuration needs only the path to the server and the
workspace. On Windows, the installer also offers **Python for the AI
assistant** (unticked by default; always in the portable folder and the
MSI): Python from python.org in `<folder>\mcp\python`, for anyone without
a Python of their own. Then:

```json
"command": "C:\\Program Files\\QElectroTech\\mcp\\python\\python.exe",
"args": ["C:\\Program Files\\QElectroTech\\mcp\\qet_mcp.py"]
```

Snap and flatpak install it too. Their QElectroTech is built to run inside
the package's sandbox, and starting it from the server has not been tested:
the tools that read a file work, exports and edits may not. If they fail,
point `QET_BINARY` at a QElectroTech installed another way.

## Using it from the Claude app

A web chat in a browser cannot start a program on your computer, so it
cannot run this server. The Claude desktop app for Windows and macOS can,
and it uses the same account as the website.

1. Install Python 3.9 or later. Nothing else is needed.
2. In the desktop app, open **Settings → Developer → Edit Config**. This
   opens `claude_desktop_config.json`.
3. Add the server, with your own paths:

   ```json
   {
     "mcpServers": {
       "qet": {
         "command": "python",
         "args": ["C:\\path\\to\\qelectrotech\\misc\\qet-mcp\\qet_mcp.py"],
         "env": {
           "QET_MCP_WORKSPACE": "C:\\Users\\you\\Documents\\drawings",
           "QET_ENABLE_SCRIPTING": "1"
         }
       }
     }
   }
   ```

   On macOS use `python3` and ordinary `/` paths. In JSON every `\` in a
   Windows path is written `\\`.
4. Quit the app completely and start it again. The tools appear under the
   chat box's tools menu.
5. If `qelectrotech` is not on your `PATH`, add `"QET_BINARY"` to the
   `env` block with the full path to the executable
   (`C:\\Program Files\\...\\qelectrotech.exe`, written with `\\`).

Only files under `QET_MCP_WORKSPACE` can be read or written (see
[What the server is allowed to touch](#what-the-server-is-allowed-to-touch)).
Leave out `QET_ENABLE_SCRIPTING` if you do not want the assistant to edit
projects; see the next section for what that switches off.

The test suite runs on Linux. The server uses nothing platform-specific,
but it has not yet been tested on Windows or macOS.

### With only a browser

If your web chat can run Python (on claude.ai, code execution), upload
`qet_mcp.py` together with your project and ask the assistant to use
`--call`:

```bash
python3 qet_mcp.py --call qet_elements '{"path": "drawing.qet"}'
echo '{"path": "drawing.qet"}' | python3 qet_mcp.py --call qet_check -
```

Pass long arguments, such as a large `qet_edit` operation list, on stdin
with `-`: Windows refuses a command line over 32,767 characters
(`WinError 206`).

It prints the tool's JSON result and exits 0, or 1 if the tool reported an
error, or 2 if the call itself was malformed. The workspace rule applies as
it does in a server. The sandbox has no QElectroTech in it, so only the
tools that read files work there: `qet_project_info`, `qet_elements`,
`qet_conductors`, `qet_items`, `qet_diff`, `qet_scan`,
`qet_element_info`, `qet_element_search` and `qet_element_build`.

## Some tools need scripting switched on

A QElectroTech with JavaScript scripting switched off refuses `--run`, and
off is the default from
[#984](https://github.com/qelectrotech/qelectrotech-source-mirror/pull/984)
onwards. These tools drive it that way, or store a script that runs when
clicked, and stop working until it is turned on:

| | |
|---|---|
| need `QET_ENABLE_SCRIPTING=1` | `qet_query`, `qet_continuity`, `qet_check`, `qet_project_new`, `qet_edit`, `qet_script_api`, `qet_script_test`, `qet_script_install`, `qet_script_remove` |
| unaffected | everything else — they read the `.qet` directly, or, in `qet_export`'s case, use a plain CLI flag |

The variable goes in the environment this server is started in, which for an
MCP client is the `env` block above; the server passes its environment
straight through to QElectroTech. It does not set the variable itself, on
purpose — a switch a program turns on for itself is not a switch. Whoever
configured this server and pointed it at a QElectroTech binary made that
choice, and their interactive QElectroTech keeps whatever its own setting
says.

Without it, those come back `"ok": false` with a `hint` naming the
variable. Older builds, from before the setting existed, need nothing.

## What the server is allowed to touch

Every path in a tool call is chosen by the model, so without a policy this
server would be a read/write primitive for anything the operating system
lets the process reach: read any project on the disk, export one somewhere
else, overwrite an unrelated file, embed an arbitrary local image or PDF.

So **data paths are confined to a workspace**:

| | |
|---|---|
| `QET_MCP_WORKSPACE` | the directories tool calls may read and write, separated by `:` (`;` on Windows) |
| unset | the directory the server was started in |
| `QET_MCP_ALLOW_ANY_PATH=1` | turns the check off entirely |

Set the workspace to the folder your drawings live in. A path outside it is
refused with an error naming what was allowed; symlinks are resolved first,
so a link planted inside the workspace is judged by where it points.

**The client does not choose what program runs.** The tools that launch
QElectroTech use the one the server found (`QET_BINARY`, the install it
ships in, or `PATH`). A call may still name `binary`, but only as that same
file or one listed by whoever configured the server:

| | |
|---|---|
| `QET_BINARY` | the QElectroTech to launch |
| `QET_MCP_BINARIES` | other executables a call may name, separated like `QET_MCP_WORKSPACE` (for comparing two builds) |
| `QET_MCP_ALLOW_ANY_BINARY=1` | turns the check off: a call can then run any program |
| `QET_MCP_ELEMENTS` | element collections a call may name as `elements_dir` besides the workspace and the installed one |

Anything else is refused, even a file inside the workspace: being there
makes it readable, not runnable. Before this rule any executable a call
named was run, with the call's own paths as arguments, so text inside a
project could steer an assistant into starting another program.

`QET_MCP_ALLOW_ANY_PATH=1` is equivalent to granting the client local
filesystem access with this process's privileges. It exists so that is a
deliberate choice rather than the default.

**Nothing is overwritten unasked.** `qet_export`, `qet_edit`,
`qet_project_new` and `qet_element_build` refuse an `output` that already
exists unless the call passes `"overwrite": true`. Replacing a file is the
one step this server cannot undo, so it is the one step it will not take on
its own.

The confinement is applied where tool arguments enter the server, not inside
each tool. Importing `qet_mcp` and calling `tool_export()` from your own
Python is not confined and is not meant to be — that is your code calling a
library, and you already chose the paths.

## What QElectroTech tells the server: `qet-assistant.json`

Each time an editor window opens, and whenever its stored scripts,
settings or live channel change, QElectroTech writes `qet-assistant.json`
in its standard data folder (`~/.local/share/QElectroTech/QElectroTech/`
on Linux, `%APPDATA%\QElectroTech\QElectroTech\` on Windows). It names
every folder actually in use, even when QElectroTech was started with
`--data-dir`, which features are on, every call a script can make, and the
stored scripts. The server reads it instead of guessing; `qet_about` shows
it. Set `QET_MCP_INFO_FILE` to read it from somewhere else.

The server also sends the assistant a short note at first contact: the two
ways of working (files, or live), the usual order of tools, and to start
with `qet_about`.

## Script buttons

QElectroTech turns every `.js` file in its scripts folder that starts with a
`// ==QETScript==` header into a command with an icon: in Projet > Scripts,
on the Scripts toolbar, in command search and in the shortcut bar. A person
can write that file by hand; an assistant uses the tools above. Both end
with the same file, and an open QElectroTech picks it up without a restart.

```js
// ==QETScript==
// @name     Add revision note
// @icon     add-revision-note.svg
// @tooltip  Puts a "Rev A" note on the folio on screen
// @shortcut Ctrl+Alt+R
// @context  canvas
// ==/QETScript==
qet.addText(qet.currentFolio(), "Rev A", 40, 40);
```

The usual round: `qet_script_api` for the calls, `qet_script_test` on a
project until the diff is what was wanted, then `qet_script_install` with
`test_project` set, so a script that fails is not stored. The assistant
never presses the button: the user does, and one Ctrl+Z undoes the run.

| | |
|---|---|
| folder | QElectroTech's data folder + `/scripts`: `~/.local/share/QElectroTech/QElectroTech/scripts` on Linux, `%APPDATA%\QElectroTech\QElectroTech\scripts` on Windows, `~/Library/Application Support/QElectroTech/QElectroTech/scripts` on macOS |
| `QET_MCP_SCRIPTS_DIR` | another folder, for a QElectroTech started with `--data-dir` |

The folder is chosen by the server, never by a call, and a script's id
becomes its file name only if it is `a-z`, `0-9`, `-` and `_`. Storing or
removing a script needs `QET_ENABLE_SCRIPTING=1` like an edit does: a
stored script runs with the user's rights when they click it.

## Macro recordings: from something done by hand to a button

In QElectroTech, Projet > Scripts > Enregistrer une macro records what you
do on a project until you click it again. It saves the project before and
after, and each step from the undo history with the folio after it. At Stop
it offers to copy a ready-made request; paste that into the assistant.

| | |
|---|---|
| `qet_recording_list` | the recordings, newest first |
| `qet_recording_read` | one recording: each step as structured changes, and the overall change |
| `qet_recording_check` | run a script on the "before" project, from the same folio and selection, and say whether the result **matches** the "after" project, or what differs |
| `qet_recording_remove` | delete one |

The usual round: read the recording, write a script that does the same in
general (on the selected elements, say, not on these exact ones),
`qet_recording_check` it until it matches, then `qet_script_install` it.

## Live mode: working in the QElectroTech you have open

Every tool above works on files, with no QElectroTech window involved. The
three `qet_live_*` tools instead act on the project open in **your**
QElectroTech, in front of you, so you can watch, stop or undo:

| | |
|---|---|
| `qet_live_status` | what is on screen: project, folio, selection, last undo step, stored scripts |
| `qet_live_run_script` | run script text on the open project: one undo step named "Assistant : …" |
| `qet_live_run_stored` | press a stored script's button |
| `qet_live_command` | an editor command from an allow-list that opens no dialog: selection, zoom, rotate, snap, group, reset wires |
| `qet_live_show_folio` | show another folio |
| `qet_live_undo_last` | undo the newest step, only if the assistant made it |
| `qet_live_screenshot` | a picture of the folio on screen, as an MCP image |

A script the assistant writes on the spot is shown to you first, with
*Exécuter*, *Refuser* or *Toujours pour cette session*; the Assistant
panel lists everything it did.

QElectroTech only listens when three things are true:

1. the server has `QET_ENABLE_SCRIPTING=1`, as for editing;
2. in QElectroTech, Configurer > Général > "Autoriser un assistant IA à agir
   sur le projet ouvert" is ticked (off by default);
3. at this start, you answered *Continuer* to the warning QElectroTech shows
   every time it starts with that setting on.

While it listens, the status bar says so and shows the assistant's last
action, with an *Arrêter* button that closes the channel for the rest of
the session. Each action is one Ctrl+Z. A script's `qet.showMessage()` is
logged instead of opening a box nobody asked for.

The channel is a local socket only your user can open. QElectroTech puts
its name and a random token in the `live` part of `qet-assistant.json`,
and clears it when the channel closes; `qet_about` says whether one is
open but never shows the token.

## Worked examples

**What did that edit change?**

```json
{"name": "qet_diff", "arguments": {"before": "a.qet", "after": "b.qet"}}
```

```json
"elements": { "moved_count": 4,
              "distinct_move_deltas": [[0.0, -80.0]],
              "relabelled": [], "info_changed": [] }
```

Four elements moved by one uniform delta; nothing was relabelled. That is
the answer a screenshot gave wrongly.

**Draw something, and check it landed**

```json
{"name": "qet_edit", "arguments": {
  "project": "in.qet", "output": "out.qet",
  "elements_dir": "/path/to/qelectrotech/elements",
  "operations": [
    {"op": "add_folio", "id": "f"},
    {"op": "set_folio_title", "folio": "$f", "title": "Starter"},
    {"op": "add_element", "id": "k1", "folio": "$f", "path": "common://.../coil.elmt", "x": 100, "y": 100},
    {"op": "add_element", "id": "k2", "folio": "$f", "path": "common://.../coil.elmt", "x": 320, "y": 100},
    {"op": "add_conductor", "folio": "$f", "from": "$k1", "from_terminal": 0, "to": "$k2", "to_terminal": 0},
    {"op": "set_conductor", "folio": "$f", "element": "$k1", "terminal": 0, "property": "num", "value": "W7"},
    {"op": "set_label", "folio": "$f", "element": "$k1", "label": "KM1"}
  ]}}
```

An op that creates something takes an `"id"`; later ops name it as `"$id"`.
A `"folio"` given as a number counts **from 0**, while `qet_elements` and
`qet_project_info` number folios from 1 as the application does: the folio
they call 1 is `"folio": 0` here. An op that fails because of this says which
index to use. Terminals are addressed by index — top to bottom, then left to right, **not**
the order the `.elmt` lists them. `qet_element_info` and `qet_element_search`
both report that index order. The answer carries a per-operation result
*and* a `qet_diff`, because "addConductor → true" says the call was
accepted, not that the file came out right:

```json
"diff": {"elements":   {"before": 11, "after": 13, "added": ["{0aa3…}", "{6f63…}"]},
         "conductors": {"before": 47, "after": 48, "added": ["4:{0aa3…}/{2904…}--{6f63…}/{2904…}"],
                        "removed": []}}
```

**Draw a symbol that does not exist yet**

```json
{"name": "qet_element_build", "arguments": {
  "output": "/path/to/collection/99_custom/my_resistor.elmt",
  "names": {"en": "Test resistor", "fr": "Résistance de test"},
  "parts": [
    {"type": "rect", "x": -10, "y": -20, "width": 20, "height": 40},
    {"type": "line", "x1": 0, "y1": -30, "x2": 0, "y2": -20},
    {"type": "line", "x1": 0, "y1": 20,  "x2": 0, "y2": 30},
    {"type": "text", "x": 14, "y": -4, "text": "R"}
  ],
  "terminals": [{"x": 0, "y": -30, "orientation": "n", "name": "1"},
                {"x": 0, "y": 30,  "orientation": "s", "name": "2"}]}}
```

Then place it with `qet_edit` like any catalogue element. Unlike a
project, a `.elmt` is not rewritten by QElectroTech on a round trip, so
generating one here is safe in a way that generating a `.qet` would not
be — there is no `toXml()` waiting to drop what this writer did not know
to emit.

**Ask a question the XML cannot answer**

```json
{"name": "qet_query", "arguments": {
  "project": "industrial.qet",
  "sql": "SELECT label, COUNT(*) AS n FROM element_nomenclature_view WHERE label <> '' GROUP BY label HAVING n > 1 ORDER BY n DESC"}}
```

```json
"rows": [{"label": "V6", "n": 7}, {"label": "V5", "n": 6}, {"label": "V4", "n": 6}]
```

Duplicate element labels in a shipped example — a design-rule question,
answered by the database that already knew it.

**How much of a corpus uses a field?**

```json
{"name": "qet_scan",
 "arguments": {"directory": "examples", "tag": "conductor", "attribute": "cable"}}
```

```json
{ "files": 24, "total": 3190, "non_empty": 0, "distinct_values": [] }
```

Across the shipped examples: 3190 conductors, not one with a cable value.

## Testing

```bash
python3 test_qet_mcp.py                      # unit + protocol, no QElectroTech needed
QET_BINARY=/path/to/qelectrotech \
QET_ELEMENTS=/path/to/qelectrotech/elements \
QET_EXAMPLES=/path/to/qelectrotech/examples \
QET_ENABLE_SCRIPTING=1 \
    python3 test_qet_mcp.py                  # everything
```

`QET_ENABLE_SCRIPTING=1` matters from #984 onwards: without it the
integration tests that drive QElectroTech through a script all fail, and
they fail as "the edit did nothing" rather than as "scripting is off", which
reads like a regression in the thing under test.

176 tests in three layers: unit (validation, script generation, the terminal
order rule, the diff, the part schema), the real stdio transport, and
integration against a built QElectroTech. Several exist because the
behaviour they pin was once wrong and looked right, and say so in their
docstrings. To check the suite itself rather than trust it, each of those
bugs was reintroduced in turn and the suite confirmed to fail: ten in the
Python, plus the hang guard on `addConductor` and the database refresh in
`ConductorCreator` in the C++.

## Notes and limits

- **Two ways of numbering folios.** Tools that read the file —
  `qet_project_info`, `qet_elements`, `qet_conductors`, `qet_diff` — number
  folios from 1, as the application does. Tools that pass a folio to
  QElectroTech's scripting API — `qet_edit` and `qet_continuity` — take an
  index counted from 0, so the folio `qet_elements` calls 1 is `0` there.
  `qet_continuity` refuses an index with no folio instead of reporting it
  clean, and each of its findings carries both `folio` (the index) and
  `folio_number` (counted from 1).
- **The project database is reachable now, through `qet_query`.** It was
  not when this server was written, which is why every other structural
  tool here re-derives its answer from the XML. Prefer the views —
  `element_nomenclature_view`, `project_summary_view`, `wiring_list_view`
  — which exist to be queried; the underlying tables are how the cache is
  arranged today and a column may move. Call `qet_query` with no `sql` to
  list both. Only `SELECT` and `WITH` are accepted, which is the rule
  QElectroTech applies to its own custom-query box, not one invented here.
  An empty result and a failed query are told apart: `row_count` 0 with no
  `error` means nothing matched, and a typo'd column name says so.
- **Texts, shapes and images are in the database too**, one row each in
  `drawing_item_view` (`uuid`, `kind`, `folio`, position and size, and a
  `description`: the shape type or the text). The uuid is the one saved in
  the file, and every `qet_edit` op that takes a text, shape or image
  `index` also takes that uuid, which does not shift the way an index
  does. `qet_element_build` gives every part of a symbol a uuid as well,
  returned in `part_uuids`; `qet_element_info` lists them in `part_list`.
- **A folio can be named by its uuid** wherever an op takes `folio` or
  `to_folio`; `qet_project_info` lists each folio's. It still names the same
  folio after an earlier op in the run adds, inserts or removes one, where
  an index would shift. A folio saved without a uuid shows it empty:
  QElectroTech gives it one on load and writes it on the next save, so it
  appears after a first `qet_edit`. Needs `qet.folioIndex()` in the build.
  The `"$id"` of an `add_folio` or `insert_folio` works the same way: it
  keeps naming that folio after a later `insert_folio` or `remove_folio` in
  the same run (on a build without `qet.folioUuid()`, it is the index the
  folio had when it was made, as before).
- **A conductor can be named by its uuid** (`qet_conductors` reports it):
  `set_conductor`, `move_conductor_segment` and `delete_conductor` take
  `"conductor": "{uuid}"` in place of `element` + `terminal`, which works
  where two conductors meet at a terminal. It is turned at run time into an
  end whose terminal carries only that conductor; where both of its ends
  are shared the op fails and its `note` says why. Needs
  `qet.conductorEnds()` in the build. A uuid names one wire, but
  `set_conductor` still changes the whole potential, the same as by
  terminal. A project saved before conductors carried a uuid has none in
  the file until it is saved once: since #1107 QElectroTech works one out
  from the wire's two ends on load and writes it on the next save, so it
  appears after a first `qet_edit`.
- **A terminal can be named by its uuid**: `terminal`, `from_terminal` and
  `to_terminal` take the terminal's uuid (as `qet_element_info` lists it)
  in place of its index, on the op's own element (for `add_conductor`, on
  that end's element). It is turned at run time into the index the call
  takes; if the element has no terminal with it the op fails and its
  `note` says so. Unlike the index, which is a sort by position, it is
  defined between two terminals at the same point. Needs
  `qet.terminalIndex()` in the build. A symbol file saved without terminal
  uuids lists them empty; QElectroTech gives the terminals of every
  project's copy of it a uuid on opening (#1118), written on the next save.
- **`qet_export` isolates its launch.** SingleApplication keys its socket
  on `applicationFilePath()`, so a second launch of the same binary path
  forwards its request to an already-running instance and returns *that*
  process's answer with no error. The tool copies the binary to a unique
  temporary path, gives it a private `HOME`, and runs it on the offscreen
  platform. A symlink would not work: `applicationFilePath()` resolves it
  back to the real path.
- **The CLI matches its flags exactly.** `--export-bom out.csv` is the
  supported form; `--export-bom=out.csv` is not recognised as an export
  at all, so the application starts its interface instead and a headless
  run hangs. The tool uses the positional form.
- **Conductor identity is the hard part of `qet_diff`.** A conductor names
  its ends with `terminal1`/`terminal2`, and the project format has two
  schemes: folio-scoped integer ids in older files, terminal-definition
  uuids plus `element1`/`element2` in newer ones. The integer ids are
  **renumbered on every save**, so keying on them — which this tool did at
  first — made all 47 conductors of an untouched folio read as removed and
  re-added the moment the other side had been through QElectroTech, which
  is exactly what `qet_edit` produces. They are now keyed by owning element
  uuid plus terminal, which is stable across a save: measured at 0 colliding
  keys over 3190 conductors in the 24 shipped examples, and 0 churn on a
  no-op edit. Where an element predates persisted uuids the end cannot be
  resolved and keeps a `#`-marked unstable key; the diff then reports
  `unstable_keys` and says so rather than pretending to be comparable.
- **Texts, shapes and images are keyed by uuid** when every one on both sides
  has one, so an edit or a move reads as a change to that item. A file saved
  before they carried a uuid has none; for such a pair (including a legacy
  file against its first re-save) that kind falls back to position, where an
  edited text reads as removed plus added and a move as a removal plus an
  addition. Each section says which it used in `keyed_by`. The folio `version` attribute is left out of the
  comparison on purpose: QElectroTech rewrites it on every save, and
  including it made every folio of any re-saved project look edited.
- **Elements** written before persisted uuids fall back to a positional key,
  which makes a move in such a file read as a remove plus an add.
- **`qet_edit` needs a build whose scripting API carries the drawing verbs.**
  Against an older one it reports exactly which methods are missing and
  changes nothing. `addElement` and the move/delete verbs shipped with the
  scripting API; `addConductor`, `rotateElement`, `setElementLabel`,
  `setElementInfo` and `setFolioTitle` are newer.
- **`elements_dir` is not optional for `common://` paths** unless the
  server is installed with QElectroTech, which fills it in. The sandboxed
  run has its own empty HOME, so QElectroTech falls back to the compiled-in
  collection path, which on a machine that never ran `make install` does not
  exist. The only symptom is `addElement` reporting that a file plainly
  present "does not resolve to an element". An absolute `.elmt` path works
  without it. On Windows and macOS, `elements_dir` needs a QElectroTech that
  reads `QET_SETTINGS_DIR` (#1178): an older one keeps its settings in the
  registry or the system preferences, never sees the path written for the
  run, and uses the collection it was installed with.
- **`set_conductor` changes the whole potential, not one segment.** That is
  what the application does — a wire number describes a potential — so name
  a terminal carrying exactly one conductor and the change reaches every
  conductor electrically joined to it. A terminal several conductors meet
  at names none of them and is refused, so address a potential from one of
  its leaves. Property names are the file's own, so `qet_conductors` reads
  back exactly what was set.
- **`link_elements` takes a folio for each end**, because a master and its
  slave are normally on different folios. Whether a pair may be linked is
  decided by QElectroTech's own `isLinkable()`, so a script cannot make a
  link the GUI would refuse.
- **An element must live inside a collection to be placeable.** This is
  not about the path syntax: an absolute `.elmt` path works, but only if
  the file sits under a directory QElectroTech knows as a collection.
  Write it under the tree you pass as `elements_dir` and `qet_edit` can
  place it, by absolute path or as `common://…`; write it anywhere else
  and `add_element` reports only "does not resolve to an element".
- **`qet_element_build` computes the `.elmt` size header, and checks it.**
  `width`/`height`/`hotspot_x`/`hotspot_y` relate to the drawing by a
  containment constraint, not a formula — the declared box runs from
  `(-hotspot_x, -hotspot_y)` to `(width - hotspot_x, height - hotspot_y)`
  and the drawing must fit inside it. The shipped collection shows authors
  picking their own margins (one element pads 2 units left and 3 right,
  another 8 and 2), so there is no convention to copy, only an invariant
  to satisfy. A drawing that escaped its box is the classic way a
  hand-written element renders clipped in the collection panel while
  looking fine in XML.
- **QElectroTech interrupts a script at 30 s** of its own accord, separately
  from this tool's `timeout`. A very long operation list will hit that
  first.
- **`qet_edit` never writes the input.** It saves to a separate file and
  diffs the two, so the original is always the thing the diff is against.
