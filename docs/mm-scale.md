# Design note: a millimetre scale for folios

Status: implemented, steps 1 to 8, in one pull request (follows #1971,
IEC 61082-1 review). See "What was built" at the end for where the code
differs from the proposal and how the open questions were answered.

## The problem

QElectroTech draws in scene pixels, and nothing fixes what a pixel is on
paper. IEC 61082-1 builds a drawing on a module M (2.5 mm recommended; 1.8,
2.0, 3.5 and 5.0 mm allowed). Grid, symbol size, line widths, text heights
and conductor clearances are all given in mm or as multiples of M. QET can't
meet any of these, or check them, until a grid step has a known size.

Today the size of a 10 px grid step on paper depends on how the folio comes
out of QET:

| Output path | Mapping | 10 px grid step |
| --- | --- | --- |
| Print / PDF via print dialog, "Fit sheet to page" (on by default) | scaled to the printable area | depends on paper and folio: about 2.86 mm (default folio on A4 landscape), about 4.04 mm (on A3 landscape) |
| Print, fit off (tiled) | 96 dpi | 2.65 mm |
| `qelectrotech --export-pdf` (CLI) | page = folio at 96 dpi | 2.65 mm |
| DXF export | fitted to a 4000 × 2700 unit sheet, no units | arbitrary |
| Dimension item | "Strictly 2 px = 1.0 mm" | reads as 5 mm |
| Terminal strip, realistic view | `realistic_px_per_mm`, default 4 | 2.5 mm |

So the code already has three px/mm conventions (3.78, 2 and 4) plus two
that fit the drawing to the page and have no fixed scale at all.

## What the code does today

- **Grid:** `Diagram::xGrid` / `yGrid` = 10 px (`sources/diagram.cpp:167`).
  The user can change the step in preferences (`FolioGrid`,
  `diagrameditor/Xgrid`, `Ygrid`). The symbol library is drawn on the
  10 px grid.
- **Folio size:** `BorderProperties` gives columns and rows in px (default
  17 × 60 px plus a 20 px header, 8 × 80 px plus a 20 px header, i.e.
  1040 × 660 px). The paper-size presets in `BorderPropertiesWidget` convert
  ISO 216 sizes at 96 dpi (`mmToPx`, `sources/ui/borderpropertieswidget.cpp:44`),
  so A3 landscape is 1587 × 1123 px.
- **Printing:** `QetPrint::drawingDpi = 96` (`sources/print/printdrawing.h:48`).
  Fit-to-page scales the folio to the printable area. With fit off, the
  folio is tiled at 96 dpi. The `print/default/fitinpage` setting defaults
  to true.
- **CLI PDF:** the page is the folio at 96 dpi (`sources/cli_export.cpp`,
  "1px @ 96dpi = 0.75pt").
- **DXF:** `DxfExport::write()` scales to fit `Createdxf::sheetWidth/Height`
  (4000 × 2700). The file carries no units.
- **Line widths:**
  - Element parts have five weights: none, thin (cosmetic pen, width 0,
    i.e. one device pixel at any zoom or printer resolution), normal 1 px,
    "ultra" 2 px, big 5 px (`CustomElementGraphicPart`).
  - Conductors have `cond_size`, default 1 px.
- **Text:** font sizes are in points, default Liberation Sans 9 pt
  (`QETApp::diagramTextsFont`). They are recorded at 96 dpi, so 9 pt is a
  12 px em.
- **Dimension item:** `DimensionItem::dimensionValueMM()` uses a fixed
  2 px/mm (`sources/qetgraphicsitem/dimensionitem.cpp:521`).

## Proposal

### 1. One scale: 10 px = M (px/mm = 10 / M)

Define the conversions in one header, for example `sources/units.h`. The
recommended module M = 2.5 mm gives 4 px/mm:

```cpp
namespace QetUnits {
	constexpr qreal pxPerMmFor(qreal module_mm) { return 10.0 / module_mm; }
	constexpr qreal mmToPx(qreal mm, qreal px_per_mm) { return mm * px_per_mm; }
	constexpr qreal pxToMm(qreal px, qreal px_per_mm) { return px / px_per_mm; }
}
```

Why 4 px/mm (M = 2.5 mm) and not 96 dpi (3.78 px/mm):

- The 10 px grid becomes exactly M. **The symbol library, the
  grid and every existing coordinate stay as they are.** Only the mapping
  to paper becomes defined. Changing the grid to 9.45 px instead would
  move every symbol and terminal off grid.
- It is the scale the realistic terminal strip already uses by default.
- Existing pen widths line up with the ISO series: 1 px = 0.25 mm and
  2 px = 0.5 mm, which already meets the 2:1 thin/thick ratio of
  clause 5.10.

