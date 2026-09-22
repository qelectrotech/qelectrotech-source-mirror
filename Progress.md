# Progress

## 2026-09-21 - CAE Reference Analysis Phase 1

Completed:
- Created `CAE_Reference_Matrix.md` as a standalone, non-implementing reference matrix.
- Compared QET current findings against public vendor evidence for EPLAN Electric P8, WSCAD ELECTRIX, Zuken E3.series, AUCOTEC Engineering Base, SEE Electrical, and AmpereSoft ProPlan / Moeller-ProPlan only where publicly supportable.
- Covered workflows: contact mirrors/cross-references, device/function/symbol/article separation, terminals/cables/potentials, project/page structure, numbering/autonum, reports, data model/project DB, and import/export compatibility.

Evidence:
- QET evidence from `Brain.md`, `Progress.md`, and earlier code traces of `sources/qetproject.*`, `sources/diagram.*`, `sources/qetgraphicsitem/*`, `sources/TerminalStrip/*`, `sources/autoNum/*`, `sources/dataBase/projectdatabase.*`, `sources/cli_export.cpp`, and export helpers.
- Public vendor sources only: EPLAN product page, WSCAD product/release pages, Zuken E3.series product page, AUCOTEC Engineering Base product page, SEE Electrical public product table, AmpereSoft ProPlan product page.

Open risks:
- Public vendor pages describe principles and capabilities, not internal data schemas.
- Proprietary native formats, report template languages, device portals, and commercial catalog licensing remain unverified and must not be copied or assumed.
- ProPlan/Moeller lineage is only treated through current public AmpereSoft ProPlan evidence; historical Eaton/Moeller internals remain unverified.

Next recommended task:
- Perform a narrow decision-prep analysis for one ownership question before architecture work: either contact/cross-reference ownership or cable/core/potential ownership, using `CAE_Reference_Matrix.md` plus QET code evidence.

## 2026-09-21 - Contact/CrossRef Ownership Decision Analysis

Completed:
- Created `Decision_Contact_CrossRef_Ownership.md` as a non-implementing decision-prep document.
- Re-read QET contact/cross-reference code evidence for `ElementData`, `Element`, `MasterElement`, `ReportElement`, `LinkElementCommand`, `ContactUsage`, `CrossRefItem`, XML link persistence, `--export-links`, and current tests.
- Compared QET current ownership against the public CAE principles captured in `CAE_Reference_Matrix.md`.
- Documented options A-D without selecting an architecture: status quo extension, derived Contact/CrossRef service, dedicated `ContactAssignment` domain model, or defer into larger Device/Function core.

Open decisions:
- Whether `group_index` remains a compatibility detail or becomes/feeds a long-term assignment key.
- Whether contact assignment is owned by master, slave, link object, or independent domain object.
- Whether `--export-links` should later expose group/contact/capacity facts or a separate export/report surface should be added.

Next recommended task:
- Prepare the next test-only stabilization slice for Master/Slave link persistence and `group_index` observability, or perform the parallel ownership decision-prep for Cable/Core/Potential.

## 2026-09-21 - Master/Slave Link Persistence Test Slice

Completed:
- Added `tst_master_slave_links` under `tests/qttest/`.
- Replaced the oversized `tests/qttest/fixtures/master_slave_links_group_index.qet` with `tests/qttest/fixtures/master_slave_links_group_index_minimal.qet`.
- No existing library element/example with `slaveContactGroups` was found; the new fixture is test-only and constructed from the QET `ElementData` parser/writer structure for `slaveContactGroups`.
- Registered the new test in `tests/qttest/CMakeLists.txt`.

Verification target:
- `--resave` must preserve the selected master link UUIDs and `group_index` attributes.
- `--export-links` must report the selected `KMS` master row as linked and the fixture must have no `UNRESOLVED` status.

Open risk:
- The minimal `slaveContactGroups` fixture is not copied from an existing library definition; it is based on QET parser/writer evidence and verified by load/resave/export tests.

## 2026-09-21 - Master/Slave Test Review and Contact/CrossRef Read-only Spec

Completed:
- Reviewed corrected `tst_master_slave_links`, minimal fixture, CMake registration, and Brain/Progress consistency.
- Re-ran focused verification for the new Master/Slave test plus affected CLI tests.
- Created `Spec_Contact_CrossRef_ReadOnly_Service.md` as a non-implementing specification.

Review result:
- No blocking findings found in the corrected test slice.
- Fixture now contains embedded `slaveContactGroups` and `group_index` values that semantically reference those groups.
- The test remains CLI/XML-only and does not claim CrossRef rendering or GUI behavior.

Specification result:
- Defined a read-only projection service over existing `Element` links, `ElementData`, `group_index`, `ContactUsage`, XML link persistence, and project state.
- Explicitly kept ownership, persistence, rendering, and Device/Function core decisions out of scope.

