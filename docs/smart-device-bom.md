# Smart Device Information and BOM CSV

## Architecture

- A `.qet` project is XML. Placed component properties are held in
  `ElementData::m_informations`, a `DiagramContext`, and are serialized by the
  existing `Element::fromXml()` and `Element::toXml()` path.
- `DeviceInformation` is a typed view over that existing context. It does not
  introduce a second data store. Updates made by the properties editor continue
  to use `ChangeElementInformationCommand`, including undo/redo and dirty state.
- The project database copies element contexts into `element_info` and exposes
  reporting fields through `element_nomenclature_view`.
- GUI and CLI BOM exports both use `BomExport::defaultQuery()` against that
  view. The existing `BOMExportDialog` remains the only GUI export entry point;
  users can still customize columns, filters, and grouped/ungrouped output.

## Device Properties

| Device field | XML/context key | Default BOM |
| --- | --- | --- |
| Device tag | `label` | Yes |
| Manufacturer | `manufacturer` | Yes |
| Part number | `manufacturer_reference` | Yes |
| Model | `model` | Yes |
| Description | `description` | Yes |
| Category | `category` | Yes |
| Quantity | `quantity` | Yes |
| Voltage rating | `voltage_rating` | Yes |
| Current rating | `current_rating` | Yes |
| Page | `folio` from the diagram | Yes |
| Notes | `notes` | Yes |

The five new context keys are `model`, `category`, `voltage_rating`,
`current_rating`, and `notes`. They are available in the existing information
editor for Simple and Master elements, in symbol definitions, in the SQL
nomenclature view, and in the export column picker.

No XML version bump or migration is required. Missing keys read as empty
strings, empty properties are omitted by the existing writer, and unknown
context keys remain preserved. Quantity and ratings stay free text so existing
unit conventions and decimal quantities are not changed implicitly.

## BOM Policy

The default BOM query exports one row per Simple or Master element. Slave
contacts, terminal-strip elements, reports, thumbnails, and conductor
definitions are excluded. A Master device is counted once and PLC I/O entries
are not expanded. The existing `exclude_from_bom` property is honored by the
SQL view, including common true representations (`true`, `1`, `yes`, `on`).

The default columns are:

`Device tag, Designation, Manufacturer, Part number, Model, Description,
Category, Quantity, Voltage rating, Current rating, Page, Notes`.

The existing **Format as bill of materials** checkbox provides grouped output;
it is off by default so the initial export remains ungrouped. Users may change
the query and column order through the existing dialog. The command-line
`--export-bom` operation uses the same default query and column order.

CSV output is UTF-8 with a BOM, semicolon-delimited, and quotes every field.
Embedded quotes are doubled and embedded newlines are retained. `QSaveFile`
provides atomic replacement; query, open, write, and commit failures are
reported without deleting an existing destination first.

## Verification

The smart-device test is registered explicitly in
`tests/qttest/CMakeLists.txt` and uses only Qt Test and Qt SQL. It is therefore
part of the normal cross-platform CMake/CTest flow:

```sh
cmake -S . -B build -DPACKAGE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`tst_smart_device` checks the metadata column contract, Simple/Master filtering,
stable page ordering, UTF-8 with Latin and non-Latin samples, CSV escaping, and
atomic write success/failure. Existing project XML save/load tests cover the
generic `DiagramContext` persistence path used by the new keys.

Manual checks:

1. Open an existing project and edit a Simple or Master element's information.
2. Set all device fields, apply, undo, redo, save, close, and reopen.
3. Open Project > Export as CSV and verify the device columns are preselected.
4. Export with grouping both disabled and enabled.
5. Verify leading-zero part numbers, non-Latin text, quotes, and newlines in a
   UTF-8 CSV reader.

Cable management, terminal-strip planning, automatic wire numbering, PLC I/O
expansion, panel layout, and manufacturer catalog integration are out of scope.
