# Brain

## Project Vision
Build a professional, data-driven electrical CAE system using QElectroTech as the technical starting point, without cloning one specific commercial CAE product.

## CAE Reference Matrix
Phase 1 CAE reference analysis is documented in `CAE_Reference_Matrix.md`.

Durable evidence:
- Public vendor sources were limited to legitimate product/help/release pages from EPLAN, WSCAD, Zuken, AUCOTEC, SEE Electrical public product information, and AmpereSoft ProPlan.
- Across the reference systems, the recurring professional CAE principle is data/object-driven engineering: device/product data, project structure, terminals/cables/wires/potentials, cross-references, reports, and manufacturing outputs are treated as coordinated views of engineering data.
- QET already has relevant primitives (`QETProject`, `Diagram`, `Element`, `MasterElement`/`SlaveElement`, `CrossRefItem`, `Terminal`, `Conductor`, `TerminalStrip`, autonum contexts, derived project database, CLI exports), but earlier traces show unclear authority across graphics-scene objects, XML, derived SQLite, terminal-strip domain objects, and report/export code.
- The matrix intentionally does not select a target architecture. It identifies two high-value next decision-analysis areas: contact/cross-reference ownership and cable/core/potential ownership.

Important unverified boundaries:
- Vendor internal schemas, native project formats, contact-comb algorithms, report template languages, and data portal licenses are not public enough to treat as implementation facts.
- ProPlan/Moeller details beyond public AmpereSoft ProPlan documentation remain unverified.

## Contact / CrossRef Ownership Decision Prep
Decision-prep analysis is documented in `Decision_Contact_CrossRef_Ownership.md`.

Durable findings:
- QET contact/cross-reference ownership is currently distributed: `ElementData` owns definition metadata and contact group declarations; placed `Element`/`MasterElement` objects own runtime links and `group_index`; `LinkElementCommand` owns mutation/undo side effects; `ContactUsage` provides graphics-free counting; `CrossRefItem` owns rendering/navigation; XML persists link UUIDs and optional group index.
- `--export-links` is useful for link/unresolved-state evidence but does not expose contact capacity, group labels, or complete contact assignment facts.
- A later architecture decision should separate presentation from authoritative assignment data, but current evidence does not justify a direct persistence rewrite yet.

Recommendation recorded as non-binding: stabilize status quo with tests first, then explore a read-only Contact/CrossRef projection/service over the existing model before considering a dedicated `ContactAssignment` domain model.

## Master / Slave Link Stabilization
Test-only stabilization slice, corrected after review:
- `tests/qttest/tst_master_slave_links.cpp` uses the real QET binary with `QT_QPA_PLATFORM=offscreen`, `QTemporaryDir`, `--resave`, and `--export-links`.
- No existing library element/example with `slaveContactGroups` was found. The fixture is therefore test-only and constructed from the QET `ElementData` parser/writer shape for `<slaveContactGroups><group ...><label>...`.
- `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` embeds a minimal master with two real `slaveContactGroups` (NO and NC) plus matching slave elements.
- Stable asserted facts: master UUID `{11111111-1111-4111-8111-111111111111}` keeps slave link UUIDs with group indices `0` and `1` after `--resave`; `--export-links` reports the `KMS` master row as linked and no unresolved link status.
- The test intentionally does not assert CrossRef rendering or GUI geometry.

Open risk:
- The fixture has no pre-existing library template because no current element definition with `slaveContactGroups` was found; its XML shape is based on the QET code parser/writer, and load/resave/export behavior is verified by the test.

## Contact / CrossRef Read-only Service Spec
Non-implementing specification is documented in `Spec_Contact_CrossRef_ReadOnly_Service.md`.

Durable points:
- The proposed service is a projection, not an ownership change. Current leading sources stay `Element` links, `ElementData`, `group_index`, XML link persistence, `ContactUsage`, and current project state.
- `CrossRefItem` remains presentation/navigation, not authoritative assignment data.
- Device/Function/Article ownership remains out of scope.
- No migration is needed for a read-only projection; persistence changes such as assignment UUIDs or explicit group IDs require a later decision.
- Review result: specification is consistent with `Decision_Contact_CrossRef_Ownership.md` Option B, the CAE matrix's projection principle, and current QET code findings. No blocking findings were identified.

Prototype outcome:
- The narrow read-only projection implementation was later approved and implemented as `sources/contactcrossrefprojectionservice.*` with direct QtTest coverage.
- The approved prototype boundary remained narrow: no XML schema changes, no ownership/persistence migration, no `LinkElementCommand` behavior change, no `CrossRefItem` rendering change, no CLI/export change, no Device/Function/Article model change, and no fixture/library changes outside tests.
- A later P1 correction added `Element::linkedElementsReadOnly() const` so the service no longer has to call the sorting/mutating `Element::linkedElements()` accessor.

## Core Principles
- Baseline first: the unchanged QElectroTech fork must build and start before analysis or development work begins.
- No source changes during the baseline phase.
- Project decisions with several plausible technical paths require explicit user approval.
- Repository files are the source of truth for project state.

## Architecture
No accepted target architecture rewrite exists. The accepted course is incremental: QElectroTech remains the chassis and new CAE logic starts as reviewed read-only projection services over existing state. The CAE core / representation split described in local `Handout.md` remains a working hypothesis, not a committed target architecture.

## Local Project Inputs
- `Handout.md` is intentionally ignored via `.gitignore` and remains local/untracked project guidance. Brain/Progress may record decisions derived from it, but the file itself is not part of a commit slice unless the user later explicitly requests a versioned/redacted equivalent.

## QElectroTech Architecture Findings
Initial repository metadata identifies QElectroTech as a Qt 6 / CMake C++17 desktop CAD/CAE editor focused on schematic drawing.

Initial source survey findings:
- `QETProject` is the top-level `.qet` aggregate. The class comment says a project is an XML document containing 0..n diagrams plus an embedded element collection; it owns diagrams, defaults, project properties, title blocks, auto-numbering contexts, a project database, a project undo stack, and terminal strips.
- `Diagram` is a `QGraphicsScene` and is the folio/page model. It manages child elements, conductors, texts, images, shapes, terminal-strip graphics, border/titleblock state, display options, guide data, folio sequential counters, and scene event handling.
- `Element` is the base graphics item for electrical symbols. It combines catalog location, rendered picture/terminals/texts, UUID, information fields, auto-numbering state, and link state.
- Element link categories currently include simple, next/previous/all report, master, slave, terminal, thumbnail, and conductor definition. The same conceptual categories appear both in `Element::kind` and `ElementData::Type`.
- `ElementData` stores typed symbol/device metadata parsed from element definitions, including master type, slave type/state, terminal type/function, PLC IO data, and optional slave contact groups.
- `Conductor` is the wire object between two `Terminal` instances. It owns conductor properties, path/profile data, text item, UUID, sequence number, and potential traversal through `relatedPotentialConductors()`.
- `Terminal` is a graphics object attached to an element and is the conductor plug point. It tracks connected conductors, terminal identity, label/name, docking point, orientation, and optional master-label behavior.
- Master/slave linking is runtime object linking by UUID. `Element::toXml()` persists linked element UUIDs and optional `group_index`; `Element::initLink()` resolves pending UUID links after project load.
- `MasterElement` owns current linked slave contacts and computes `contactUsage()` and `contactCapacity()` from slave element data and declared contact groups. This is the current closest code point to a contact mirror/contact capacity source of truth.
- `CrossRefItem` renders master cross-reference/contact/PLC table visuals from linked elements and project `XRefProperties`; it is a graphics representation, not an independent data model.
- The project database is an in-memory SQLite wrapper rebuilt from live project objects. It tracks diagrams, diagram info, elements, element info, terminals, and conductors, then exposes read-only views/queries for nomenclature, summaries, and wiring lists.
- Terminal strips are separate project-owned domain objects (`TerminalStrip`) with data, physical terminals, real terminals, grouping/levels, and bridges; they are serialized under project XML, not just normal diagram items.
- Export/report logic is split: BOM and wiring read database views, while net export walks conductor potentials directly from live objects. This gives two observable sources for connectivity/report consistency.

## Data Model
The current data model is hybrid:
- XML is the persisted project/source format (`QETProject::toXml/readProjectXml`, `Diagram::toXml/fromXml`, `Element::toXml/fromXml`, `Conductor::toXml/fromXml`, terminal-strip XML).
- Live Qt objects are the authoritative runtime graph (`QETProject`, `Diagram`, `Element`, `Terminal`, `Conductor`, `TerminalStrip`).
- SQLite is a derived project database for reporting/querying, not the primary persistence model.

## Device Model
No separate device/domain aggregate was found in this first pass. Device-like data is distributed across `Element`, `ElementData`, `DiagramContext` element information fields, master/slave links, terminal elements, and terminal-strip objects.

## Function Model
Function semantics are partially represented by element categories (`ElementData::Type` / `Element::kind`), master/slave/report links, element information keys, and PLC IO metadata. A separate function object/model was not identified in the initial survey.

## Symbol Model
Symbols are centered on element definitions and placed `Element` instances. `ElementData::fromXml()` reads metadata from catalog definition XML, while placed `Element::fromXml()` persists placement, UUID, terminal mappings, dynamic texts, element information overrides, links, and PLC overrides.

## Connection Model
Connections are conductor-to-terminal edges. `Conductor` connects exactly two `Terminal` objects and `Terminal` maintains its conductor list. Potentials/nets are computed by graph traversal via `Conductor::relatedPotentialConductors()` and related terminal helpers.

## Page Model
Pages/folios are `Diagram` instances owned by `QETProject`. Folio order is project list order; `QETProject::folioIndex()` and diagram order change handling update database/report-facing folio data.

## Cross Reference Model
Cross references are currently link-driven:
- Element links are persisted as linked UUIDs and resolved after load.
- `MasterElement`, `SlaveElement`, and `ReportElement` implement link semantics.
- `CrossRefItem` paints cross-reference/contact/PLC visuals.
- `XRefProperties` are project defaults serialized under `newdiagrams/xrefs`.

Phase 2 Master/Slave/Kontaktspiegel trace:
- Element definition metadata starts in `.elmt` `<definition link_type="master|slave">` and `<kindInformations>`. Examples found in `elements/10_electric/20_manufacturers_articles/loxone/...`: masters declare `kindInformation name="type">coil</...`; slaves declare `type`, `state`, and `number`.
- `ElementData::fromXml()` reads definition metadata from `<definition type="element">`; `kindInfoFromXml()` maps master type, slave type/state/contact count, terminal type/function, optional `slaveContactGroups`, and PLC master data.
- Placed diagram elements are serialized by `Element::toXml()` with placement, UUID, terminals, dynamic texts, element informations, and `links_uuids/link_uuid`. `group_index` is persisted when a linked slave is assigned to a master contact/PLC group.
- On load, `Element::fromXml()` stores linked UUIDs in `tmp_uuids_link`; `Element::initLink()` resolves them through `ElementProvider` and restores `group_index` into `m_group_index_map`.
- User/UI linking path goes through `LinkSingleElementWidget`, optional `ContactGroupSelectionDialog`, and `LinkElementCommand`. The command enforces master/slave compatibility, PLC/non-PLC compatibility, undo/redo, group index assignment, terminal-label propagation, and PLC variable propagation onto slaves.
- `MasterElement::linkToElement()` appends slave elements, triggers the slave reciprocal link, creates a `CrossRefItem` when needed, and emits `linkedElementChanged()`.
- `MasterElement::contactUsage()` counts linked slaves by `ElementData::SlaveState` and `m_contact_count`; `contactCapacity()` sums declared `ElementData::SlaveContactGroup` capacities. Both use `ContactUsage`, which deliberately counts contact quantity and treats changeover contacts as one `SW`, not one NO plus one NC.
- `MasterElement::isFull()` is slot-oriented: if contact groups exist, group count limits linked slaves; otherwise it falls back to `max_slaves`. This is not the same number as contact capacity.
- `CrossRefItem` observes project XRef changes, project diagram order/removal, element link changes, and selected diagram info changes. `updateLabel()` recalculates geometry and clickable maps; `paint()` draws either cross or contact-style references. For PLC masters, `CrossRefItem` builds the clickable map, while PLC table drawing is also present in `Element` painting paths.
- `CrossRefItem::NOElements()` and `NCElements()` still classify display columns from `kindInformations()` strings and include `SW` in both visual columns; that is why `ContactUsage` exists separately to avoid double-counting switch contacts for usage/capacity logic.
- Existing automated coverage is narrow: `tests/qttest/tst_contactusage.cpp` covers `ContactUsage` rules only. No first-pass test was found for full XML save/load of `group_index`, `LinkElementCommand` undo/redo, `CrossRefItem` rendering/click-map correctness, or end-to-end master/slave persistence.

## Contact Mirror Model
There is no standalone contact mirror model in the first-pass findings. Existing contact mirror behavior appears embedded in `MasterElement::contactUsage()`, `MasterElement::contactCapacity()`, `ElementData::SlaveContactGroup`, and `CrossRefItem` rendering. This area needs a deeper dedicated pass.

Phase 2 refinement:
- The current "Kontaktspiegel" is not a single model. It is a combination of linked element state (`connected_elements`), group assignment (`m_group_index_map`), usage/capacity tally (`ContactUsage` via `MasterElement`), and visual rendering (`CrossRefItem`).
- Capacity and "full" are distinct concepts in current code: capacity counts contacts, while `isFull()` counts occupied link/group slots.
- Display and domain logic diverge for changeover contacts: visual cross references put `SW` contacts in both NO and NC columns, while usage counts them once.

## Terminal / Cable Model
Terminals exist at two levels:
- `Terminal` is the graphical/electrical connection point on an `Element`.
- `TerminalStrip` models terminal strips separately with physical/real terminals, grouping, ordering, levels, and bridge operations.
Cable-specific domain objects were not identified in this first pass; CLI export refers to cable/wiring-style outputs, but cable modeling needs deeper investigation.

