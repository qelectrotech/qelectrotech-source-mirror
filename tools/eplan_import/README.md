# EPLAN project import (.epdz -> QElectroTech)

Turns an EPLAN export archive (`.epdz`, the archive EPLAN writes for EPLAN eVIEW / eBUILD) into an
editable QElectroTech project: one folio per page, every EPLAN symbol as a part with its terminals,
conductors where EPLAN drew them, interruption points as linked report arrows, coils and their
contacts as linked masters and slaves, and the page frame and the other graphics as folio graphics.

In QElectroTech: **File > Import EPLAN project (.epdz)…**. Without Python 3, or in a build without
scripting, the dialog still imports the archive, with each page as a picture (`EpdzImport`).

## Layout

- `eplan_zw1/epdz_nach_qet.py` - the import program (reading the archive, translating it).
- `eplan_zw1/eplan_grafik.py` - EPLAN drawing primitives -> QElectroTech element files and folio graphics.
- `eplan_zw1/qet_plan.py` - a project as a list of edit operations, made by a headless QElectroTech.
- `eplan_zw1/qet_backend.py` - finds QElectroTech, `misc/qet-mcp/qet_mcp.py` and 7-Zip.
- `eplan_zw1/tests/` - tests on an export archive the test builds itself.

## Running it

```
python3 eplan_zw1/epdz_nach_qet.py <export.epdz> [target.qet] [--unpacked DIR] [--pdf] [--no-collection]
python3 eplan_zw1/epdz_nach_qet.py --check
```

The program writes no `.qet` by hand: it lets QElectroTech carry out a list of edit operations through
the scripting API (`qet_mcp.tool_project_new` / `tool_edit`), then adds the folio graphics to the saved
file. It prints `PROGRESS <percent> <step>` for each step and `done: <project>` at the end.

Beside the project it writes `<name>_devices.csv`, `<name>_connections.csv`, the folder `<name>_3D` with
the `.e3d` installation spaces and, in `_epdz_import_<name>/import_report.txt`, what was translated.

Requirements: Python 3.9 or later (standard library only), a QElectroTech built with scripting. Optional:
fontTools and the Tahoma font for exact text widths, PyMuPDF for pictures that are not PNG, 7-Zip when the
program unpacks the archive itself (QElectroTech's dialog unpacks it and passes `--unpacked`).

| Variable | Meaning |
|---|---|
| `QET_BINARY` | the QElectroTech to run (the dialog sets it) |
| `QET_ENABLE_SCRIPTING` | `1`: the headless runs need scripting (the dialog sets it for them) |
| `QET_CUSTOM_ELEMENTS_DIR` | the user collection; the parts made from the EPLAN symbols go to `EPLAN_import/EPDZ` in it, or with `--no-collection` into the work folder only |
| `QET_MCP` | `qet_mcp.py`, when it is neither beside the install nor in the source tree |
| `SEVENZIP` | a 7-Zip program, when the archive is not unpacked already |
| `EPLAN_FONT` | the font of the texts (default Tahoma) |

## Tests

```
python3 tools/eplan_import/eplan_zw1/tests/test_epdz_nach_qet.py
```

The import tests run when `QET_BINARY` names a QElectroTech built with scripting; ctest
(`tst_epdzimportpython`) sets it.
