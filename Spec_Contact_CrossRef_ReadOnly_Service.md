# Spec - Contact / CrossRef Read-only Service

Status: historical pre-implementation specification. No architecture decision is made here.

Outcome note, 2026-09-22:
- A narrow read-only prototype was later implemented and reviewed as `sources/contactcrossrefprojectionservice.*` with direct QtTest coverage in `tests/qttest/tst_contactcrossrefprojectionservice.cpp`.
- The implementation follows this document's intended boundary: read-only projection over loaded `QETProject` state, no persistence ownership, no XML schema change, no UI/rendering dependency, and no Device/Function-Core decision.
- Duplicate assignment is implemented as a derived validation/diagnostic on the read-only projection, not as ownership, repair, persistence, UI behavior, or schema migration.
- This document remains the rationale/specification record; it is not a claim that implementation is still unapproved or pending.

## Purpose

Define a possible read-only projection layer for Master/Slave contact and cross-reference facts in QET/MAM. The goal is to make existing facts queryable and testable without changing ownership, persistence, rendering, or UI behavior.

This is Option B from `Decision_Contact_CrossRef_Ownership.md`: a derived service over the existing model.

## Inputs / Leading Sources

Primary QET sources remain unchanged:
- Placed elements: `Element`, `MasterElement`, `SlaveElement`, `ReportElement`.
- Definition metadata: `ElementData`, including `m_slave_state`, `m_contact_count`, `m_slave_contact_groups`.
- Runtime links: `Element::linkedElements()`, `connected_elements`, and `Element::groupIndexForElement()`.
- Persistence source: XML `links_uuids/link_uuid/@uuid` plus optional `@group_index`.
- Existing counting rule: `ContactUsage` and `MasterElement::contactUsage/contactCapacity()`.
- Presentation only: `CrossRefItem`.
- Current CLI observability: `--export-links`.

The read-only service must not become a persistence source. It derives facts from current project state.

## Derived Data

Candidate projection records:

- `ContactMasterProjection`
  - master UUID
  - master label/name
  - folio/index
  - master kind/type, e.g. coil/protection/plc where available
  - declared contact groups from `ElementData::m_slave_contact_groups`
  - linked slave count
  - computed usage via `ContactUsage`
  - computed capacity via declared groups
  - validation state summary

- `ContactAssignmentProjection`
  - master UUID
  - slave UUID
  - slave label/name
  - slave folio/index
  - slave type/state/subtype/contact count from `ElementData`
  - group index, if present
  - referenced group metadata, if group index resolves
  - validation state: resolved, missing group index, out-of-range group index, type mismatch, duplicate group assignment, orphan/unresolved if observable

- `CrossRefProjection`
  - master UUID
  - linked slave UUIDs/labels/folios
  - CrossRef display eligibility based on current project XRef settings and link state
  - no geometry, no painter state, no hover rectangles

## Projection vs Ownership

This service is a projection:
- It reads existing runtime objects and metadata.
- It may compute validation results.
- It may expose deterministic data for reports/tests/export.
- It must not assign links, mutate groups, allocate UUIDs, rewrite XML, or change `CrossRefItem`.

Ownership remains with current QET structures until a later explicit decision:
- Links are still owned by placed `Element` relationships.
- `group_index` is still existing link metadata stored on the current element map and persisted in XML.
- Contact group declarations are still element-definition metadata.
- Rendering/navigation remains in `CrossRefItem`.

## API / Data Shape Proposal

This proposal was written before the prototype. The later prototype kept the first API narrow and read-only; the shape below is retained as historical design intent, not as a pending approval gate.

Possible C++-level shape:

```cpp
struct ContactAssignmentProjection {
    QUuid masterUuid;
    QUuid slaveUuid;
    QString masterLabel;
    QString slaveLabel;
    int masterFolio = 0;
    int slaveFolio = 0;
    int groupIndex = -1;
    bool groupIndexResolves = false;
    ContactUsage::Type slaveContactType = ContactUsage::Other;
    int slaveContactCount = 1;
    QStringList groupTerminalLabels;
    QStringList validationMessages;
};
```

Possible service boundary:

```cpp
class ContactCrossRefProjectionService {
public:
    QList<ContactMasterProjection> masters(const QETProject &) const;
    QList<ContactAssignmentProjection> assignments(const QETProject &) const;
};
```

The service should depend on domain/runtime objects, not UI widgets. It should not depend on `CrossRefItem` internals.

## Boundaries To CrossRefItem / Rendering

In scope:
- Report which slaves a master references.
- Report whether declared groups and assigned indices resolve.
- Report contact usage/capacity facts that CrossRef rendering can also use.

Out of scope:
- Drawing contact mirrors.
- Pixel geometry, bounding rectangles, hover maps.
- PDF/SVG hyperlink geometry.
- Visual ordering unless an existing stable non-visual ordering is explicitly defined.

## Boundaries To Device / Function Core

This service must not define the future Device/Function/Article core.

Allowed:
- Preserve labels, UUIDs, folio references, and current element metadata as observed facts.
- Flag where article/device/function information is missing.

Not allowed:
- Introduce article master data.
- Replace element labels with device IDs.
- Decide IEC 81346 function/location ownership.
- Merge report links, terminals, cable cores, or PLC IO into a broad CAE core.

## Tests Needed Before Prototype

Minimum regression base:
- Existing `tst_master_slave_links` with `slaveContactGroups`, `group_index`, `--resave`, and `--export-links`.
- Existing `tst_contactusage` counting rules.

Before service implementation:
- Add or specify a direct QtTest fixture load that asserts:
  - master groups are read from embedded `ElementData`
  - group index 0 resolves to NO labels 13/14
  - group index 1 resolves to NC labels 21/22
  - usage and capacity match
  - invalid group index is reported, not silently accepted

After service implementation:
- Unit test projection records with one valid fixture.
- Add one negative fixture or synthetic model for out-of-range `group_index`.
- Verify no UI dependency and no `CrossRefItem` dependency.
- Keep CLI/export changes out of the first service patch unless explicitly approved.

## Migration Need

None for a read-only projection over the existing model.

Potential later migrations, not approved here:
- Add stable assignment UUIDs.
- Persist contact assignments separately from `links_uuids`.
- Replace `group_index` with explicit group identity.
- Make service-backed export include group/contact facts.

Any persistence change requires a separate migration decision.

## Open Decisions

- Should `group_index` remain the public assignment reference, or should a later group ID be introduced?
- Should report links remain outside the projection service?
- Should PLC master/slave assignments be included in v1 or explicitly excluded?
- Should validation warnings be exposed to CLI/export, project checks, UI, or tests only?
- Should `--export-links` be extended, or should a separate `--export-contact-links` style surface be specified?
- Should projection ordering follow diagram order, master linked order, group index order, or a stable UUID/label sort?

## Risks

- A read-only service can accidentally become a second source of truth if it caches mutable state.
- Including PLC behavior too early could pull in terminal-label and PLC-variable side effects from `LinkElementCommand`.
- Current examples did not contain `slaveContactGroups`; test confidence relies on QET parser/writer evidence plus constructed test-only fixture.
- CrossRef visual behavior remains separate and unverified by this spec.

## Recommendation For Next Step

[Likely] Review and freeze this specification first. If accepted, implement only a narrow projection prototype that:
- reads current `QETProject` state,
- emits deterministic projection records,
- has no persistence writes,
- has no UI/rendering dependency,
- is covered by direct QtTest using the corrected minimal fixture.
