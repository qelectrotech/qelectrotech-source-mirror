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
| `qet_project_info` | title, format version, folios, element and conductor counts |
| `qet_elements` | placed elements: uuid, type, position, label, information bag |
| `qet_conductors` | conductors and their documentation fields; filter by attribute |
| `qet_diff` | **what an edit actually changed** — element moves, adds, removes, relabels; conductor changes; and folio fields, texts, shapes, images, symbol text fields and terminal strips |
| `qet_scan` | sweep a directory of projects, counting nodes carrying an attribute |
| `qet_element_info` | a `.elmt`: translated names, terminals, info fields, part counts |
| `qet_export` | run a headless export (pdf, png, svg, bom, cables, wires, wiring, nets, links, info) |
| `qet_edit` | **change a project** — place, move, rotate, label, wire, number, cross-reference, add text, shapes and images, restyle a symbol's text fields, delete; then diff the result |
| `qet_element_build` | **author a `.elmt`** — draw a new symbol, with terminals to wire it by |
| `qet_project_new` | **start from nothing** — an empty project with a title and folios |
| `qet_element_search` | **find a symbol** in a collection by name (any language), type or terminal count |
| `qet_check` | **design-rule checks** — duplicate labels, unlabelled masters, unnumbered conductors, empty folios |
| `qet_query` | **ask the project database** — read-only SQL over the views and tables |

`qet_export` and `qet_edit` launch QElectroTech. Everything else parses the
file directly, which is faster, needs no display, and cannot be confused by
a dialog.

## Running it

```bash
# list the tools and exit
misc/qet-mcp/qet_mcp.py --list

# speak MCP on stdin/stdout
misc/qet-mcp/qet_mcp.py
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
        "QET_ENABLE_SCRIPTING": "1"
      }
    }
  }
}
```

## Five tools need scripting switched on

A QElectroTech with JavaScript scripting switched off refuses `--run`, and
off is the default from
[#984](https://github.com/qelectrotech/qelectrotech-source-mirror/pull/984)
onwards. Five tools here drive it that way and stop working until it is
turned on:

| | |
|---|---|
| need `QET_ENABLE_SCRIPTING=1` | `qet_query`, `qet_continuity`, `qet_check`, `qet_project_new`, `qet_edit` |
| unaffected | everything else — they read the `.qet` directly, or, in `qet_export`'s case, use a plain CLI flag |

The variable goes in the environment this server is started in, which for an
MCP client is the `env` block above; the server passes its environment
straight through to QElectroTech. It does not set the variable itself, on
purpose — a switch a program turns on for itself is not a switch. Whoever
configured this server and pointed it at a QElectroTech binary made that
choice, and their interactive QElectroTech keeps whatever its own setting
says.

Without it, those five come back `"ok": false` with a `hint` naming the
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

Two arguments are deliberately **not** confined: `binary` (the
`qelectrotech` executable) and `elements_dir` (the element collection).
Those are configuration, chosen once by whoever runs the server, and both
normally live in `/usr` or a build tree — outside any sensible workspace.
Confining them would reject the ordinary case while stopping nothing.

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
  "binary": "/path/to/qelectrotech",
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
Terminals are addressed by index — top to bottom, then left to right, **not**
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
  "binary": "/path/to/qelectrotech", "project": "industrial.qet",
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
- **Texts, shapes and images have no uuid**, so `qet_diff` cannot say "the same
  text, edited": an edited text reads as the old one removed and a new one
  added, both shown. Shapes and images are keyed by position, so a restyle
  or rescale *is* reported as a change to that item, but a move reads as a
  removal plus an addition. The folio `version` attribute is left out of the
  comparison on purpose: QElectroTech rewrites it on every save, and
  including it made every folio of any re-saved project look edited.
- **Elements** written before persisted uuids fall back to a positional key,
  which makes a move in such a file read as a remove plus an add.
- **`qet_edit` needs a build whose scripting API carries the drawing verbs.**
  Against an older one it reports exactly which methods are missing and
  changes nothing. `addElement` and the move/delete verbs shipped with the
  scripting API; `addConductor`, `rotateElement`, `setElementLabel`,
  `setElementInfo` and `setFolioTitle` are newer.
- **`elements_dir` is not optional for `common://` paths.** The sandboxed
  run has its own empty HOME, so QElectroTech falls back to the compiled-in
  collection path, which on a machine that never ran `make install` does not
  exist. The only symptom is `addElement` reporting that a file plainly
  present "does not resolve to an element". An absolute `.elmt` path works
  without it.
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
