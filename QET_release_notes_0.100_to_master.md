# QElectroTech — Release notes: 0.100 → current master (0.200.1-dev)

*Covers the period 26 January 2026 – 9 October 2026: 2,253 commits and 669 merged pull requests. Numbers in brackets are GitHub PRs (#nnn) or bug tracker entries (bugtracker #nnn).*

---

## Highlights

- **Qt 6 is now the platform.** The Qt 6 migration is complete, Qt 5 support has been removed, and Windows, macOS, Flatpak and Snap packages are built on Qt 6 / KF6.
- **Rewritten auto-numbering.** Renumbering of existing elements, new numbering parts (alphabetic, cyclic/modulo, zero-padded formats), undo/redo for counters, global rules in the settings, and importing rules from another project.
- **Many more drawing and editing tools.** Editable resize/rotate/skew handles for shapes and pictures, an Arc tool and a fillet tool, Group/Ungroup, Align and Snap to grid, mirroring symbols, rotating groups, text-box width handles, and dropping pictures onto a sheet.
- **Productivity UI.** Command search, Ctrl+G jump to element or cell, an element picker and shortcut bar at the cursor, a context command bar, mouse gestures, repeat-last-command, configurable shortcuts, toolbars and gestures (Customise window), and paste under the cursor.
- **Wiring, BOM and PLC.** A from-to wiring list export, a PLC Manager, slave contact groups and a full contact comb, terminal potential grouping, a material list lookup, smart-device BOM metadata, a generic device wizard, and drag and drop onto terminal strips.
- **Import and export.** EPLAN Data Portal (.edz) import, PDF page import, and much better DXF export (layers, symbols as blocks with attributes). PDF export gains clickable cross-reference links and can be made reproducible.
- **Command line, scripting and AI assistants.** Headless export and verification from the command line, a JavaScript scripting API (off by default), script buttons and a script manager, a macro recorder, and an MCP server (`qet-mcp`) with a "live mode" so an AI assistant can work on an open project.
- **Dark themes and icons.** All toolbar and menu icons are now SVG, there are "qet" and "qet-dark" icon themes, an optional dark folio canvas, and many dark-theme fixes.
- **English is the source language.** The interface text is now written in English (previously French), "folio" is renamed "sheet" in the English UI, and Korean has been added as a new language.
- **Stability.** A large number of crash, use-after-free and memory-leak fixes, rotating crash-recovery snapshots, a hardened crash reporter, diagnostic logging, and reproducible project saving.

---

## New features

### Auto-numbering
- Renumber elements that are already placed [#704]; a broad overhaul of the formulas for elements, conductors and sheets [#1282]
- Alphabetic numbering (a, b … z, aa, ab …) [#594]; wrap-and-carry (cyclic/modulo) numbering [#593]
- A display format (zero mask) you can set for each numbering part [#634]
- Quick reset buttons [#626]; edit the increment and preview the next number in the dock [#697, bugtracker #331]
- Undo/redo now covers auto-numbering counter changes [#645]
- Global auto-numbering rules in the application settings, applied to new projects [#924]
- Reuse the numbering rules from another project [#851]

### Drawing, shapes, pictures and text
- Editable resize, rotate and skew handles for shapes and pictures, plus picture transparency and faster image handling [#809, #810, #817, #1024]
- Arc tool with a handle to pull a half arc in or out [#1202, #1211]; fillet tool to round the corner between two lines [#1212]
- Group and Ungroup items on a sheet [#1074]; click a member of a selected group to select it alone [#1110]; rotate a group as one piece [#660, #1111]
- Align left/centre/right/top/middle/bottom, with icons [#1087, #1153]; Snap to grid for selected symbols, pictures, texts and shapes [#1073, #1152, #1154]
- Mirror symbols horizontally and vertically on a sheet [#1354]; an option to keep the texts of rotated symbols horizontal [#1355]
- Free texts can keep a width they wrap to, set in their properties or by dragging a corner handle [#1320, #1321, #1341, #1342]
- Drag-to-resize the width of dynamic element texts [#591, #977]; a centre rotation point for dynamic text fields [#694]
- Labels on pictures [#1068]; drop picture files onto a sheet to add them [#1416]; the crop, colours and original of a picture undo together [#1310]
- Import a PDF page as an image [#772, #811]
- Remember the last-used shape and text style for the session [#656]
- Custom guides [#509]; a finer snap grid for dragged texts [#1040]
- View > Show to hide texts, shapes and pictures [#1268, bugtracker #301]
- Sheet background colour picker with adaptive border and title block, remembered between runs [#968, #1011]; a custom application colour picker [#979]
- A permanent ID for shapes, texts, pictures and symbol parts [#1065]; lasting UUIDs for projects, sheets, symbols, wires and terminals, including those in older files [#883, #884, #1105, #1107, #1118, #1122]

### Element (symbol) editor
- Live cursor-coordinate readout [#589, #761]
- Export a symbol to SVG [#637]
- Optional alignment for static texts [#569]; `anchor="alignment"` for static texts in element files [#1404]
- Show terminal names [#536]; check terminal names when saving [#1159]
- Scale element, keeping terminals on the grid [#1378]
- An adjustable background frame [#1164]
- Mirror and flip the selection in place instead of across the origin [#813]
- The Information tab is available for Slave and Terminal types, and auto_num_locked, potential_isolating and exclude_from_bom can be edited [#721, #765, #767, #1399]
- New element type: line definition [#464]

### Sheets and navigation
- Duplicate sheets with all their metadata [#473]; undo/redo for adding, deleting and reordering sheets [#590]; insert a sheet above or below [#658]
- Paper sizes A0–A5 [#1362]
- Keep the row and column headers visible, and optionally show the cell limits across the drawing [#1038, #1048]
- Ctrl+G jumps to an element or to a sheet cell such as B13 [#586, #1036]
- Centre on cursor [#1190]; the search view scrolls to the selected hit [#706]
- Auto-select the active sheet in the elements panel tree [#443]; a newly added sheet is selected in the project tree [#805]
- The main window title shows unsaved changes (macOS modified dot) [#624]
- 3D mouse (SpaceMouse/SpacePilot) support on Linux, Windows and macOS, with speed, direction and twist-to-zoom settings [#635, #1022, #1023, #1025, #1026, #1028, #1032]

### Productivity and customisation
- Command search: type part of a command's name and press Enter [#1049]
- Element picker at the cursor (Insert) [#1052]; shortcut bar at the cursor (S), customisable in place and with pinned elements [#1053, #1054, #1060]
- A command bar beside the selection [#1055]; repeat the last drawing or placing command with Enter [#1056]; right-drag mouse gestures [#1057, #1364]
- Place elements by double-clicking in the collection, and repeat with A [#1042]; placement and sheet references in the right-click menu [#1041, #1050]
- Configurable shortcuts with a dedicated preferences page (grouped, searchable, icons, menu locations) [#587, #760, #821, #1325, #1326]
- Toolbars settings page (icon size, text with icons, lock) and a combined Customise window [#1327, #1365]
- Paste under the cursor; Ctrl+Shift+V pastes at the origin [#878, #917, #1208]; Ctrl+D duplicates with a configurable offset [#993]
- Tab cycles through the selection; select all conductors or text fields [#874]
- Keyboard accessibility: F10 opens the menu bar, the drawing tools are in a menu, and the context-menu key opens the sheet menu [#875, #876, #877]
- Save and load the configuration, in full or in part, to a file [#1166, #1410]
- Dialog sizes are remembered [#1165]; QColorDialog custom colours are kept across restarts [#918, #1370]
- Edit several wires at once in the Selection properties panel [#502, #1205]
- One-click conductor colour on the toolbar; F2 remembers the colour for the session; double-click to choose a wire colour [#888, #929, #1172]
- Paste element information from one symbol onto others [#1376]
- Element information suggests values already used in the project [#1021, bugtracker #217]
- A position-lock checkbox for elements [#803]
- The Edit menu is split into submenus [#1346]
- Settings for the folder prefixes of the user collection [#1168]
- A local tracker for the time spent on each project [#583]

### Conductors and wiring
- Auto-break conductors [#639, #700]
- Optional hops where wires cross [#1200]
- An optional limit on the number of wires per terminal, wiring terminals in a chain rather than a star [#1272, #1273]
- Wire routing around symbols (scripting/MCP) [#1245, #1256, #1258]
- Linked cross-references inherit the conductor line style [#967]

### Wiring list, BOM and reports
- A from-to wiring list built on new terminal/conductor tables in the project database, with a dialog, export, Cable column, terminal index/UUID and numeric wire order [#447, #462, #628, #629, #630, #835, #1197, #1248]
- Exclude specific elements from the BOM [#472]; options to leave junctions and contact blocks out of the parts list [#1318]
- Smart device metadata in the BOM export [#830]
- A material list (CSV) lookup for the element article fields [#1088, #1119]
- Custom SQL reports with read-only enforcement, preview and import/export [#896, #983]

### Master/slave, PLC and terminals
- PLC Manager [#562, #588, #623, #669, #703, #774, #1001]; PLC terminal names and cross-references [#766, #796]; "Hide linked elements" and "Hide full masters" filters [#858, #881]
- Slave contact groups (label transfer, terminal assignment) [#545]; a full contact comb that shows every slave defined by the master [#1010]
- Contact usage counting (NO/NC/SW breakdown, used vs. declared capacity) and an advisory slave limit [#444, #825, #826, #827, #828, #831]
- Terminal names in cross-references, with an option to hide them; terminal names shown in export [#673]
- Configurable distance between the label and the slave cross-reference [#771]; an option for stacking cross-references at the sheet bottom [#1311]
- Automatic terminal numbering tool, with letter numbering and strip selection [#449, #741]
- Terminal potential grouping [#1396]; a potential isolation option for terminals [#471]
- Drag and drop free terminals onto a terminal strip [#1199]; natural sort for terminal ordering
- Width/height/depth properties for elements [#723]; cabinet thumbnails generated from the manufacturer and reference [#1170]
- Generic device wizard: a box symbol with terminals on any side, for devices too rare to draw a symbol for [#1418]

### Templates (macros)
- A User Templates collection with its own tab [#451, #455]

### Import and export
- **EPLAN Data Portal (.edz)** parts import into element collections [#513, #555]
- **DXF:** named layers, deterministic entity order, symbols as blocks with attributes and hidden part data, slave/master cross-references, and wrapped text [#740, #750, #1075, #1076, #1079, #1324, #1350, #1351, #1352, #1353]
- **PDF:** clickable cross-reference links [#490], component info as invisible annotations [#739], and repeatable output when SOURCE_DATE_EPOCH is set [#1257, #1286, #1317]
- Folio properties automatically add a title block's custom variables [#495]

### Command line, scripting and AI assistants
- **Headless CLI:** export to PDF, PNG, SVG, DXF, cable list and wire list; info, BOM, nets, links, check-elements, resave; `--set-titleblock`; `--show-terminals` [#483, #485, #489, #493, #731, #1078]
- **JavaScript scripting API:** read, export, edit, draw, cross-reference and query a project, with UUID lookups. It is **off by default** and asks before enabling [#891, #970, #980, #984, #1098, #1106, #1114, #1125, #1219, #1264]
- Script buttons, a script manager and a macro recorder [#1220, #1221, #1227]
- **`misc/qet-mcp`:** a Model Context Protocol server over QET projects, installed with QElectroTech (and with Python offered by the Windows installer). Help → Connect an AI assistant, and live mode (ask-first, an Assistant panel, and project create/open/print/save), plus a shared house style [#969, #1128, #1136, #1137, #1138, #1222–#1225, #1228, #1265, #1266, #1312–#1316]

### Diagnostics and robustness features
- Diagnostic logging rework with rotation, a ring buffer, crash-time flush and an export UI [#646, #647]; an event-loop watchdog [#665]
- Three rotating crash-recovery snapshots [#1167]; a hardened crash reporter that keeps every dump [#905, #1181]
- A backup dialog when opening an existing project [#534]
- Project load timing in the log [#560]
- Reload element drawings to refresh placed elements [#889]

---

## Bug fixes (selection)

### Crashes and memory safety
- Use-after-free and teardown crashes: backup threads [#512], restore of backups [#711, #1031], project/diagram teardown [#789], IPC file forwarding [#868], the terminal strip dock [#1162], closing without saving [#1231], and the template editor [#1407]
- Crashes when opening a project without a sheet [#797], with an unlinked contact [#1234], or with invalid corner layout [#670]
- Element editor zoom crash [#799]; huge symbol sizes [#1232]; NaN and infinite coordinates rejected on load [#792, #996, #1161, #1194]
- Dynamic text colour [#693]; cancelling collection load [#743, #995]; templates tree on Qt 6 [#795]; online-installer Qt SQLite handle [#1046]
- Memory leaks found by AddressSanitizer and Valgrind [#515, #518, #519, #1360]; uninitialised members [#1358, #1359, #1387, #1388]
- Thread-safety fixes for data paths and the elements panel [#514, #1163]

### Saving, loading and file format
- Saving is now reproducible: deterministic element, conductor, title block and XML ordering, and a second save changes nothing [#779, #814, #844, #846, #1030, #1109]
- QDom serialization hardened against invalid data (CVE-2026-15037) [#568]
- Font descriptions are written in a version-stable format, and salvaged fonts are reported [#582, #592]
- The read-only state is cleared after Save As [#497]; Save As on Snap adds the .qet extension [#716]
- Older parts lists keep their rows [#1077]; legacy sequential attributes are read correctly [#1385, #1386]
- Wires no longer move to another symbol after saving an older project [#1417]; replacing a symbol keeps its wires [#1116]
- Text position stays stable after a font-size change [#501]; wire text rotation is preserved [#707]

### Editing and the diagram editor
- Several copy/paste fixes: broken conductors, alignment, PLC data, master/slave links and underscore labels [#746, #770, #899, #909, #910, #911, #982, #997, #1188]
- Undo stack fixes [#925, #1039, #1192]; group rotation [#921]; the first click in the element editor no longer moves elements [#520]
- Junction dots on wide conductors and on shared lines [#862, #1299]; straight wires drawn diagonally [#1127]; unnecessary bridge routing [#890]
- Duplicating sheets renews UUIDs and keeps text positions [#477, #544, #856, #895]
- Multi-line symbol text alignment [#1155, #1160, #1176, #1302, #1357]
- Cross-reference overlap and positioning fixes [#712, #1263, #1287]

### Title blocks, reports and BOM
- Title block variables: empty page-level overrides, bare %name variables, unset variables, names with a slash, and file names cut at the first dot [#572, #717, #719, #725, #989, #1121, #1140]
- The summary table is ordered by sheet [#864]; custom summary SQL is kept [#887]; queries return all their rows [#1066] and are capped [#985]
- Terminal and contact blocks appear in the parts list [#849]; the wire-name export no longer double-counts [#724]
- Sheet dates are correct in the project database [#1134]; the database is rebuilt only once per load [#840]

### UI, theming and platform
- Dark theme: element icons [#620, #695, #744, #1092], slave cross-references [#718], the search field, and imported image colours [#1349]
- Locale and translation loading: regional locales, the lang/ path, double-click on Windows, and an English fallback [#496, #498, #559]
- Windows: Unicode and long paths in collections [#524, #1243, #1244], file association without admin rights [#1149], and MS Shell Dlg fonts in PDF [#1012, #1029]
- macOS: bundle identifier and file dialogs [#829], Fusion dark palette [#894], properties dialog placement [#715], file-open events, arm64 bundle on Sequoia, and the black screen after PDF export
- Snap: libxcb-cursor crash on Ubuntu 24.04 [#517]
- Recent files menu updates during the session [#863]; cancelling the potential selector [#584]; dock layout restored on Qt 6 [#769]

### Performance
- Fixed the Qt 6 load-time regression in XmlElementCollection [#570]; removed the pugixml pass from ElementFactory [#567]
- Faster saving of projects with pictures [#1067]; the element style regex is compiled once [#841]; picture cache fixes [#842]

---

## Translations
- **New language: Korean** [#419–#431]
- **English is now the source language** of the interface [#1390]; "folio" is renamed "sheet" in the English UI [#1216]; hundreds of untranslated or broken English strings are fixed [#1124, #1261, #1389]
- Updated: German, Polish, French (with real plural forms), Brazilian Portuguese, Spanish (complete), Russian, Chinese (zh-CN), Czech, Turkish, Hungarian, Dutch (Belgium) and Catalan
- Translation suggestions for 29 phrase books [#1006]; singular/plural fixes [#938, #971]

---

## Build, packaging and infrastructure
- **Qt 6 migration completed and Qt 5 removed** [#540, #541, #546, #547, #698, #705, #709, #880]; C++20 [#1123]
- qmake `.pro`/`.pri` files removed; CMake only [#1374]
- Optional precompiled headers (`QET_ENABLE_PCH`) [#680]; builds without KF5/KF6 [#533, #557, #1086]; Qt < 6.5 [#1085]; QtPdf optional
- SQLite kept for the project database; the export can be enabled via a CMake option
- FetchContent dependencies pinned [#958]; PugiXML and GoogleTest are no longer installed [#838]; SingleApplication updated to v3.5.6
- **CI:** Linux build and unit tests [#872]; Windows build and tests [#1361, #1425]; Windows MSI with WiX and signing (SignPath); monthly nightly builds
- **Packaging:** Flatpak and Snap moved to Qt 6/KF6 (Flatpak runtime 6.11, Snap core24); the macOS arm64 bundle uses CMake; the NSIS installer is on 3.x with fonts and more installer languages; `publiccode.yml` added
- New example project: Shelly single-phase and three-phase designs

---

## Contributors

Thanks to everyone who contributed to this release, including: ispyisail, Laurent Trinques (scorpio810), Kellermorph, Andre Rummler, Beat Hangartner, Shane Ringrose, Jeff Patterson, jp2images, plc-user, Dieter Mayer, Levi Jetzer (IBSYSLevi), Magnus Hellströmer (elevatormind), ChuckNr11, Gerhard Schwanzer, saschbe, enesgursoy6110, philippeagray, Kyle-Code-CA and jkh (정광호), zi-mozhuang, jindongjie, neitri, Alfmat01, cezlom, Vinícius Santos, zakb120, pafri, geri1701, achim and joshua, and to the translators Gábor (hu), Ronny (nl-BE), Pawel (pl) and Antoni (ca).
