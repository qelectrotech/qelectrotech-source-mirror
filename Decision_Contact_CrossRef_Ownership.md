# Decision Analysis - Contact / CrossRef Ownership

Status: analysis only. No architecture decision is made here.

## Scope

This note prepares a later decision about where contact assignment, contact capacity, contact mirror data, and cross-reference ownership should live in the QET-based MAM system.

Inputs:
- QET code evidence from `sources/properties/elementdata.*`, `sources/qetgraphicsitem/element.*`, `masterelement.*`, `slaveelement.*`, `reportelement.*`, `crossrefitem.*`, `sources/undocommand/linkelementcommand.*`, `sources/contactusage.h`, `sources/cli_export.cpp`, and `tests/qttest/tst_contactusage.cpp`.
- Prior findings in `Brain.md`.
- Public reference principles in `CAE_Reference_Matrix.md`.

Non-goals:
- No implementation plan is selected.
- No source/test/fixture changes are proposed as approved work.
- No vendor-internal behavior is assumed.

## QET Current State

### Element Definition Metadata

- [Certain] `ElementData` stores element link/category metadata. It distinguishes master/slave/report/terminal-like roles and stores slave state, slave type, contact count, and master-side `SlaveContactGroup` declarations.
- [Certain] `ElementData::fromXml()` parses slave metadata from element definition `kindInformations`, including slave `type`, `state`, and `number`.
- [Certain] For master elements, `ElementData::fromXml()` parses `slaveContactGroups/group` into `m_slave_contact_groups`, with type, subtype, `contactCount`, `terminalCount`, and labels.

Ownership implication: contact capacity metadata starts in element-definition data, but actual assignment of a placed slave to a master/group is not owned by `ElementData`.

### Placed Element Link State

- [Certain] `Element` owns `connected_elements` and `m_group_index_map`. `Element::toXml()` persists links as `<links_uuids><link_uuid uuid=... group_index=.../>`; `Element::fromXml()` reads them into pending `LinkInfo`; `Element::initLink()` resolves UUIDs after project load.
- [Certain] `group_index` is stored on the element-side map keyed by linked element, not as an independent link object.
- [Certain] `MasterElement::linkToElement()` appends slave elements, calls the reciprocal slave link, creates/updates `CrossRefItem` when required, connects PLC slave-position updates, and emits `linkedElementChanged()`.
- [Certain] `ReportElement` uses the same broad element-link mechanism for previous/next report links, but report links are not contact assignments.

Ownership implication: QET currently treats links as direct object relationships between placed `Element` instances. Link metadata is distributed across both the linked elements and the master-owned group-index map.

### Link Command / Undo-Redo

- [Certain] `LinkElementCommand::isLinkable()` encodes compatibility rules for report links and master/slave links, including PLC master/slave matching.
- [Certain] `LinkElementCommand::makeLink()` calls `Element::linkToElement()` and then sets `group_index` for slave/master links. For PLC masters it also propagates terminal-label usage and PLC variables onto the slave.
- [Certain] `undo()` removes group indices, resets terminal master-label flags, clears PLC variables where relevant, and restores previous linked elements through `makeLink(m_linked_before)`.
- [Certain] Report links may trigger potential-property conflict UI (`PotentialSelectorDialog`) on first redo; that path is adjacent to report links, not contact mirror ownership.

Ownership implication: current command layer owns important behavior beyond link creation: group assignment, terminal label propagation, PLC variable propagation, and undo restoration. Any ownership change must preserve these side effects explicitly.

### Contact Usage / Capacity

- [Certain] `ContactUsage` is a header-only tally independent of graphics. It counts NO/NC/SW/Other and deliberately counts a changeover (`SW`) once, avoiding the display double-count risk from CrossRef NO/NC visual columns.
- [Certain] `MasterElement::contactUsage()` maps each linked slave's `ElementData::m_slave_state` and `m_contact_count` into `ContactUsage`.
- [Certain] `MasterElement::contactCapacity()` sums `ElementData::m_slave_contact_groups`.
- [Certain] `MasterElement::isFull()` uses group-slot count when groups exist; otherwise it falls back to `max_slaves`. It is not the same thing as contact-count capacity.

Ownership implication: usage/capacity is currently computed by `MasterElement`, not by `CrossRefItem`, but it still depends on runtime linked elements and element metadata.

### CrossRef Rendering / Navigation

- [Certain] `CrossRefItem` is a `QGraphicsObject` tied to an `Element`. It listens to project XRef settings, diagram order/removal, linked element changes, and slave movement. It renders cross/contact/PLC-table views and maintains hovered-contact rectangles for navigation/PDF link injection.
- [Certain] `CrossRefItem::updateLabel()` builds geometry and contact navigation maps; display is skipped for non-PLC masters with no linked elements.
- [Certain] PLC masters are special: `CrossRefItem` keeps a table/navigation representation even when visual drawing is delegated elsewhere.

