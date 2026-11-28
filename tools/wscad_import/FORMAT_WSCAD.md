# WSCAD project (ELECTRIX / SUITE 6.x and later): the format

WSCAD does not publish the format of its projects. This description was
worked out for interoperability, to open one's own WSCAD projects in
QElectroTech: from the files of projects made with WSCAD ELECTRIX 2025
(program version 7.3, database version 6.4) and with older SUITE versions,
checked against the PDF export of the same projects. The layout of the page
files was read from the metadata of WSCAD's installed .NET assemblies
(type and method names, constants); none of WSCAD's code was run, copied or
translated. Reader: `wscad.py`; translation: `wscad_to_qet.py`; tests:
`test_wscad_import.py`, which write projects in this format themselves.

## Use

```
python wscad_to_qet.py <WSCAD project folder> [<target.qet>] [--pdf]
```

In QElectroTech: **File > Import WSCAD project…**. Without a target the
project goes to `<project folder>/QET/<folder name>.qet`; next to it
`<name>_open_questions.txt`, `<name>_devices.csv` and
`<name>_connections.csv`.

Requirements: Python 3.10 or newer with the package `cryptography` (or
`pycryptodome`) for AES; a QElectroTech built with scripting, as for the
sPlan import (`tools/splan_import/README.md`). The elements of the project
are written into the user collection below `Imported/WSCAD/`.

## Project folder

| File | Content |
|---|---|
| `Database/WSCAD6.db` | SQLite: the logical view (pages, symbols, terminals, connections, potentials, cable cores, texts) |
| `<structure>_<name>.<no>.wsELD` | a schematic page (e.g. `=A1+ST_Plan.0001.wsELD`) |
| `.wsFLD`, `.wsCBD`, `.wsTEC`, `.wsCBC`, `.wsPLC`, … | fluid, cabinet, terminal, cable, PLC pages and reports (not imported) |
| `PRJDATA6.db`, `article.db` | project properties, the articles of the project (not read) |

## Page file (.wsELD and the other .ws* pages)

```
0   4  file mark (01 02 00 00)
4   4  version (6.4.0.0)
8   4  counter
12  5 × 19 bytes section entries:
       +0 bool used, +1 index, +2 kind, +3 uint32 offset, +7 uint32 length, +11 double
```

Kind 1 = header (user, Base64), kind 2 = page, kind 5 = info (page content,
date), kind 3 = preview images. Kinds 2 and 5 are packed:

```
+0  4  version             +4  4  unpacked length
+8  1  key length (32)     +9  1  IV length (16)
+10    key, IV, 3 check bytes (key[0], key[1], IV[0])
then   AES-CBC with PKCS7 padding over a GZip stream
```

Key and IV are stored in the section itself: this is obfuscation, not
encryption with a secret. Unpacked: 4 bytes version + UTF-8 XML
`<WS><Solution><Project><File>…`.

## Page XML

The element and property names are WSCAD's own, partly German.

| Node | Meaning |
|---|---|
| `Header/PaperSize val="297 210"` | sheet size in mm |
| `Template` | frame and title block: `rect`, `polyline`, `text`, `PH` (a field with a value, frame `wd`/`hg`), `logo/image` |
| `SL` | symbol: `type="kind.subkind"`, `x`/`y` origin, `hid` = Symbol.SymbolID of the database |
| `SL/line`, `polyline`, `polygon`, `rect`, `kreis` (circle), `text`, `image` | graphics relative to the origin |
| `SL/Pin` | terminal: `pos`, `connDir` (1 up, 2 right, 4 down, 8 left), `nr`/`posNR`/`styleNR`, `nm`/`posNM`, `hideNR`/`hideNM` |
| `SL/VPin` | the text of a terminal only (PLC channel list) |
| `SL/PHL` | placeholder: `xpath="../P[@id='BMKZ']"`, `style`, `hide` (present = invisible), `wd`/`hg` |
| `SL/DataBlock params="QV,False;Funktionstext,True;…"` | lines below the designator; `True` = hidden; line spacing 1.2 × size |
| `SL/QV` | cross reference; several `P id="QV" lineid=…` one below the other |
| `SL/PinGP` | a group of terminals (contact group, PLC channel) with its own `P`, `PHL`, `PH`, `QV` |
| `CN/polyline` | a drawn wire |
| `Cableline` | a cable stroke |
| `text` (directly in File) | a free text |

Coordinates in mm, **y downwards**, two decimals.

Properties (`P id=…`): `BMKZ` designator (`prd` = shown value),
`BtName` equipment name, `ReferenzName` symbol name, `Funktionstext`
function text, `Artikel` article, `Hersteller` manufacturer,
`Leitungsname` wire name; title block: `Blattnummer` sheet number,
`Dateiinhalt` page title, `Funktion` plant, `Ort` location. Multi-language
texts are `<P><T id="DE">…</T><T id="EN">…</T></P>`.

