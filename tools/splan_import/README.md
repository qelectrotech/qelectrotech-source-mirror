# sPlan import (.spl8 → QElectroTech)

Reads a schematic of sPlan 8 (Abacom) and draws it anew as a QElectroTech
project: the same devices and the same connections, but on the grid, with
straight wires and standard symbols. In QElectroTech: **File > Import sPlan
schematic (.spl8)…**, or directly:

```
python splan_to_qet.py <schematic.spl8> [<target.qet>] [--pdf] [--compare <original.pdf>]
```

Without a target the project goes to `<folder of the schematic>/QET/<name>.qet`.
Next to it the import writes `<name>_open_questions.txt` (what the schematic
does not say unambiguously), with `--pdf` a PDF, and with `--compare` a PDF
with the original page and the new sheet side by side
(`<name>_comparison.pdf`); pictures go to `<name>_images/`, the tidied
intermediate state to `<name>_tidy.qet`. Like the project itself, these
files are written anew on every import and replace older ones; the dialog
asks before it replaces an existing project.

Cross references (master/slave labels and report arrows) are written in the
form `/folio/column` (`/%F/%c`), as sPlan and EPLAN show them.

Requirements:
- Python 3.10 or newer, no packages for the import itself. Optional:
  Pillow (images pasted into the schematic), pymupdf (`--compare`).
- A QElectroTech built with scripting (the Qt Qml module, see
  `QET_HAS_SCRIPTING` in CMakeLists.txt): the import has QElectroTech make
  the project through its scripting API, driven by the MCP server
  `misc/qet-mcp/qet_mcp.py`. The dialog runs the QElectroTech it belongs
  to; from a shell, set `QET_BINARY`, `QET_COMMON_ELEMENTS_DIR` and
  `QET_CUSTOM_ELEMENTS_DIR`.

## Files

| File | Task |
|---|---|
| `splan8.py` | reader of the binary format (see `FORMAT_SPL8.md`) |
| `splan_analysis.py` | geometry → parts with terminals, nets, potential names, open questions |
| `splan_to_qet.py` | recognise the function, choose the symbol, lay the sheet out, make the project, check the layout |
| `part_mapping.py` | optional mapping of sPlan parts to elements (see below) |
| `test_splan_import.py` | unit tests; they write their own schematics |
| `../qet_sheet/` | the drawing library: grid, rails, taps, cross-reference arrows, master/slave |

Tests: `python -m unittest discover -s tools/splan_import -p "test_*.py"`.

## Rules for the new drawing

- **Symbols** (common collection): normally open/closed contact → contact
  (slave), contactor coil → coil (master), terminal → terminal (one per
  level of a multi-level terminal), circuit breaker → 3-pole circuit
  breaker, RCD → RCBO 1P+N, motor (AC/DC), NTC. Everything else (power
  supplies, PLC modules, drives, devices shown as an image) becomes a
  labelled device box with the terminals of the sPlan part, written into
  the user collection below `Imported/sPlan/`.
  sPlan's libraries are German; the function is recognised from those
  library names.
- **Layout**: potentials such as 24 VDC, 48 VDC, L1 → a rail on top; GND,
  0 V, M, N, PE → a rail at the bottom. Rails of the same name on different
  sheets become folio reports. Devices in rows in the order of the
  original (top to bottom, left to right), terminal strips closed and by
  number, every device as far as possible straight above or below its
  partner. A row that does not fit goes on a continuation sheet.
- **Wires**: a shortest tree per net; between two rows every wire gets a
  track of its own (no conductors on top of each other). Potential names
  without a rail become cross-reference arrows with the target as text.
- **Cross references**: coil ↔ contacts by the sPlan parent id, else by
  the same designator (also across sheets).
- **Nothing is lost**: texts without a part stand at the bottom of the
  sheet; sheets without a circuit (data sheets, photos) become
  documentation sheets; images next to a circuit get a sheet of their own.
- At the end QElectroTech's layout check runs and its fixes are applied.

## Mapping parts to elements (optional)

sPlan knows no articles. Which device a part is, is printed on the symbol
or only shown in a picture. Whoever imports can decide it in a mapping file
`<target>_mapping.json` (format in `part_mapping.py`); the import then
translates it the same way every time:

```
python splan_to_qet.py <schematic.spl8> <target.qet> --mapping-template   # skeleton: every device
python splan_to_qet.py <schematic.spl8> <target.qet> [--mapping <file.json>]
```

- `channels`: per sPlan terminal an element, its terminal and an address,
  e.g. one element per input of a PLC module; terminals with the same
  `group` share one element. Terminals without an entry go into a box of
  their own and into the open questions.
- `standard: true`: keep the standard symbol, set the article data only.
- The elements may come from any collection (`common://`, `custom://`,
  `company://`).
- Every article is listed with its reason in the open questions.

Without a mapping file the import uses the common collection and its own
device boxes only.
