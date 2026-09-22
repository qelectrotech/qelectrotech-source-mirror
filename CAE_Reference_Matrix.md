# CAE Reference Matrix - Phase 1

Scope: non-implementing comparison of QElectroTech (QET) with public vendor evidence for EPLAN Electric P8, WSCAD ELECTRIX, Zuken E3.series, AUCOTEC Engineering Base, SEE Electrical, and AmpereSoft ProPlan / former Moeller-ProPlan lineage where publicly verifiable.

Confidence labels:
- [Certain] Direct QET code/Brain evidence or direct public vendor statement.
- [Likely] Public vendor statement supports the principle, but not every workflow detail is public.
- [Unverified] Public source is sparse, ambiguous, reseller-only, or old; do not use as design fact without deeper validation.

## Sources Used

### QET evidence

- `Brain.md` architecture traces through phases 1-10.
- `Progress.md` phase evidence and test/backlog notes.
- Code/file evidence referenced in those traces: `sources/qetproject.*`, `sources/diagram.*`, `sources/qetgraphicsitem/element.*`, `sources/qetgraphicsitem/masterelement.*`, `sources/qetgraphicsitem/slaveelement.*`, `sources/qetgraphicsitem/crossrefitem.*`, `sources/qetgraphicsitem/terminal.*`, `sources/qetgraphicsitem/conductor.*`, `sources/TerminalStrip/*`, `sources/autoNum/*`, `sources/dataBase/projectdatabase.*`, `sources/cli_export.cpp`, `sources/wiringlistexport.*`, `sources/bomexport.*`.

### Public vendor sources

- EPLAN Electric P8 product page: device driven design, component/device/cable data, auto connecting, numbering, cross referencing, reports, wire/parts lists, Excel export, ERP/PDM integration. URL: https://www.eplan.com/us-en/products/eplan-electric-p8/
- WSCAD ELECTRIX product/release pages: terminal charts, material lists, standards-oriented documentation, compatibility, interfaces, PLC/manufacturing integrations, part repository, cross-reference/terminal/cable release evidence. URLs: https://www.wscad.com/en/electrix/ and https://www.wscad.com/en/electrix/release-notes/
- Zuken E3.series product page: single-platform electrical design, intelligent database, object orientation, schematics, wiring diagrams, cabinet/harness layouts, checks, reports, manufacturing documentation, PLM/PDM links. URL: https://www.zuken.com/en/product/e3series/
- AUCOTEC Engineering Base product page: central data model / single source of truth, object instance with diagrams/data sheets/lists as representations, ERP/PLM/3D integration, flexible digital twin. URL: https://www.aucotec.com/en/engineering-base
- SEE Electrical public product table via IB-CADDY: real-time lists, automatic references, cable definition, terminal plan/cable terminal row plan, function/location/product database aspects, cross-reference navigator, report generator, DXF/DWG/DXB/XML exports. URL: https://electrical.ib-caddy.com/en/products/
- AmpereSoft ProPlan official product page: object utilization, material database, DIN EN 81346 project structure, ProjectExplorer, terminal/cable/wire/PLC lists, cross-reference lists, potentials management, XML/AutomationML and DXF/DWG/PDF/Excel interfaces. URL: https://www.amperesoft.net/en/toolsystem/proplan

## Matrix by Workflow

### 1. Kontaktspiegel / Querverweise

- QET current state: [Certain] Master/slave relationships are implemented by `MasterElement`, `SlaveElement`, link UUIDs/group indexes, `ContactUsage`, and `CrossRefItem` rendering. `Brain.md` records that the contact mirror is not a standalone domain model; it is split across linked element state, group assignment, contact usage/capacity, and rendering.
- Reference principle: [Certain] EPLAN publicly states automated cross referencing and device-driven design. SEE Electrical lists automatic references, contact mirrors, contact workload checks, and a cross-reference navigator. AmpereSoft ProPlan lists cross-reference lists and object utilization across plan types. WSCAD release notes publicly mention master/slave symbol group copy behavior and contact comb cross-references.
- Transferable concept: Separate stable link identity from the rendered contact mirror; provide explicit validation for overloaded/missing contacts; expose navigable cross-reference data independent from graphics.
- Not transferable / unclear: Exact vendor contact-comb algorithms, UI layout rules, and proprietary project schemas are not public enough to copy.
- QET risk: Current behavior is hard to regression-test end-to-end because persistence, capacity logic, undo, and rendering are split.
- Open decision: Should QET introduce a dedicated contact/cross-reference domain model, or first harden the existing split model with tests only?
- Confidence: [Certain] for QET split and vendor cross-reference principle; [Unverified] for vendor internals.

### 2. Device / Function / Symbol / Article Separation

