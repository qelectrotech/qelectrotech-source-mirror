# EPLAN `.edz` part import

Imports a part from the **EPLAN Data Portal** (`.edz`) into a QElectroTech
element collection. The EPLAN Data Portal is the de-facto parts catalogue for
electrical CAD (ifm, Phoenix Contact, Siemens, Rittal, Weidmüller, Schneider…),
and components ship as `.edz`; this lets a QET user drop one straight into a
project.

## Using it

Right-click a writable collection folder in the elements panel →
**Import an EPLAN part (.edz)…** → choose the file. The generated element is
written into that folder and the panel reloads.

## What is imported

A `.edz` is a 7-Zip archive containing a part definition (`*.part.xml`), an EPLAN
macro (`*.ema`), a product image and metadata. Only the **portable** part data is
used:

- **Terminals** — each `<functiontemplate>` with a physical connection
  designation becomes a QET terminal, labelled from its connection description.
- **Element information** — manufacturer, order number, designation and comment
  fill the QET element-information fields used by the BOM / nomenclature.
- **Localized names** — one `<name lang="xx">` per language the part provides, so
  the element labels itself in any QET UI language.
- **Symbol** — a generic symbol is generated (a body rectangle with one
  west-facing terminal per pin). The EPLAN `.ema` macro geometry is **not**
  reproduced: it is tokenised EPLAN PXF that references EPLAN's own symbol
  libraries. A generic symbol is sufficient for wiring and BOM; swap in a nicer
  symbol in the element editor afterwards if desired.

## Structure

| File | Role |
|------|------|
| `edzarchive.*` | Extract the `.edz` to a temp dir; locate `part.xml` |
| `edzsevenzip.*` | 7-Zip extraction via the bundled LZMA SDK (no external 7-Zip) |
| `edzpart.*` | Parse `part.xml` into a portable model |
| `edzelementbuilder.*` | Build the `.elmt` element from the model |
| `edzimporter.*` | Orchestrate the above and write into a collection folder |
| `lzma/` | Vendored public-domain LZMA SDK (decode-only subset) |

The non-UI classes are deliberately decoupled from the widget so they can be
tested headless.

## Terms of Use / Legal notice

The `.edz` files themselves are downloaded from the **EPLAN Data Portal**
(data.eplan.com), which is operated by EPLAN Software & Service GmbH & Co. KG.
The Data Portal Terms of Use (§ 5.4 at time of writing) restrict use of
downloaded data to EPLAN products.

**QElectroTech does not endorse or encourage violation of those terms.**
Whether use of an `.edz` outside EPLAN products is permitted under your
specific licence depends on the agreement you have with EPLAN and the
individual manufacturer. You are solely responsible for ensuring that your use
of `.edz` data complies with the applicable Terms of Use and any applicable
law. If in doubt, contact EPLAN or the part manufacturer directly.

QET's `.edz` reader parses only the portable, factual part data
(pin designations, order numbers, manufacturer names). It does not reproduce
any EPLAN-proprietary geometry or symbol data.

## Bundled LZMA SDK

`lzma/` contains the decode-only subset of Igor Pavlov's **LZMA SDK** (public
domain — see `lzma/LZMA-SDK-LICENSE.txt`), so a `.edz` can be unpacked without an
external 7-Zip install. Only 7-Zip-format `.edz` files are supported; zip-format
packages are detected and reported but not yet handled.

## Manufacturer macro packages (`sources/import/macropackage/`)

Some manufacturers are not in the EPLAN Data Portal and offer all their macros as one download instead.
Beckhoff: any product page on beckhoff.com > Documentation and downloads > Macros > "EPLAN macros from 2.7"
(ZIP, 66 MB, myBeckhoff login). The package is not part of this repository. What was found in it (package of
30 Oct 2026, checked with throw-away scripts; counts in brackets):

- **Layout**: `Beckhoff/<area>/<product family>/<name>.ema`, e.g.
  `Beckhoff/IO/EtherCAT Coupler BECKHOFF EKxxxx/EK9300.ema` (1923 macros in the areas Embedded PC, IO, IPC,
  Motion, MX-System, Vision).
- **Macros** are the window macros an `.edz` carries too: XML with the root `EplanPxfRoot`
  (`Type="WindowMacro"`, all 1923), one `MacroVariant` per representation type and variant. `MacroFuncType`
  is 1 = multi-line (8466 variants; the descriptions say "allpolige Darstellung"), 2 = single-line (4249,
  "einpolige Darstellung"), 4 = overview (3263, "Übersichtsmakro"), 8 (2); there is **no** panel layout
  (6) in this package. A PLC card has one multi-line variant per channel, often one more with all channels,
  and the overview (EL3064: A–D = channels 1–4, E = all, overview).
- **Objects of a variant**: functions `O17` (black box with frame `S89x5`; connection point with designation
  `A185`, description `A186`, function definition `A1381/A1382/A1383`, channel `P20407`), free lines `O31`,
  arcs `O32`, texts `O30`, polylines `O34`, rectangles `O89`, and graphics `O5` (sub-objects `S31x5` line,
  `S32x5` arc, `S34x5` polyline, `S89x5` rectangle, `S30x5` text, `S144x5` a further graphic) placed by
  `O144` (`R9` = the graphic). A placement is `S40x1201` (`A762` position) with `S39x761` (`A751..A754`, the
  2x2 matrix column by column; scale and quarter turns occur). In a polyline an empty point lifts the pen.
- **Article data** is EPLAN parts management XML, the format of the `part.xml` of an `.edz`: root
  `partsmanagement type="EPLAN.PartsManagement"`, one `<part>` per article with `P_ARTICLE_PARTNR`
  ("BEC.EK9300"), `P_ARTICLE_ORDERNR`, `P_ARTICLE_TYPENR`, `P_ARTICLE_DESCR1` ("de_DE@…;en_US@…;"),
  `P_ARTICLE_MANUFACTURER` ("BEC"), `P_ARTICLE_PRODUCTGROUP` (a number), width/height/depth, and
  `<functiontemplate>` rows (connection designation and description, `functiondefcategory/-group/-id`,
  channel `idx1`). `Beckhoff/Article_DB.xml` holds 2495 parts; beside most macros lies
  `<macro>_Article_Data.xml` with the parts that use this macro (1 to 433), in MX-System `<type>.xml`;
  168 parts are only in those. A part names its macro in
  `P_ARTICLE_GROUPSYMBOLMACRO="$(MD_MACROS)\Beckhoff\IO\…\EK9300.ema"` (2491 of 2495; all exist);
  several articles share a macro (1724 macros for 2495 parts), 199 macros are named by no article of the
  big file.

`MacroPackage` reads this as an index (2694 entries: every part once, and the macros no part names by their
file name) and keeps it in the cache folder until the ZIP changes. `EplanPartImporter::importMacroPackage()`
then builds, per article asked for, the generic element from the part data (`EdzElementBuilder`) and one
element per macro variant (`EplanMacroReader::readVariant()` + `EplanElementBuilder`), and gives a PLC card
its channel table from the function templates (300.1.x / 300.99.x: 2 DI, 3 DQ, 4 AI, 5 AQ). The collection
folder is manufacturer / product family / type; the Beckhoff folders have short names in
`macropackage.cpp` (`FAMILIES`), any other package keeps its folder names.