Phase 3 Terminal/Kabel/Potential trace:
- Graphical terminals are `Terminal` graphics objects attached to an `Element`. They own the live list of attached `Conductor` objects, expose `uuid()`, `stableUuid()`, `name()`, `baseName()`, `terminalType()`, `dockConductor()`, `addConductor()`, `removeConductor()`, `isLinkedTo()`, and `canBeLinkedTo()` in `sources/qetgraphicsitem/terminal.h/.cpp`.
- `Terminal::stableUuid()` is the important reporting identity fallback: if a catalog terminal has no UUID, it derives a reproducible UUID v5 from local terminal position and orientation, explicitly to support persistence/reporting of older element definitions.
- `Conductor` is the live wire between two `Terminal *` endpoints. Its constructor adds itself to both terminals; the destructor removes itself. `Conductor::toXml()` stores conductor UUID, endpoint element UUIDs/labels/names, terminal UUIDs/names, path/sequence/properties; `fromXml()` reloads UUID/properties/path and preserves legacy behavior for files without conductor UUIDs.
- Potential traversal is live-object based. `Conductor::relatedPotentialConductors()` recursively walks conductors attached to both endpoints and asks `relatedPotentialTerminal()` for additional equipotential terminals. `relatedPotentialTerminal()` bridges folio reports through linked report elements and terminal elements through their other terminals, unless `potential_isolating=true` is set on the terminal element.
- Terminal strips are a separate project-level domain model. `TerminalStrip` owns `PhysicalTerminal` objects and `TerminalStripBridge` objects; each `PhysicalTerminal` is composed of one or more `RealTerminal`s. A `RealTerminal` can wrap a placed terminal `Element`, persists only `element_uuid`, and exposes label, cross-reference, conductor text, type/function/LED helpers.
- Terminal strip XML is project-level, not diagram-level: `QETProject::toXml()` writes `<terminal_strips>` from `m_terminal_strip_vector`, and load reads that section back into `TerminalStrip` objects. Terminal strip graphics items are separate diagram items persisted by `TerminalStripItemXml` with strip UUID and item position.
- Terminal strip load resolves saved `real_terminal element_uuid` values through `ElementProvider::freeTerminal()`, so strip membership is linked to terminal element instances, not directly to arbitrary conductor endpoint terminals.
- Terminal strip undo coverage exists as command classes under `sources/TerminalStrip/UndoCommand/`: add strip, add terminal to strip, bridge terminals, change terminal level, change strip color/data, group terminals, and sort strip. Conductor text/property changes use `QPropertyUndoCommand` in `Conductor::displayedTextChanged()`, and terminal-strip operations have their own command family.
- The cable model is currently field-level, not a dedicated domain object in the traced files. `ConductorProperties::toXml()` and `fromXml()` persist `cable` and `bus` attributes, but `ConductorPropertiesWidget` disables the cable and bus line edits and carries a TODO "Add Kabel and Bus to qet". `RealTerminal::cable()` and `RealTerminal::cableWire()` currently return empty strings.
- Wiring exports have multiple sources of truth. `--export-wiring` queries the derived SQLite `wiring_list_view` after forcing `project.dataBase()->updateDB()`. `--export-nets` walks live conductors and `relatedPotentialConductors(true, &t_list)` to produce potential/net JSON. `--export-cables`/`--export-wires` use `WiringListExport`, which parses project XML conductors, filters conductor-definition placeholder elements, merges linked placeholder halves, and writes from/to CSV fields.
- `projectDataBase::addConductor()` inserts both endpoint terminals first, watches `Conductor::propertiesChange`, and writes conductor UUID, diagram UUID, stable endpoint terminal UUIDs, endpoint element UUIDs, and wire text. `projectDataBase::updateConductor()` updates only the mutable text column. `wiring_list_view` joins conductor rows to terminal rows and left-joins labels/diagram position to avoid silently dropping wires.
- UI wiring list (`WiringListDialog`) also reads `wiring_list_view`, refreshes the database before querying, and warns when conductors are excluded because an endpoint is not attached to an element.
- Existing automated coverage found for this trace is narrow. `tests/qttest/tst_conductorselfretrace.cpp` covers conductor self-retrace/path behavior. Test fixtures include conductor XML with endpoint UUIDs and cable/bus attributes. No first-pass test was found for terminal strip XML round-trip, potential traversal across terminal strips/reports/isolating terminals, wiring/net export equivalence, cable fields, or terminal-strip undo/redo.

Phase 3 interpretation boundaries:
- Belegt: conductors connect graphical terminals; potential traversal crosses report links and terminal elements; terminal strips persist as project objects pointing at terminal elements; cable/bus are persisted conductor properties but UI/domain use is incomplete.
- Nicht belegt: a canonical standalone cable entity, cable-wire assignment logic, terminal-strip bridge participation in potential traversal beyond terminal-element behavior, or end-to-end tests for exports and strip persistence.
- Architekturfrage: should terminal strips be the electrical source of truth, a report/layout layer over terminal elements, or a separate domain layer synchronized with graphical terminal elements and conductors?

## Article Model
Article/order data appears stored as element information fields rather than as a dedicated article entity. `BomExport` reads fields such as designation, manufacturer, manufacturer reference, model, quantity, voltage/current rating, and notes from `element_nomenclature_view`.

## Numbering / Autonum / Conductor Text Model
Phase 4 Nummerierung/Autonum/Leitertext trace:
- Project-level autonum schemas are `NumerotationContext` values stored in `QETProject` hashes for conductors, elements, and folios. Accessors include `conductorAutoNum()`, `elementAutoNum()`, `folioAutoNum()`, current-autonum names, and formula helpers in `sources/qetproject.cpp`.
- `NumerotationContext` is an ordered list of parts serialized as `<part type value increase initialvalue modulus format>`. Accepted part types include `unit`, `unitfolio`, `ten`, `tenfolio`, `hundred`, `hundredfolio`, `wrap`, `alpha`, `string`, `idfolio`, `folio`, `plant`, `locmach`, `elementline`, `elementcolumn`, and `elementprefix`.
- `NumerotationContextCommands::next()/previous()` advances counters by part strategy and implements wrap-and-carry/borrow behavior. This is the counter engine used by both conductor and element autonum paths.
- `autonum::AssignVariables::formulaToLabel()` is the formula resolver. It replaces folio/titleblock/project variables, element position/prefix variables, conductor variables `%wf/%wv/%wc/%ws`, and sequence placeholders such as `%sequ_`, `%seqw_`, `%seqt_`, `%seqh_`, `%sequf_`, `%seqtf_`, `%seqhf_`, `%seqa_`; it also writes updated sequence state back through the `sequentialNumbers&` reference.
- Per-object sequence state is `autonum::sequentialNumbers`. It persists as `<sequentialNumbers>` for both `Element` and `Conductor`; legacy attribute-based sequence imports still exist for backwards compatibility.
- Per-folio sequence maxima are cached on `Diagram` in hashes such as `m_elmt_unitfolio_max` and `m_cnd_unitfolio_max`. `Diagram::loadElmtFolioSeq()` and `loadCndFolioSeq()` synchronize folio-specific counters back into the current project `NumerotationContext`.
- Diagram-level conductor defaults are stored in `Diagram::defaultConductorProperties`; diagram-level selected conductor autonum name is `Diagram::m_conductors_autonum_name`, serialized as the `conductorAutonum` diagram attribute and edited through `DiagramPropertiesDialog`.
- `ConductorAutoNumerotation` is the main conductor numbering workflow. For a new conductor, it reads the diagram's selected conductor autonum, converts context to formula, stores it in `ConductorProperties::m_formula`, initializes the conductor's `sequentialNumbers`, advances the project context with `NumerotationContextCommands::next()`, wraps that counter change in `SetAutoNumContextCommand`, then applies the resolved text through `AssignVariables::formulaToLabel()`.
- Conductor potential propagation is explicit. `ConductorAutoNumerotation::applyText()` creates `QPropertyUndoCommand`s for the target conductor and all conductors in `relatedPotentialConductors()`. `Conductor::displayedTextChanged()` also propagates edited text/formula across the same potential through one undo macro. `Conductor::setPropertyToPotential()` can propagate either full properties or text/formula/function/tension/color/section only.
- `Conductor::setProperties()` recalculates `properties().text` from `m_formula` via `AssignVariables::formulaToLabel()` whenever a formula exists, updates the visible text item, and emits `propertiesChange()`.
- Database update path is signal-based. `projectDataBase::watchConductor()` connects `Conductor::propertiesChange` to `conductorPropertiesChanged()`, and `updateConductor()` updates only the `conductor.text` column. `wiring_list_view` therefore receives wire-number changes through this mutable text column.
- Element autonum uses the same core pieces. `Element::setUpFormula()` reads the current project element formula/context, stores `formula` in element informations, clears and initializes the element `sequentialNumbers`, advances the project element context, and `Element::actualLabel()` resolves the formula on demand with `AssignVariables::formulaToLabel()`.
- Terminal numbering is separate from `NumerotationContext`. `TerminalNumberingDialog` scans placed terminal elements, groups by label prefix, sorts by prefix/folio/coordinates, generates suffix counters, and returns a macro of `ChangeElementInformationCommand`s that update terminal element `label` fields. `QETDiagramEditor::slot_terminalNumbering()` pushes that macro onto the active undo stack.
- Folio autonum is project-level context too; titleblock/default folio integration is visible in the autonumbering dock and project XML, but detailed folio renumbering UI was not fully traced in this bounded pass.
- Export impact is split: `--export-wiring` reads `wire_number` from SQLite `wiring_list_view`; `--export-nets` chooses the smallest non-empty `ConductorProperties::text` in a potential as `wire_no`; `ConductorNumExport` counts visible conductor text items excluding report-to-report conductors; `WiringListExport` exports terminal names and conductor attributes such as tension/color/section/function, but not the conductor number itself in its current CSV fields.
- XML persistence is broad: project autonum contexts persist under `conductors_autonums`, `folio_autonums`, and `element_autonums`; diagrams persist selected conductor autonum/default conductor properties; conductors/elements persist their own sequence state and conductor properties/text/formula.
- Existing automated coverage found in this pass is weak for autonum behavior. Fixtures contain autonum sections and `<sequentialNumbers/>`, but no first-pass test was found for `NumerotationContextCommands`, `AssignVariables::formulaToLabel`, `ConductorAutoNumerotation` undo/counter burn prevention, potential propagation, terminal numbering, or export effects.

Phase 4 interpretation boundaries:
- Belegt: autonum schemas live in project contexts; object-level sequence state lives on elements/conductors; conductor text is recalculated from formula and propagated across potentials; project DB wire numbers are refreshed from `propertiesChange`; exports consume the resolved text, not the raw formula.
- Nicht belegt: one canonical numbering domain model. Numbering behavior is spread across project contexts, diagram selected names and folio hashes, element/conductor sequence structs, UI dialogs, undo commands, and export/database readers.
- Architekturfrage: should future numbering be centralized as a service/domain layer, or remain distributed across element/conductor/diagram/project objects with tests guarding the current behavior?

## XML / File Format Compatibility / Load-Save Lifecycle
Phase 5 XML-/Dateiformat-Kompatibilitaet und Projekt-Load/Save trace:
- `.qet` project files are plain UTF-8 XML, not a zip/container format in the traced code path. `QETProject::openFile()` reads all bytes from the `.qet` path, parses them with `QDomDocument::setContent()`, and `QETProject::write()` writes `QETProject::toXml()` through `QET::writeXmlFile()`, which uses `QSaveFile`, UTF-8, no BOM, and `toString(4)`.
- The project root is `<project>`. `QETProject::toXml()` writes version, title, project UUID, sorted embedded titleblock templates, project properties, local usage tracking, `<newdiagrams>`, each `<diagram order=...>`, project-level `<terminal_strips>`, then the embedded `<collection>`.
- Project versioning is attribute-based. `QetVersion::toXmlAttribute()` writes the current version; `QetVersion::fromXmlAttribute()` reads it. `QETProject::readProjectXml()` warns on future versions and warns specially for projects at or below QET 0.6 because compatibility is only partial after QET 0.9.
- Project UUID compatibility: when a root project UUID exists, `openFile()` restores it before building content. If no project UUID exists, it derives one from the raw file bytes so legacy files get a stable project identity across repeated loads of the same bytes.
- Project load order is explicit: after version/title/defaults/properties are processed, `readProjectXml()` blocks database signals and full rebuilds, then loads embedded elements collection, diagrams, terminal strips, refreshes content, unblocks the database, and runs one final `m_data_base.updateDB()`.
- Diagram load/save is also explicit. `Diagram::toXml()` writes diagram UUID, titleblock/border, default conductor properties, selected conductor autonum, freeze flags, element/conductor folio sequential caches, elements, conductors, independent text, images, shapes, tables, and terminal-strip graphics items. `Diagram::fromXml()` restores diagram UUID or derives a deterministic fallback for missing/duplicate UUIDs, then loads default conductor/autonum data, elements, graphics items, terminal strip graphics items, and finally conductors.
- Element load compatibility includes legacy terminal IDs. `Element::fromXml()` maps XML terminal IDs to runtime `Terminal*` objects, rejects conflicts, loads linked element UUIDs/group indices, loads or creates element UUID, and imports old sequence attributes if present before falling back to `<sequentialNumbers>`.
- Conductor compatibility has two endpoint modes. Modern save writes endpoint element UUIDs plus terminal UUIDs/names. If a terminal has no UUID, `Conductor::toXml()` writes legacy integer terminal IDs from the element-order address table. `Conductor::fromXml()` generates a runtime conductor UUID if none is present and deliberately does not persist that generated UUID on resave, to avoid legacy files changing on every load/save.
- Terminal strip compatibility is project-level plus diagram-level. Project `<terminal_strips>` deserialize into `TerminalStrip` domain objects after diagrams are loaded; each `RealTerminal` resolves by terminal element UUID. Diagram-level `TerminalStripItemXml` stores strip UUID plus graphics position and can keep a pending strip UUID until the project strip is available.
- Autonum compatibility is split between project defaults, diagram folio sequential caches, and per-object sequence state. Project `NumerotationContext` XML stores named conductor/folio/element contexts; `sequentialNumbers` exists on elements/conductors; old sequence attributes are still read by element/conductor retro-compatibility paths.
- Save determinism is actively guarded in several places. Project save sorts titleblock template names, default XRef keys, and conductor/folio/element autonum keys because they come from hash maps. Diagram save stable-sorts elements and conductors by content-derived position/terminal keys because scene item order and UUID fallback behavior can otherwise change across runs.
- The deterministic conductor sort deliberately avoids conductor UUID as primary key because legacy conductors often lack persisted UUIDs. It uses canonicalized endpoint terminal sort keys derived from parent element position and terminal position.
- Database rebuild timing is lazy and explicitly controlled. `projectDataBase::updateDB()` no-ops while update-blocked, skips full repopulation if content is unchanged, otherwise repopulates diagram, diagram info, element, element info, and conductor/terminal tables, then emits `dataBaseUpdated()`. `QETProject::readProjectXml()` blocks rebuilds during bulk load and performs one rebuild after all content is loaded/refreshed.
- Database schema is derived, not a persisted project file section. It is an in-memory SQLite database keyed by project UUID connection name and built from loaded objects. The terminal table uses `(terminal stable UUID, element UUID)` because terminal catalog UUIDs are not per-instance.
- Export equivalence currently has multiple paths. `--export-resave` writes `project.toXml()` for round-trip diffs. `--export-wiring` reads the derived SQLite `wiring_list_view`. `--export-nets` traverses the live conductor/potential graph. `WiringListExport` parses project XML directly for cable/wire style CSV. These are useful cross-check surfaces, but no single code path proves equivalence between them.
- Existing test coverage is partial. `tst_diagramsortkeys.cpp` covers numeric ordering of diagram sort keys. `tst_conductorselfretrace.cpp` loads a `.qet` fixture through the application binary and exports SVG to catch a conductor path regression. The fixture includes project XML, embedded collection, conductor endpoint UUIDs, and empty autonum sections. No first-pass tests were found for full project XML round-trip determinism, legacy terminal-id conductor loading, project/diagram UUID fallback stability, terminal-strip project+item round-trip, or DB/export equivalence.

Phase 5 interpretation boundaries:
- Belegt: `.qet` is XML; load order is project defaults/properties -> embedded collection -> diagrams -> terminal strips -> refresh -> DB rebuild; save order is deterministic in several hash/scene-sensitive areas; database is derived and rebuilt after load; exports read from different representations.
- Nicht belegt: a single canonical project-domain model independent of XML, graphics scene, and derived DB. The current lifecycle relies on object construction side effects and final database rebuild.
- Architekturfrage: should future changes establish one canonical round-trip/equivalence test suite before altering load/save, DB, exports, terminal strips, or autonum behavior?

