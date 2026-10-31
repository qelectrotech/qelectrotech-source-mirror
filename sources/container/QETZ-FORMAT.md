# The `.qetz` project file, format 4

A `.qetz` is a QElectroTech project saved as a zip archive: the same project
a `.qet` holds, split into files, with the engineering data in an SQLite
database. This document describes format 4. `tst_qetcontainer` checks that
the schema below is the one QElectroTech writes.

The code is `qetcontainer.cpp` (the zip and its files), `qetcontainerdb.cpp`
(the database) and `qetzip.cpp` (reading and writing zip archives).

## 1. Principles

- **One writer.** A `.qet` and a `.qetz` are written from the same project
  document. Saving a `.qetz` splits that document into files and a database;
  opening one puts it back together, and the program reads it exactly as a
  `.qet`. Opening a `.qetz` and saving it as `.qet` gives the file a plain
  `.qet` save gives.
- **Each fact once.** A value moved into the database is taken out of the
  folio file. Nothing is stored in both.
- **What a thing is, against where it is drawn.** The database holds the
  engineering data; the folio files hold the drawing:

  | | database | folio file |
  |---|---|---|
  | Symbol | its information (label, comment, …), prefix, links | which symbol it is (`type`), position, rotation |
  | Label shown on a folio | the value | the text item: position, font, rotation |
  | Wire | its ends, number, function, voltage, colour, section | its path, the number's text position and font |
  | Folio | title-block fields and custom variables | everything drawn on it |
  | Terminal strip | its terminals, their order and bridges | the strip drawn on a folio |

  Cables are kept in `project.xml` in this format.
- **Nothing computed is kept.** What QElectroTech works out again when it
  builds a folio is left out of the folio file: a wire's description of its
  ends (`element1_label`, `element1_name`, `element1_linked`,
  `terminalname1`, and the same for end 2), the text of a symbol's text
  item when it shows the symbol's information or a composition of it, and,
  when every folio, symbol and wire has a uuid of its own, a symbol's list
  of its terminals.
- **Nothing is lost.** Anything the database does not model stays in the
  XML as it was. A block is moved only if it can be rebuilt exactly.
- **The database holds the project's data only.** QElectroTech's own
  working database (search, numbering, cross-references) is built from it
  when the project opens; it is not stored.

## 2. The archive

| Entry | Content |
|---|---|
| `mimetype` | `application/x-qet-project+zip`, the first entry, stored uncompressed (files written before October 2026 say `application/x-qelectrotech-project+zip`; both are read) |
| `manifest.xml` | `<qetz format="4" min-reader="4" writer="QElectroTech …"/>` |
| `project.xml` | the project document, each part replaced by `<qetz-part file="…"/>` |
| `project.sqlite` | the engineering data (section 4) |
| `folios/<uuid>.xml` | one `<diagram>` per folio, named by the folio's uuid (or its position when it has none) |
| `elements/<categories>/<name>` | the project's symbol definitions, as in the symbol collection |
| `titleblocks/<name>.titleblock` | the project's title-block templates |
| `images/<sha256>.png` | each folio picture once, as a file instead of base64 text (`.bin` if it is not a PNG) |

Names that would collide get `~2`, `~3`… before the extension. A picture
in a folio file keeps the name of its file in the attribute `qetz-file`.

A reader refuses the whole archive if an entry is encrypted, uses a
compression method it cannot read, has a name that is absolute, holds `..`,
`\` or `:`, or repeats another entry's name, or if the archive holds more
than 100 000 entries or more than 2 GiB once unpacked. Entries the format
does not name are ignored.

## 3. Versions

`manifest.xml` gives two numbers: `format`, the format written, and
`min-reader`, the oldest format allowed to read the file.

- A reader opens a file whose `min-reader` is not above its own format.
- If `format` is above its own, the project opens **read-only**, with a
  message: saving it could drop what that reader does not know. *Save As*
  writes a copy in the reader's own format.
- If `min-reader` is above its own format, the file is refused with a
  message.

The database repeats the format in `meta` (key `format`) and in
`PRAGMA user_version`.

**Rules for a new format:**

1. New data goes in a new column or table. An older reader skips both.
2. A change an older reader would misread raises `min-reader`.
3. Every format keeps a saved file in `tests/qttest/fixtures` that must
   keep opening.

| Format | Written by | Reading |
|---|---|---|
| 2 | QElectroTech 0.200.1 development builds (#1462) | read: rows keyed by position (`f1`, `e1`, `c1`) where an item had no uuid of its own, every value as text |
| 3 | #1473 | read: as format 4, but with keys and constraints on its tables, and the computed text left in the folio files |
| 4 | this document | read and written |

## 4. The database

### 4.1 Schema

The tables are written **without keys or constraints**: each has the
columns and types below, but no `PRIMARY KEY`, `NOT NULL`, `UNIQUE` or
`REFERENCES`. With text uuids, keys and their indexes were half the file. A
reader builds the indexes it needs once the file is read, and checks itself
what the keys would have (section 5).

<!-- schema: tst_qetcontainer checks it against the database written -->
```sql
CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT);
CREATE TABLE project_property (ord INTEGER PRIMARY KEY, name TEXT, value TEXT, show INTEGER, attrs TEXT);
CREATE TABLE folio (uuid TEXT PRIMARY KEY, pos INTEGER NOT NULL UNIQUE,
    "title" TEXT, "author" TEXT, "date" TEXT, "folio" TEXT, "indexrev" TEXT, "filename" TEXT,
    "locmach" TEXT, "plant" TEXT, "auto_page_num" TEXT, props_attrs TEXT);
