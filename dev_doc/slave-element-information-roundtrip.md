# Slave element information is not persisted consistently

## Problem

`Slave` elements already own an `ElementData::m_informations` context. Project
XML loading and saving also round-trip that context for every element basetype,
and dynamic element text can read values such as `quantity_auxiliary4` from a
Slave instance.

The remaining write and database paths were inconsistent:

- the element editor hid and disabled the Informations tree for Slave elements;
- the diagram/folio element-properties dialog omitted its Informations tab for
  Slave instances;
- `ElementScene::toXml()` omitted `<elementInformations>` when saving a Slave
  element definition;
- full internal-database rebuilds excluded Slave elements from both `element`
  and `element_info`;
- the incremental database path and the full rebuild derived `element.sub_type`
  differently, so a Slave subtype could disappear after `updateDB()`.

This made manually inserted information work in a `.qet` project while the
same data could not be authored and preserved consistently through the UI and
could not be queried reliably from the internal project database.

## Reproduction

1. Create or open a Slave element containing a dynamic text bound to
   `quantity_auxiliary4`.
2. Add this information manually to a placed instance in project XML:

   ```xml
   <elementInformation name="quantity_auxiliary4" show="1">this is quantity_auxiliary4</elementInformation>
   ```

3. Open and re-save the project. The dynamic text renders and the project
   instance keeps the value.
4. Open the element definition in the element editor. On an unmodified build,
   the Informations tab is unavailable for Slave elements and the definition
   writer does not serialize the context.
5. Rebuild the internal database. On an unmodified build, the Slave is absent
   from `element`, `element_info`, and consequently
   `element_nomenclature_view`.

## Expected behavior

- Slave information can be edited in the same Informations tree used by other
  supported basetypes.
- Placed Slave instances expose an Informations tab in the diagram/folio
  element-properties dialog without replacing their PLC/link tab.
- Saving and reopening a Slave `.elmt` definition preserves non-empty
  `<elementInformation>` entries.
- The internal database represents every supported placed element, including
  Slave instances, and preserves their Slave subtype across incremental and
  full rebuild paths.
- BOM inclusion remains independently controllable through
  `exclude_from_bom`, with Slave elements excluded by default.
- Slave label inheritance is enabled by default and can be disabled per placed
  Slave. Each project cross-reference profile (`coil`, `protection`,
  `commutator`, and `plc`) controls its default and separator. An inherited
  label therefore combines the Master and local Slave labels as `K1-A1` or,
  with a custom separator, for example `K1:A1`.

## Implementation

- Enable and show the Informations tree for `ElementData::Slave`.
- Add `ElementInfoWidget` to the diagram/folio properties tabs for
  `Element::Slave`.
- Add `ElementData::Slave` to the `ElementScene::toXml()` information write
  condition.
- Add `ElementData::Slave` to both internal-database population masks.
- Use one basetype-aware subtype conversion for incremental insertion and full
  database rebuilds.
- Normalize a missing, null, or empty `exclude_from_bom` to `true` for Slave
  definitions and placed instances while preserving an explicit `false`.
- Resolve a missing, null, or empty `inherit_label` from the linked Master's
  project cross-reference profile (whose backward-compatible default is
  `true`), expose it as a Slave-only checkbox, and calculate the displayed
  label without overwriting the Slave's own XML `label` value.
- Persist the inheritance default and separator in both application settings
  and project `<xref>` XML for all four Master types.
- Migrate a legacy PLC Slave label that exactly duplicates its linked Master's
  label to an empty local suffix, avoiding duplicated or stale labels after a
  Master rename.

## Compatibility and behavior change

Project XML parsing and instance serialization already accept Slave
information, so no project format migration is required.

Because `element_nomenclature_view` reads all rows from the internal `element`
and `element_info` tables except rows marked `exclude_from_bom = 'true'`, Slave
elements are excluded from unfiltered nomenclature and BOM queries by default.
This also applies to legacy Slave XML where the key is missing or empty. An
explicit `exclude_from_bom=false` remains the opt-in for listing a Slave as a
separate BOM item.

Legacy Slave XML without `inherit_label` follows its project profile; legacy
profiles have the compatible defaults `inherit_label=true` and
`label_separator="-"`. Explicit `inherit_label=false` preserves a standalone
Slave label. PLC link data remains Master-driven, but the channel's own label
is no longer destroyed by linking or unlinking.

## Test fixture

`dev_doc/test_cases/qet_quantity_auxiliary4_slave_minimal.qet` is an anonymized
one-folio project
with two identical PLC Slave instances. Both contain a dynamic text bound to
`quantity_auxiliary4`; only one instance contains the corresponding XML value.
It can be used to verify rendering, project round-trip, and internal-database
population without project-specific tags or manufacturer data.

## Related UI test failure

Dragging an element from a file-backed collection could emit
`QDomDocument called with unopened QIODevice` and fail to create the object.
This was not a `/tmp` or company-collection permission problem:
`ElementsLocation::xml()` passed its `QFile` to `QDomDocument::setContent()`
without opening it. The file is now opened read-only before XML parsing.