## Regression Test Map
Phase 6 evidence-based regression-test map:
- Test infrastructure is enabled by default through top-level `PACKAGE_TESTS` and `add_subdirectory(tests)`. `tests/CMakeLists.txt` wires Catch, GoogleTest, GoogleMock, modal quit regression, and QtTest. The relevant maintained coverage for the traced workflows is mostly under `tests/qttest`.
- `tests/qttest/CMakeLists.txt` registers focused Qt tests and one real-binary integration-style test. Header-only tests exist for `DiagramSortKeys` and `ContactUsage`; `tst_conductorselfretrace` depends on the built `qelectrotech` target and drives `--export-svg` against a `.qet` fixture with `QT_QPA_PLATFORM=offscreen`.
- Existing Master/Slave/Kontaktspiegel coverage is narrow. `tst_contactusage.cpp` verifies `ContactUsage` counting and capacity rules, including multi-contact slaves, changeovers, missing/invalid declared counts, and a realistic mix. It does not instantiate `Element`, `MasterElement`, `SlaveElement`, `LinkElementCommand`, `CrossRefItem`, PDF links, XML links, or undo/redo.
- Current CLI hooks useful for later Master/Slave tests: `--export-links` reports linkable elements, linked names, folio, and unresolved state; `--export-pdf` calls cross-reference PDF link injection; `--resave` exposes persisted XML. These hooks can verify link persistence and unresolved state, but do not prove contact mirror drawing unless paired with PDF/SVG assertions or direct renderer tests.
- Terminal/Potential/Wiring coverage is partial. `tst_conductorselfretrace.cpp` loads `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, exports SVG, and scans conductor path data for self-retracing. This is a real binary/export regression, but it covers geometry only, not electrical net correctness, terminal strips, bridges, wiring CSV, or DB equivalence.
- Existing terminal/potential CLI hooks are strong: `--export-nets` walks the live potential graph using `Conductor::relatedPotentialConductors(true, &t_list)`; `--export-wiring` reads SQLite `wiring_list_view`; `--export-cables` uses `WiringListExport` over `project.toXml()`; `--export-wires` uses `ConductorNumExport`. These are ideal future equivalence checks because they observe different representations of the same project.
- TerminalStrip coverage was not found in current tests. The source has project-level `TerminalStrip::toXml/fromXml`, diagram-level `TerminalStripItemXml`, real/physical terminal grouping, bridges, sorting, and terminal-strip undo commands, but no test currently exercises strip add/group/bridge/round-trip/export behavior.
- Autonum/Undo coverage was not found in current tests. Phase 4 identified `NumerotationContextCommands`, `AssignVariables`, `ConductorAutoNumerotation`, `SetAutoNumContextCommand`, `ChangeElementInformationCommand`, and `QPropertyUndoCommand` as relevant paths, but no existing test covers counter advance/rollback, visible label persistence, conductor text propagation, or undo/redo after save/load.
- XML round-trip/legacy coverage is indirect. `tst_diagramsortkeys.cpp` guards numeric ordering helper behavior used for deterministic diagram save ordering. `tst_conductorselfretrace` uses a real `.qet` fixture, and the fixture includes modern conductor endpoint UUIDs plus conductor properties, but no test asserts whole-project resave determinism, legacy terminal-id conductor loading, missing UUID fallback stability, terminal-strip round-trip, or legacy autonum import.
- DB/Export coverage is narrow. `tst_smart_device.cpp` validates BOM default columns, SQLite query filtering, CSV quoting/UTF-8 BOM, ordering, and atomic write using an in-memory schema. It does not load a `.qet` project or verify that `projectDataBase::updateDB()` populates views equivalently to live objects or XML parsing.
- IPC/modal regression tests exist but are not directly part of the high-risk CAE workflows. `tests/modal-quit-regression` is CTest-registered only on non-Apple Unix and needs gdb/Python/symbols. `tests/ipc-regression` is documented but not registered in CTest; it needs Xvfb, openbox, xdotool, Qt 6, Debug build, and a large project.
- Cable/conductor field coverage is mostly absent. `ConductorProperties` persists `cable`, `conductor_color`, and `conductor_section`; `WiringListExport` reads/merges these XML attributes for cable CSV. The current fixture contains empty values for these fields, so it does not protect non-empty cable/color/section behavior.

Phase 6 workflow map:
- Master/Slave/Kontaktspiegel: existing coverage = `ContactUsage` unit rules only. Missing = element definition metadata -> placed element links -> `LinkElementCommand` undo/redo -> `group_index` persistence -> `MasterElement::contactUsage/contactCapacity` from real linked elements -> `CrossRefItem` rendering/PDF/SVG evidence -> `--export-links` unresolved state. Minimal future tests = one small fixture with a master coil, NO/NC/SW slaves including grouped contacts, saved/resaved XML link/group checks, undo/redo link command checks, and a CLI `--export-links` assertion.
- Terminal/Potential/Wiring: existing coverage = conductor SVG path self-retrace only. Missing = terminal identity/stable UUID, report/terminal traversal, isolating terminals, terminal strips and bridges, live-net JSON, SQLite wiring CSV, XML cable CSV equivalence. Minimal future tests = fixture exporting `--export-nets`, `--export-wiring`, `--export-cables`, and `--export-wires`, with assertions that the same conductors/terminals/wire numbers appear consistently.
- Autonum/Undo: existing coverage = none found. Missing = project default autonum schemas, formula resolution, counter state, label/text assignment, conductor potential propagation, command undo/redo, XML persistence, DB/export reflection. Minimal future tests = direct QtTest for `NumerotationContextCommands` plus project-level fixture exercising one element number and one conductor number through undo/redo and `--resave`/export.
- XML Roundtrip/Legacy: existing coverage = helper ordering and one real fixture export, no full XML diff gate. Missing = deterministic `--resave`, old terminal-id conductor endpoint load, missing project/diagram/element/conductor UUID fallback behavior, terminal-strip item resolution, old sequence attributes. Minimal future tests = a fixture matrix and normalized XML comparison after `--resave`, with separate legacy endpoint and missing-UUID cases.
- DB/Export Equivalence: existing coverage = BOM CSV SQL helper only. Missing = `QETProject` load -> DB rebuild -> `wiring_list_view`/nomenclature views compared with live graph/XML exporters. Minimal future tests = run `--export-wiring`, `--export-cables`, `--export-nets`, `--export-bom`, and `--info` over the same fixture and compare stable IDs/counts/labels.
- TerminalStrip: existing coverage = none found. Missing = strip creation/load/save, real/physical terminal mapping, grouping, levels, bridges, diagram graphics item link to project strip, undo commands. Minimal future tests = small project fixture with terminal elements assigned to a strip, grouped and bridged terminals, terminal-strip item on a diagram, `--resave` round-trip, and later direct model/undo QtTests.
- Kabel-/Leiterfelder: existing coverage = empty attributes in existing fixture only. Missing = non-empty cable, bus, function, wire color, wire section, formula/text propagation, XML merge behavior in `WiringListExport`, and CSV/export preservation. Minimal future tests = two connected conductor halves with intentionally asymmetric field values, then assert merged cable CSV and wiring/wires exports.

Phase 6 risk prioritization:
- Highest risk before any source change: XML round-trip determinism and legacy compatibility, DB/export equivalence, terminal/potential/wiring semantics, terminal-strip bridge/group behavior, master/slave link/contact mirror persistence, and autonum undo/redo.
- Useful technical hooks already exist: QtTest/CTest, real application binary via `QET_TEST_BINARY_PATH`, offscreen Qt platform, `QTemporaryDir`, `--resave`, `--export-links`, `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info`.
- Current gap: no broad fixture library of small purpose-built `.qet` projects that lock high-risk business behavior before implementation work.

## Minimal Fixture Specification
Phase 7 fixture/test-design findings:
- Existing examples are useful as source material and smoke-test corpus, but most are too large or too broad to be the primary regression fixtures. `examples/industrial.qet`, `examples/perceuse.qet`, `examples/photovoltaique.qet`, `examples/tableau_domestique.qet`, and `examples/2612_ats_singlephase.qet` contain many relevant real-world constructs, while `tests/qttest/fixtures/qet_bug_repro_resaved.qet` is already a compact binary-driven test fixture for one conductor geometry regression.
- Existing element definitions can seed purpose-built fixtures: master coils under `elements/10_electric/10_allpole/310_relays_contactors_contacts/01_coils/`, slave NO/NC/SW and multi-pole contacts under `02_contacts_cross_referencing/`, terminal elements under `130_terminals_terminal_strips/`, terminal-strip diagram symbols under `130_terminals_terminal_strips/90_terminal_strips_diagram/`, and cable/mux/demux symbols under `120_cables_wiring/`.
- CLI hooks are sufficient for stable black-box checks for several candidates: `--resave` for XML round-trip, `--export-links` for link status, `--export-nets` for live-potential JSON, `--export-wiring` for SQLite wiring CSV, `--export-cables` for XML-derived cable CSV, `--export-wires` for conductor-number export, `--export-bom` for DB/BOM checks, `--info` for project/folio metadata, and `--export-svg` for visual/geometry-oriented assertions.
- QtTest hooks remain necessary for undo/redo and model-level behavior that CLI cannot observe, especially `LinkElementCommand`, `SetAutoNumContextCommand`, `QPropertyUndoCommand`, `TerminalStrip` undo commands, and direct `NumerotationContextCommands` behavior.

Phase 7 fixture specifications:
- XML/Roundtrip/Legacy fixture:
  - Purpose: lock deterministic load/save behavior, modern UUID endpoint handling, legacy terminal-id fallback, missing UUID behavior, and legacy autonum/sequence compatibility before changing persistence.
  - Reusable inputs: `tests/qttest/fixtures/qet_bug_repro_resaved.qet` for modern endpoint UUIDs and current binary fixture pattern; `examples/2612_ats_singlephase.qet` for mixed conductor endpoint styles and conductor formula/number data; `examples/grafcet.qet` or `examples/convertisseur.qet` for older integer terminal endpoint style.
  - Later new fixture contents: a small 1-2 folio project with one modern conductor, one intentionally legacy integer-terminal conductor, one missing project/diagram/element UUID case if accepted, one saved linked element, and minimal embedded collection.
  - Stable assertions: `--resave` exits 0; normalized XML keeps project structure, diagram order, link UUIDs, conductor endpoints, conductor number/formula, and expected generated/preserved UUID policy; second `--resave` is byte- or normalized-equivalent to first output.
  - Risks/instability: raw XML attribute order and whitespace should not be the only oracle unless current serializer is explicitly accepted as canonical; generated fallback UUID policy differs by object type; date/titleblock metadata can make naive diffs noisy.
  - Abort criteria: fixture requires hand-editing XML whose load semantics are ambiguous; resave changes cannot be classified as intentional normalization vs. data loss; multiple legacy endpoint representations appear equally valid.
- Export-equivalence fixture:
  - Purpose: compare live graph, derived SQLite DB, XML-derived cable export, and conductor-number export for one small known project.
  - Reusable inputs: `examples/2612_ats_singlephase.qet` has numbered/formula conductors and mixed endpoints; `examples/convertisseur.qet` has simple numeric conductor numbers; `sources/cli_export.cpp` documents `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info`.
  - Later new fixture contents: a small project with 3-5 conductors, two potentials, one shared wire number across a potential, one terminal element, one simple device with BOM-relevant fields, and non-empty labels/terminal names.
  - Stable assertions: `--export-nets` JSON contains expected terminal set per net; `--export-wiring` CSV contains expected from/to labels and wire numbers; `--export-cables` and `--export-wires` agree on known conductor numbers where their models overlap; `--info` reports expected folio count.
  - Risks/instability: the exporters intentionally expose different columns and abstractions; sort orders may differ; labels can fall back to element names; wiring CSV has UTF-8 BOM.
  - Abort criteria: no stable common key can be identified across exporters; expected output would depend on GUI-only state; exporter behavior conflicts with existing documented semantics.
- Master/Slave/Kontakte fixture:
  - Purpose: lock link persistence, contact count/capacity, grouped contacts, unresolved link reporting, and minimum cross-reference observability.
  - Reusable inputs: master coils such as `01_coils/bobine3.elmt`; NO/NC contacts such as `02_contacts_cross_referencing/01_auxiliary_contacts/con_simple.elmt` and `con_simple_nf.elmt`; changeover contact `contnonc.elmt`; multi-pole power contacts such as `02_power_contacts/com_puiss4.elmt`; real example link data in `examples/2612_ats_singlephase.qet` and `examples/tableau_domestique.qet`.
  - Later new fixture contents: one coil/master linked to one NO, one NC, one SW/changeover, and one multi-contact slave; at least one `group_index`; one intentionally unresolved master/slave link only if an error-path test is desired.
  - Stable assertions: `--export-links` reports expected linkable count, linked names, and zero unresolved for valid fixture; `--resave` preserves link UUIDs and group indices; optional SVG/PDF check only asserts existence of cross-reference output markers, not pixel-perfect layout.
  - Risks/instability: visual CrossRef layout depends on fonts, rendering, and project XRef settings; names/labels may be localized or fallback-derived; group-index UI semantics may need a direct QtTest instead of CLI only.
  - Abort criteria: expected link ownership cannot be determined from XML alone; group assignment semantics require a product decision; rendering assertion would be too brittle across platforms.
- TerminalStrip/Bridges fixture:
  - Purpose: lock project-level terminal strip serialization, real/physical terminal mapping, grouped terminals, bridge persistence, and diagram terminal-strip item resolution.
  - Reusable inputs: terminal elements such as `borne_2.elmt`, `borne_finale.elmt`, `bornedouble.elmt`; diagram terminal-strip symbols under `90_terminal_strips_diagram/`; examples with terminal-strip XML markers include `examples/industrial.qet`, `examples/perceuse.qet`, `examples/photovoltaique.qet`, `examples/2612_ats_singlephase.qet`, and `examples/Projet_vierge.qet`.
  - Later new fixture contents: one strip with 3-4 real terminal elements, two terminals grouped into one physical terminal, one bridge across two same-level terminals, and one diagram `TerminalStripItem` pointing at the project strip.
  - Stable assertions: `--resave` preserves project `<terminal_strips>`, real terminal UUID references, group order, bridge UUID/member data, and diagram terminal-strip item UUID/position; later QtTest can assert `TerminalStrip::isBridgeable`, `isBridged`, `groupTerminals`, and undo commands directly.
  - Risks/instability: existing examples contain terminal elements but not necessarily minimal modern `TerminalStrip` domain objects; bridge impact on electrical potential was not proven in earlier traces; UI-generated ordering may be hard to reproduce by hand.
  - Abort criteria: cannot create/identify a minimal valid `TerminalStrip` XML shape without GUI generation or source implementation; bridge semantics require a domain decision rather than a regression oracle.
- Autonum/Undo fixture:
  - Purpose: lock numbering schema load/save, formula resolution, counter state, conductor text propagation, and undo/redo boundaries.
  - Reusable inputs: `examples/Projet_vierge.qet` contains named conductor autonum schemas with `%F-%seqt_1`; `examples/2612_ats_singlephase.qet` contains many conductor `num`/`formula` values; code paths from Phase 4 identify `NumerotationContextCommands`, `AssignVariables`, `ConductorAutoNumerotation`, and undo commands.
  - Later new fixture contents: one diagram with selected conductor autonum, one element autonum, one conductor with resolved text/formula, and stored sequential state before/after one increment.
  - Stable assertions: direct QtTest for `NumerotationContextCommands::next/previous` and carry/borrow; project fixture `--resave` preserves selected autonum names, formulas, and `sequentialNumbers`; export assertions verify resolved conductor text appears in `--export-wires` or `--export-wiring`.
  - Risks/instability: undo/redo is not observable through current CLI; UI-driven assignment can modify several distributed states; date/folio variables can make formula results context-sensitive.
  - Abort criteria: expected counter owner cannot be identified for the fixture; assertion requires simulating GUI-only workflow without a stable QtTest hook.
- Kabel-/Leiterfelder fixture:
  - Purpose: lock non-empty cable, wire color, wire section, function, bus, formula/text, and XML-to-CSV merge behavior.
  - Reusable inputs: cable/mux elements under `120_cables_wiring/`, especially `m_cable_02p_mux.elmt`, `m_cable_02p_demux.elmt`, `m_cable_designation.elmt`, `kabel3g.elmt`, and `kabel4g.elmt`; real examples with conductor color/size and formula values include `examples/cablage-eclairages_sikli-v5.qet`, `examples/convertisseur.qet`, and `examples/2612_ats_singlephase.qet`.
  - Later new fixture contents: two conductor halves of one cable with intentionally asymmetric non-empty `cable`, `conductor_color`, `conductor_section`, `function`, `bus`, and `num/formula`; one plain conductor without cable data as control.
  - Stable assertions: `--export-cables` CSV contains merged cable/color/section data; `--export-wires` contains expected conductor numbers; `--export-wiring` still reports correct endpoints/wire numbers; `--resave` preserves non-empty attributes.
  - Risks/instability: `WiringListExport` merges XML-derived halves while DB wiring export may not expose all cable fields; existing examples often leave `cable`, `conductor_color`, and `conductor_section` empty; CSV escaping/BOM needs normalization.
  - Abort criteria: merge behavior cannot be specified without changing product semantics; expected output differs between XML-derived and DB-derived exporters for legitimate reasons.

Phase 7 recommended suite shape:
- Prefer several tiny purpose-built fixtures over using large examples as canonical expected-output tests. Large examples remain useful as smoke corpus for `--resave`, `--export-svg`, and no-crash exporter checks.
- Keep expected outputs semantic first: normalized XML nodes/attributes, CSV rows keyed by stable columns, JSON nets keyed by sorted terminal labels/UUIDs, and only minimal SVG/PDF structural assertions.
- Before implementing fixtures, decide normalization rules and accepted comparison granularity so tests do not freeze accidental whitespace, row order, localization, font metrics, or platform rendering behavior.

## Test Expectation And Normalization Rules
Phase 8 normalization findings:
- XML output from `QET::writeXmlFile()` is UTF-8, LF, no BOM, and `QDomDocument::toString(4)`. `--resave` also writes `project.toXml().toString(4)`. This makes serialized XML suitable for repeatability checks, but future tests should still compare semantically unless the specific test is intentionally about serializer determinism.
- CLI CSV exports are not uniform. `--export-wiring` writes semicolon CSV with a UTF-8 BOM and only quotes fields containing `;`, quotes, or line breaks. `--export-links` writes semicolon CSV without BOM and the same conditional quoting helper. `--export-bom` always quotes every field and includes a UTF-8 BOM. `--export-cables` comes from `WiringListExport` and writes translated French headers through `tr()`, semicolon separators, no explicit quoting helper, and no BOM in the CLI path. `--export-wires` is one conductor number per line, sorted by text.
- JSON exports are indented `QJsonDocument` output. `--info` reports project title, total diagram/element/conductor/free-terminal counts, and per-page title/folio/geometry/counts. `--export-nets` reports a `list` of nets with sequential net numbers, selected `wire_no`, and terminal objects with element label, terminal name, and folio.
- SVG/PDF exports are rendering artifacts. They are useful to prove the render pipeline runs, pages/files are produced, and specific structural markers exist, but they are poor oracles for pixel-perfect geometry because fonts, antialiasing, Qt rendering, platform PDF internals, and page geometry can shift.
- CLI execution should use the built app binary, temporary output directories/files, and for graphics exports an offscreen Qt platform where appropriate. Existing tests already use `QT_QPA_PLATFORM=offscreen` for `tst_conductorselfretrace`; modal regression also sets offscreen.

Phase 8 normalization rules by area:
- XML normalization:
  - Stable comparable: root/project version attributes when expected, project UUID policy, diagram count/order/title, element UUID/type/prefix/label/link UUIDs/group_index, conductor endpoints/terminal names/num/formula/properties, terminal-strip UUID references, autonum context names/formulas, `sequentialNumbers` presence/content.
  - Normalize/ignore: XML declaration absence/presence, trailing whitespace, indentation, line endings, attribute order unless the test explicitly targets serializer byte determinism, volatile saved date/time/path metadata, generated fallback UUIDs in tests that intentionally omit UUIDs, and titleblock/date fields unless part of the fixture purpose.
  - Robust assertions: parse with DOM; compare selected paths/attributes; for determinism tests run `--resave` twice and compare normalized second output to first; for legacy tests assert intentional normalization rather than raw input preservation.
  - Unsuitable assertions: whole-file raw diff as a default oracle, byte-stable comparison across hand-edited legacy fixtures, and assertions that all generated UUIDs must persist identically unless that object’s policy is already proven.
- CSV normalization:
  - Stable comparable: header identity after BOM removal, row count, selected columns, rows keyed by stable business columns such as conductor UUID for `--export-wiring`, element label plus folio for `--export-links`, BOM label/manufacturer reference/folio for `--export-bom`, and normalized endpoint labels/terminals for cable/wiring equivalence.
  - Normalize/ignore: UTF-8 BOM, CRLF/LF, optional quoting differences between exporters, row ordering unless source code defines an explicit order, localized `--export-cables` header labels, empty trailing lines, and CSV field whitespace where exporter trims/merges values.
  - Robust assertions: parse semicolon CSV with quote handling; compare maps or sorted rows; assert exact values only in selected stable columns; for `--export-wires`, compare sorted multiset of wire numbers because duplicates are meaningful.
  - Unsuitable assertions: raw text diff between different CSV exporters, relying on translated `WiringListExport` headers for behavior, or comparing DB wiring rows directly to cable rows without a defined shared key.
- JSON net and info normalization:
  - Stable comparable: `--info` total diagram/element/conductor/free-terminal counts, page count/order, and page titles for purpose-built fixtures; `--export-nets` terminal sets, folio values, and selected wire numbers after sorting.
  - Normalize/ignore: sequential `net` numbers when graph traversal order is not the behavior under test, order of nets, order of terminals within a net, project title unless fixture-specific, and page pixel dimensions unless testing render geometry.
  - Robust assertions: parse JSON; represent each net as a sorted set of `(folio, element, terminal)` plus optional `wire_no`; compare sets independent of order; assert counts separately from membership.
  - Unsuitable assertions: raw JSON byte diff, array-order assertions for nets/terminals, and using `wire_no` alone as a net identity when several nets may share or omit numbers.
- SVG/PDF boundaries:
  - Stable comparable: export command exit code, produced file count matching diagram count, non-empty files, SVG root/viewBox/title presence, absence/presence of known structural path regressions such as self-retracing conductor paths, PDF page count if a stable PDF parser is available, and presence of internal-link annotations only when testing link injection.
  - Normalize/ignore: path ordering, exact floating-point coordinates except for targeted geometry regressions, font names/metrics, antialiasing, PDF object numbers, timestamps/metadata, compression, and rendered pixels unless the test owns a strict visual fixture.
  - Robust assertions: inspect SVG XML structurally; for conductor geometry use domain-specific path-shape detection as `tst_conductorselfretrace` does; for PDF prefer page count/link-count structural checks over visual diffs.
  - Unsuitable assertions: pixel-perfect PNG/PDF comparisons across platforms, raw PDF diffs, and broad SVG path-count diffs for complex examples.
- CLI execution environment:
  - Stable setup: run with a temporary output directory, deterministic fixture path, explicit binary path, and `QT_QPA_PLATFORM=offscreen` for SVG/PDF/PNG tests that do not require real GUI interaction. Check exit code before parsing outputs.
  - Normalize/ignore: stdout progress wording except minimal success/failure evidence, absolute temporary paths in stdout, elapsed time, and GUI/font environment details.
  - Robust assertions: command exits expected code; expected output files exist; parser can read output; stderr/stdout only checked for key failure markers when negative testing.
  - Unsuitable assertions: full stdout log snapshots or assumptions about macOS interactive GUI prompts.
- Platform skip/abort rules:
  - Stable skip: tests requiring Linux-only tooling should use explicit skips like existing `modal_quit_regression` with CTest skip code `77`; macOS should not attempt Linux-only `gdb`/Xvfb/openbox/xdotool flows. Missing optional external tools should skip only if the test is explicitly tool-dependent.
  - Abort/fail: parser failure, missing generated output after a successful command, application crash/nonzero exit for positive fixtures, changed semantic XML/CSV/JSON values, or unresolved links in fixtures expected to be fully linked.
  - Unverified boundary: interactive GUI startup remains outside automated baseline unless user is present for macOS prompts.
- Large smoke-example criteria:
  - Stable comparable: command exit code, no crash/timeout, expected number of produced files for all diagrams, non-empty outputs, parsable JSON/CSV/XML, and broad counts from `--info`.
  - Normalize/ignore: exact export rows, SVG/PDF geometry, ordering and labels not explicitly owned by the smoke test, and any localized text.
  - Robust assertions: run selected large examples as no-crash/load/export smoke corpus with generous timeouts and minimal semantic checks.
  - Unsuitable assertions: using large examples as golden behavior for contact, terminal-strip, autonum, or cable semantics because their intent is broad and not minimal.

Phase 8 open risks:
- No current helper library exists for these normalizations; adding one later is implementation work.
- Some comparison keys are only stable after purpose-built fixtures define labels/UUIDs deliberately.
- `--export-cables` currently uses translated headers and lacks robust CSV quoting, so tests should focus on parsed row values and may need locale/environment control.
- PDF link and page assertions need a reliable local parser choice before implementation.

## Regression Test Plan Backlog
Phase 9 prioritized non-implementing backlog:
- Priority P0 means protect before risky model/load-save/export work. P1 means add once P0 fixture/helper shape is accepted. P2 means valuable smoke or platform coverage after the core suite exists.
- Planned hooks deliberately reuse existing infrastructure first: QtTest/CTest, the built `qelectrotech` binary, `QTemporaryDir`, `QT_QPA_PLATFORM=offscreen`, and existing CLI commands. No production source seam or target architecture is selected here.

P0 backlog:
- `tst_cli_roundtrip_xml::modern_and_legacy_project_resave_is_semantically_stable`
  - Ziel/Risiko: locks `.qet` load/save, deterministic ordering, legacy conductor endpoints, UUID fallback policy, link UUID preservation, and autonum sequence persistence before persistence changes.
  - Fixtures: later purpose-built `roundtrip_modern_legacy.qet`; reusable starting evidence from `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, `examples/2612_ats_singlephase.qet`, `examples/grafcet.qet`, and `examples/convertisseur.qet`.
  - Helpers/Normalisierung: XML DOM loader, path/attribute selector, whitespace/line-ending/attribute-order normalization, volatile metadata ignore list, UUID-policy assertions, double-`--resave` comparison.
  - Hook: CLI integration test using `--resave` twice through the built binary.
  - Akzeptanz: command exits 0; both outputs parse; normalized second output equals first; selected diagrams/elements/conductors/link UUIDs/group indices/autonum contexts/sequential numbers are preserved according to fixture policy.
  - Abort/Skip: abort if fixture load/resave cannot classify a change as expected normalization vs. data loss; skip only when built binary is unavailable.
  - Aufwand: medium-high because fixture policy and XML helper must be created carefully.