Other modules (1.8, 2.0, 3.5, 5.0 mm) would be a later project setting
(px/mm = 10 / M). This proposal fixes M = 2.5 mm for the first version;
see "What was built" for the selectable module.

### 2. A project setting, off for existing projects

Add a project property next to `drawing_standard` (#2005), for example
`drawing_scale = "px"` (today's behaviour, default) or `drawing_scale = "mm"`. New projects
with the IEC drawing standard would default to `"mm"`. Projects saved
"as before" open and print exactly as they do now.

A project with `drawing_scale = "mm"` opened in an older QElectroTech still works:
the attribute is ignored, so it prints fit-to-page as today. The
newer-project warning from #2005 covers saving it back.

### 3. Folio sizes from paper sizes

In an mm-scale project, the paper-size presets convert with
`QetUnits::mmToPx`, not 96 dpi. For A3 landscape that is 420 × 297 mm =
1680 × 1188 px, and a printed folio matches the paper at 1:1. The frame
does not line up with the 10 px grid: 1188 is not a multiple of 10, and the
columns and rows (whole pixels, plus the 20 px header) rarely divide the
paper exactly. The existing fit keeps one pixel for the border line and
leaves up to 3 px more, so up to 4 px (1 mm) of the paper stays unused
(A3 landscape comes out 1676 px wide). That is harmless at 1:1, so cells
are not snapped to the grid.
Existing folio sizes are not touched.
Only a preset that the user picks again recomputes columns and rows.

### 4. Print and export to scale

Add a third choice to the print dialog beside "Fit sheet to page" and
tiling: **"To scale (M = 2.5 mm)"**, the default in mm-scale projects.

- Print / PDF: render with `pixels_per_unit = printer->resolution() / (pxPerMm * 25.4)`,
  i.e. treat the folio as 101.6 dpi instead of 96. The folio is still
  recorded as a `QPicture` at 96 dpi (`drawingDpi` keeps text right). Only
  the playback scale changes, so geometry and text shrink together by
  96 / 101.6 ≈ 0.945.
- CLI PDF: page size = folio px / `pxPerMm` mm, in mm-scale projects.
- If the folio doesn't fit the printable area (printer margins), fall back
  to tiling at scale or warn. Never shrink silently: a scaled print isn't
  to scale.
- DXF: in mm-scale projects, write coordinates in mm (1 px = 0.25 mm) and
  set `$INSUNITS` = 4 (millimetres), not the fit to a 4000 × 2700 sheet.
  This alone makes DXF output usable in CAD at 1:1.

### 5. Line widths in mm

In mm-scale projects, show and enter widths in mm, and offer the ISO
128-20 series as presets:

| Preset | mm | px at 4 px/mm |
| --- | --- | --- |
| Very thin | 0.18 | 0.72 |
| Thin | 0.25 | 1 |
| | 0.35 | 1.4 |
| Thick | 0.5 | 2 |
| | 0.7 | 2.8 |
| | 1.0 | 4 |

Storage stays in px (`condsize`, `style="line-weight:…"`), so files stay
readable by older versions. Points to settle:

- The cosmetic "thin" weight is one device pixel, which on a 600 dpi
  printer is 0.04 mm. In mm-scale projects it should print as 0.18 or
  0.25 mm.
- "Big" (5 px) is 1.25 mm, which isn't in the series. Leave it, or map it
  to 1.0 mm?

### 6. Text heights in mm

ISO 3098 and IEC 61082 give text height as capital height h
(2.5 / 3.5 / 5.0 / 7.0 mm). Fonts stay stored in points. In mm-scale
projects the text-size fields show h in mm, converted with the font's cap
height:

```
em_px = pt * 96 / 72
h_mm  = em_px * capHeightRatio / pxPerMm
```

For Liberation Sans (cap height 1409/2048 ≈ 0.688 em), the default 9 pt comes out
at about 2.07 mm, and h = 2.5 mm is about 10.9 pt. The ratio comes from
`QFontMetricsF::capHeight()`, not a constant.

### 7. Dimensions and terminal strips

- **Dimension item:** in mm-scale projects, use `QetUnits::pxToMm` (4 px/mm)
  instead of the fixed 2 px/mm. Existing projects keep 2 px/mm, or every
  dimension already drawn would show half its value. Alternatively, store
  the scale on the item.
- **Terminal strip, realistic view:** default `realistic_px_per_mm` to
  `QetUnits::pxPerMm`. It already is 4.

### 8. Rules that need a unit (later)

Once a grid step is M, these become possible. They're out of scope here
but are why the scale matters:

- Conductor clearance check (7.1.3.4): parallel conductors at least 1M
  (10 px) apart, at least 2M (20 px) where text runs between them.
- Text not touching or crossing lines (7.1.3.5).
- A warning when the user changes the grid step from 10 px in an mm-scale
  project, because the step is then no longer M.

## Order of work

Each step is one commit, and none changes an existing project:

1. `QetUnits` header and the `drawing_scale` project property (no behaviour change).
2. Paper-size presets at 4 px/mm in mm-scale projects.
3. "To scale" printing, GUI PDF and CLI PDF.
4. DXF in mm with `$INSUNITS`.
5. Line widths in mm, with ISO presets.
6. Text heights in mm.
7. Dimension item at 4 px/mm in mm-scale projects.
8. Clearance and text-overlap checks.

## Open questions for maintainers

1. Should the scale be a separate project property or come with the IEC
   drawing standard? Should NFPA projects get the same mapping (NFPA
   drawings are usually in inches)?
2. Is 4 px/mm acceptable as the fixed internal scale, or do we want M to
   be selectable from the first version? (Answered: M is selectable, see
   "What was built".)
3. Should "To scale" become the default print mode for mm-scale projects,
   given that it can need tiling on printers with large margins?
4. Should the cosmetic "thin" weight change meaning in mm-scale projects,
   or get a new weight?
5. Should the dimension item move to 4 px/mm everywhere, with existing
   projects migrated, or only in mm-scale projects?

## What was built

The open questions were answered like this for the first version:

1. A separate project property, `drawing_scale`, chosen in Project
   properties > General. It is not tied to the drawing standard, and new
   IEC projects do not default to it yet. NFPA projects can use it too.
   The key is not `scale`, because many title blocks already use a
   project variable of that name.
2. M is selectable from this version: 1.8, 2.0, 2.5, 3.5 or 5.0 mm
   (IEC 61082-1, 5.7.3.2), chosen in Project properties > General next to
   the drawing scale ("Module M:", enabled when the scale is Millimetres).
   It is the project property `drawing_module`; 2.5 mm is the default and
   stores nothing, so every existing project and file stays as it is. A
   value this QElectroTech does not know is kept as typed and read as
   2.5 mm. Everything that needs a unit reads px/mm = 10 / M from
   `QETProject::pxPerMm()`: paper presets, printing to scale, the CLI PDF,
   DXF, the thin pen, line widths and text heights in mm, the dimension
   item and the finding texts of the clearance checks. The checks themselves
   stay at 10 px (1 M) and 20 px (2 M), because the grid step is M whatever
   M is.
   Changing M (or the scale) in an open project redraws what depends on it
   (the thin pen of symbols, the text of dimension items) and the print
   dialog reads it when opened. **Stored pixel values are not rescaled**:
   a folio, a conductor width in px or a text size in points keep their
   values, so the geometry does not move and only what it measures on
   paper changes. The terminal strip's realistic view keeps its own stored
   `realistic_px_per_mm` (default 4).
3. Yes: "To scale (M = %1 mm)", filled with the project's M, is a check
   in the print dialog, shown only in mm-scale projects and checked by
   default there. Printed on paper, a folio larger than the page is split
   over several pages at scale, never shrunk. In a PDF each folio gets a
   page of its own size (the standard sheet when the folio is within 3 pt
   of one and not larger), so one folio is one page at 1:1, with its
   cross-reference links. The print preview still shows the paper pages.
4. In mm-scale projects the thin weight of symbols is drawn as 0.18 mm
   (0.72 px at M = 2.5 mm, i.e. 0.18 * 10 / M px, not cosmetic) on screen and on paper. Big stays 5 px
   (1.25 mm). The element editor is unchanged.
5. Only in mm-scale projects. Existing dimensions keep 2 px/mm.

Other differences from the proposal:

- Folio sizes (step 2): the frame does not land on the 10 px grid, see
  section 3.
- Text heights (step 6) are given in independent texts, conductor texts
  and the element text table. Conductor texts keep whole points. The
  default-conductor pages of the project and the application stay in
  points.
- DXF (step 4) still declares AutoCAD R10 (`$ACADVER` AC1006), which is
  older than `$INSUNITS`. A strict R10 reader may ignore the unit.
- The clearance checks (step 8) are on by default only in mm-scale
  projects. They report at most 200 findings per folio and check, and
  then one more finding saying how many were left out. A wire number over
  a wire of its own potential is not reported, and a text over another
  potential is reported once, however many of its wires it crosses.
- Changing the scale or M in Project properties is not undoable, like
  the other settings of that page.
- The grid preferences show a note when the grid step is not 10 px.

## Not in scope

- Changing the internal coordinate system or rescaling the symbol library.
- Changing the 96 dpi recording of text (`drawingDpi`).
- Imperial modules for NFPA projects.