Ownership implication: `CrossRefItem` owns presentation, hit testing, and navigation details. It should not be treated as authoritative contact assignment data without a deliberate decision.

### Persistence / Export / Tests

- [Certain] XML persistence stores link UUIDs and optional `group_index`; unresolved link state can occur when UUIDs cannot be resolved.
- [Certain] `--export-links` writes linkable elements, link type, linked labels/folios, folio, and unresolved status. It does not export contact capacity, group labels, or full contact assignment details.
- [Certain] Existing focused coverage is `tests/qttest/tst_contactusage.cpp`, which verifies counting rules only. It does not instantiate placed elements, `LinkElementCommand`, XML round-trip, `CrossRefItem`, or `--export-links`.

Ownership implication: current observable surfaces are sufficient for basic link/unresolved checks, but not enough for a complete contact ownership invariant.

## Reference Principles From CAE Matrix

- [Certain] Public reference systems consistently advertise object/data-driven engineering, cross-references, reports/lists, and device/product data coordination.
- [Certain] EPLAN public material states device-driven design, automated numbering/cross referencing, and reports/lists.
- [Certain] SEE Electrical public product information lists contact mirrors, contact workload checks, cross-reference navigation, real-time lists, and function/location/product database aspects.
- [Certain] AmpereSoft ProPlan public material describes object utilization across plan types, cross-reference lists, material database use, and DIN EN 81346 project structure.
- [Certain] AUCOTEC and Zuken public material emphasize shared object/data models as sources for diagrams, reports, and documentation.
- [Unverified] Exact vendor contact-comb data structures, native schema, UI behavior, and algorithms are not public and must not be copied or assumed.

Transferable principle: contact/cross-reference behavior should be a validated data relationship with reports/navigation/rendering as views. Non-transferable detail: vendor-specific internal schema and visual layout behavior.

## Decision Options

### Option A - Extend Status Quo In Place

Description: Keep `Element`/`MasterElement`/`CrossRefItem` ownership as-is. Add narrow tests and small local fixes around current behavior when later approved.

Prerequisites:
- Accept that `connected_elements` plus `m_group_index_map` remains the runtime source for master/slave/contact assignment.
- Define minimum invariants for link UUID persistence, `group_index`, usage/capacity, unresolved link export, and CrossRef rendering observability.

Advantages:
- Lowest migration cost.
- Preserves current `.qet` XML shape.
- Low risk of breaking existing projects.
- Fits current implementation and recent test-only infrastructure.

Risks:
- Keeps assignment metadata distributed across placed elements and command side effects.
- Does not resolve long-term device/function/symbol separation.
- CrossRef/report/export consistency can still drift unless tests are strong.

Compatibility impact:
- Minimal; existing XML can remain unchanged.

Test need:
- P0 master/slave fixture for `--resave` and `--export-links`.
- Direct QtTest for `LinkElementCommand` undo/redo and `group_index`.
- Later renderer/navigation test for `CrossRefItem` if stable enough.

Migration need:
- None initially.

Reversible / irreversible:
- Highly reversible if limited to tests and small fixes.

Open questions:
- Is current `m_group_index_map` ownership acceptable for MAM, or only a temporary compatibility layer?
- Which CrossRef visual facts can be tested without brittle geometry assertions?

### Option B - Contact/CrossRef Service Over Existing Model

Description: Add a read-only or narrowly mutating service/facade over current `Element` links to compute contact assignments, usage/capacity, unresolved state, and export/report-friendly projections. The underlying XML and element links remain unchanged initially.

Prerequisites:
- Clear service contract: no hidden new persistence at first.
- Service must preserve `LinkElementCommand` side effects or make them explicit.
- Decide whether service is read-only first or allowed to perform link mutations later.

Advantages:
- Creates one query surface for contact/cross-reference facts without immediate XML migration.
- Reduces duplication between UI, exports, tests, and future reports.
- Can bridge current model toward a stronger domain model.

Risks:
- Can become a second source of truth if it caches state or mutates inconsistently.
- Needs careful naming/scope to avoid broad architecture drift.
- May hide, not solve, weak ownership of `group_index`.

Compatibility impact:
- Low if read-only and derived from current XML/runtime links.

Test need:
- Unit tests for projection outputs from small in-memory or fixture-loaded projects.
- Regression comparison with `MasterElement::contactUsage/contactCapacity`.
- CLI/export tests can assert service-backed facts once exposed.