- QET current state: [Certain] Placed `Element` objects carry UUIDs, element metadata, dynamic texts, terminals, and link state; element definitions are XML `.elmt` assets; article fields exist primarily as element/project info and derived DB/export fields. There is no verified central article/device master model comparable to commercial part databases.
- Reference principle: [Certain] EPLAN emphasizes device-driven design and product data from the EPLAN Data Portal. Zuken states E3.series is database-driven/object-oriented with component libraries and design data management. AUCOTEC describes one object instance with diagrams, data sheets, and lists as different representations. ProPlan describes material database transfer and project-wide object management. SEE Electrical lists function/location/product database aspects and database list navigation.
- Transferable concept: Model device identity, symbol representation, function/location structure, and article/material data as related but distinct concerns.
- Not transferable / unclear: Vendor data portal schemas, subscription catalogs, and proprietary classifications cannot be assumed.
- QET risk: Article/report/export features may remain fragile if symbol placement stays the dominant source of truth.
- Open decision: What minimum open article/device schema is acceptable before importing manufacturer-like data?
- Confidence: [Certain] for reference principle; [Likely] for gap severity based on QET traces.

### 3. Klemmen / Kabel / Potentiale

- QET current state: [Certain] `Terminal` and `Conductor` form the graphical/live connection graph. `Conductor::relatedPotentialConductors()` supports potential traversal. `TerminalStrip` is a separate project-level model with real/physical terminals and bridges. Cable fields exist in `ConductorProperties`; no standalone verified cable object was found in earlier traces.
- Reference principle: [Certain] EPLAN publicly mentions component, device and cable data plus wire reports. WSCAD documents terminal charts, wire/cable manufacturing data, terminal/cable release fixes, and interfaces. Zuken has E3.cable/harness modules and database-driven checks. SEE Electrical lists cable management, multi-level terminals, terminal plans, cable plans, wiring lists, and bridge handling. ProPlan lists terminal/cable/wire lists, potential management, assignable terminal connections, terminal management, and cable/wiring plans.
- Transferable concept: Treat terminals, terminal strips, wires/conductors, cables/cores, and potentials as separate but cross-checkable data views.
- Not transferable / unclear: 3D routing, wire length calculation, manufacturing-machine interfaces, and vendor terminal-strip editors are product-specific.
- QET risk: Existing exports observe different sources of truth: live graph (`--export-nets`), derived SQLite (`--export-wiring`), XML-derived wiring list exporters, and terminal-strip XML. Divergence is likely without tests and a clearer data ownership rule.
- Open decision: Which object owns cable/core identity in QET: conductor properties, a future cable model, or terminal-strip/wiring database projection?
- Confidence: [Certain] for QET and reference presence; [Unverified] for exact bridge/electrical-potential semantics in vendors.

### 4. Project / Page Structure

- QET current state: [Certain] `QETProject` is the `.qet` aggregate; `Diagram` represents folios/pages; folio order and metadata feed DB/report-facing data. Project persistence is plain XML, not a zip/container in the traced path.
- Reference principle: [Certain] EPLAN describes project documentation and integration; WSCAD states standards-compliant project documentation and data compatibility across prior versions. AUCOTEC uses a central model with diagrams/data sheets/lists as representations. ProPlan explicitly supports arbitrary DIN EN 81346 project hierarchy in a tree linked to plan types. SEE Electrical lists project management and project tree configuration.
- Transferable concept: Preserve a navigable project tree with explicit function/location/product/page aspects rather than deriving all structure from drawing sheets.
- Not transferable / unclear: Exact project database layout, revision systems, and collaboration backends vary widely and are not public enough to reproduce.
- QET risk: Page order/title metadata is clear, but higher-level function/location/product hierarchy is not yet a verified first-class model.
- Open decision: Should function/location/product aspects be introduced as project structure first, or attached incrementally to existing element/folio metadata?
- Confidence: [Certain] for QET folios and public project-structure principles.

### 5. Numbering / Autonum

- QET current state: [Certain] `NumerotationContext`, `AssignVariables`, conductor/element/folio auto-numbering, `sequentialNumbers`, and undo commands implement numbering across project, diagram, element, conductor, and terminal contexts. Earlier traces found no broad autonum/undo regression coverage.
- Reference principle: [Certain] EPLAN publicly states automated numbering. SEE Electrical lists real-time component numbering, terminal/cable numbering, and user-defined marking methods. ProPlan supports externally supplied EQIDs/cable numbers via interfaces and DIN EN 81346 project structure. WSCAD emphasizes standards-compliant documentation and assignment of symbols/parts; release notes mention renumbering pages/elements. Zuken mentions automation and checks but public page details are less numbering-specific.
- Transferable concept: Numbering must be formula/config driven, scoped by function/location/page/wire type as needed, and auditable through undo/save/load/export.
- Not transferable / unclear: Vendor numbering languages and exact scoping rules are proprietary or insufficiently public.
- QET risk: Distributed counter state can drift between visible labels, XML sequence state, conductor potential propagation, and exports.
- Open decision: What numbering scopes are mandatory for MAM: page, function, location, potential, terminal strip, cable, wire type, or article group?
- Confidence: [Certain] for QET internals and SEE/EPLAN/ProPlan principles; [Likely] for WSCAD/Zuken numbering parallels.

### 6. Evaluations / Reports

