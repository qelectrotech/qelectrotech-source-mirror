# PLC IO Semantics - Read-Only Boundary

Date: 2026-09-22
Status: analysis/spec slice, no product-code change

## Scope

This slice defines the minimum PLC semantics needed for the next CAE step. It
does not introduce a PLC/domain core, XML schema change, migration, UI change,
or blocking rule.

The next CAE layer should keep PLC handling as a read-only projection over
existing QElectroTech state.

## Minimum Semantics

For each PLC master channel, the projection needs only:

- IO direction: derived from `ElementData::PlcIO::type`.
  - Input: `EntreeDigitale`, `EntreeAnalogique`, `EntreeUniverselle`.
  - Output: `SortieDigitale`, `SortieAnalogique`, `SortieUniverselle`.
- Address: `ElementData::PlcIO::address`.
- Signal kind/type: same `PlcIOType` enum, limited to digital/analog/universal
  input/output as currently represented. No deeper electrical or datatype model
  is present in the inspected code.
- Terminal reference: `ElementData::PlcIO::effectiveTerminals()`, with
  `terminalCount` as the declared count and generated `T1..T4` labels when
  explicit labels are empty.
- Slave assignment: `group_index` maps a linked PLC slave to the master's IO
  row index.
- Terminal/potential context: only observed from existing terminals/conductors
  and the live potential graph. It should be reported as context or warning
  evidence, not treated as PLC-owned truth.

## Sources Of Truth

Authoritative PLC master channel data:

- `ElementData::PlcMasterData::ios`.
- Each `ElementData::PlcIO` row: `type`, `address`, `functionText`, `comment`,
  `crossRef`, `terminalCount`, `terminals`.

Authoritative assignment data:

- Runtime master/slave links on placed `Element` objects.
- `group_index` from `Element::groupIndexForElement()` for the linked slave.
- XML persistence remains the existing `links_uuids/link_uuid/@group_index`
  compatibility format.

Authoritative connectivity evidence:

- Graphical `Terminal` objects attached to placed elements.
- `Conductor` endpoint links.
- Potential traversal through `Conductor::relatedPotentialConductors()` and the
  existing report/terminal bridging behavior.
- Terminal-strip data remains separate project-level evidence and should not be
  merged into PLC ownership in this slice.

Projection-only slave data:

- `plc_type`, `plc_address`, `plc_function`, `plc_comment`, `plc_crossref`,
  `plc_tc`, `plc_t1`..`plc_t4` on the slave are copied by
  `LinkElementCommand`/`Element::setElementData()` for display/formula use.
- These fields are not the source of truth for PLC IO semantics. They may be
  checked against the master row as stale-copy warnings only.

## Read-Only Warning Rules

These rules are suitable as non-blocking projection/validation messages:

- PLC master has no IO rows.
- Linked PLC slave has missing `group_index`.
- Linked PLC slave has `group_index` outside `PlcMasterData::ios`.
- More than one linked PLC slave resolves to the same IO row.
- IO row has an empty address.
- IO row declares `terminalCount` outside the currently represented `1..4`
  display range.
- IO row has explicit terminal labels whose count does not match
  `terminalCount`.
- Linked slave terminal count is lower than the IO row terminal label count.
- Slave `plc_*` fields differ from the resolved master IO row.
- Optional later warning: resolved PLC slave terminals do not provide enough
  potential/terminal evidence to explain the IO row terminals. This must remain
  evidence-only until terminal mapping semantics are specified.

## Not Implemented Now

Do not implement these in the next slice:

- Blocking errors or link prevention.
- Any XML schema or `.elmt`/`.qet` migration.
- Editing or repairing `group_index`.
- Writing, regenerating, or normalizing slave `plc_*` fields.
- Inferring PLC IO direction from address syntax.
- Enforcing vendor-specific address formats.
- Creating a PLC datatype/channel/device model.
- Mapping PLC rows to terminal-strip `RealTerminal` objects as ownership.
- Potential-based auto-assignment or automatic terminal pairing.
- UI/rendering changes in `CrossRefItem`, `Element::drawPlcTable()`, or property
  widgets.
- Default KF/ECM behavior.

## Recommended Next Slice

Implement the smallest read-only service/API extension on
`PlcIoProjectionService`:

- expose normalized direction (`input`/`output`) derived from `PlcIOType`;
- expose `terminal_count` and `terminal_labels`;
- add validation messages for missing/out-of-range/duplicate `group_index`,
  empty address, and terminal-count/label-count mismatch;
- keep all diagnostics warning-only and deterministic;
- cover it with one focused QtTest using existing temporary-fixture patterns,
  without product persistence, UI, schema, or migration changes.