- `tst_cli_export_equivalence::wiring_nets_cables_wires_and_info_agree_on_core_fixture`
  - Ziel/Risiko: protects equivalence across live potential traversal, derived SQLite wiring view, XML-derived cable export, conductor-number export, and project info counts.
  - Fixtures: later purpose-built workflow export fixture with 3-5 conductors, two potentials, one terminal element, one BOM-relevant device, stable labels/terminal names; examples remain smoke/reference only.
  - Helpers/Normalisierung: semicolon CSV parser with BOM handling, JSON net canonicalizer, sorted multiset for `--export-wires`, shared endpoint/wire-number key mapping, temp-output helper.
  - Hook: CLI integration using `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info`.
  - Akzeptanz: expected conductor counts and folio counts match; JSON terminal sets and CSV endpoint/wire rows agree on the fixture-owned subset; BOM row for known device is present; outputs parse and are non-empty.
  - Abort/Skip: abort if no stable shared key exists across exporters; skip only when CLI binary is unavailable.
  - Aufwand: high because it needs several normalizers and deliberately scoped equivalence rules.
- `tst_master_slave_links::resavePreservesGroupIndexedMasterSlaveLinks`
  - Ziel/Risiko: protects master/slave link persistence, `group_index`, unresolved-link reporting, and minimum contact mirror observability before contact changes.
  - Fixtures: later `master_slave_contacts_minimal.qet` with one master coil, NO, NC, SW/changeover, one multi-contact slave, at least one group assignment; element definitions from relay/contact folders.
  - Helpers/Normalisierung: XML link extractor, `--export-links` semicolon CSV parser, stable element label/folio/link-type keys, optional contact-count expected table.
  - Hook: CLI `--resave` plus `--export-links`; later direct QtTest may cover `LinkElementCommand` separately.
  - Akzeptanz: valid fixture reports zero unresolved links; expected links and labels appear; resave preserves link UUIDs and group indices; contact mix remains distinguishable from XML metadata.
  - Abort/Skip: abort if group ownership semantics cannot be determined from fixture/source evidence; skip rendering assertions when SVG/PDF parser/render environment is not stable.
  - Aufwand: medium-high due to link fixture construction and grouped-contact oracle.
- `tst_terminal_potential_exports::terminal_reports_isolating_and_wire_exports_match`
  - Ziel/Risiko: protects terminal identity, potential traversal, report/terminal bridging behavior, isolating terminal behavior, and export consistency.
  - Fixtures: later shared workflow export fixture with at least two potentials, stable wire numbers, and stable terminal endpoint names.
  - Helpers/Normalisierung: JSON net canonicalizer, wiring CSV parser, normalized terminal tuple `(folio, element, terminal)`, expected potential membership table.
  - Hook: CLI `--export-nets`, `--export-wiring`, `--export-wires`; optional `--resave` preservation check.
  - Akzeptanz: each expected net contains exactly the fixture-owned terminal set after order normalization; isolating terminal prevents traversal where expected; wiring rows contain matching endpoints and wire numbers.
  - Abort/Skip: abort if report/terminal traversal expectation cannot be derived without a product decision; skip only when binary unavailable.
  - Aufwand: high because the fixture must own electrical semantics tightly.
- `tst_terminal_strip_roundtrip::strip_group_bridge_and_item_survive_resave`
  - Ziel/Risiko: protects project-level terminal-strip objects, real/physical terminal mapping, grouping, bridge persistence, and diagram terminal-strip item resolution.
  - Fixtures: later `terminal_strip_bridge_minimal.qet` with one strip, 3-4 real terminal elements, grouped physical terminal, one bridge, and one diagram item referencing the strip.
  - Helpers/Normalisierung: XML terminal-strip extractor, UUID/member/order comparison, bridge membership canonicalizer.
  - Hook: CLI `--resave`; later direct QtTest for `TerminalStrip::isBridgeable`, `isBridged`, `groupTerminals`, and undo commands.
  - Akzeptanz: project `<terminal_strips>` survives; real terminal element UUID references resolve/persist; group order and bridge membership persist; diagram item keeps strip UUID and position.
  - Abort/Skip: abort if a valid minimal strip XML cannot be produced without GUI generation; abort if bridge-potential semantics are treated as product behavior rather than persistence.
  - Aufwand: high because no existing minimal fixture or tests were found.

P1 backlog:
- `tst_autonum_contexts::numerotation_context_next_previous_and_formula_resolution`
  - Ziel/Risiko: protects counter engine, carry/borrow behavior, formula substitution, and stable sequence state independent of GUI.
  - Fixtures: no `.qet` needed for the direct unit part; later tiny fixture for project-level selected conductor/element autonum.
  - Helpers/Normalisierung: expected context table, formula result table, stable locale/date-free variables.
  - Hook: QtTest directly against `NumerotationContextCommands`, `AssignVariables`, and small project XML via `--resave` for persistence.
  - Akzeptanz: next/previous round trips for unit/ten/hundred/wrap/alpha cases; formula resolves expected labels without volatile variables; fixture preserves selected names/formulas/sequential numbers.
  - Abort/Skip: abort if formula expectation depends on date/titleblock/user locale not owned by the test.
  - Aufwand: medium.
- `tst_undo_workflows::link_and_numbering_commands_undo_redo_without_state_leak`
  - Ziel/Risiko: protects `LinkElementCommand`, `SetAutoNumContextCommand`, `QPropertyUndoCommand`, and terminal-strip undo command families that CLI cannot observe.
  - Fixtures: reuse later master/slave, autonum, and terminal-strip minimal fixtures or construct objects in QtTest once fixture shape is accepted.
  - Helpers/Normalisierung: command-state snapshots for links/group indices/context counters/conductor text/terminal-strip bridge state.
  - Hook: QtTest with direct commands and `QUndoStack`.
  - Akzeptanz: redo applies exactly one expected state change; undo restores previous state; repeated redo/undo does not burn counters or duplicate links/bridges.
  - Abort/Skip: abort if constructing required objects requires GUI-only dialog behavior instead of model/command hooks.
  - Aufwand: high.