CREATE TABLE folio_property (folio_uuid TEXT NOT NULL REFERENCES folio, ord INTEGER NOT NULL,
    name TEXT, value TEXT, show INTEGER, attrs TEXT, PRIMARY KEY (folio_uuid, ord));
CREATE TABLE element (uuid TEXT PRIMARY KEY, folio_uuid TEXT NOT NULL REFERENCES folio,
    "prefix" TEXT, "freezeLabel" INTEGER, info_attrs TEXT, links_attrs TEXT);
CREATE TABLE element_info (element_uuid TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,
    name TEXT, value TEXT, show INTEGER, attrs TEXT, PRIMARY KEY (element_uuid, ord));
CREATE TABLE link (element_uuid TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,
    linked_uuid TEXT, attrs TEXT, PRIMARY KEY (element_uuid, ord));
CREATE TABLE conductor (uuid TEXT PRIMARY KEY, folio_uuid TEXT NOT NULL REFERENCES folio,
    "element1" TEXT, "terminal1" TEXT, "element2" TEXT, "terminal2" TEXT, "num" TEXT,
    "formula" TEXT, "function" TEXT, "tension_protocol" TEXT, "conductor_color" TEXT,
    "conductor_section" TEXT, "cable" TEXT, "bus" TEXT, "freezeLabel" INTEGER);
CREATE TABLE terminal_strip (uuid TEXT PRIMARY KEY, ord INTEGER NOT NULL UNIQUE,
    installation TEXT, location TEXT, name TEXT, comment TEXT, description TEXT);
CREATE TABLE strip_terminal (strip_uuid TEXT NOT NULL REFERENCES terminal_strip,
    pos INTEGER NOT NULL, level INTEGER NOT NULL, element_uuid TEXT,
    PRIMARY KEY (strip_uuid, pos, level));
CREATE TABLE strip_bridge (uuid TEXT PRIMARY KEY, strip_uuid TEXT NOT NULL REFERENCES terminal_strip,
    ord INTEGER NOT NULL, color TEXT);
CREATE TABLE strip_bridge_terminal (bridge_uuid TEXT NOT NULL REFERENCES strip_bridge,
    ord INTEGER NOT NULL, element_uuid TEXT, PRIMARY KEY (bridge_uuid, ord));
```

### 4.2 What each table holds

| Table | One row per | Comes from |
|---|---|---|
| `meta` | setting of the file | `format`; `props_attrs` (attributes of the project's `<properties>`); `terminal_strips` = `1` when the strips are in the database |
| `project_property` | project property (project-wide variable) | `<project><properties><property>` |
| `folio` | folio, in folio order (`pos` from 1) | the title-block attributes of `<diagram>` |
| `folio_property` | custom title-block variable of a folio | `<diagram><properties><property>` |
| `element` | symbol placed on a folio | `<element>`: its label prefix and frozen-label flag |
| `element_info` | piece of symbol information, in order | `<elementInformations><elementInformation>` |
| `link` | cross-reference link of a symbol, in order | `<links_uuids><link_uuid>` |
| `conductor` | wire | `<conductor>`: its two ends (symbol uuid and terminal), number, formula, function, voltage/protocol, colour, section, cable, bus, frozen-label flag |
| `terminal_strip`, `strip_terminal`, `strip_bridge`, `strip_bridge_terminal` | terminal strip, terminal, bridge, bridged terminal | `<terminal_strips>` |

A value is NULL when the XML has no such attribute.

- `freezeLabel` is 1 for `true`, 0 for `false`. Any other text stays as an
  attribute in the folio file.
- `show` is 0 or 1. Any other text goes in `attrs`.
- `attrs`, `props_attrs`, `info_attrs` and `links_attrs` are JSON objects of
  the attributes that have no column, and the entry's tag when it is not the
  usual one. In a project QElectroTech saves they are all NULL. They hold
  something only for older or hand-edited files. Whitespace between
  entries, which only a hand-edited file has, is kept in them as
  `qetz-ws`.

### 4.3 Keys

Every row is keyed by its item's **uuid**, which stays in the folio file:
that is how a folio file's item finds its row. Folios, symbols and wires
are keyed this way.

An item with no uuid of its own (none, or one another item of the same kind
shares) gets a key made from the item itself: a version 5 uuid in the
namespace `{3f0c6c1e-5b8e-4b8a-9d55-6b2f5f0f9a41}` from its XML text, how
many identical items came before it, and a counter that avoids taken keys.
The key is the same on every save and never depends on where the item is.
It is written on the item as `qetz-key` (a symbol or wire) or `qetz-folio`
(a folio).

### 4.4 Markers

A block moved into the database leaves `<qetz-db block="…"/>` where it was:
`properties`, `elementInformations`, `links_uuids` or `terminal_strips`.
Reading puts the block back where its marker is; a block without a marker
was never moved.

Only a block that can be rebuilt exactly is moved: entries of the expected
tag with plain text values. Terminal strips are moved only if their rows
rebuild the same XML. Anything else stays in the folio file or
`project.xml` unchanged.

## 5. Reading a file safely

A `.qetz` is untrusted. A reader:

- runs nothing from the database's schema (`PRAGMA trusted_schema = OFF`,
  read-only) and refuses a database where any table of section 4.1 is not a
  plain table (a view, say);
- refuses the file, with a message saying what is wrong, if the database
  and the folio files disagree: an item whose row is missing or on another
  folio, an item twice, a row no item has or a row twice, or information,
  links or properties of a symbol or folio that does not exist.

A damaged archive is caught by the zip format's own checksums.