Next recommended task:
- Review the read-only service specification. If accepted, decide whether to prototype the narrow projection service as test-covered code without persistence or UI changes.

## 2026-09-21 - Read-only Service Spec Review

Completed:
- Reviewed `Spec_Contact_CrossRef_ReadOnly_Service.md` against QET code findings, `Decision_Contact_CrossRef_Ownership.md`, `CAE_Reference_Matrix.md`, Brain, and Progress.
- Confirmed the spec remains read-only and does not approve ownership, persistence, UI/rendering, Device/Function core, CLI/export, or migration changes.

Review result:
- No blocking findings.
- Prototype boundary is testable: derive deterministic projection records from `QETProject`/`Element`/`ElementData`/`group_index`/`ContactUsage` without writes or UI dependencies.

Next recommended task:
- If explicitly approved, implement a narrow read-only projection prototype with direct QtTest coverage. Do not change XML persistence, CrossRef rendering, CLI/export behavior, or Device/Function ownership in that slice.

## Current Phase
PLC/CAE feature work paused for documentation-only status correction.

## Current Objective
Keep interactive GUI/runtime status separate from PLC/CAE feature work and document the next optional Runtime-QA/installation slice.

## Completed
- Local repository inspected non-destructively on 2026-09-21.
- Current branch verified as `master`.
- Current commit verified as `d7052e396b42c2b7b14b6b2f7aecb5d0a0f54cd0`.
- `origin` verified as `https://github.com/WeltenbummlerKA/qelectrotech.git`.
- Official project source verified from project/GitHub evidence as `https://github.com/qelectrotech/qelectrotech-source-mirror.git`.
- `upstream` configured to `https://github.com/qelectrotech/qelectrotech-source-mirror.git`.
- Homebrew build dependencies available: `cmake 4.4.3`, `qt 6.11.2`, `sqlite 3.53.4`, `ninja 1.13.2`.
- Out-of-source CMake configure completed in `build/baseline` with `-DBUILD_WITH_KF=OFF`.
- Baseline build completed successfully and produced `build/baseline/qelectrotech.app`.
- CTest completed successfully: 11/11 tests passed.
- Non-interactive application checks completed: `--help` and `--version` both exited successfully; version output is `0.200.1-dev`.
- Initial read-only source survey completed for project/folio/element/link/contact/terminal/conductor/database/export/undo areas.
- `Brain.md` updated with first-pass architecture findings and open questions.
- Read-only end-to-end trace completed for element definition metadata, placed element persistence, UUID/group-index linking, contact usage/capacity, CrossRef rendering, undo command path, save/load, and current tests.
- `Brain.md` updated with Phase 2 master/slave/contact trace findings.
- Read-only end-to-end trace completed for graphical terminals, conductors, potential traversal, terminal strips, cable/bus fields, wiring/net exports, XML persistence, undo command surfaces, and current test coverage.
- `Brain.md` updated with Phase 3 terminal/cable/potential trace findings.
- Read-only end-to-end trace completed for project autonum schemas, formula resolution, sequence/counter state, element/conductor/terminal numbering, conductor text propagation, undo/redo, XML persistence, database updates, and export impact.
- `Brain.md` updated with Phase 4 numbering/autonum/conductor text trace findings.
- Read-only end-to-end trace completed for `.qet` XML file structure, project/diagram/element/conductor/terminal-strip/autonum load/save lifecycle, version/legacy paths, UUID fallbacks, deterministic save ordering, database rebuild timing, export equivalence, and current test coverage.
- `Brain.md` updated with Phase 5 XML/load-save compatibility trace findings.
- Read-only regression-test map completed for master/slave/contact mirror, terminal/potential/wiring, autonum/undo, XML round-trip/legacy, DB/export equivalence, terminal strips, and cable/conductor fields.
- `Brain.md` updated with Phase 6 test coverage and risk findings.
- Read-only fixture specification completed for XML/roundtrip/legacy, export equivalence, master/slave/contacts, terminal strip/bridges, autonum/undo, and cable/conductor fields.
- `Brain.md` updated with Phase 7 fixture/test-design findings.
- Read-only expectation/normalization rules completed for future regression tests across XML, CSV, JSON, SVG/PDF, CLI environment, platform handling, and large smoke examples.
- `Brain.md` updated with Phase 8 test-normalization findings.
- Read-only non-implementing regression-test backlog completed for XML/roundtrip/legacy, export equivalence, master/slave/contact mirror, terminal/potential/wiring, terminal strips/bridges, autonum/undo, cable/conductor fields, and large smoke examples.
- `Brain.md` updated with Phase 9 prioritized test-plan backlog.
- Read-only implementation sequence and review checklist completed for the first future test-only infrastructure slice.
- `Brain.md` updated with Phase 10 first-slice implementation/review findings.
- Implemented `tst_cli_roundtrip_xml` as a test-only QtTest under `tests/qttest/`.
- Registered `tst_cli_roundtrip_xml` in `tests/qttest/CMakeLists.txt` following the existing `tst_conductorselfretrace` binary/CTest pattern.
- The new test uses existing fixture `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, runs two `--resave` passes in `QTemporaryDir`, sets `QT_QPA_PLATFORM=offscreen` for child CLI processes, parses XML with Qt DOM, and compares a normalized project tree.
- Targeted build of `tst_cli_roundtrip_xml` completed successfully.
- Targeted CTest verification passed for `tst_cli_roundtrip_xml`.
- Regression CTest verification passed for `tst_conductorselfretrace` and `tst_cli_roundtrip_xml` together.
- Review completed for `tests/qttest/tst_cli_roundtrip_xml.cpp`, `tests/qttest/CMakeLists.txt`, and the Progress/Brain entries.
- Review checklist result: no production code changes found, no existing fixtures changed, deterministic temp output via `QTemporaryDir`, offscreen child CLI processes, no GUI assumption, no large golden dumps, focused CTest integration.
- Re-run verification after review passed for `tst_cli_roundtrip_xml` and the affected short CLI test set.
- Next P0 slice defined as a planning item only: export-equivalence helpers and a minimal fixture for `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info`.
- Implemented `tst_cli_export_equivalence` as a test-only QtTest under `tests/qttest/`.
- Added new fixture later consolidated as `tests/qttest/fixtures/workflow_exports_minimal.qet`, derived from the existing compact fixture but with stable conductor numbers `W005`-`W011` and non-empty cable/color/section/function fields.
- Registered `tst_cli_export_equivalence` in `tests/qttest/CMakeLists.txt`.
- The new test runs `--export-nets`, `--export-wiring`, `--export-cables`, `--export-wires`, `--export-bom`, and `--info` into `QTemporaryDir`, parses JSON/CSV semantically, normalizes BOM/quotes/line endings/order, and compares stable core facts.
- Targeted build and CTest verification passed for `tst_cli_export_equivalence`.
- Affected CLI-test regression verification passed for `tst_conductorselfretrace`, `tst_cli_roundtrip_xml`, and `tst_cli_export_equivalence`.
- Review completed for `tests/qttest/tst_cli_export_equivalence.cpp`, the workflow export fixture, `tests/qttest/CMakeLists.txt`, and Progress/Brain entries.
- Review checklist result: test-only scope, no production changes, stable fixture semantics for asserted fields, `QTemporaryDir`, offscreen CLI execution, semantic CSV/JSON parsing, no large golden dumps, focused CTest integration, and no existing fixture changes.
- Re-run verification after review passed for `tst_cli_export_equivalence` and the affected short CLI test set.
- Next helper-extraction task defined as a planning item only.
- Added test-only helper `tests/qttest/cli_test_utils.h`.
- Extracted shared CLI runner, file reader, JSON object reader, semicolon CSV parser, and CSV column set helper from `tst_cli_roundtrip_xml.cpp` and `tst_cli_export_equivalence.cpp`.
- Kept XML canonicalization local in `tst_cli_roundtrip_xml.cpp` because it is specific to first/second `--resave` comparison policy.
- Updated `tst_cli_roundtrip_xml.cpp` and `tst_cli_export_equivalence.cpp` to use the shared helper without changing asserted behavior.
- Targeted build passed for `tst_cli_roundtrip_xml` and `tst_cli_export_equivalence`.
- Affected CLI-test regression verification passed for `tst_conductorselfretrace`, `tst_cli_roundtrip_xml`, and `tst_cli_export_equivalence`.
- Review completed for `tests/qttest/cli_test_utils.h`, `tst_cli_roundtrip_xml.cpp`, `tst_cli_export_equivalence.cpp`, qttest CMake registration, and Progress/Brain entries.
- Review checklist result: helper remains test-only and narrow; offscreen/temp-dir behavior preserved; CSV/JSON helpers retain existing semantics; XML policy remains local; no production or fixture changes found.
- Re-run verification after review passed for both CLI tests plus `tst_conductorselfretrace`.
- Next P0 domain-slice options A/B documented, with technical recommendation to implement Terminal/Potential export coverage first.
- Implemented `tst_terminal_potential_exports` as a test-only QtTest under `tests/qttest/`.
- Reused the shared workflow export fixture for stable terminal/potential export assertions.
- Registered `tst_terminal_potential_exports` in `tests/qttest/CMakeLists.txt`.
- The new test runs `--export-nets`, `--export-wiring`, `--export-wires`, and `--info` into `QTemporaryDir`, using `cli_test_utils.h` for CLI/CSV/JSON helpers.
- Assertions compare stable wire numbers, exact expected live-net terminal sets, wiring endpoint terminal pairs, conductor count, element count, and wire-number export set.
- Targeted build and CTest verification passed for `tst_terminal_potential_exports`.
- Affected CLI-test regression verification passed for `tst_conductorselfretrace`, `tst_cli_roundtrip_xml`, `tst_cli_export_equivalence`, and `tst_terminal_potential_exports`.
- Completed and committed the Contact Projection, PLC IO Projection, Duplicate Validation, and PLC Warning-Spec/-Extension slices through `86c5fc9076d295044c41f489e95162b095dfdc8a`.
- PLC warnings remain read-only and do not change UI, persistence, XML schema, Device/Core ownership, or migration behavior.

## In Progress
- None.

## Pending
- Decide later whether and how to mark the verified baseline.
- Full interactive GUI smoke test remains unverified and should only run with the user present for any macOS permission prompts.
- A later optional Runtime-QA/installation slice may verify startup interactively, but it must be handled separately from PLC/CAE feature work.
- Runtime-QA/installation slice scope, if approved later: clarify the exact bundle path, perform a controlled local install or bundle launch test, let the user handle any macOS dialogs, and document the observed result.
- Dedicated deep dives still pending for UI/domain coupling, project database lifecycle, export equivalence, terminal-strip/potential integration tests, and automated regression coverage design.
- Tests for full master/slave XML round-trip, link undo/redo, group-index persistence, CrossRef click map/render behavior, and PLC link propagation remain unverified/missing in this pass.
- High-priority future regression candidates identified, but not implemented: CLI fixture matrix, XML round-trip determinism, DB/export equivalence, terminal-strip round-trip, autonum undo/redo, and master/slave link persistence/render checks.
- Future fixture creation still requires an explicit implementation phase and expected-output policy before files are added.
- Future test implementation still requires explicit approval and should start with normalization helpers before adding behavioral assertions.
- Phase 9 backlog remains a specification only; no test files, fixtures, helpers, or CMake registrations were created.
- Phase 10 first-slice plan remains a specification only; no test harness, helpers, fixtures, CMake registrations, or documentation files were created.
- Full P0 fixture matrix, export equivalence tests, master/slave tests, terminal-strip tests, autonum/undo tests, cable/conductor-field tests, PDF checks, and large smoke examples remain unimplemented.
- Interactive GUI behavior remains unverified.
- Full CTest suite has not been re-run after the PLC warning extension.
- Default KF/ECM build behavior remains open because the active baseline uses `-DBUILD_WITH_KF=OFF`.

## Blocked
- None.

## Decisions Required
- None for the completed baseline build.

## Last Verified Build
- Date: 2026-09-21.
- Configure: `/opt/homebrew/bin/cmake -S . -B build/baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_KF=OFF -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt -DCMAKE_MAKE_PROGRAM=/opt/homebrew/bin/ninja`.
- Build: `/opt/homebrew/bin/cmake --build build/baseline --parallel`.
- Result: success, generated `build/baseline/qelectrotech.app`.
- Build commit reported by CMake: `d7052e396b42c2b7b14b6b2f7aecb5d0a0f54cd0`.

## Tests
- `/opt/homebrew/bin/ctest --test-dir build/baseline --output-on-failure`: 11/11 tests passed.
- Non-interactive binary checks:
  - `build/baseline/qelectrotech.app/Contents/MacOS/qelectrotech --help`: exited 0 and printed CLI usage.
  - `build/baseline/qelectrotech.app/Contents/MacOS/qelectrotech --version`: exited 0 and printed `0.200.1-dev`.
- First test-only patch verification:
  - `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_roundtrip_xml --parallel`: passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_roundtrip_xml --output-on-failure`: 1/1 passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 2/2 passed.