- `tst_cable_conductor_fields::xml_cable_fields_survive_and_export_cables_merges`
  - Ziel/Risiko: protects non-empty `cable`, `bus`, function, color, section, formula/text, and XML-to-CSV merge behavior.
  - Fixtures: later `cable_conductor_fields_minimal.qet` with two cable halves, asymmetric fields, one plain conductor control; element definitions from `120_cables_wiring/`.
  - Helpers/Normalisierung: XML conductor property extractor, semicolon CSV parser, normalized cable-row keys, sorted wire-number multiset.
  - Hook: CLI `--resave`, `--export-cables`, `--export-wires`, and `--export-wiring`.
  - Akzeptanz: non-empty attributes persist in XML; `--export-cables` contains expected merged cable/color/section/function fields; `--export-wires` and DB wiring still report expected conductor numbers/endpoints.
  - Abort/Skip: abort if merge semantics differ legitimately between XML-derived and DB-derived exporters and no fixture-owned assertion can be defined.
  - Aufwand: medium-high.
- `tst_svg_pdf_boundaries::render_exports_are_structurally_valid_without_pixel_oracles`
  - Ziel/Risiko: protects basic render/export pipeline and cross-reference link injection boundaries without freezing platform rendering.
  - Fixtures: reuse compact master/slave fixture and existing conductor self-retrace fixture.
  - Helpers/Normalisierung: SVG XML structural parser; PDF page/link parser choice still open; file-count/non-empty checks.
  - Hook: CLI `--export-svg` and `--export-pdf` with `QT_QPA_PLATFORM=offscreen`.
  - Akzeptanz: produced file count matches diagram count; files non-empty and parse structurally; optional PDF link/page counts match fixture-owned expectations.
  - Abort/Skip: skip PDF link assertions until a stable parser is selected; fail on exporter crash or missing output.
  - Aufwand: medium.

P2 backlog:
- `tst_large_examples_smoke::selected_examples_load_resave_and_export_without_crash`
  - Ziel/Risiko: protects broad real-world load/export survival without treating large examples as semantic golden files.
  - Fixtures: existing `examples/industrial.qet`, `examples/perceuse.qet`, `examples/photovoltaique.qet`, `examples/tableau_domestique.qet`, `examples/2612_ats_singlephase.qet`.
  - Helpers/Normalisierung: timeout wrapper, temp directory, output file count/non-empty checks, parse-only XML/JSON/CSV checks, broad `--info` counts.
  - Hook: CLI `--resave`, `--info`, selected exports such as `--export-svg` and CSV/JSON exports.
  - Akzeptanz: no crash/timeout; outputs parse; broad counts are plausible and stable enough for each selected example; no exact row/render golden comparisons.
  - Abort/Skip: skip examples that require missing optional modules or exceed agreed timeout; fail on crash or unparseable output.
  - Aufwand: low-medium after helpers exist.
- `tst_platform_skip_policy::gui_ipc_and_modal_regressions_are_not_run_on_wrong_platform`
  - Ziel/Risiko: prevents fragile GUI/IPC checks from producing false failures on macOS or hosts without Linux GUI tooling.
  - Fixtures: existing `tests/modal-quit-regression/*` and `tests/ipc-regression/README.md` requirements.
  - Helpers/Normalisierung: CTest property/skip-code check, optional tool detection checklist.
  - Hook: CTest metadata/static test-infrastructure inspection or small CMake-level validation later.
  - Akzeptanz: Linux-only flows use explicit skip/requirements; macOS does not attempt gdb/Xvfb/openbox/xdotool flows; missing optional tools produce skip only for tool-dependent tests.
  - Abort/Skip: abort if changing CTest behavior would affect packaging or CI policy beyond test scope.
  - Aufwand: low.

Phase 9 required helper backlog before behavioral assertions:
- `XmlProjectNormalizer`: parse `.qet`, select stable nodes/attributes, canonicalize whitespace/order, expose fixture-owned policies for UUID and volatile metadata.
- `SemicolonCsvReader`: remove UTF-8 BOM, parse quotes/semicolons/newlines, normalize CRLF/LF, expose rows as keyed maps or sorted multisets.
- `NetJsonCanonicalizer`: parse `--export-nets`, represent each net as sorted terminal tuples plus optional wire number.
- `CliRunner`: built-binary path, temp output management, `QT_QPA_PLATFORM=offscreen`, timeout, stdout/stderr key-marker handling, command evidence.
- `SvgStructureReader`: parse SVG root/viewBox/title/path markers without pixel or broad path-count assertions.
- `PdfStructureReader`: only after a stable local parser is selected; page/link counts only, no raw PDF diff.

Phase 9 risks/unverified:
- This backlog intentionally does not decide future production architecture; it only orders regression safety work.
- Exact fixture file names, helper class names, and CMake target names are proposed names, not implemented or accepted API.
- Terminal-strip bridge electrical effect remains unverified; current P0 terminal-strip item is a persistence test, not a potential-traversal oracle.
- Undo tests require direct object/command construction and may expose missing non-GUI seams; that is a testability finding, not approval to refactor production code.
- Large examples are demoted to smoke tests because they are too broad and unstable as semantic golden files.

Phase 9 recommended sequence:
- First implement test-only helpers and XML round-trip P0, then export equivalence P0, then master/slave and terminal/potential P0, then terminal-strip persistence P0. Add P1 undo/autonum/cable tests once fixtures and helpers are stable, and keep P2 smoke/platform tests separate from semantic regressions.

## First Test Infrastructure Slice Plan
Phase 10 non-implementing implementation/review findings:
- The first future patch should be deliberately smaller than the full P0 backlog. It should prove the test-only infrastructure shape with one existing fixture before adding new fixtures or broad export equivalence tests.
- Existing precedent is `tests/qttest/tst_conductorselfretrace.cpp`: a QtTest binary depends on `qelectrotech`, receives `QET_TEST_BINARY_PATH`, uses `QTemporaryDir`, sets `QT_QPA_PLATFORM=offscreen`, invokes the real binary, and asserts structured output rather than GUI interaction.
- Existing registration precedent is `tests/qttest/CMakeLists.txt`: each QtTest has its own executable, `add_test()`, targeted includes/libs, and optional `add_dependencies(... qelectrotech)` when the real application binary is needed.
- Existing compact fixture precedent is `tests/qttest/fixtures/qet_bug_repro_resaved.qet`; the first slice can use this fixture for harness/roundtrip mechanics without adding a new fixture.

First slice boundary:
- Include only test-only files under `tests/qttest/` and possibly `tests/qttest/fixtures/` in a later patch. The first planned patch should avoid `sources/`, `elements/`, application resources, and production CMake outside test registration unless test infrastructure absolutely requires it.
- Use the existing compact fixture first. Do not create the purpose-built P0 fixture matrix in the same patch.
- Implement helper infrastructure plus one minimal roundtrip smoke/assertion test. Do not implement export equivalence, master/slave, terminal-strip, autonum, cable, PDF, or large-example tests in the first patch.

Planned implementation order for the first test-only patch:
- Step 1: shared CLI runner helper.
  - Zweck: centralize binary path checks, temp output paths, process timeout, environment setup, offscreen Qt platform, and compact stdout/stderr evidence for future CLI-based tests.
  - Spaetere Dateien/Verzeichnisse: likely `tests/qttest/cli_test_utils.h` or `tests/qttest/testhelpers/cli_runner.h`; used first by a new `tests/qttest/tst_cli_roundtrip_xml.cpp`.
  - Akzeptanz: helper runs the built `qelectrotech` binary from `QET_TEST_BINARY_PATH`, fails with clear QtTest messages, writes only to `QTemporaryDir`, enforces timeout, and sets `QT_QPA_PLATFORM=offscreen` only for test process execution.
  - Review-Checkpunkte: no production include dependency beyond Qt/Core/Test; no absolute machine paths; no command shell string concatenation; arguments passed as `QStringList`; stdout/stderr not snapshotted wholesale.
  - Risiken: helper may accidentally hide nonzero exit details or over-normalize failures.
  - Nicht-Ziele: no generic framework, no GUI automation, no macOS permission prompts, no CI service-specific behavior.
- Step 2: minimal XML normalization helper.
  - Zweck: parse `.qet` XML outputs and compare stable structural facts without freezing whitespace, line endings, or attribute ordering.
  - Spaetere Dateien/Verzeichnisse: likely `tests/qttest/xml_test_utils.h` or `tests/qttest/testhelpers/xml_project_normalizer.h`.
  - Akzeptanz: helper loads XML via Qt XML APIs, reports parse errors, can compare root name, diagram count, conductor/link/autonum sections selected by the first test, and supports explicit ignore policy for formatting/volatile fields.
  - Review-Checkpunkte: no raw whole-file golden dump as the primary oracle; no broad XPath-like string matching when DOM is available; no mutation of fixture files; fixture-specific expectations visible in the test.
  - Risiken: too much normalization can miss data loss; too little normalization can create brittle failures.
  - Nicht-Ziele: no full canonical serializer, no schema validator, no legacy policy beyond what the first existing fixture proves.
- Step 3: first minimal fixture selection.
  - Zweck: prove infrastructure with an existing repo fixture before adding new project data.
  - Spaetere Dateien/Verzeichnisse: use existing `tests/qttest/fixtures/qet_bug_repro_resaved.qet`; no new fixture in the first patch.
  - Akzeptanz: `QFINDTESTDATA()` resolves the fixture; test does not overwrite it; resave outputs go to temp files only.
  - Review-Checkpunkte: fixture purpose stated narrowly as infrastructure/roundtrip smoke, not as full XML/legacy oracle; no large examples used as goldens.
  - Risiken: existing fixture may not cover all future P0 roundtrip risks, so it must not be oversold.
  - Nicht-Ziele: no handcrafted legacy endpoint fixture, no master/slave fixture, no terminal-strip fixture.
- Step 4: first P0 test skeleton.
  - Zweck: create the first executable for `tst_cli_roundtrip_xml` with one test that runs `--resave` twice on the existing compact fixture and verifies parseability plus normalized second-output stability.
  - Spaetere Dateien/Verzeichnisse: `tests/qttest/tst_cli_roundtrip_xml.cpp`; registration in `tests/qttest/CMakeLists.txt`.
  - Akzeptanz: test depends on `qelectrotech`, uses `QET_TEST_BINARY_PATH`, runs two `--resave` commands into `QTemporaryDir`, parses both outputs, and asserts normalized second output equals first for the limited selected structure.
  - Review-Checkpunkte: no production source change; no fixture overwrite; no exact raw XML diff unless scoped as a secondary diagnostic; no GUI launch; timeout bounded.
  - Risiken: `--resave` command behavior or output path semantics must be verified in implementation; if CLI syntax differs, stop and adjust test design rather than forcing shell workarounds.
  - Nicht-Ziele: no broad fixture matrix, no export equivalence, no undo, no PDF/SVG additions.
- Step 5: CTest/CMake integration.
  - Zweck: register the new test consistently with existing QtTest patterns.
  - Spaetere Dateien/Verzeichnisse: `tests/qttest/CMakeLists.txt`.
  - Akzeptanz: new executable builds under existing `PACKAGE_TESTS`; `add_test()` registers it; `add_dependencies(... qelectrotech)` and `QET_TEST_BINARY_PATH` mirror `tst_conductorselfretrace`; required Qt components are already available.
  - Review-Checkpunkte: target is isolated; no global CMake churn; no package/install behavior change; no unrelated test rewrites; platform skip is explicit only if a real platform dependency is found.
  - Risiken: linking extra QET sources can accidentally pull production dependency graph; first slice should prefer CLI black-box over linking internals.
  - Nicht-Ziele: no CI workflow creation, no packaging change, no enabling IPC/modal tests.
- Step 6: local verification commands.
  - Zweck: prove the future patch is buildable and testable on the baseline build shape.
  - Spaetere Dateien/Verzeichnisse: no additional files; commands documented in `Progress.md` after implementation.
  - Akzeptanz: configure/build succeeds; targeted `ctest -R tst_cli_roundtrip_xml --output-on-failure` passes; existing `tst_conductorselfretrace` still passes; full CTest remains considered if runtime is acceptable.
  - Review-Checkpunkte: command output summarized, not pasted wholesale; failures keep temp data only if needed and outside repo; no source edits to make tests pass.
  - Risiken: full CTest runtime/platform behavior may differ across hosts; targeted verification should be primary for the first patch.
  - Nicht-Ziele: no GUI smoke, no Homebrew changes unless dependencies unexpectedly missing.
- Step 7: documentation update.
  - Zweck: record exactly what the first patch adds, what remains unverified, and what the next test-only slice should do.
  - Spaetere Dateien/Verzeichnisse: `Progress.md` and `Brain.md` only.
  - Akzeptanz: docs mention changed test-only files, commands run, pass/fail evidence, open risks, and next P0 backlog item.
  - Review-Checkpunkte: no claims that XML/legacy coverage is complete; no architecture decision implied.
  - Risiken: documentation may overstate a smoke test as semantic coverage.
  - Nicht-Ziele: no production design document, no target architecture commitment.

First patch review checklist:
- Scope: only `tests/qttest/*`, `tests/qttest/CMakeLists.txt`, and project docs may change; `sources/`, `elements/`, production resources, build system outside the necessary test registration, and existing fixtures stay untouched unless explicitly approved.
- Determinism: outputs go to `QTemporaryDir`; assertions parse XML/structured data; no machine-specific paths, timestamps, locale-dependent strings, raw large dumps, or wall-clock assumptions.
- Platform robustness: no interactive GUI start; `QT_QPA_PLATFORM=offscreen` for the child process; skip only for a documented missing binary/tool/platform precondition; positive tests fail on crash/nonzero/missing output.
- CTest hygiene: one focused executable, one `add_test`, dependency on `qelectrotech` only when needed, bounded timeout in the test, no global CMake churn.
- Evidence quality: failure messages identify command, exit status, parse problem, and relevant output path without dumping full logs or secrets.
- Regression value: first test proves helper and `--resave` mechanics on an existing fixture; it must not pretend to cover all legacy UUID/link/terminal-strip/autonum risks.
- Review red flags: production code touched, existing fixture modified, large golden output added, raw PDF/SVG/image diff introduced, shell command strings used, temp files written into repo, GUI permission prompt required, platform-specific test run unguarded.

Phase 10 acceptance/abort rules:
- Accept the first implementation patch only if it is test-only, deterministic, locally runnable through CTest, and demonstrably leaves production source untouched.
- Abort implementation if `--resave` syntax/output behavior is ambiguous, if the existing compact fixture cannot be used without modification, if a production code seam appears necessary, or if adding the test requires choosing between competing fixture/architecture semantics.
- Defer new fixture creation until the helper/harness patch is reviewed and accepted.

Phase 10 risks/unverified:
- The exact helper file layout remains a proposal; implementation may choose flat headers or a small `testhelpers/` folder, but should stay under `tests/qttest/`.
- The first slice is infrastructure-heavy and intentionally low semantic coverage; it reduces later risk by proving mechanics, not by covering the full P0 matrix.
- PDF parser choice, terminal-strip electrical semantics, undo command construction, and export equivalence helper details remain outside the first slice.

## First Test-Only Patch Implementation
Implemented first test-only slice findings:
- `tests/qttest/tst_cli_roundtrip_xml.cpp` adds a focused QtTest that drives the built QElectroTech binary through `QET_TEST_BINARY_PATH`, mirroring the existing binary-test pattern from `tst_conductorselfretrace`.
- The test uses the existing fixture `tests/qttest/fixtures/qet_bug_repro_resaved.qet`; no fixture was added or modified.
- The test executes `qelectrotech --resave <input> <output>` twice inside `QTemporaryDir`: fixture -> `first.qet`, then `first.qet` -> `second.qet`.
- Child CLI processes receive `QT_QPA_PLATFORM=offscreen`; no interactive GUI path is used.
- The test verifies both resave outputs exist, parse as XML with Qt DOM, have `project` as document root, retain at least one `diagram`, and compare equal after a local canonicalization pass that sorts attributes and ignores formatting-only whitespace.
- Helper code is intentionally local to `tst_cli_roundtrip_xml.cpp` for this first slice. No shared helper headers were created yet, avoiding premature helper API design.
- `tests/qttest/CMakeLists.txt` registers `tst_cli_roundtrip_xml` with `add_executable`, `add_test`, `add_dependencies(... qelectrotech)`, `Qt::Test`, `Qt::Xml`, and `QET_TEST_BINARY_PATH`.

