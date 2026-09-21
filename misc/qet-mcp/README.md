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
| `qet_diff` | **what an edit actually changed** — moves with deltas, adds, removes, relabels, conductor field changes |
| `qet_scan` | sweep a directory of projects, counting nodes carrying an attribute |
| `qet_element_info` | a `.elmt`: translated names, terminals, info fields, part counts |
| `qet_export` | run a headless export (pdf, png, svg, bom, cables, wires, wiring, nets, links, info) |

Only `qet_export` launches QElectroTech. Everything else parses the file
directly, which is faster, needs no display, and cannot be confused by a
dialog.

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
      "args": ["/path/to/qelectrotech/misc/qet-mcp/qet_mcp.py"]
    }
  }
}
```

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

**How much of a corpus uses a field?**

```json
{"name": "qet_scan",
 "arguments": {"directory": "examples", "tag": "conductor", "attribute": "cable"}}
```

```json
{ "files": 24, "total": 3190, "non_empty": 0, "distinct_values": [] }
```

Across the shipped examples: 3190 conductors, not one with a cable value.

## Notes and limits

- **The project database is not reachable from outside the application.**
  `projectDataBase::newQuery()` and `isReadOnlySelect()` are C++-internal
  and the JavaScript scripting API exposes no SQL binding, so structural
  queries here are done over the XML. A `--query` CLI verb, or a scripting
  binding, would let this server expose the guarded read-only SQL surface
  instead, and would be a better foundation.
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
  **renumbered on every save**, so keying on them made all 47 conductors of
  an untouched `ArduinoLCD.qet` read as 29 removed and 29 re-added the
  moment the other side had been through QElectroTech. Ends are now keyed
  by owning element uuid plus terminal, which is stable across a save:
  measured at 0 colliding keys over 3190 conductors in the 24 shipped
  examples, and 0 churn on a re-saved but otherwise untouched project.
  Where an element predates persisted uuids the end cannot be resolved and
  keeps a `#`-marked unstable key; the diff then reports `unstable_keys`
  and says so rather than pretending to be comparable.
- **Elements** written before persisted uuids fall back to a positional key,
  which makes a move in such a file read as a remove plus an add rather
  than as a move.
- Read-only by design. Nothing here writes to a project.
