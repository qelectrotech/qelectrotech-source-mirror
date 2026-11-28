# sPlan 8 (.spl8): the structure as decoded independently

Abacom does not publish the format, and there is no free reader for it.
This description was worked out from the bytes of sPlan 8 files alone: the
example files that come with sPlan 8 and two private drawings (not
distributed). The meaning of the part fields was checked against the help
of the program (topics "What is a component?", "Contacts", "Parent-child
relationships", "Junction dots"). No code of sPlan was used or looked at.

Marks: **FACT** = shown by the reader on every file (each file is read to
its last byte, every count lands on the next valid object) · **CHECKED** =
the meaning is backed by the sPlan help or by a comparison with the printed
PDF · **GUESS** = derived, not confirmed independently.

Reader: `splan8.py` (its functions carry the section numbers of this file).
Tests: `test_splan_import.py` writes files in this format and reads them.

## 1. Basic types
- FACT: an uncompressed Delphi stream, little endian. No checksums.
- FACT: `i32` = int32, `f32` = float32, `u8` = byte.
- FACT: a string `ws` = `i32` character count + UTF-16LE (no terminating
  zero). A font name = `i32` + Pascal short string (`u8` length +
  Latin-1), e.g. `05 00 00 00 05 'Arial'`.
- FACT: coordinates are `f32` in 0.1 mm, origin top left, y downwards. An
  A4 landscape frame runs from (20, 20) to (2920, 2040); the inner frame is
  at 72 (7.2 mm), the title block starts at y ≈ 1760.

## 2. File header
- FACT: bytes 0–7 `07 'SPLAN80'` (a Pascal short string), bytes 8–15 the
  version (`46 40 …` or `48 40 …`, both read the same way).
- FACT: then `u8`, `ws`, `ws`, `i32`, `i32` (meaning unknown).

## 3. Sheet
- FACT: `ws` sheet name, `i32` width in mm, `i32` height in mm (210 × 297;
  portrait: swapped), `i32` grid, `i32`, `i32` (scroll position), `f32`
  zoom, `i32` **number of objects − 1**, then the objects.
- FACT: the objects are followed by a sheet footer (8 × `FF`, `f64` 0.0
  and 1.0, sheet size 2800 × 1900 as `f32` …). Its length differs between
  files (102 or 104 bytes); the reader therefore looks for the next valid
  sheet header (a name and a plausible sheet size). After the last sheet
  only this footer follows.

## 4. Object header (every kind of object)
- FACT: `u8` type, then 23 bytes: 3 bytes stroke (byte 2 = line width in
  0.1 mm, e.g. 1, 2, 5), 11 bytes (size and automatic mode of a junction
  dot, else 0), 4 bytes fill colour, 4 bytes line colour, 1 byte flag.

| Type | Kind | After the header |
|---|---|---|
| 01 | ellipse/circle | `i32` n, n+1 points (as a polyline), 47 bytes |
| 02 | rectangle | 5 points (4 corners + closing point), 31 bytes |
| 03 | text | section 5 |
| 04 | polyline | `i32` n, n+1 points, 21 bytes (section 6) |
| 05 | polygon (closed) | `i32` n, n+1 points, 27 bytes |
| 06 | group ("clip") | `i32` children − 1, the children (section 10) |
| 07 | part | section 10 |
| 08 | junction dot | 1 point |
| 09 | image | 4 corners, a zlib stream (content: `u32` length + BMP file), 4 bytes |
| 0A | multi-line text | 4 corners, `u8`, font, 9 bytes, `i32` lines, the lines as `ws`, 7 bytes |
| 0B | contact (terminal marking of a part) | like a text (section 5), then `ws` contact name |
| 0C | Bezier curve | like a polyline |

## 5. Text
- FACT: header, the 4 corners of the text frame, `ws` text, font, 14 bytes
  attributes (byte 5 = font style), `ws` active link (URL or file, else
  empty), 16 bytes GUID, 22 bytes.
- FACT: when a free text holds a variable (`<PAGENO>` …), a second `ws`
  with the evaluated text follows ("<PAGENO>" → "1"). CHECKED: the
  variables as the help lists them.
- FACT: the rotation follows from the frame: corner 0 → corner 1 is the
  reading direction (0° and ±90° occur); corner 0 → corner 3 points to the
  base line. Font height = length of corner 0 → corner 3.

## 6. Polyline, arrow ends
- FACT: 21 bytes after the points; byte 5 = line end at the start, byte 6
  = at the end (0 = none, 2 = arrow; 1, 5, 10, 11 occur too). CHECKED:
  exactly the lines printed with an arrow carry the value 2.
- FACT: in older sheets an arrow is a small polygon of its own (type 05,
  6 points, ≤ 3 mm) at the end of the line.

## 7. Areas
- FACT: rectangle, polygon and ellipse as in section 4; ellipses are stored
  as a polyline already (32 points).

## 8. Junction dot
- FACT: one point. CHECKED (help "Junction dots"): shows that crossing
  lines are connected.

## 9. Image
- FACT: a zlib stream right after the 4 corners, without a length; the
  zlib decoder finds its end. Content `u32` + BMP.

## 10. Group and part
- FACT: `i32` children − 1, then the children (any objects, nested).
- FACT: a part (07) continues with:
  - the designator as a text without a type byte (section 5) + `ws` the
    evaluated designator (e.g. "IC1-1" of "<PARENT_ID>-<CHILDNO>"),
  - the value as a text without a type byte (section 5); when it holds a
    variable, `ws` with the evaluated value follows,
  - 4 bytes switches (visible / numbered automatically / parts list –
    GUESS),
  - `ws` the **name of the library part** (e.g. "Schliesser", "Öffner",
    "Netzteil" in the German libraries; several languages separated by
    `\r`; once only the control character 07), CHECKED by the help
    ("caption in the library"),
  - `ws` designator template of the automatic numbering ("K",
    "<PARENT_ID>"), `i32` running number (−1 = off),
  - `f32`, `f32`, `i32`, `u8` (position of the child caption and more,
    GUESS),
  - 4 × `ws` extra texts 1–4 for the parts list (CHECKED: help "Changing
    existing components"),
  - `u8` is parent, `u32` **own id**, `u32` **parent id** (0 = none), `u8`.
- CHECKED: in the example "Parent-Child.spl8" the four gates IC1-1 …
  IC1-4 carry the id of the 7400 as their parent id; auxiliary contacts
  carry the id of their contactor. The import makes coil ↔ contact
  (master/slave) of it.

## 11. Sheet footer
- See section 3. GUESS: print area, margins, grid, view position.

## 12. Connections (there is no net list in the file)
sPlan stores no terminals and no nets: a part is, as the help says, only a
group of drawing elements. The import derives the connections from the
geometry (`splan_analysis.py`) by these rules:

1. A wire is a polyline at the top level (not inside a part), not in the
   frame or title block (frame lines lie in the border strip together
   with the middles of their segments; an area belongs to the frame when
   its centre lies there).
2. Two wires are connected when an end of one lies on the other (distance
   ≤ 0.3 mm) or a junction dot lies on both. Crossings without a dot are
   not connected (help "Junction dots"). A wire end that crosses another
   wire and ends at most 1.5 mm behind it is taken as connected (a tap
   drawn past the rail by hand); this is listed as an open question.
3. A terminal is a wire end on a line of a part or inside its frame
   (≤ 0.3 mm). Terminal blocks: every end on one side (top/bottom) is one
   screw.
4. Terminal name: the nearest sPlan "contact" of the part, else the
   nearest text in the part; collective texts such as "PE I L I N" are
   split into single names at their position in the text. Without a text:
   a number by position (→ open questions).
5. A potential name is the text at the arrow end or at the free end of a
   wire. Texts are assigned to arrows globally, so that arrows close to
   each other do not swap their texts. Texts along the wire are often
   nearer to the neighbouring wire than to their own; but the drawing puts
   all texts of a group at the same offset next to their wire (side and
   distance differ from sheet to sheet: the base line or the top at the
   wire). So the offset across the wire that fits most neighbouring texts
   is what is measured.
6. Images (type 09) on which wires end are devices without data; their
   terminal names are only in the picture (→ open questions).

## Open
- The meaning of the header fields (section 2), of the 4 switch bytes and
  of the two `f32` of a part.
- Ellipses and arcs: start and end angle in the 47 bytes (not needed for
  the import, the points suffice).
- Multi-line text: alignment and line spacing in the 9 bytes.