Migration need:
- None for read-only service; later migration possible if service becomes canonical.

Reversible / irreversible:
- Mostly reversible if introduced as derived projection.
- Less reversible if UI/export paths are widely switched before service invariants are mature.

Open questions:
- Should the service be allowed to call `LinkElementCommand`, or should it only observe?
- Which layer owns error reporting for unresolved/overfilled/invalid group assignments?

### Option C - Dedicated ContactAssignment Domain Model

Description: Introduce a first-class domain object for master/slave contact assignment, with explicit identity, master UUID, slave UUID, group index, contact type/count, terminal labels, and validation state.

Prerequisites:
- Stable schema for assignment identity and persistence.
- Migration/read compatibility for existing `<link_uuid group_index=...>` XML.
- Clear ownership boundaries between placed elements, domain assignments, and CrossRef rendering.

Advantages:
- Cleanest ownership for contact/cross-reference data.
- Makes reports, exports, validation, and tests more straightforward.
- Aligns well with public CAE reference principle of data relationships feeding diagram/report views.

Risks:
- Highest near-term migration risk.
- Requires deciding canonical schema before broader device/function/article model is settled.
- Could conflict with existing master/slave/report link behavior if overgeneralized.
- Requires careful compatibility handling for old projects and current UI assumptions.

Compatibility impact:
- Medium to high. Even if old XML remains readable, save format or internal load path may change.

Test need:
- Migration tests from current XML.
- Round-trip tests preserving links and groups.
- Undo/redo tests for assignment creation/removal/reassignment.
- CrossRef rendering/export/report tests from assignment model.

Migration need:
- Required. Existing links must be converted or mirrored into assignments.

Reversible / irreversible:
- Reversible only if introduced as internal projection first.
- Potentially irreversible if `.qet` save format changes without dual-read/write strategy.

Open questions:
- Should assignments have stable UUIDs?
- Should report links share this model or remain separate?
- Should contact capacity come from master element definition, article/device data, or both?

### Option D - Defer To Larger Device/Function Core

Description: Do not solve contact ownership separately. Wait for a broader Device/Function/Article domain core, then make contact assignments part of that larger model.

Prerequisites:
- Acceptance that contact/cross-reference work pauses except for regression tests.
- A later decision process for device/function/article ownership.

Advantages:
- Avoids premature local design.
- Could produce a cleaner long-term model if the broader core is defined soon.
- Better fit if MAM requires commercial-grade device/article/function separation first.

Risks:
- Blocks high-risk contact/cross-reference hardening.
- Current distributed behavior remains exposed while other work proceeds.
- Larger core decision may take longer and carry more uncertainty.

Compatibility impact:
- Unknown until broader core is defined.

Test need:
- Still needs status-quo regression tests to prevent drift while waiting.

Migration need:
- Likely high later.

Reversible / irreversible:
- Reversible as a planning posture, but delays concrete risk reduction.

Open questions:
- Is the broader device/function core urgent enough to justify delaying contact ownership?
- What minimum contact invariants must be frozen before the larger core exists?

## Technical Recommendation, Not Decision

[Likely] Recommended order:
1. Choose Option A as a short stabilization posture only: add/keep regression coverage around the existing XML/link/group behavior before any model change.
2. Use Option B as the first architectural exploration candidate: a derived Contact/CrossRef projection over the existing model, explicitly read-only at first.
3. Defer Option C until migration criteria and device/function/article boundaries are clearer.
4. Do not choose Option D as the only path unless the project intentionally prioritizes a larger device/function core before contact work.

Reasoning:
- QET already has working primitives, and current `.qet` compatibility matters.
- The highest immediate risk is unverified drift across persistence, undo, CrossRef rendering, and exports.
- A derived service/projection can clarify ownership without immediately rewriting project persistence.

Confidence: [Likely]. The recommendation is based on direct QET code evidence and public CAE reference principles, but not on a completed migration prototype.

## Open Decisions

- Is `group_index` a compatibility detail or the long-term assignment key?
- Should contact assignment be owned by master, slave, link object, or independent domain object?
- Should report links remain separate from contact assignments?
- Should contact capacity be defined by element XML, article/device data, or a future combination?
- Which CrossRef visual/rendering facts are stable enough for regression tests?
- Should `--export-links` eventually expose group/contact/capacity facts, or should a separate export surface be used?

## Risks / Unverified

- Vendor internals remain unverified; only public principles are usable.
- Existing test coverage does not prove placed-element link behavior.
- PLC-specific link side effects make a generic contact model riskier than normal relay contact links.
- Existing XML compatibility must be preserved unless a later migration decision explicitly approves otherwise.