- P0 export-equivalence verification:
  - `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_export_equivalence --parallel`: passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_export_equivalence --output-on-failure`: 1/1 passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 3/3 passed.
- P0 export-equivalence review verification:
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_export_equivalence --output-on-failure`: 1/1 passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 3/3 passed.
- Helper extraction verification:
  - `/opt/homebrew/bin/cmake --build build/baseline --target tst_cli_roundtrip_xml tst_cli_export_equivalence --parallel`: passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 3/3 passed.
- Helper extraction review verification:
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 3/3 passed.
- P0 Terminal/Potential verification:
  - `/opt/homebrew/bin/cmake --build build/baseline --target tst_terminal_potential_exports --parallel`: passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_terminal_potential_exports --output-on-failure`: 1/1 passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(terminal_potential_exports|cli_export_equivalence|cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 4/4 passed.
- Contact/CrossRef read-only projection prototype verification:
  - `cmake -S . -B build/baseline`: passed after qttest CMake registration changes.
  - `cmake --build build/baseline --target tst_contactcrossrefprojectionservice`: passed.
  - `cmake --build build/baseline --target qelectrotech`: passed; rebuilt `qelectrotech.app/Contents/MacOS/qelectrotech` with `sources/contactcrossrefprojectionservice.cpp`.
  - `ctest --test-dir build/baseline -R 'tst_contactcrossrefprojectionservice|tst_master_slave_links|tst_contactusage' --output-on-failure`: 3/3 passed.
  - `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure`: 6/6 passed.
- Contact/CrossRef read-only prototype review verification:
  - `cmake --build build/baseline --target qelectrotech tst_contactcrossrefprojectionservice`: passed; no rebuild work needed.
  - `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure`: 6/6 passed.
- Contact/CrossRef negative projection test-only slice verification:
  - `cmake --build build/baseline --target tst_contactcrossrefprojectionservice`: passed.
  - `ctest --test-dir build/baseline -R 'tst_contactcrossrefprojectionservice' --output-on-failure`: 1/1 passed.
  - `cmake --build build/baseline --target qelectrotech tst_contactcrossrefprojectionservice`: passed; no rebuild work needed after the focused test build.
  - `ctest --test-dir build/baseline -R 'tst_cli_roundtrip_xml|tst_cli_export_equivalence|tst_terminal_potential_exports|tst_master_slave_links|tst_contactcrossrefprojectionservice|tst_contactusage' --output-on-failure`: 6/6 passed.
- First patch review verification:
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R tst_cli_roundtrip_xml --output-on-failure`: 1/1 passed.
  - `/opt/homebrew/bin/ctest --test-dir build/baseline -R "tst_(cli_roundtrip_xml|conductorselfretrace)" --output-on-failure`: 2/2 passed.