First patch verification:
- Initial sandboxed build failed because CMake/Ninja could not write build files under `/Volumes/MAM Home/...`; rerun with approved build escalation succeeded.
- `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_roundtrip_xml --parallel` passed.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_roundtrip_xml --output-on-failure` passed: 1/1.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 2/2.

First patch boundaries:
- Production source under `sources/` was not changed.
- `elements/` and existing fixtures were not changed.
- Production/top-level CMake was not changed; only `tests/qttest/CMakeLists.txt` was edited for qttest registration.
- The test does not claim full legacy, UUID, link, terminal-strip, autonum, export-equivalence, or GUI coverage. It is a roundtrip infrastructure smoke test and determinism guard for the first resaved output.

First patch risks/unverified:
- Canonicalization is intentionally minimal and local. It is suitable for first/second resave stability but not yet a general XML policy helper for the full P0 fixture matrix.
- The existing fixture is useful for mechanics and a known conductor project, but it does not cover master/slave, terminal-strip, autonum, cable-field, or legacy endpoint scenarios by itself.
- Submodule working trees show untracked `.DS_Store` files, and the build tree remains untracked as expected from previous baseline work; these were not deleted.

Recommended next implementation slice:
- After review, extract shared CLI/XML helper code only if a second CLI test needs it, then implement the next P0 export-equivalence slice with a deliberately minimal fixture under explicit approval.

## First Patch Review And Next P0 Slice
Review result for first test-only patch:
- `tests/qttest/tst_cli_roundtrip_xml.cpp` stays inside test scope, uses only Qt test/process/XML/temp APIs, invokes the built binary with `QStringList` arguments, writes resave outputs only under `QTemporaryDir`, sets `QT_QPA_PLATFORM=offscreen`, and avoids GUI automation.
- `tests/qttest/CMakeLists.txt` adds one focused executable/test target and mirrors the existing binary-test pattern: dependency on `qelectrotech`, `QET_TEST_BINARY_PATH`, `Qt::Test`, and `Qt::Xml`.
- Existing fixture `tests/qttest/fixtures/qet_bug_repro_resaved.qet` is only read via `QFINDTESTDATA`; no fixture modifications were found.
- Progress/Brain entries are consistent with the implemented scope: infrastructure smoke/roundtrip guard only, not full P0 coverage.
- No review finding blocks the patch.

Review verification:
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_roundtrip_xml --output-on-failure` passed: 1/1.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 2/2.
- Production code check found no tracked diff under `sources/`, `elements/`, top-level `CMakeLists.txt`, or `tests/qttest/fixtures`.

Next P0 slice definition, not implemented:
- Name: `tst_cli_export_equivalence::wiring_nets_cables_wires_bom_and_info_share_core_facts`.
- Goal/risk: compare the stable overlap between live potential traversal (`--export-nets`), derived SQLite wiring view (`--export-wiring`), XML-derived cable/wire export (`--export-cables` and `--export-wires`), BOM export (`--export-bom`), and project summary (`--info`) before changing wiring/database/export behavior.
- Scope: test-only files under `tests/qttest/`, one new minimal fixture only after explicit approval, qttest CMake registration only. No production source changes.
- Planned helpers: shared `CliRunner` extracted from current local pattern only if reused; `SemicolonCsvReader` with BOM/quote/line-ending handling; `NetJsonCanonicalizer`; small output-file/temp-dir utilities. XML normalizer extraction is optional and should not happen unless the new test needs it.
- Minimal fixture contents: one folio, 3-5 conductors, two distinct potentials, stable element labels, stable terminal names, one terminal element if needed for traversal, one simple BOM-relevant device, at least one repeated wire number within a potential, and one control conductor outside that potential. Cable/color/section fields should be included only if they can be asserted without depending on unresolved cable-domain semantics.
- Acceptance criteria: all CLI commands exit 0; generated CSV/JSON/XML-adjacent outputs parse; `--info` reports expected folio/conductor/element counts; `--export-nets` terminal sets match the fixture-owned potential table; `--export-wiring` contains the expected endpoint/wire rows; `--export-wires` contains the sorted multiset of expected wire numbers; `--export-bom` contains the known device row; `--export-cables` agrees only on fields explicitly owned by the fixture.
- Skip/abort rules: abort if fixture construction requires ambiguous GUI-only state, if no stable shared key exists across exporters, if exporter outputs legitimately disagree by design for the proposed assertion, or if implementing the test requires production seams. Skip only for missing built binary/tool preconditions.
- Non-goals: no broad large-example smoke, no master/slave/contact assertions, no terminal-strip bridge semantics, no PDF/SVG checks, no raw CSV/JSON golden dumps, no full stdout snapshots.

## Export Equivalence Test-Only Slice
Implemented P0 export-equivalence slice findings:
- `tests/qttest/tst_cli_export_equivalence.cpp` adds a focused QtTest that drives the built QElectroTech binary with `QET_TEST_BINARY_PATH`, writes all outputs to `QTemporaryDir`, and sets `QT_QPA_PLATFORM=offscreen` for child CLI processes.
- Shared fixture `tests/qttest/fixtures/workflow_exports_minimal.qet` was added as a test-only fixture. It is based on the existing compact QET fixture but intentionally adds stable conductor numbers `W005`-`W011` and non-empty conductor fields: `cable="CABLE-A"`, `conductor_color="BK"`, `conductor_section="1.5"`, and `function="CTRL"`.
- Existing fixtures were not modified.
- The test runs six CLI hooks over the same fixture: `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info`.
- The test parses JSON and semicolon CSV semantically. CSV parsing handles UTF-8 BOM, quoted fields, semicolons, CRLF/LF, and empty trailing lines. Assertions compare sets/counts rather than raw dumps or row order.
- Stable asserted facts: one diagram, nine elements, seven conductors; net count equals conductor count; net wire numbers equal `W005`-`W011`; wiring CSV wire numbers equal the same set; cable CSV has seven rows and stable `BK`/`1.5`/`CTRL` fields; `--export-wires` emits the same wire-number set; BOM row count equals element count and folio is `1 OF 1`.
- `tests/qttest/CMakeLists.txt` registers `tst_cli_export_equivalence` as a test-only qttest target depending on `qelectrotech`.

Verification:
- `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_export_equivalence --parallel` passed.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_export_equivalence --output-on-failure` passed: 1/1.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 3/3.

Boundaries and risks:
- Production source under `sources/` was not changed.
- `elements/` was not changed.
- The fixture is still derived from a compact existing project rather than a hand-authored from-zero minimal project. It is minimal for the tested CLI overlap but does not establish terminal-strip, master/slave, autonum/undo, or PDF/SVG behavior.
- Helper code remains local to the two CLI tests. If a third CLI test is added, extract shared `CliRunner`, CSV parser, and JSON canonicalizer first to avoid duplication.

Recommended next step:
- Review the export-equivalence slice. If clean, extract shared test helpers before implementing the next P0 domain slice, likely master/slave link persistence or terminal/potential export coverage.

## Export Equivalence Review And Helper Extraction Plan
Review result for P0 export-equivalence slice:
- `tests/qttest/tst_cli_export_equivalence.cpp` stays inside test scope and uses only Qt test/process/JSON/temp utilities.
- CLI execution uses the built binary via `QET_TEST_BINARY_PATH`, passes arguments as `QStringList`, writes only to `QTemporaryDir`, and sets `QT_QPA_PLATFORM=offscreen`.
- CSV parsing is semantic and handles UTF-8 BOM, quotes, semicolons, CRLF/LF, and empty trailing lines. JSON parsing is semantic and assertions compare counts/sets rather than large raw dumps.
- `tests/qttest/fixtures/workflow_exports_minimal.qet` has stable, explicit conductor numbers and conductor fields used by the export workflow tests. Existing upstream fixtures were not modified.
- `tests/qttest/CMakeLists.txt` adds one focused qttest target and follows the existing real-binary test pattern.
- Progress/Brain documentation is consistent with the implementation scope.
- No review finding blocks the patch.

Review verification:
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_export_equivalence --output-on-failure` passed: 1/1.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 3/3.
- Production code check found no tracked diff under `sources/`, `elements/`, or top-level `CMakeLists.txt`.

Next helper-extraction task, not implemented:
- Create a small test-only helper header under `tests/qttest/`, preferably `tests/qttest/cli_test_utils.h` unless a subfolder is explicitly preferred later.
- Move shared CLI process execution from `tst_cli_roundtrip_xml.cpp` and `tst_cli_export_equivalence.cpp` into a helper such as `CliResult runQetCli(QStringList arguments, int timeout_ms = 30000)`.
- Move reusable file reading and parse checks into helpers: `readFile`, `readJsonObject`, and a JSON parse assertion that fails clearly on invalid JSON.
- Move `parseSemicolonCsv` and `columnSet` into a CSV helper section while preserving BOM/quote/CRLF behavior exactly.
- Move XML helpers from `tst_cli_roundtrip_xml.cpp` only if doing so does not broaden their policy beyond current first/second resave comparison.
- Keep all helpers header-only or test-only `.cpp` linked only by qttest targets; do not touch production source.
- Acceptance: both existing CLI tests keep the same assertions and still pass; no fixture changes; no behavioral broadening; no raw golden dumps introduced.
- Non-goals: no new domain test, no new fixture, no production seam, no PDF/SVG parser, no large smoke matrix.

## CLI Test Helper Extraction
Implemented helper extraction findings:
- Added `tests/qttest/cli_test_utils.h` as a header-only, test-only helper file.
- Extracted `CliResult`, `runQetCli`, `readFile`, `readJsonObject`, `parseSemicolonCsv`, and `columnSet` into `CliTestUtils`.
- Updated `tst_cli_roundtrip_xml.cpp` to use `CliTestUtils::runQetCli`.
- Updated `tst_cli_export_equivalence.cpp` to use `CliTestUtils::runQetCli`, `readFile`, `readJsonObject`, `parseSemicolonCsv`, and `columnSet`.
- Kept XML helpers local in `tst_cli_roundtrip_xml.cpp`. Their current behavior is specific to the first/second `--resave` comparison and should not be generalized until a second XML roundtrip test needs the same policy.
- No new tests or fixtures were added in this helper extraction slice.

Verification:
- `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_roundtrip_xml tst_cli_export_equivalence --parallel` passed.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 3/3.

Boundaries and risks:
- Production source under `sources/` was not changed.
- `elements/` was not changed.
- Existing fixture files were not changed.
- Helper API is intentionally narrow and covers only current CLI-test needs. It should not become a broader test framework unless future duplication justifies it.

Recommended next step:
- Review the helper extraction. If clean, choose the next P0 domain slice: master/slave link persistence or terminal/potential export coverage are the two strongest candidates from the current backlog.

## Helper Extraction Review And Next P0 Options
Review result for CLI helper extraction:
- `tests/qttest/cli_test_utils.h` is test-only, header-only, and narrow: CLI execution, file/JSON reading, semicolon CSV parsing, and column-set extraction only.
- `runQetCli` preserves the existing behavior from both CLI tests: `QET_TEST_BINARY_PATH`, `QStringList` arguments, `QT_QPA_PLATFORM=offscreen`, timeout, bounded stdout/stderr evidence.
- `tst_cli_roundtrip_xml.cpp` still owns XML parsing/canonicalization locally; no broad XML policy was introduced.
- `tst_cli_export_equivalence.cpp` keeps the same assertions while using the shared file/JSON/CSV helpers.
- No production source, element collection, top-level CMake, or existing fixture changes were found.
- No review finding blocks the helper extraction.

Review verification:
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 3/3.
- Production code check found no tracked diff under `sources/`, `elements/`, top-level `CMakeLists.txt`, or existing fixtures.

P0 option A, Master/Slave-Link-Persistenz:
- Implemented test: `tst_master_slave_links::resavePreservesGroupIndexedMasterSlaveLinks`.
- Purpose: lock persisted master/slave links, linked UUIDs, `group_index`, unresolved-link reporting, and minimum contact mirror observability before contact/cross-reference changes.
- Fixture: new minimal `master_slave_contacts_minimal.qet`; one master coil plus NO, NC, SW/changeover, and one multi-contact slave, with stable labels and at least one grouped contact assignment. Existing relay/contact element definitions can be referenced/embedded, but existing fixtures must remain unchanged.
- Hooks: `--resave` for XML link/group persistence and `--export-links` CSV for link/unresolved status. Optional SVG/PDF rendering remains out of scope for the first slice.
- Helpers: reuse `runQetCli`, `parseSemicolonCsv`, `columnSet`; add only tiny XML link extractors local to the test unless repeated later.
- Acceptance: valid fixture exports zero unresolved links; expected linked element rows are present; resave preserves link UUIDs and `group_index`; fixture-owned contact types remain identifiable.
- Abort conditions: group assignment cannot be represented without GUI-only ambiguous state; link ownership semantics are unclear; expected `--export-links` rows cannot be keyed stably.
- Risk: higher fixture-semantics risk than option B because correct `group_index` and contact grouping are tied to current UI/link-command behavior.

P0 option B, Terminal/Potential-Export-Coverage:
- Proposed test: `tst_terminal_potential_exports::terminal_reports_isolating_and_wire_exports_match`.
- Purpose: lock terminal identity, potential traversal, live-net JSON, SQLite wiring CSV, conductor-number export, and fixture-owned terminal membership before terminal/potential/export changes.
- Fixture: shared minimal `workflow_exports_minimal.qet`; stable conductor numbers and stable terminal endpoint names. Existing upstream fixtures remain unchanged.
- Hooks: `--export-nets`, `--export-wiring`, `--export-wires`, and optionally `--resave`.
- Helpers: directly reuse `runQetCli`, `readJsonObject`, `parseSemicolonCsv`, and set comparison patterns from export-equivalence test.
- Acceptance: each expected net has exactly the fixture-owned sorted terminal set; wire numbers match across nets/wiring/wires; wiring endpoints match expected terminal tuples; resave preserves conductor endpoints/numbers if included.
- Abort conditions: report/terminal traversal requires GUI-only setup; terminal element bridge or isolating behavior cannot be represented unambiguously; live-net and DB export disagree for a reason not owned by the fixture.
- Risk: lower implementation risk than option A because existing CLI helpers and export-equivalence assertions already exercise most required machinery.

Technical recommendation:
- Implement option B first. It extends the verified CLI/export-equivalence path, reuses the current helper set, and can be scoped around stable conductor/potential facts. Option A should follow after a dedicated fixture-design pass for `group_index` and contact grouping semantics.

## Terminal Potential Export Test-Only Slice
Implemented P0 Terminal/Potential slice findings:
- `tests/qttest/tst_terminal_potential_exports.cpp` adds a focused QtTest for terminal/potential export coverage.
- `tst_terminal_potential_exports` now reuses `tests/qttest/fixtures/workflow_exports_minimal.qet`, the same stabilized fixture used by export-equivalence coverage, for asserted wire numbers and terminal endpoints.
- Existing fixtures were not modified.
- The test runs four CLI hooks over the same fixture: `--export-nets`, `--export-wiring`, `--export-wires`, and `--info`.
- The test uses `CliTestUtils::runQetCli`, `readJsonObject`, `readFile`, `parseSemicolonCsv`, and `columnSet`; all outputs are written under `QTemporaryDir` and child CLI processes run with `QT_QPA_PLATFORM=offscreen`.
- Stable asserted facts: one diagram, nine elements, seven conductors; wire numbers `W005`-`W011`; exact sorted live-net terminal sets from `--export-nets`; wiring endpoint terminal pairs from `--export-wiring`; and the same wire set from `--export-wires`.
- No terminal-strip bridge, report-link traversal, or isolating-terminal behavior is asserted in this slice because those semantics were not needed for a stable first Terminal/Potential coverage test.
- `tests/qttest/CMakeLists.txt` registers `tst_terminal_potential_exports` as a test-only qttest target depending on `qelectrotech`.