### Symbol kinds (`SL type`, the part before the dot)

1 standard, 2 coil/main function, 3 contact, 4 terminal, 6 graphics,
7 master, 10 connection symbol (corner, T, bridge, crossing), 11 cable
shield, 12 plug, 14 arrow with a target (interruption point), 15 PLC card,
16 PLC channel, 19 socket, 20 cable core, 23 wire attribute (wire number,
potential name).

### Texts (`style`)

`size` font size in mm (default 2.5; the em in the PDF is 0.933 × size),
`winkel` angle in degrees counter-clockwise, `align` bits, calibrated on
the PDF:

| Bits | Meaning |
|---|---|
| 2 | right-aligned |
| 4 | centred (4+2 too) |
| 8 | anchor at the bottom (base line); multi-line: the first line at the anchor |
| 16 (without 8) | anchor in the middle |
| neither 8 nor 16 | anchor at the top |
| no `align` | base line, left |

`PH`/`PHL` with `wd`/`hg`: aligned in the frame; `tc:5` cuts at the frame,
`tc:6` wraps into lines. Line breaks in texts: a line break or `\a`. The
font of WSCAD's PDF export is Roboto Mono (monospaced).

### Arcs

`kreis` with `as`/`ag`: start angle and span in degrees (GDI: clockwise
with y down). `type="KreisH" qd=…`: a half circle, `qd` = quarter bits
(3 = 0–180°, 6 = 90–270°, 12 = 180–360°, 9 = 270–90°).

### PLC channel address (`P type="WSPlcChannelAddress"`)

`0#|#True#|#True#|#EN|T|I|L|DE|T|E#|##|#0#|#7#|#1#|#.#|#` → fields
separated by `#|#`: 1 bit addressing, 3 prefix per language
(`language|T|prefix`, separated by `|L|`), 5 byte, 6 bit, 8 separator →
"E0.7"; without bit addressing "AEW1".

## Database (WSCAD6.db)

| Table | Columns (excerpt) |
|---|---|
| `Page` | PageID, FileName, FileType (wsELD …), Width, Height, AspF (=), AspL (+) |
| `Symbol` | SymbolID, ParentID, ItemType, BMK (`=A1+ST-K1`), PageID, x, y, xBase, yBase |
| `Pin` | SymbolID, PinID, Name, Number, x, y (relative to xBase/yBase) |
| `Connection` | SymbolID1, Pin1, SymbolID2, Pin2, PageID, Colorcode, Dimension, Potential, WireName, CableID |
| `Data`, `String` | properties per id and type (57 article, 63 manufacturer …) |

- `Pin.PinID` counts the `<Pin>` of the symbol in file order; the relative
  position always matches. `xBase/yBase` differs from the page origin for
  rotated arrows, so the position is taken from the page.
- `Connection` lists the logical pairs across corners and T symbols;
  bridges (terminals, distributors) are listed as a complete net (each with
  each).
- Pages are ordered by groups (structure + page kind, in the order of their
  first PageID, as in the PDF export) and by number within a group.

## Translation to QElectroTech (wscad_to_qet.py)

- A folio per page, A4/A3 landscape, at 96 dpi, the origin at scene (5, 5)
  (QET's `Diagram::margin`); the frame and title block of the page as folio
  graphics, with an empty title block template embedded in the project;
  page title, sheet, plant and location into the folio properties.
- A symbol with `hid` and terminals → an element (its own graphics, texts,
  terminals in the WSCAD direction) in `custom://Imported/WSCAD/`; the
  designator as label; article, manufacturer, designation and function text
  as element information. Right-aligned labels lie by their width to the
  right in the element (QET keeps the right edge when the value is set).
- Wires: `Connection` per page, a spanning tree per folio (the connections
  with a drawn route first, then the shortest). Colour, cross section and
  potential of the connection go to the conductor, the same across a pair
  of folio reports, else QET refuses to link them.
- Arrows with a target → linked folio reports. They are paired over every
  page by wire name: first the pairs whose cross references point at each
  other (`/2.0` = sheet 2 of the same structure, `=A1+P1/3.2` another one),
  then the cross reference of the arrow, then the next later sheet. The
  arrow on the earlier sheet goes.
- Coil (2) / master (7) and contacts (3) of the same designator →
  master/slave; WSCAD's cross-reference texts stay, QET's are off.

## Open

- The conductors follow QElectroTech's routing, not the route drawn in
  WSCAD (the scripting API has no call to set the corners of a conductor).
- PE wires are drawn dashed by WSCAD (line type of the potential); the
  import draws them solid.
- PLC cards (15) and channels (16) become elements, not linked as PLC
  master and channels.
- Wire attributes (23: wire number, potential name) are not taken over as
  conductor numbers.
- `.wsPRAX` (packed project backups) are not read, only project folders.
- Fluid, cabinet and report pages are not imported.