- QET current state: [Certain] Reports/export surfaces include BOM export, conductor number export, wiring list export, live-net JSON, SQLite wiring view, and `--info`. The project database is derived and rebuilt, not the primary persistence model.
- Reference principle: [Certain] EPLAN publicly lists reports, wire lists, parts lists, automatic evaluations, and Excel export. WSCAD lists material lists, terminal diagrams, test reports, control cabinet documentation, manufacturing outputs, and one-click BOM/terminal chart workflows. Zuken states reports and manufacturing documentation. AUCOTEC describes bills of material and documents generated from the digital twin. SEE Electrical lists report generator, graphical lists, BOM/device/product/cable/terminal/wire lists. ProPlan lists parts, PLC, cross-reference, EQID, terminal, cable and wiring plans/lists.
- Transferable concept: Reports should be projections from one validated model and should cross-check against each other in tests.
- Not transferable / unclear: Exact report templates, report languages, and commercial output formats.
- QET risk: Multiple report generators may disagree because they read live graph, XML, or SQLite projections differently.
- Open decision: Which report fields are canonical, and which are derived/format-only?
- Confidence: [Certain].

### 7. Data Model / Project Database

- QET current state: [Certain] Runtime authority is Qt object graph (`QETProject`, `Diagram`, `Element`, `Terminal`, `Conductor`, `TerminalStrip`). Persistence is XML. SQLite project DB is a derived reporting/search/export projection rebuilt from project content.
- Reference principle: [Certain] AUCOTEC positions Engineering Base around one central data model/single source of truth. Zuken states intelligent database and object-oriented consistency. ProPlan states genuine object utilization and ProjectExplorer/material database. EPLAN and WSCAD public pages emphasize device/product data and integrations, but do not expose internal schemas.
- Transferable concept: A clear authoritative domain layer matters more than format choice; diagrams, reports, and lists should be representations of domain objects.
- Not transferable / unclear: AUCOTEC/Zuken internal databases and multi-user collaboration models cannot be inferred from marketing pages.
- QET risk: A graphics-scene-centric object graph plus derived DB is workable for drawing, but risky for professional CAE workflows if object identity and ownership remain implicit.
- Open decision: Should future work first define canonical domain objects, or wrap existing Qt objects with validated projections?
- Confidence: [Certain] for QET and public high-level principles; [Unverified] for vendor schemas.

### 8. Import / Export / Compatibility

- QET current state: [Certain] `.qet` is XML read/written by `QETProject`; CLI/export paths include resave, SVG/PDF-related export surfaces, nets JSON, wiring/cable/wire/BOM CSV and info. Compatibility has version attributes and selected fallback paths; old project compatibility remains partial per earlier trace.
- Reference principle: [Certain] EPLAN states Excel export and ERP/PDM integration. WSCAD lists compatibility with WSCAD/ELECTRIX versions, DXF/DWG, PDF, part repositories, PLC/manufacturing/ERP/PLM integrations. Zuken states PLM/PDM and MCAD integration plus manufacturing outputs. AUCOTEC states ERP/PLM/3D/tool integration. SEE Electrical lists DWG/DXF/DXB/XML exports and printer/bookmark outputs. ProPlan lists DXF/DWG/PDF/BMP/JPG/TIF/Excel/BMEcat/XML/AutomationML and ERP/PPS interfaces.
- Transferable concept: Define import/export contracts as compatibility surfaces with regression fixtures and normalized comparisons.
- Not transferable / unclear: Commercial exchange format mappings, EPLAN-native/ProPlan-native files, and manufacturer portal data licenses.
- QET risk: Compatibility work can accidentally become vendor cloning or fragile format chasing unless scoped to open formats and QET-owned schemas.
- Open decision: Which open formats matter first for MAM: QET XML stability, CSV reports, AutomationML, DXF/DWG, PDF/SVG, or ERP/PLM exchange?
- Confidence: [Certain] for public format categories; [Unverified] for any proprietary native-format compatibility.

## Cross-System Lessons Without Architecture Decision

- Commercial systems consistently market object/data-driven workflows where symbols, pages, terminal plans, reports, and manufacturing outputs are representations of shared engineering data.
- QET already has important primitives: stable UUIDs, XML persistence, link commands, terminal strips, potential traversal, auto-numbering contexts, derived database, and CLI exports.
- QET's main gap is not the absence of every feature; it is unclear authority across representations: graphics scene, XML, derived SQLite, terminal-strip domain objects, and report/export code.
- A safe next analysis step is to pick one workflow and define evidence-only decision criteria, not implementation: either contact/cross-reference ownership or cable/core/potential ownership.

## Unverified / Do Not Assume

- Do not assume exact EPLAN, WSCAD, E3.series, Engineering Base, SEE Electrical, or ProPlan internal database schemas.
- Do not assume license permission to import vendor macros, device portals, native project files, or proprietary templates.
- Do not assume WSCAD AI claims are relevant to QET architecture beyond public workflow categories.
- ProPlan/Moeller lineage: AmpereSoft ProPlan is publicly documented and appears to continue the ProPlan product family; older Moeller/Eaton-specific ProPlan details are not sufficiently public for detailed workflow claims in this matrix.