Verification:
- `/opt/homebrew/bin/cmake --build build/baseline --target tst_terminal_potential_exports --parallel` passed.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_terminal_potential_exports --output-on-failure` passed: 1/1.
- `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(terminal_potential_exports|cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure` passed: 4/4.

Boundaries and risks:
- Production source under `sources/` was not changed.
- `elements/` was not changed.
- Existing fixture files were not changed.
- Fixture is suitable for terminal/potential export coverage but does not prove terminal-strip bridges, report cross-folio traversal, isolating terminals, master/slave links, or autonum undo/redo.

Recommended next step:
- Review the Terminal/Potential slice. If clean, proceed to P0 option A planning/implementation for Master/Slave link persistence, with special attention to fixture semantics for `group_index`.

## Contact CrossRef Read-only Projection Prototype
Implemented prototype:
- `sources/contactcrossrefprojectionservice.h/.cpp` adds a narrow non-UI projection service. It reads existing `QETProject` / `Diagram` / `Element` runtime state and does not write XML, mutate links, allocate UUIDs, touch `LinkElementCommand`, touch `CrossRefItem`, or alter UI/export surfaces.
- `ContactMasterProjection` exposes master UUID, label, folio, master type, declared contact groups, linked-slave count, `ContactUsage` usage, declared capacity, and validation messages.
- `ContactAssignmentProjection` exposes master/slave UUIDs, labels, folios, `group_index`, group-resolution state, slave contact type/subtype/count, resolved group metadata, and validation messages.
- Group metadata is derived from `ElementData::m_slave_contact_groups`. Link assignment is derived from `Element::linkedElementsReadOnly()` and `Element::groupIndexForElement()`. Usage/capacity reuse `MasterElement::contactUsage()` and `MasterElement::contactCapacity()`.
- Ordering is deterministic for tests: masters sort by folio, label, UUID; assignments sort by group index, label, UUID.
- The service deliberately remains a projection, not an owner: no caching contract, no persistence source, no UI/rendering dependency, no Device/Function core decision.

Test implementation:
- `tests/qttest/tst_contactcrossrefprojectionservice.cpp` loads the existing corrected test fixture `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` through `QETProject`.
- The test sets CLI-like safe environment details before load: `QT_QPA_PLATFORM=offscreen`, `QT_HASH_SEED=0`, QCoreApplication organization/application metadata, `QETProject::setBackupEnabled(false)`, `QET::QetMessageBox::setNonInteractive(true)`, and `QDomImplementation::ReturnNullNode`.
- The test asserts one master `KMS`, two contact groups, `group_index` 0 -> NO terminals `13/14`, `group_index` 1 -> NC terminals `21/22`, linked slave labels `KMS-NO` / `KMS-NC`, usage 1 NO + 1 NC, capacity 1 NO + 1 NC, and no validation messages.
- `tests/qttest/CMakeLists.txt` registers `tst_contactcrossrefprojectionservice`. Because direct `QETProject` load needs the real project object graph, the target compiles the QET source list except `main.cpp`, links `SingleApplication`, SQLite, pugixml, Qt/KF-private libraries, adds all `sources/` include directories, and includes `qelectrotech.qrc` plus `ico/icon-themes.qrc`. Without those QET resources, the direct project-load test segfaulted during fixture construction; adding resources made the test pass.

Verification:
- `cmake --build build/baseline --target tst_contactcrossrefprojectionservice` passed.
- `cmake --build build/baseline --target qelectrotech` passed after registering the service in `cmake/qet_compilation_vars.cmake`.
- `ctest --test-dir build/baseline -R 'tst_contactcrossrefprojectionservice|tst_master_slave_links|tst_contactusage' --output-on-failure` passed: 3/3.
- `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed: 6/6.

Risks / boundaries:
- The qttest target is heavy because QET does not yet expose a small library target for project-loading tests. This is acceptable for prototype evidence but should be reviewed before merge.
- The service currently reports validation messages for missing/out-of-range group indices and type mismatch only; duplicate group assignment and PLC-specific behavior remain unverified.
- CrossRef rendering/click geometry remains out of scope and unverified.
- No new fixture was added for invalid `group_index`; negative coverage remains a next-step candidate.

Recommended next step:
- Review the Contact/CrossRef read-only projection prototype. If clean, decide whether to refine build integration/test target weight or add the first negative projection test for invalid/out-of-range `group_index`.

## Contact CrossRef Read-only Projection Prototype Review
Review result:
- No blocking findings found in `sources/contactcrossrefprojectionservice.h/.cpp`, `cmake/qet_compilation_vars.cmake`, `tests/qttest/tst_contactcrossrefprojectionservice.cpp`, or related Progress/Brain notes.
- API remains narrow and reversible: `masters(QETProject&)` and `assignments(QETProject&)` return value projections only. The non-const project reference is driven by existing QET APIs such as `Diagram::elements()` rather than by intended mutation; link traversal now uses `Element::linkedElementsReadOnly() const` to avoid the sorting side effect of `Element::linkedElements()`.
- Prototype is read-only by inspection: no XML writes, no UUID allocation, no `set*` calls, no link command usage, no cache ownership, no project DB writes, no CLI/export surface change, and no UI/CrossRef rendering dependency.
- The service boundary follows the reviewed spec: current `Element` links, `group_index`, `ElementData`, and `ContactUsage` are sources of truth; projection records do not become owners.

CMake/testtarget assessment:
- Current qttest target is functionally valid but heavy: `tst_contactcrossrefprojectionservice` compiles nearly the full `QET_SRC_FILES` minus `main.cpp`, adds all `sources/` include directories, links `SingleApplication`, SQLite, pugixml, Qt/KF-private libraries, and includes QET resources.
- This is acceptable for the narrow prototype because direct `QETProject` fixture loading otherwise lacks the application object graph/resources. It should not be repeated casually for many tests.
- Future cleanup candidate: introduce a shared qttest helper/library target or a project-load test library so future domain-projection tests do not duplicate heavyweight target wiring.

Coverage gaps:
- Positive projection is covered for one valid master with two linked slaves and resolving `group_index` values.
- Missing negative projection cases:
  - slave linked to master with missing `group_index`;
  - slave linked with out-of-range `group_index`;
  - slave contact type differs from resolved group type;
  - duplicate slave assignments to the same contact group;
  - master with no declared `slaveContactGroups`;
  - PLC master/slave projection boundary remains explicit unverified/out of v1 scope.

Recommended next step:
- Add a focused test-only negative projection slice using the existing service API. Prefer one small new test fixture or a carefully scoped synthetic project-load fixture that covers missing/out-of-range `group_index` and type mismatch first; leave duplicate assignment and PLC boundary for a follow-up unless they can be represented without new semantics.

## Contact CrossRef Negative Projection Test Slice
Implemented test-only negative projection coverage:
- `tests/qttest/tst_contactcrossrefprojectionservice.cpp` now derives temporary XML variants from `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` in `QTemporaryDir`; no new fixture file was added.
- Missing `group_index`: the NC slave link keeps the UUID but drops `group_index`. Expected projection: assignment `group_index == -1`, `group_index_resolves == false`, assignment validation contains `missing group_index`, and master validation contains the linked-slave missing-index message.
- Out-of-range `group_index`: the NC slave link uses `group_index="99"`. Expected projection: assignment preserves `99`, does not resolve, assignment validation contains `out-of-range group_index 99`, and master validation contains the linked-slave out-of-range message.
- Type mismatch: the NC slave link uses `group_index="0"`, which resolves to the NO group. Expected projection: assignment resolves the group, but validation contains `slave contact type NC differs from group type NO`.

Verification:
- `cmake --build build/baseline --target tst_contactcrossrefprojectionservice` passed.
- `ctest --test-dir build/baseline -R 'tst_contactcrossrefprojectionservice' --output-on-failure` passed: 1/1.
- `cmake --build build/baseline --target qelectrotech tst_contactcrossrefprojectionservice` passed.
- `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed: 6/6.

Boundaries:
- No production code or service API was changed.
- No persistence/XML schema, UI/rendering, `LinkElementCommand`, CLI/export, or Device/Function core behavior was changed.
- Duplicate group assignment and PLC semantics remain intentionally untested in this slice.

Recommended next step:
- Review the negative projection test slice. If clean, the next technical priority is duplicate-assignment coverage if it can be represented without new semantics; otherwise address the qttest target weight by extracting shared project-load test wiring.

## Important Source Locations
- `CMakeLists.txt`: top-level CMake build definition.
- `INSTALL.md`: current build instructions and dependency list.
- `README.md`: project overview, repository URL, version, license, and technical choices.
- `sources/`: main application source tree.
- `elements/`: bundled QElectroTech element collection.
- `tests/`: test subdirectory enabled by default via `PACKAGE_TESTS`.
- `sources/qetproject.h/.cpp`: project aggregate, XML load/save, default properties, diagrams, terminal strips, project DB, undo stack.
- `sources/diagram.h/.cpp`: folio/page scene, graphics items, XML import/export, selection, page properties.
- `sources/qetgraphicsitem/element.h/.cpp`: placed element/symbol runtime object, terminals, text groups, UUID links, XML.
- `sources/properties/elementdata.h/.cpp`: element kind/type metadata, master/slave/contact/PLC/terminal metadata.
- `sources/qetgraphicsitem/masterelement.cpp`, `slaveelement.cpp`, `reportelement.cpp`, `crossrefitem.*`: linking and cross-reference/contact rendering.
- `sources/qetgraphicsitem/conductor.h/.cpp`, `terminal.h/.cpp`: conductor-terminal connection model and potential traversal.
- `sources/TerminalStrip/`: terminal-strip domain model, UI, graphics item, and undo commands.
- `sources/dataBase/projectdatabase.*`: derived SQLite reporting database.
- `sources/bomexport.cpp`, `sources/conductornumexport.cpp`, `sources/wiringlistexport.*`, `sources/cli_export.cpp`: export/report surfaces.
- `sources/undocommand/` and `sources/editor/UndoCommand/`: project/diagram/editor undo-command families.

## Dependencies
Verified from `INSTALL.md` and `CMakeLists.txt`:
- CMake.
- C++17 compiler.
- Qt 6 with Widgets and required private GUI headers.
- SQLite3.
- Qt Linguist tools.
- pugixml, fetched by CMake if no system package is available.
- Qt Test when building tests.
- KDE Frameworks/KF6 optional and normally disabled on macOS.
- QtPdf optional.

Local baseline dependency versions verified on 2026-09-21:
- CMake 4.4.3.
- Ninja 1.13.2.
- Qt 6.11.2.
- SQLite 3.53.4 via Homebrew; CMake linked the macOS SDK SQLite 3.54.0 during configure.

## Technical Constraints
- Out-of-source builds only.
- macOS builds normally use `-DBUILD_WITH_KF=OFF`.
- Missing Qt6 private GUI headers are a configure-time hard failure.
- The verified macOS baseline used Ninja and Homebrew Qt with `CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt`.
- Interactive GUI behavior is not yet verified; only non-interactive CLI startup checks are verified.

## Compatibility Requirements
Target platforms from project documentation: Windows, GNU/Linux, macOS, and BSDs.

## Accepted Decisions
- `origin` is the local fork: `https://github.com/WeltenbummlerKA/qelectrotech.git`.
- `upstream` is the official QElectroTech source mirror: `https://github.com/qelectrotech/qelectrotech-source-mirror.git`.

## Rejected Approaches
- No in-source build.
- No source modifications during baseline.

## Open Architecture Questions
- Is the future device/function model meant to remain element-centric, or should it become a separate project-domain layer referenced by symbols?
- Should contact mirror/contact capacity become its own domain model instead of remaining embedded in `MasterElement` plus `CrossRefItem` rendering?
- How should terminal strips, graphical terminal elements, and conductor potentials relate in one canonical terminal/cable model?
- Which source of truth should future reports use: live object graph, derived SQLite views, XML round-trip, or a new domain database?
- Cable modeling is unclear after the first pass and needs focused investigation.
- Cable modeling remains field-level after Phase 3: persisted conductor attributes exist, but cable/bus UI is disabled and no canonical cable domain object was found in the traced files.
- Potential traversal currently depends on live graphics/element objects. It is unclear whether future reporting should trust live traversal, derived SQLite views, XML parsing exports, or consolidate them behind one tested domain source.
- Terminal strips point at terminal elements by element UUID. It is unclear whether future Klemmen logic should continue to require terminal elements on folios or also support non-graphical/reserved terminals as first-class project objects.
- Terminal strip bridges and conductor potentials need explicit behavioral verification: the traced potential traversal sees terminal elements and reports, but no test or direct proof was found that terminal-strip bridge state itself alters electrical net traversal.
- Numbering/autonum is spread across project defaults, diagram sequential state, element/conductor sequence structs, and UI; exact lifecycle needs deeper tracing before changes.
- Phase 4 confirms numbering/autonum is distributed rather than centralized: project `NumerotationContext`s, diagram selected names/folio hashes, per-object `sequentialNumbers`, visible labels/text, undo commands, and exports all participate.
- Undo coverage is uneven: conductor autonum counter increments use `SetAutoNumContextCommand`, conductor text/property changes use `QPropertyUndoCommand`, terminal numbering uses `ChangeElementInformationCommand`, while diagram default conductor property changes in `DiagramPropertiesDialog` still carry a TODO for undo.
- Export equivalence risk remains: live graph exports, SQLite wiring view, CSV conductor number export, and XML wiring export consume different subsets of conductor text/properties.
- XML/save determinism has been improved in targeted areas, but Phase 5 did not find a broad round-trip test that locks the whole `.qet` output down.
- Legacy compatibility is intentionally mixed: some missing UUIDs are derived and persisted on next save, while conductor UUIDs generated for legacy files are runtime-only to avoid churn.
- The project database is derived from live objects and not serialized; any load/save or object-construction change can affect report/export output even when XML parsing still succeeds.
- UI and domain behavior are tightly coupled through `QGraphicsScene`, `QGraphicsItem`, dialogs, and undo commands; seams for non-GUI logic are not yet established.
- For master/slave links, should group assignment be owned by the master, the slave, the link itself, or a future independent contact model? Current code stores it in `Element::m_group_index_map` keyed by linked element.
- Should `kindInformations()` string data remain part of contact/XRef logic, or should all contact state use `ElementData` consistently? Current code uses both.
- Is PLC table rendering supposed to live in `CrossRefItem`, `Element` painting, or one canonical renderer? Current evidence shows PLC-specific paths in both areas.
- Which end-to-end behaviors require tests before any contact-spiegel change: XML round-trip, undo/redo, group assignment, capacity/fullness, rendering, or CLI/PDF export links?

## Research Findings
- GitHub organization `qelectrotech` lists `qelectrotech-source-mirror` as the main QET repository.
- `README.md` points source cloning to `https://github.com/qelectrotech/qelectrotech-source-mirror.git`.
- Project homepage is `https://qelectrotech.org/`.
- License is GNU/GPL per `README.md`; source headers specify GPL version 2 or later.

## Lessons Learned
- The local repository already existed at the expected fork commit before baseline documentation began.
- Homebrew dependency setup resolved the missing active toolchain for baseline work.
- The unchanged QElectroTech baseline at commit `d7052e396b42c2b7b14b6b2f7aecb5d0a0f54cd0` configures, builds, and passes its current CTest suite on this host.
- CLI startup checks are useful for safe non-interactive verification, but they do not prove interactive macOS GUI operation.
- First-pass architecture evidence points to a mature graphics-scene-centric application, not a cleanly separated CAE domain core.
- The safest next analysis is a narrow trace of one workflow at a time: load/save/linking/contact mirror/terminal strip/export, instead of broad refactoring assumptions.
- Master/slave/contact behavior is spread across element metadata parsing, placed element persistence, UI selection, undo commands, runtime link objects, and graphics rendering. Any future change here has high regression risk unless tests are added first.
- Terminal/cable/potential behavior is split across live graphics items, project-level terminal-strip objects, conductor properties, a derived SQLite reporting database, XML-parsing exports, and CLI live graph exports. Any future change here needs equivalence tests before source changes.
- Numbering changes have high regression risk because a visible number can be derived from a formula, stored as resolved text, advanced through a project counter, cached as per-object sequence state, mirrored into SQLite, and consumed differently by each export path.
- Load/save changes have high regression risk because project XML, graphics-scene item creation, element collection resolution, terminal/conductor identity, terminal-strip resolution, project DB rebuild, and exports are coupled by lifecycle timing rather than one isolated parser.

## Contact/CrossRef Negative Projection Slice Review
- Review result: no blocking findings in `tests/qttest/tst_contactcrossrefprojectionservice.cpp`. The negative cases remain test-only, derive temporary XML variants from `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` inside `QTemporaryDir`, and do not modify service/API/production code.
- Verified negative cases: missing `group_index`, out-of-range `group_index`, and slave/group contact type mismatch. Expectations assert deterministic projection/validation messages; duplicate assignment and PLC semantics remain explicitly unverified.
- Verification on 2026-09-21: `cmake --build build/baseline --target qelectrotech tst_contactcrossrefprojectionservice` succeeded with no rebuild work; `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Forbidden-area check: `git diff -- sources elements CMakeLists.txt` showed no diff against prohibited production/element/top-level CMake paths for this review task. Existing approved prototype files under `sources/contactcrossrefprojectionservice.*` and `cmake/qet_compilation_vars.cmake` remain part of prior work, not changed by this review.
- Next options: first address qttest target weight if more project-load service tests are expected; otherwise add duplicate-assignment coverage only if semantics can be stated without service/API changes. PLC behavior remains a later boundary analysis.

## CMake/Testtarget Deweighting Analysis
- Detailed analysis is documented in `Decision_Test_Target_Deweighting.md`.
- Current evidence: `tests/qttest/CMakeLists.txt` builds most existing qttest targets narrowly, while `tst_contactcrossrefprojectionservice` constructs `QET_QTTEST_APP_SRC` from `QET_SRC_FILES` minus `sources/main.cpp`, globs all `sources/*.h` include directories, embeds `qelectrotech.qrc` and `ico/icon-themes.qrc`, and links broad application dependencies.
- Reason for weight: the test loads a real `.qet` fixture through `QETProject`, which pulls in the application object graph and resources. Earlier prototype verification recorded that missing QET resources caused a crash during project-load testing.
- Options captured: shared project-load QtTest app library/target, setup-only test-support helper, curated minimal source-subset target, CLI-only black-box tests, and later production core split.
- Recommendation: prefer a qttest-private shared project-load support target or registration function first, with a small setup helper only if duplication grows. This preserves the verified QET chassis behavior while preventing more copy-pasted full-app CMake blocks.
- Confidence: Likely. A true source-subset/core split may reduce build weight more, but it is higher risk and would become an architecture decision rather than a testtarget cleanup.
- Next decision: approve or reject the test-only CMake refactor that migrates only `tst_contactcrossrefprojectionservice` to a shared project-load test target/function with unchanged assertions and fixtures.

## Project-Load QtTest CMake Refactor
- Implemented Option A from `Decision_Test_Target_Deweighting.md` as a qttest-private CMake registration function `add_qet_project_load_qtest` in `tests/qttest/CMakeLists.txt`.
- Migrated only `tst_contactcrossrefprojectionservice` to the helper. The helper preserves the existing source/resource/library bundle: `QET_SRC_FILES` minus `sources/main.cpp`, `qelectrotech.qrc`, `ico/icon-themes.qrc`, qttest include directories, pugixml, SingleApplication, SQLite, Qt/KF private libraries, and `BUILD_WITHOUT_KF` when applicable.
- No assertions, fixture content, production source, production CMake outside qttest, XML schema, UI/rendering, `LinkElementCommand`, CLI/export, or Device/Function-Core behavior were changed.
- Verification on 2026-09-21: `cmake --build build/baseline --target qelectrotech tst_contactcrossrefprojectionservice` succeeded and re-ran CMake; focused `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Effect: maintenance/registration weight is lower for future project-load qttests; compile/link weight is intentionally not lower yet because the real `QETProject` load still needs the app object graph and resources.
- Remaining risk: the helper can become a default for tests that should remain lighter CLI or unit tests. Future use should require a project-load justification.

## Project-Load QtTest CMake Refactor Review
- Review result: no blocking findings in `tests/qttest/CMakeLists.txt`. `add_qet_project_load_qtest` is scoped to qttest, currently used only by `tst_contactcrossrefprojectionservice`, and preserves the previous source/resource/library bundle for project-load tests.
- Existing lightweight tests remain on direct `add_executable`/CLI patterns. No assertion file, fixture, production source, production CMake outside qttest, XML schema, UI/rendering, `LinkElementCommand`, CLI/export, or Device/Function-Core behavior changed.
- Verification on 2026-09-21: `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Forbidden-area check: `git diff -- sources elements CMakeLists.txt tests/qttest/tst_contactcrossrefprojectionservice.cpp tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` showed no diff.
- Next task: duplicate-assignment coverage for Contact/CrossRef only if the expected semantics can be expressed with the existing read-only service/API and temporary test variants. Stop before implementation if duplicate assignment requires API changes, production behavior changes, new fixture semantics, XML schema assumptions, UI/rendering assumptions, or PLC/Device-Core decisions.

## Contact/CrossRef Duplicate Assignment Test Slice
- Implemented as test-only extension of `tests/qttest/tst_contactcrossrefprojectionservice.cpp`; no new fixture file was added.
- The duplicate case derives a temporary `.qet` variant from `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` in `QTemporaryDir`.
- Variant details: the NC slave link is changed from `group_index="1"` to `group_index="0"` and that embedded slave definition's state is changed from `NC` to `NO`, producing two NO slaves resolved to the same master contact group without introducing a type-mismatch validation case.
- Existing `ContactCrossRefProjectionService` API can express this deterministically without changes: both assignments resolve to group 0, master usage reports `NO=2` with capacity `NO=1`, and no new validation semantics are invented by the test.
- Verification on 2026-09-22: `cmake --build build/baseline --target tst_contactcrossrefprojectionservice` passed; focused `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Forbidden-area check: `git diff -- sources elements CMakeLists.txt tests/qttest/fixtures/master_slave_links_group_index_minimal.qet` showed no diff.
- Remaining boundary: the service currently observes over-capacity via usage/capacity and resolved assignments; it does not emit a duplicate-assignment validation message. Adding such a message would be a service/API behavior decision, not part of this slice.

## Contact/CrossRef Duplicate Assignment Test Slice Review
- Review result: no blocking findings in `tests/qttest/tst_contactcrossrefprojectionservice.cpp`.
- Scope check: the slice remains test-only, creates the duplicate case only as a temporary XML variant in `QTemporaryDir`, leaves fixture files unchanged, and does not change production service/API behavior.
- Assertion boundary is correct for the current API: the test observes duplicate assignment through resolved projection records and usage-vs-capacity (`NO=2`, capacity `NO=1`), and deliberately does not assert a duplicate-specific validation message.
- PLC remains outside this slice; no PLC, UI/rendering, persistence, XML schema, `LinkElementCommand`, CLI/export, or Device/Function-Core behavior is claimed or tested.
- Verification on 2026-09-22: focused `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Next options: A discuss explicit Duplicate-Validation as a service/API extension; B prepare PLC boundary analysis; C prepare a report/commit-ready review of the accumulated branch. Technical recommendation: choose C first, because the branch now contains several connected test/support/prototype slices and should be audited as a coherent package before broadening semantics.

## Contact/CrossRef Duplicate Validation Decision
- Decision: duplicate assignment is a read-only Projection validation/diagnostic, not UI behavior, persistence state, XML schema migration, or Device/Core ownership.
- Implementation shape: `ContactAssignmentProjection` exposes `duplicate_group_assignment`; affected assignments receive `duplicate group_index N assignment`, and the master projection receives `group_index N is assigned to M linked slaves`.
- Boundary: the check is derived only from loaded `Element` links and resolving `group_index` values. It does not create, repair, delete, or rewrite assignments.
- PLC remains handled by `PlcIoProjectionService`'s existing `duplicate_group_index` flag; no PLC semantic broadening was made in this slice.

## Commit-Readiness Review
- Current branch remains `master` at baseline commit `d7052e396b42c2b7b14b6b2f7aecb5d0a0f54cd0`; no commit, push, branch switch, reset, or staging was performed.
- Remotes remain `origin=https://github.com/WeltenbummlerKA/qelectrotech.git` and `upstream=https://github.com/qelectrotech/qelectrotech-source-mirror.git`.
- Proposed slices:
  - Local baseline/documentation: `.gitignore`, `Progress.md`, `Brain.md`.
  - CAE and decision documentation: `CAE_Reference_Matrix.md`, `Decision_Contact_CrossRef_Ownership.md`, `Spec_Contact_CrossRef_ReadOnly_Service.md`, `Decision_Test_Target_Deweighting.md`.
  - CLI/test infrastructure and export fixtures: `tests/qttest/cli_test_utils.h`, CLI/export/terminal/master-slave tests, minimal qttest fixtures, and related `tests/qttest/CMakeLists.txt` entries.
  - Contact/CrossRef read-only prototype: `sources/contactcrossrefprojectionservice.*`, `cmake/qet_compilation_vars.cmake`, and `tests/qttest/tst_contactcrossrefprojectionservice.cpp`.
  - qttest project-load CMake helper: the `add_qet_project_load_qtest` part of `tests/qttest/CMakeLists.txt`.
- Working-tree cleanup concerns before commit: `build/` is an untracked build tree (~388 MB) and should not be committed; multiple `.DS_Store` files exist in the root, sources, tests, icon folders, and submodules. `SingleApplication` and `elements` show dirty submodule state due to untracked `.DS_Store` files inside those submodules.
- Fixture sizes checked after deduplication: `workflow_exports_minimal.qet` ~91 KB and `master_slave_links_group_index_minimal.qet` ~10 KB. No large test fixture remains from the earlier master/slave attempt.
- Final focused verification on 2026-09-22: `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Recommended next step: clean/ignore unwanted local artifacts before any commit preparation, then review the accumulated diff by the logical slices above. After that, decide whether to keep documentation-heavy analysis files in the same branch or separate them from test/prototype code.

## Working Tree Artifact Cleanup
- Local-only ignore cleanup: `.git/info/exclude` now ignores `build/`. This is not a tracked project change and keeps the 388 MB local build tree out of commit candidates.
- Removed local `.DS_Store` Finder artifacts from the repository tree, including submodule working trees.
- After cleanup, `find . -name .DS_Store -print` returned no paths.
- Submodule dirtiness caused by untracked `.DS_Store` files is cleared: `git -C SingleApplication status --short` and `git -C elements status --short` returned empty output, and `git submodule status` shows clean pinned commits.
- Remaining working-tree entries are the intended project/documentation/test/prototype changes; no commit, staging, push, branch switch, reset, submodule reset, submodule clean, or checkout was performed.

## Code/Test Slice Review
- Review result: no blocking findings in the current code/test slice.
- Reviewed files: `sources/contactcrossrefprojectionservice.*`, `cmake/qet_compilation_vars.cmake`, `tests/qttest/CMakeLists.txt`, `tests/qttest/cli_test_utils.h`, new qttest files, and new qttest fixtures.
- Scope check: the projection service is read-only over loaded `QETProject`/`Element` state; no persistence/XML schema, UI/rendering, `LinkElementCommand`, CLI/export behavior, or Device/Function-Core behavior is changed by the service.
- Test/CMake check: lightweight CLI tests remain direct qttest executables depending on `qelectrotech`; `add_qet_project_load_qtest(...)` is used only for the project-load projection-service test.
- Fixture check: new fixtures are test-only and small enough for review (`master_slave_links_group_index_minimal.qet` ~10 KB, shared `workflow_exports_minimal.qet` ~91 KB). No build outputs, duplicate byte-identical fixtures, or large golden dumps were found in the fixture set.

## Slice B Fixture Deduplication
- Stopper: `tests/qttest/fixtures/export_equivalence_minimal.qet` and `tests/qttest/fixtures/terminal_potential_minimal.qet` were byte-identical (`sha256 360cf59c0245481d5f9d562121e1fddf384bdd9ae5e9c590cba840399eeb726f`), while only the two CLI export tests referenced them.
- Decision: use one shared, role-neutral test-only fixture named `tests/qttest/fixtures/workflow_exports_minimal.qet` because both tests assert different stable export views over the same workflow graph. This avoids preserving two identical files for speculative future semantics.
- Updated tests: `tst_cli_export_equivalence` and `tst_terminal_potential_exports` both resolve `fixtures/workflow_exports_minimal.qet`.
- Removed duplicate: `tests/qttest/fixtures/terminal_potential_minimal.qet`. The old `export_equivalence_minimal.qet` content was renamed to the shared fixture name.
- Verification on 2026-09-22: focused `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure` passed 6/6.
- Suggested code/test commit split: CLI/export regression tests and helper; Contact/CrossRef read-only projection service and projection tests; optional qttest project-load helper as part of the projection-service commit or as a separate small test-build commit.

## Slice C P1 Read-only Link Accessor Fix
- Stopper: `ContactCrossRefProjectionService` previously called `Element::linkedElements()`, whose inline implementation sorts `connected_elements` before returning it. That made the projection technically mutating despite no intentional ownership, persistence, UI, command, or export changes.
- Fix decision: add a minimal `Element::linkedElementsReadOnly() const` accessor returning a copy of the current `connected_elements` order without sorting, while preserving the existing `linkedElements()` behavior unchanged for all other callers.
- Service boundary after fix: `ContactCrossRefProjectionService` now reads master/slave links through `linkedElementsReadOnly()` and still uses `groupIndexForElement()` for group assignment lookup. No `Element`, `MasterElement`, `SlaveElement`, `LinkElementCommand`, `CrossRefItem`, XML/persistence, UI/rendering, CLI/export, fixture, or Device/Function-Core behavior was otherwise changed.
- Remaining review point: the new accessor exposes current internal order; the service already applies deterministic sorting for assignments and uses only counts/validation over the unsorted copy where order is irrelevant.

## PLC IO Semantics Read-only Boundary
- Minimal PLC semantics for the next CAE slice are IO direction from `ElementData::PlcIOType`, address, current signal kind/type, IO terminal labels/count, linked slave assignment via `group_index`, and terminal/potential evidence as context only.
- Source of truth remains master `ElementData::PlcMasterData::ios` plus placed master/slave links and `Element::groupIndexForElement()`. Existing XML `links_uuids/link_uuid/@group_index` remains compatibility persistence for the assignment.
- Slave `plc_type`, `plc_address`, `plc_function`, `plc_comment`, `plc_crossref`, `plc_tc`, and `plc_t1`..`plc_t4` are projection copies populated for display/formula behavior, not PLC truth.
- Warning-only candidates: no IO rows, missing/out-of-range/duplicate `group_index`, empty address, invalid terminal count, terminal label/count mismatch, insufficient slave terminals, and stale slave `plc_*` copy values.
- Explicitly out of scope: blocking link errors, schema migration, repairing assignments, writing slave projection fields, inferring address semantics, vendor-specific validation, PLC datatype/device core, terminal-strip ownership, potential-based auto-assignment, UI/rendering changes, and Default-KF/ECM behavior.
- Next smallest implementation slice: extend `PlcIoProjectionService` with normalized direction, terminal count/labels, and deterministic warning messages, covered by one focused QtTest using existing temporary-fixture patterns.

## PLC IO Projection Warning Slice
- Implemented in `PlcIoProjectionService` as read-only projection fields only: `direction`, `terminal_count`, existing/effective `terminal_labels`, warning flags, and deterministic warning strings.
- Current warning coverage: unlinked master IO row, out-of-range `group_index`, duplicate `group_index`, empty master IO address, and explicit terminal-label count mismatch against `terminalCount`.
- Boundary preserved: no UI/rendering changes, no persistence/XML schema changes, no migration, no slave `plc_*` rewrite, no address-format inference, and no blocking link behavior.
- Focused coverage is in `tests/qttest/tst_plcioprojectionservice.cpp` using the existing temporary fixture pattern.
- Next smallest candidate: decide whether to add stale slave `plc_*` copy warnings or slave-terminal-count evidence warnings; both should remain warning-only and projection-derived.
