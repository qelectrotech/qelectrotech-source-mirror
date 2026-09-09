# Smart Device Information and BOM CSV

## Architecture

- A `.qet` project is XML, rooted at `project`. `QETProject::toXml()` and
  `write()` serialize it; the file constructor, `readProjectXml()` and
  `readDiagramsXml()` load it (`sources/qetproject.cpp`). Diagrams are
  serialized by `Diagram::toXml()/fromXml()` (`sources/diagram.cpp`).
- `.elmt` symbol definitions are stored in collections and embedded into a
  project by `XmlElementCollection`. `Element::buildFromXml()` builds their
  graphical representation (`sources/qetgraphicsitem/element.cpp`).
- A placed component is an `Element`. `ElementData` holds its type and
  `m_informations`, a `DiagramContext` key/value map. Instance properties are
  read/written by `Element::fromXml()/toXml()` as `elementInformations` with
  `elementInformation name="..."` children.
- `ElementPropertiesWidget` hosts `ElementInfoWidget`; the latter creates
  rows from `QETInformation::elementInfoKeys()`. Edits use
  `ChangeElementInformationCommand`, including live editing and undo/redo.
- Existing reporting uses the derived SQLite `projectDataBase`,
  `element_nomenclature_view`, `ElementQueryWidget`, `BOMExportDialog` and
  `QetGraphicsTableFactory`. `cli_export.cpp` also provides `--export-bom`.
  The database is not the authoritative project storage.

## Device Properties

`sources/properties/deviceinformation.h/.cpp` provide a small value type
and mapping to the existing context. There is no parallel device store.
`toContext(existing_context)` preserves unrelated properties and visibility
flags. Callers changing a placed element should use
`ChangeElementInformationCommand` to retain undo/redo and dirty tracking.

| Device field | XML/context key or source | Origin | CSV BOM |
| --- | --- | --- | --- |
| Device tag | `label` | Existing; BOM resolves formula labels | Yes |
| Manufacturer | `manufacturer` | Existing | Yes |
| Part number | `manufacturer_reference` | Existing | Yes |
| Model | `model` | New | Yes |
| Description | `description` | Existing | Yes |
| Category | `category` | New | Yes |
| Quantity | `quantity` | Existing | Yes |
| Page | Resolved diagram folio | Derived | Yes |
| Voltage rating | `voltage_rating` | New | No; stored metadata |
| Current rating | `current_rating` | New | No; stored metadata |
| Notes | `notes` | New | Yes |

No XML version bump or migration is needed. Absent properties remain empty;
empty properties are omitted by the existing XML writer. Older keys keep
their original meaning. Generic context loading preserves unknown keys.
As before, serialization trims leading/trailing whitespace. Compatibility
with every historical QET release is not implied.

Quantity remains text, as in existing QET properties, so decimal quantities
and existing unit conventions are not silently changed. A missing quantity
stays empty in the BOM: it is not silently assumed to be one. Ratings are
text (for example `24 V DC` and `16 A`). No numeric aggregation is performed.

The five new rows are automatically available in the existing information
tab for normal/master symbols and in the symbol definition editor. No new
panel is required. Notes use the existing single-line property editor;
programmatically loaded multiline values are supported by XML and CSV.
New keys are also exposed in the existing SQL nomenclature view.

## BOM Policy

`sources/devicebomexport.h/.cpp` collect current in-memory project data,
including unsaved property changes, without a SQL cache dependency.

- One row per Simple or Master element.
- Slave symbols, terminals, page reports, thumbnails and conductor definitions
  are excluded; a master is counted once, not once per contact representation.
  Even unlinked slave symbols are excluded, matching this device-level policy.
- The existing `exclude_from_bom` property is respected.
- Empty tags and part numbers do not discard otherwise eligible elements.
- Rows follow project page order, then device tag, then UUID for stable ties.
  `Page` is the resolved folio label shown by QET; an empty label falls back
  to the one-based project page position.
- Identical part numbers remain separate rows. Grouping is deferred because
  conflicting manufacturers, ratings, tags and quantities need explicit rules.
- Auxiliary accessories are not expanded into extra rows in this MVP.
- A PLC master can be listed as one physical Master device, but its embedded
  PLC I/O entries are never expanded into BOM rows.

The Project menu action **Export BOM as CSV** exports fixed columns:
`No, Device tag, Manufacturer, Part number, Model, Description, Category,
Quantity, Page, Notes`. The legacy configurable export and CLI export retain
their previous behavior. Voltage and current ratings deliberately remain
device metadata in this MVP because they were not part of the requested BOM
schema; they can be added later without changing the stored project data.

CSV is UTF-8 with a BOM, comma-delimited, CRLF record endings, double-quoted
fields and doubled embedded quotes. Embedded newlines are retained. Values
are exported verbatim so device tags and part numbers are not altered. CSV
quoting does not disable spreadsheet formulas: only export trusted projects,
or import all columns as **text** when inspecting untrusted project data. Text
import also preserves leading zeros.
`QSaveFile` commits atomically; open, write and commit errors are reported.
Cancel does not write a file. Read-only projects can still be exported.

## Verification

On the configured Windows/MSYS2 Qt5 system, from the repository root:

```powershell
.\tests\qttest\run-smart-device-tests.ps1
```

The runner builds the application with `debug` and `no_kf5`, then links a QtTest
executable against the real application objects. Use `-SkipApplicationBuild`
only after building the current application sources. Results and generated
sample `.qet`, CSV and properties screenshot are under
`build-debug/smart-device-tests/`. CMake's explicit source list includes the
new files; qmake discovers them through its existing globs.

Tests cover absent/legacy properties, context/XML round trips, custom-key
preservation, project save/reopen, BOM filtering and ordering, duplicate part
numbers, CSV encoding/escaping, write success/failure, properties UI undo/redo,
and the existing database report view.

Manual checks:

1. Open an existing project and edit a normal or master element's information
   tab. Existing fields and the new Model/Category/ratings/Notes rows must show.
2. Enter manufacturer, part number, quantity and new properties. Apply, undo,
   redo, save, close and reopen. Verify the same values are present.
3. Give two elements the same part number on different pages. Export through
   Project > Export BOM as CSV. Expect separate rows with their own tags/pages.
4. Enable the existing exclusion checkbox for one element and export again.
   Its row must be absent; changing a property without saving must affect CSV.
5. Open CSV with a UTF-8 CSV reader (import text columns when using Excel).
   Check Turkish characters, leading-zero part numbers, commas and quotes.
6. Cancel export and try a non-writable destination. Cancellation must not
   create a file; failure must show an error without destroying an existing CSV.

Suggested review pieces: (1) device mapping and property/report registration,
(2) collector and CSV menu action, (3) tests and documentation. No cable,
PLC I/O, catalog integration or cabinet-layout feature is added.
