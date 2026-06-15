# Database‑driven from‑to wiring list — Proof of Concept

> ## ⚠️ NOT FOR PRODUCTION USE
> This branch is a **proof of concept / demonstration** to support discussion
> [#503](https://github.com/qelectrotech/qelectrotech-source-mirror/discussions/503).
> It is **not production‑ready**, has **not** been through full review or QA, and
> the schema, SQL view, table format and GUI wording are all **subject to change**.
> Do not ship it, and do not rely on the `wire_list_view` schema staying stable.

## What this demonstrates

A **from‑to wiring list** built additively on QElectroTech's existing
`projectDataBase`, plus a GUI action to drop it onto a folio:

1. **`wire_list_view`** — for every conductor, resolves both ends to
   `element : terminal` on a folio number, with the wire number:

   | wire | from_point | from_folio | to_point | to_folio |
   |------|------------|------------|----------|----------|
   | 1001 | X2:4       | 10         | 10QF2    | 10       |
   | 1011 | XV2:1      | 8          | 8F2      | 8        |

   It is rebuilt from the scene in `updateDB()` like the existing tables
   (no stored state), and **handles the cross‑folio case** (each end can be
   on a different folio).

2. **Project → "Add a wiring list"** — a menu action that renders the view as
   a table on the current folio and spills onto new folios, exactly like the
   existing *Add a nomenclature* / *Add a summary* actions. No new rendering
   code: it reuses QET's generic `ProjectDBModel`.

## What's in the branch

| Commit | Contents |
|--------|----------|
| DB slice | `terminal` + `conductor` tables + `wire_list_view` in `projectdatabase.cpp/.h` (additive, existing tables untouched) |
| GUI action | `m_add_wiring_list` action + `QetGraphicsTableFactory::createAndAddWiringList` |
| Demo (this) | these support files |

## How to try it

1. Build QElectroTech from this branch as usual (`cmake -S . -B build && cmake --build build`).
2. Open any wired project, e.g. `examples/industrial.qet`.
3. **Project → Add a wiring list.** A from‑to table appears on the folio and
   spills onto new "Wiring list" folios.

### Quick visual without the menu

To see the view rendered without using the menu, generate a demo project that
points an existing table at `wire_list_view`:

```bash
python3 docs/wiring-list-demo/make_demo.py        # writes examples/wire_list_demo.qet
```

Then open `examples/wire_list_demo.qet` and go to the **Nomenclature** folio —
its table now shows the wiring list. (This file is a throwaway; delete it when done.)

## Known limitations (because it's a PoC)

- **Coverage:** the terminal scan uses QET's existing element‑table filter
  (`Simple | Terminal | Master | Thumbnail`), so conductors that terminate on
  **folio‑report arrows** (`NextReport`/`PreviousReport`) or **slave** contacts
  are not yet resolved. Element‑to‑element wires are complete and correct.
- **No column‑picker dialog** — the wiring‑list query is fixed (unlike the
  nomenclature/summary, which let you choose columns).
- **Not reviewed for performance** on very large projects.
- The view schema is provisional and **will change** if this goes forward.

## Feedback

This exists to make the #503 idea concrete and testable. Schema, naming and
scope are all open to change based on maintainer feedback.