## Analysis Evidence
- Project model: `sources/qetproject.h/.cpp`.
- Folio/page model: `sources/diagram.h/.cpp`.
- Element/symbol model: `sources/qetgraphicsitem/element.h/.cpp`, `sources/properties/elementdata.h/.cpp`.
- Master/slave/report/cross-reference model: `sources/qetgraphicsitem/masterelement.cpp`, `slaveelement.cpp`, `reportelement.cpp`, `crossrefitem.h`.
- Terminal/conductor/potential model: `sources/qetgraphicsitem/terminal.h`, `conductor.h`, `cli_export.cpp`.
- Terminal strip model: `sources/TerminalStrip/terminalstrip.h`, `terminalstripdata.h`.
- Project database/reporting: `sources/dataBase/projectdatabase.h/.cpp`, `sources/bomexport.cpp`, `sources/conductornumexport.cpp`, `sources/cli_export.cpp`.
- Undo/redo surface: `QETProject` and `Diagram` expose/use `QUndoStack`; concrete commands are under `sources/undocommand/`, `sources/TerminalStrip/UndoCommand/`, and `sources/editor/UndoCommand/`.
- Phase 2 trace: `.elmt` metadata examples under `elements/10_electric/20_manufacturers_articles/loxone/`, `sources/properties/elementdata.cpp`, `sources/qetgraphicsitem/element.cpp`, `sources/undocommand/linkelementcommand.*`, `sources/ui/linksingleelementwidget.cpp`, `sources/ui/contactgroupselectiondialog.cpp`, `sources/qetgraphicsitem/masterelement.*`, `sources/contactusage.h`, `sources/qetgraphicsitem/crossrefitem.*`, and `tests/qttest/tst_contactusage.cpp`.
- Phase 3 trace: `sources/qetgraphicsitem/terminal.*`, `sources/qetgraphicsitem/conductor.*`, `sources/TerminalStrip/realterminal.*`, `physicalterminal.*`, `terminalstrip.*`, `TerminalStrip/UndoCommand/*`, `sources/xml/terminalstripitemxml.*`, `sources/qetproject.cpp`, `sources/conductorproperties.cpp`, `sources/ui/conductorpropertieswidget.cpp`, `sources/dataBase/projectdatabase.cpp`, `sources/wiringlistexport.cpp`, `sources/cli_export.cpp`, `sources/ui/wiringlistdialog.cpp`, and `tests/qttest/tst_conductorselfretrace.cpp`.
- Phase 4 trace: `sources/autoNum/assignvariables.*`, `sources/autoNum/numerotationcontext.*`, `sources/autoNum/numerotationcontextcommands.*`, `sources/conductorautonumerotation.*`, `sources/undocommand/setautonumcontextcommand.*`, `sources/qetproject.*`, `sources/diagram.*`, `sources/qetgraphicsitem/element.cpp`, `sources/qetgraphicsitem/conductor.cpp`, `sources/ui/diagrampropertiesdialog.cpp`, `sources/ui/terminalnumberingdialog.cpp`, `sources/undocommand/changeelementinformationcommand.*`, `sources/dataBase/projectdatabase.cpp`, `sources/cli_export.cpp`, `sources/conductornumexport.cpp`, `sources/wiringlistexport.cpp`, and `tests/qttest/fixtures/qet_bug_repro_resaved.qet`.
- Phase 5 trace: `sources/qetproject.*`, `sources/diagram.*`, `sources/qet.cpp`, `sources/qetversion.*`, `sources/qetgraphicsitem/element.cpp`, `sources/qetgraphicsitem/conductor.cpp`, `sources/TerminalStrip/terminalstrip.cpp`, `sources/xml/terminalstripitemxml.cpp`, `sources/autoNum/numerotationcontext.cpp`, `sources/autoNum/assignvariables.cpp`, `sources/dataBase/projectdatabase.*`, `sources/cli_export.cpp`, `sources/wiringlistexport.cpp`, `tests/qttest/tst_diagramsortkeys.cpp`, `tests/qttest/tst_conductorselfretrace.cpp`, and `tests/qttest/fixtures/qet_bug_repro_resaved.qet`.
- Phase 6 test map: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/qttest/CMakeLists.txt`, `tests/qttest/tst_contactusage.cpp`, `tests/qttest/tst_conductorselfretrace.cpp`, `tests/qttest/tst_diagramsortkeys.cpp`, `tests/qttest/tst_smart_device.cpp`, `tests/qttest/tst_qetstrings.cpp`, `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, `tests/modal-quit-regression/*`, `tests/ipc-regression/README.md`, `sources/cli_export.cpp`, `sources/wiringlistexport.cpp`, `sources/conductornumexport.cpp`, and `sources/conductorproperties.cpp`.
- Phase 7 fixture specification: `examples/*.qet`, `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, `elements/10_electric/10_allpole/310_relays_contactors_contacts/`, `elements/10_electric/10_allpole/130_terminals_terminal_strips/`, `elements/10_electric/10_allpole/120_cables_wiring/`, `sources/cli_export.cpp`, and existing QtTest/CTest files.
- Phase 8 normalization rules: `sources/qet.cpp`, `sources/cli_export.cpp`, `sources/wiringlistexport.cpp`, `sources/conductornumexport.cpp`, `sources/bomexport.cpp`, `tests/qttest/tst_conductorselfretrace.cpp`, `tests/modal-quit-regression/*`, and `tests/ipc-regression/*`.
- Phase 9 test-plan backlog: `Brain.md` Phase 6-8 findings, `tests/qttest/CMakeLists.txt`, `tests/qttest/tst_contactusage.cpp`, `tests/qttest/tst_conductorselfretrace.cpp`, `tests/qttest/tst_diagramsortkeys.cpp`, `tests/qttest/tst_smart_device.cpp`, `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, `tests/modal-quit-regression/*`, `tests/ipc-regression/README.md`, `sources/cli_export.cpp`, `sources/wiringlistexport.cpp`, `sources/conductornumexport.cpp`, and high-risk source areas documented in earlier phases.
- Phase 10 implementation/review plan: `tests/qttest/CMakeLists.txt`, `tests/qttest/tst_conductorselfretrace.cpp`, `tests/qttest/fixtures/qet_bug_repro_resaved.qet`, current CTest/QtTest registration pattern, and Phase 7-9 fixture/helper/backlog findings in `Brain.md`.
- First test-only patch implementation: `tests/qttest/tst_cli_roundtrip_xml.cpp` and `tests/qttest/CMakeLists.txt`.
- P0 export-equivalence implementation: `tests/qttest/tst_cli_export_equivalence.cpp`, `tests/qttest/fixtures/workflow_exports_minimal.qet`, and `tests/qttest/CMakeLists.txt`.
- Helper extraction implementation: `tests/qttest/cli_test_utils.h`, `tests/qttest/tst_cli_roundtrip_xml.cpp`, and `tests/qttest/tst_cli_export_equivalence.cpp`.
- P0 Terminal/Potential implementation: `tests/qttest/tst_terminal_potential_exports.cpp`, shared `tests/qttest/fixtures/workflow_exports_minimal.qet`, and `tests/qttest/CMakeLists.txt`.
- Contact/CrossRef read-only projection prototype: `sources/contactcrossrefprojectionservice.h`, `sources/contactcrossrefprojectionservice.cpp`, `cmake/qet_compilation_vars.cmake`, `tests/qttest/tst_contactcrossrefprojectionservice.cpp`, and `tests/qttest/CMakeLists.txt`.
- Contact/CrossRef read-only projection prototype review: no blocking findings; API is narrow/read-only and leaves ownership, persistence, CLI/export, UI/rendering, `LinkElementCommand`, and Device/Function core untouched. CMake/testtarget weight remains the main follow-up risk.
- Contact/CrossRef negative projection test-only slice: `tests/qttest/tst_contactcrossrefprojectionservice.cpp` now creates temporary XML variants from `master_slave_links_group_index_minimal.qet` in `QTemporaryDir` and verifies missing `group_index`, out-of-range `group_index`, and slave/group contact type mismatch validation. No new fixture file was added.

## Known Issues
- A crash report from a Codex/ChatGPT launch context showed a very early Qt/Cocoa/AppKit startup abort before project, CAE, or PLC logic was reached. Treat this as a start-context/uninstalled-dev-bundle finding, not as evidence of a PLC/CAE defect.
- The project is not installed yet; any GUI/runtime conclusion requires a controlled separate Runtime-QA/installation check.
- `.gitignore` already has a local modification intentionally adding `.DS_Store` and `Handout.md`.
- `Handout.md` is a local unversioned project instruction file and must remain ignored/unversioned unless the user later explicitly decides to version a redacted/project-safe equivalent.
- CMake configure initialized/fetched project submodules as part of the existing project build flow.
- CMake reported optional scripting disabled: `Qt Qml module not available: JavaScript scripting (--run) disabled`.
- CMake reported missing Vulkan headers; this did not block configure/build.
- Build warnings observed in unchanged upstream source: self-assignment warning in `elementsmover.cpp`, ignored `nodiscard` result in `qet.cpp`, and an existing TODO pragma message in `openelmtcommand.cpp`.

## Next Planned Step
If runtime confidence is needed, run a separate optional Runtime-QA/installation slice: clarify bundle path, test local install or bundle launch under user supervision, have the user answer any macOS dialogs, and document the result. Do not mix this with PLC/CAE feature work.

## Change Log
- 2026-09-21: Created baseline progress record and documented repository/remotes.
- 2026-09-21: Recorded build blocker caused by missing active CMake/Ninja/Qt toolchain.
- 2026-09-21: Installed/provided Homebrew dependencies, configured and built unchanged baseline, ran tests, and verified non-interactive executable startup.
- 2026-09-21: Completed bounded read-only source survey and documented first-pass architecture findings in `Brain.md`.
- 2026-09-21: Completed read-only Phase 2 master/slave/contact mirror/cross-reference trace and documented findings in `Brain.md`.
- 2026-09-21: Completed read-only Phase 3 terminal/cable/potential trace and documented findings in `Brain.md`.
- 2026-09-21: Completed read-only Phase 4 numbering/autonum/conductor text trace and documented findings in `Brain.md`.
- 2026-09-21: Completed read-only Phase 5 XML/load-save compatibility trace and documented findings in `Brain.md`.
- 2026-09-21: Completed read-only Phase 6 regression-test map and documented coverage gaps and future test candidates in `Brain.md`.
- 2026-09-21: Completed read-only Phase 7 minimal fixture specification and documented reusable sources, planned fixture contents, assertions, risks, and abort criteria in `Brain.md`.
- 2026-09-21: Completed read-only Phase 8 expectation/normalization rules and documented robust comparison boundaries in `Brain.md`.
- 2026-09-21: Completed read-only Phase 9 prioritized non-implementing regression-test backlog and documented test names, hooks, helpers, fixtures, acceptance criteria, skip rules, effort, and risks in `Brain.md`.
- 2026-09-21: Completed read-only Phase 10 implementation sequence and review checklist for the first test-only infrastructure slice; no tests, fixtures, helpers, or source changes were created.
- 2026-09-21: Implemented first test-only patch `tst_cli_roundtrip_xml`, registered it in qttest CMake, built it, and verified it with targeted CTest runs.
- 2026-09-21: Reviewed first test-only patch against checklist, found no blocking findings, re-ran focused CTest verification, and documented the next P0 export-equivalence slice.
- 2026-09-21: Implemented P0 export-equivalence test-only slice, added minimal fixture, registered qttest target, built it, and verified new plus affected CLI tests.
- 2026-09-21: Reviewed P0 export-equivalence test-only slice against checklist, found no blocking findings, re-ran focused CTest verification, and documented next helper-extraction task.
- 2026-09-21: Extracted shared test-only CLI/CSV/JSON helpers into `tests/qttest/cli_test_utils.h`, updated both CLI tests, built them, and verified affected CLI tests.
- 2026-09-21: Reviewed helper extraction against checklist, found no blocking findings, re-ran focused CTest verification, and documented P0 options A/B with recommendation for Terminal/Potential export coverage first.
- 2026-09-21: Implemented P0 Terminal/Potential export coverage test-only slice, added minimal fixture, registered qttest target, built it, and verified new plus affected CLI tests.
- 2026-09-21: Implemented narrow Contact/CrossRef read-only projection prototype over loaded `QETProject`/`Element` state; added direct QtTest using `master_slave_links_group_index_minimal.qet`; rebuilt app target and verified 6 relevant tests.
- 2026-09-21: Reviewed Contact/CrossRef read-only projection prototype; re-ran focused build/CTest verification; documented CMake/testtarget-weight risk and next negative projection-test task.
- 2026-09-21: Implemented test-only negative Projection slice for missing/out-of-range `group_index` and contact type mismatch using temporary XML variants; re-ran focused build/CTest verification.

- 2026-09-21: Reviewed negative Contact/CrossRef projection slice; found no blocking issues, re-ran focused verification (6/6 relevant tests passed), and documented current project status and next decision options.
- 2026-09-21: Completed read-only CMake/Testtarget deweighting analysis for `tst_contactcrossrefprojectionservice`; created `Decision_Test_Target_Deweighting.md` with options, risks, acceptance criteria, abort rules, and recommendation.

- 2026-09-21: Implemented test-only CMake refactor for project-load qttests: introduced `add_qet_project_load_qtest` in `tests/qttest/CMakeLists.txt`, migrated only `tst_contactcrossrefprojectionservice`, kept assertions/fixture unchanged, and re-ran focused verification (6/6 relevant tests passed).

- 2026-09-21: Reviewed qttest project-load CMake refactor; found no blocking findings, confirmed assertions/fixture and forbidden areas unchanged, re-ran focused verification (6/6 relevant tests passed), and documented duplicate-assignment stop criteria.
- 2026-09-22: Implemented test-only Duplicate-assignment coverage in `tst_contactcrossrefprojectionservice` using a temporary XML variant from `master_slave_links_group_index_minimal.qet`; no API/service/fixture change; focused verification passed 6/6.
- 2026-09-22: Reviewed Duplicate-assignment coverage; found no blocking findings, confirmed scope remains Projection/Usage-vs-Capacity without Duplicate-Validation or PLC claims, and re-ran focused verification (6/6 relevant tests passed).
- 2026-09-22: Prepared review/commit-readiness report without staging/committing; documented working-tree status, proposed logical slices, unwanted build/.DS_Store artifacts, final focused test snapshot (6/6), and next cleanup/review actions.
- 2026-09-22: Cleaned local working-tree artifacts for commit preparation: added local `.git/info/exclude` rule for `build/`, removed `.DS_Store` files, and confirmed submodule dirtiness from Finder artifacts was cleared.
- 2026-09-22: Corrected documentation-slice stale-state findings in the read-only service spec, qttest deweighting decision note, and current next planned step; prepared for documentation-slice re-review.
- 2026-09-22: Reviewed code/test commit slice; found no blocking findings, confirmed fixtures are small and test-only, reran focused verification (6/6 relevant tests passed), and prepared suggested code/test commit split.
- 2026-09-22: Implemented Slice C P1 fix: added `Element::linkedElementsReadOnly() const` as a mutation-free link-list copy accessor, switched `ContactCrossRefProjectionService` away from mutating `linkedElements()`, and re-ran focused verification.
- 2026-09-22: Reviewed Slice C P1 API fix; focused tests passed 6/6, then corrected documentation history/current-status wording without code/test/fixture changes.
- 2026-09-22: Deduplicated byte-identical Slice B fixtures by renaming the shared export/terminal fixture to `workflow_exports_minimal.qet`, updating both CLI export tests, removing the duplicate test-only fixture, and re-running focused CLI tests.
- 2026-09-22: Decided Duplicate Assignment belongs in read-only Contact/CrossRef Projection validation; added duplicate group-assignment diagnostics to the service/API and focused QtTest coverage without UI, persistence, XML schema, or Device/Core changes.
- 2026-09-22: Prepared Duplicate Assignment validation projection as its own commit slice; roadmap remains PLC semantics analysis/spec next, with interactive GUI, full CTest, deeper PLC semantics, and default KF/ECM behavior still open.
- 2026-09-22: Completed PLC IO semantics analysis/spec slice in `Spec_PLC_IO_Semantics_ReadOnly.md`. Boundary: PLC truth remains `ElementData::PlcIO` plus master/slave `group_index`; slave `plc_*` fields are projection copies only; next implementation should be a warning-only `PlcIoProjectionService` extension.
- 2026-09-22: Implemented the smallest PLC IO read-only projection slice: normalized direction, terminal count/labels, and deterministic warning flags/messages for unlinked rows, out-of-range and duplicate `group_index`, empty addresses, and terminal-count/label mismatch; focused `tst_plcioprojectionservice` build/CTest passed.
- 2026-09-22: Documented that GUI remains unverified; the early Qt/Cocoa/AppKit crash report is a start-context/uninstalled-dev-bundle finding before project/CAE/PLC logic, and any runtime check belongs in a separate controlled Runtime-QA/installation slice.
