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
