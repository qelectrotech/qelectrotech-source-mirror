#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""
qet-mcp — a Model Context Protocol server over QElectroTech projects.

WHY THIS EXISTS

Verifying a QET change by screenshot is unreliable. Twice in one review
session a screenshot was read as showing a defect that the saved file
proved had not happened: once "dragging a multi-selection leaves the
symbols behind and detaches their labels" (the XML showed all four
elements moved and no label moved), and once "Apply does nothing" (Apply
was disabled because a required field was empty). Both times the pixels
misled and the model told the truth.

So the primary tools here read the *model*, not the screen, and the
primary tool is qet_diff: do the thing, then ask what actually changed.

DESIGN

Most tools parse the .qet XML directly and never launch QElectroTech.
That is deliberate: it is fast, deterministic, needs no display, and
cannot be confused by a dialog. Only qet_export shells out to the
binary, and it carries the launch traps with it (see _run_qet).

The project database would be a better query surface than XML, but it is
not reachable from outside the application: projectDataBase::newQuery()
and isReadOnlySelect() are C++-internal and the JavaScript scripting API
exposes no SQL binding. Until it does, structure lives here.

PROTOCOL

Line-delimited JSON-RPC 2.0 on stdin/stdout, per MCP's stdio transport.
Nothing but protocol goes to stdout; diagnostics go to stderr.
No third-party dependencies — the MCP SDK is not assumed to be present.

`--call <tool> [arguments]` runs one tool without an MCP client, for an
assistant that can execute Python but cannot launch a server (a web chat
with code execution). It goes through the same dispatcher, so the
workspace policy applies exactly as it does over stdio.
"""

from __future__ import annotations

import json
import re
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path, PurePath, PurePosixPath, PureWindowsPath

SERVER_NAME = "qet-mcp"
SERVER_VERSION = "0.1.0"
DEFAULT_PROTOCOL = "2025-06-18"

EXPORT_FORMATS = {
    "pdf": "--export-pdf",
    "png": "--export-png",
    "svg": "--export-svg",
    "dxf": "--export-dxf",
    "bom": "--export-bom",
    "cables": "--export-cables",
    "wires": "--export-wires",
    "wiring": "--export-wiring",
    "nets": "--export-nets",
    "links": "--export-links",
    "info": "--info",
}


# --------------------------------------------------------------------------
# model reading
# --------------------------------------------------------------------------

def _root(path: str) -> ET.Element:
    p = Path(path).expanduser()
    if not p.is_file():
        raise ValueError(f"no such file: {p}")
    try:
        return ET.parse(p).getroot()
    except ET.ParseError as exc:
        raise ValueError(f"{p.name} is not parseable XML: {exc}") from exc


def _element_info(el: ET.Element) -> dict:
    """The <elementInformations> bag, as a plain dict."""
    out = {}
    bag = el.find("elementInformations")
    if bag is not None:
        for info in bag.findall("elementInformation"):
            name = info.get("name")
            if name:
                out[name] = (info.text or "").strip()
    return out


def _folios(root: ET.Element):
    """Yield (index, diagram) for each folio, 1-based as the UI numbers them."""
    for i, d in enumerate(root.iter("diagram"), start=1):
        yield i, d


def _elements(root: ET.Element):
    for i, d in _folios(root):
        for el in d.iter("element"):
            yield i, el


def _wires(diagram: ET.Element):
    """The folio's conductors: children of <conductors> only. A folio's wire
    numbering rule is also saved as a <conductor> tag, under <autonum>, and
    is not a wire."""
    return diagram.findall("conductors/conductor")


def _conductors(root: ET.Element):
    definitions = _definition_terminals(root)
    for i, d in _folios(root):
        index = _terminal_index(d, definitions)
        for c in _wires(d):
            yield i, c, index


def _uuid_key(value: str) -> str:
    return (value or "").strip().strip("{}").lower()


# Orientation as a placed symbol's <terminal> record writes it (an int,
# Qet::Orientation) or as a definition does (n/e/s/w).
_ORIENTATIONS = {"n": 0, "e": 1, "s": 2, "w": 3, "0": 0, "1": 1, "2": 2, "3": 3}

# Where QElectroTech docks a wire, relative to the terminal's position in
# its definition (Terminal's constructor, Terminal::terminalSize = 4). A
# placed symbol's <terminal> record is written at that point.
_DOCK_OFFSET = {0: (0.0, 4.0), 1: (-4.0, 0.0), 2: (0.0, -4.0), 3: (4.0, 0.0)}


def _definition_terminals(root: ET.Element) -> dict:
    """Map each symbol stored in the project ("embed://" + its path in the
    <collection>) to its terminals: {terminal uuid: (x, y, orientation)},
    the position being the one in the definition."""
    out = {}

    def walk(node, path):
        for child in node:
            if child.tag == "category":
                walk(child, path + [child.get("name", "")])
            elif child.tag == "element":
                terminals = {}
                for t in child.findall("definition/description/terminal"):
                    try:
                        terminals[_uuid_key(t.get("uuid"))] = (
                            float(t.get("x")), float(t.get("y")),
                            _ORIENTATIONS.get((t.get("orientation") or "n")[:1], 0))
                    except (TypeError, ValueError):
                        continue
                terminals.pop("", None)
                if terminals:
                    out["embed://" + "/".join(path + [child.get("name", "")])] = terminals

    collection = root.find("collection")
    if collection is not None:
        walk(collection, [])
    return out


def _element_row(folio: int, el: ET.Element) -> dict:
    info = _element_info(el)
    etype = el.get("type", "")
    return {
        "folio": folio,
        "uuid": el.get("uuid", ""),
        "type": etype,
        "name": etype.rsplit("/", 1)[-1].removesuffix(".elmt"),
        "x": el.get("x"),
        "y": el.get("y"),
        "label": info.get("label", ""),
        "info": info,
    }


def _terminal_index(diagram: ET.Element, definitions: dict | None = None) -> dict:
    """Map a folio's terminal ids to an identity that survives a save.

    A conductor names its ends with terminal1/terminal2, which are plain
    integers scoped to the folio -- and QElectroTech reassigns them on every
    write, in whatever order it happens to serialise the elements. The same
    untouched conductor comes back as terminal1="1" terminal2="16" before a
    save and terminal1="34" terminal2="15" after one. Keying a conductor on
    that pair, which this tool used to do, made every conductor in the file
    read as removed-and-re-added whenever the "after" side had been through
    QElectroTech -- which is exactly the case qet_edit produces, so the
    conductor half of the diff was noise precisely when it was needed.

    So resolve each id to (owning element uuid, terminal position and
    orientation inside that element). Element uuids are persisted and
    stable; the terminal's local geometry comes from the element definition
    and does not move when the element moves. That pair is the same basis
    QET's own Terminal::stableUuid() uses for terminals with no uuid of
    their own, and it is stable for the same reasons.

    Conductors in the corpus carry no element1/element2 attribute -- 0 of
    47 in ArduinoLCD.qet, 0 of 67 in 741.qet -- so this mapping has to be
    built from the elements rather than read off the conductor.

    A conductor can also name its ends by terminal uuid (element1 +
    terminal1), and QElectroTech writes that form as soon as the terminal
    has a uuid -- which, since a project gives every terminal one on
    opening, is the first save of any older file. So the same untouched
    conductor is written in the numbered form before a save and the uuid
    form after it. With @p definitions (from _definition_terminals()), a
    uuid end is resolved too, keyed (element uuid, terminal uuid), to the
    very same identity as the numbered end: the terminal's definition
    position, moved to where the wire docks, is where the placed symbol's
    <terminal> record is.
    """
    index = {}
    for el in diagram.iter("element"):
        uuid = el.get("uuid", "")
        if not uuid:
            # Old enough to predate persisted element uuids. Leaving these
            # ids unresolved is deliberate: keyed on terminal geometry
            # alone, every element of the same type collapses together --
            # in schema_indus.qet that merged nine distinct conductors onto
            # one key, which is worse than the instability it was meant to
            # fix. An unresolved end keeps them apart and stays visibly
            # marked with a "#" so the caller can see the diff is on the
            # unstable footing that file forces.
            continue
        records = []
        for t in el.iter("terminal"):
            tid = t.get("id")
            if tid is None:
                continue
            key = (f"{uuid}@{t.get('x','?')},{t.get('y','?')}"
                   f",{t.get('orientation','?')}")
            index[tid] = key
            records.append((t, key))
        for tuuid, (x, y, o) in (definitions or {}).get(el.get("type", ""), {}).items():
            dx, dy = _DOCK_OFFSET[o]
            for t, key in records:
                try:
                    if (abs(float(t.get("x")) - (x + dx)) < 1e-6
                            and abs(float(t.get("y")) - (y + dy)) < 1e-6
                            and _ORIENTATIONS.get((t.get("orientation") or "")[:1]) == o):
                        index[(_uuid_key(uuid), tuuid)] = key
                        break
                except (TypeError, ValueError):
                    continue
    return index


def _conductor_key(folio: int, c: ET.Element, index: dict) -> str:
    """Identify a conductor by its two ends, in whichever scheme it uses.

    The project format has two, and a file can hold both at once -- the
    same folio, after an edit, carries legacy conductors and new ones:

    - legacy: terminal1/terminal2 are the folio-scoped integer ids, and
      there is no element1/element2. Resolve them through index.
    - current: terminal1/terminal2 are terminal uuids from the element
      *definition*, with element1/element2 naming the placed instances.
      The terminal uuid alone is not an identity -- two coils of the same
      type have the same one on both ends, so a conductor between them
      would key as a self-loop -- so it is the (instance, terminal) pair
      that identifies an end.
    """
    ends = []
    for elem_attr, term_attr, name_attr in (("element1", "terminal1", "terminalname1"),
                                            ("element2", "terminal2", "terminalname2")):
        tid = c.get(term_attr, "?")
        owner = c.get(elem_attr)
        if owner:
            ends.append(index.get((_uuid_key(owner), _uuid_key(tid)))
                        or f"{owner}/{tid or c.get(name_attr, '?')}")
        else:
            # An id with no element behind it stays visible as itself
            # rather than silently collapsing conductors onto one key.
            ends.append(index.get(tid, f"#{tid}"))
    # A conductor is undirected: whichever end QET happens to write first,
    # it is the same connection.
    return f"{folio}:" + "--".join(sorted(ends))


def _conductor_row(folio: int, c: ET.Element, index: dict | None = None) -> dict:
    return {
        "folio": folio,
        "uuid": c.get("uuid", ""),
        "key": _conductor_key(folio, c, index or {}),
        "num": c.get("num", ""),
        "formula": c.get("formula", ""),
        "cable": c.get("cable", ""),
        "bus": c.get("bus", ""),
        "function": c.get("function", ""),
        "color": c.get("conductor_color", ""),
        "section": c.get("conductor_section", ""),
        "type": c.get("type", ""),
    }


# --------------------------------------------------------------------------
# tools
# --------------------------------------------------------------------------

def tool_project_info(path: str) -> dict:
    root = _root(path)
    folios = []
    for i, d in _folios(root):
        folios.append({
            "index": i,
            "uuid": d.get("uuid", ""),
            "title": d.get("title", ""),
            "elements": sum(1 for _ in d.iter("element")),
            "conductors": len(_wires(d)),
        })
    return {
        "file": str(Path(path).expanduser()),
        "title": root.get("title", ""),
        "version": root.get("version", ""),
        "folio_count": len(folios),
        "element_count": sum(f["elements"] for f in folios),
        "conductor_count": sum(f["conductors"] for f in folios),
        "folios": folios,
    }


def tool_elements(path: str, folio: int | None = None,
                  name_contains: str | None = None, limit: int = 200) -> dict:
    rows = []
    for i, el in _elements(_root(path)):
        if folio is not None and i != folio:
            continue
        row = _element_row(i, el)
        if name_contains and name_contains.lower() not in row["name"].lower():
            continue
        rows.append(row)
    return {"count": len(rows), "truncated": len(rows) > limit,
            "elements": rows[:limit]}


ITEM_KINDS = ["text", "shape", "image", "table", "element_text"]


def tool_items(path: str, folio: int | None = None, kind: str | None = None,
               limit: int = 500) -> dict:
    """Every drawn item that is not a symbol or a wire, with its uuid.

    Free texts, shapes, pictures, tables and the text fields of symbols --
    the items a qet_edit op or a qet_diff entry names by uuid. Folios are
    numbered from 1, as in qet_elements. An item saved before these items
    carried a uuid has "" here; QElectroTech gives it one on the next save.
    """
    if kind is not None and kind not in ITEM_KINDS:
        raise ValueError(f"kind must be one of {ITEM_KINDS}, not {kind!r}")
    ex = _extras(_root(path))
    rows = []
    for name, records in (("text", ex["texts"]), ("shape", ex["shapes"]),
                          ("image", ex["images"]), ("table", ex["tables"])):
        for r in records:
            rows.append({"kind": name, "uuid": r["uuid"], **r["label"], **r["value"]})
    for k, v in ex["element_texts"].items():
        rows.append({"kind": "element_text", "uuid": ex["element_text_uuids"][k],
                     "folio": ex["element_text_folios"][k], "element": k[0],
                     "source": k[1], "bound_to": k[2], "n": k[3], **v})
    rows = [r for r in rows if (folio is None or r["folio"] == folio)
            and (kind is None or r["kind"] == kind)]
    rows.sort(key=lambda r: (r["folio"], ITEM_KINDS.index(r["kind"])))
    return {"count": len(rows), "truncated": len(rows) > limit, "items": rows[:limit]}


def tool_conductors(path: str, folio: int | None = None,
                    attribute: str | None = None,
                    non_empty: bool = False, limit: int = 200) -> dict:
    rows = []
    for i, c, ix in _conductors(_root(path)):
        if folio is not None and i != folio:
            continue
        row = _conductor_row(i, c, ix)
        if attribute is not None:
            value = c.get(attribute, "")
            if non_empty and not value.strip():
                continue
            row["value"] = value
        rows.append(row)
    return {"count": len(rows), "truncated": len(rows) > limit,
            "conductors": rows[:limit]}


# No "version": that attribute is the file-format stamp QElectroTech rewrites
# on every save, so diffing it made every folio of any re-saved project look
# edited, and it is not something a script can set (see setFolioProperty).
_FOLIO_FIELDS = ("title", "author", "plant", "locmach", "indexrev",
                 "folio", "filename",
                 # the frame: attribute names as the file writes them
                 "cols", "colsize", "rows", "rowsize", "displaycols", "displayrows",
                 "titleblocktemplate")


def _plain_text(html: str) -> str:
    """The visible text of an independent text's HTML, which is what a
    person means by "the text". The file stores a whole HTML document."""
    import re
    body = re.search(r"<body[^>]*>(.*)</body>", html or "", re.S)
    inner = body.group(1) if body else (html or "")
    inner = re.sub(r"<[^>]+>", "", inner)
    for a, b in (("&lt;", "<"), ("&gt;", ">"), ("&amp;", "&"), ("&quot;", '"'), ("&#39;", "'")):
        inner = inner.replace(a, b)
    return " ".join(inner.split())


def _angle(value: str) -> str:
    """A rotation in degrees, reduced to [0, 360) so equal angles compare equal.

    QElectroTech writes the same angle in more than one way: rotating a symbol
    and undoing it leaves its text fields at "-270" where they were "90", or
    "-90" where they were "270". Compared as written, that read as a change.
    Anything that is not a number is returned unchanged.
    """
    try:
        deg = float(value) % 360
    except (TypeError, ValueError):
        return value
    return f"{deg:g}"


def _extras(root: ET.Element) -> dict:
    """Everything a folio holds besides elements and conductors.

    Independent texts, shapes and images are records, not a keyed dict:
    files written since they carry a uuid are compared by it (see
    _diff_items), so an edited or moved text reads as that text, changed.
    Older files have only position to go on, and there a change reads as
    the old one removed and a new one added, with both shown.
    """
    folios, folio_uuids, texts, shapes, images, tables = {}, {}, [], [], [], []

    def record(el, label, value):
        return {"uuid": el.get("uuid", ""), "key": tuple(label.values()),
                "label": label, "value": value}

    for n, d in _folios(root):
        folios[n] = {f: d.get(f, "") for f in _FOLIO_FIELDS}
        folio_uuids[n] = d.get("uuid", "")
        for tb in d.findall("tables/graphics_table"):
            tables.append(record(
                tb, {"folio": n, "name": tb.get("name", "")},
                {"x": tb.get("x", ""), "y": tb.get("y", ""), "width": tb.get("width", ""),
                 "height": tb.get("height", ""), "rows_shown": tb.get("display_n_row", "")}))
        # The folio's own items only: direct children of its <inputs>,
        # <shapes> and <images>. Symbols in older files carry their own
        # <inputs><input> texts, which iter() would count as free texts
        # (122 extra in schema_indus.qet).
        for t in d.findall("inputs/input"):
            texts.append(record(
                t, {"folio": n, "x": t.get("x", ""), "y": t.get("y", ""),
                    "text": _plain_text(t.get("text", ""))},
                {"rotation": _angle(t.get("rotation", "0")),
                 "font": t.get("font", ""), "color": t.get("color", "")}))
        for sh in d.findall("shapes/shape"):
            pen, brush = sh.find("pen"), sh.find("brush")
            shapes.append(record(
                sh, {"folio": n, "type": sh.get("type", ""),
                     "from": [sh.get("x1", ""), sh.get("y1", "")],
                     "to": [sh.get("x2", ""), sh.get("y2", "")]},
                {"line_color": pen.get("color", "") if pen is not None else "",
                 "line_style": pen.get("style", "") if pen is not None else "",
                 "line_width": pen.get("widthF", "") if pen is not None else "",
                 "fill": (brush.get("color", "") if brush is not None and
                          brush.get("style", "") != "NoBrush" else "none"),
                 "rotation": _angle(sh.get("rotation", "0"))}))
        for im in d.findall("images/image"):
            images.append(record(
                im, {"folio": n, "x": im.get("x", ""), "y": im.get("y", "")},
                {"scale": im.get("size", ""), "rotation": _angle(im.get("rotation", ""))}))

    element_texts, element_text_uuids, element_text_folios = {}, {}, {}
    for n, d in _folios(root):
        for el in d.iter("element"):
            uuid = el.get("uuid", "")
            seen = {}
            for t in el.iter("dynamic_elmt_text"):
                src = t.get("text_from", "")
                what = (t.findtext("info_name") if src == "ElementInfo"
                        else t.findtext("composite_text") if src == "CompositeText"
                        else t.findtext("text")) or ""
                base = (uuid, src, what)
                seen[base] = seen.get(base, 0) + 1
                fs = (t.get("font", "").split(",") + ["", ""])[1]
                element_text_uuids[base + (seen[base],)] = t.get("uuid", "")
                element_text_folios[base + (seen[base],)] = n
                element_texts[base + (seen[base],)] = {
                    "x": t.get("x", ""), "y": t.get("y", ""), "size": fs,
                    "frame": t.get("frame", ""), "rotation": _angle(t.get("rotation", "")),
                    "width": t.get("text_width", ""),
                    "shows": t.findtext("text") or ""}

    strips = {}
    for st in root.iter("terminal_strip"):
        data = st.find("terminal_strip_data")
        if data is None:
            continue
        info = {i.get("name"): (i.text or "") for i in data.iter("information")}
        strips[data.get("uuid", "")] = {
            "installation": info.get("installation", ""),
            "location": info.get("location", ""),
            "name": info.get("name", ""),
            "terminals": sum(1 for _ in st.iter("real_terminal"))}
    return {"folios": folios, "folio_uuids": folio_uuids, "texts": texts, "shapes": shapes,
            "images": images, "tables": tables, "strips": strips,
            "element_texts": element_texts, "element_text_uuids": element_text_uuids,
            "element_text_folios": element_text_folios}


def _usable_ids(*sides) -> bool:
    """Whether uuids can identify items: present on every item, unique on each
    side. Copying a symbol keeps its text fields' uuids, so a project can
    hold the same field uuid twenty times; keying on it would merge them."""
    return (any(sides) and all(all(s) for s in sides)
            and all(len(set(s)) == len(s) for s in sides))


def _diff_keyed(a: dict, b: dict, label) -> dict:
    """added / removed / changed for two dicts keyed by identity."""
    changed = []
    for k in sorted(set(a) & set(b), key=str):
        delta = {f: [a[k][f], b[k][f]] for f in a[k] if a[k][f] != b[k].get(f)}
        if delta:
            changed.append({"item": label(k), "changed": delta})
    return {"before": len(a), "after": len(b),
            "added": [label(k) for k in sorted(set(b) - set(a), key=str)][:50],
            "removed": [label(k) for k in sorted(set(a) - set(b), key=str)][:50],
            "changed": changed[:50]}


def _diff_items(a: list, b: list) -> dict:
    """_diff_keyed() over _extras() records of one kind.

    Keyed on uuid only when every item on both sides has one. A file saved
    before these items carried a uuid has none, and the first save by a
    current QElectroTech gives them one, so a mixed pair falls back to
    position for the whole kind rather than reading as everything removed
    and re-added. On uuid, position is part of what is compared, so a move
    is a change to that item.
    """
    by_uuid = _usable_ids([r["uuid"] for r in a], [r["uuid"] for r in b])

    def key(r):
        return r["uuid"] if by_uuid else str(r["key"])

    def keyed(rs):
        return {key(r): ({**r["label"], **r["value"]} if by_uuid else r["value"])
                for r in rs}

    labels = {key(r): ({**r["label"], "uuid": r["uuid"]} if by_uuid else r["label"])
              for r in a + b}
    out = _diff_keyed(keyed(a), keyed(b), lambda k: labels[k])
    out["keyed_by"] = "uuid" if by_uuid else "position"
    return out


def _diff_folios(a: dict, b: dict) -> dict:
    """Folio fields, keyed by the folio's uuid when every folio has one.

    By uuid, a folio moved to another position is reported once, under
    "reordered", instead of as every folio after it changing its fields.
    Without uuids (older files) folios are keyed by position, and a
    removal or reorder in the middle shifts every later index -- the note
    says so when the count changed.
    """
    ua, ub = a["folio_uuids"], b["folio_uuids"]
    by_uuid = _usable_ids(list(ua.values()), list(ub.values()))
    changed, reordered, added, removed = [], [], [], []
    if by_uuid:
        pos_a = {u: n for n, u in ua.items()}
        pos_b = {u: n for n, u in ub.items()}
        for u in sorted(set(pos_a) & set(pos_b), key=lambda u: pos_b[u]):
            fa, fb = a["folios"][pos_a[u]], b["folios"][pos_b[u]]
            delta = {f: [fa[f], fb[f]] for f in _FOLIO_FIELDS if fa[f] != fb[f]}
            if delta:
                changed.append({"folio": pos_b[u], "uuid": u, "changed": delta})
            if pos_a[u] != pos_b[u]:
                reordered.append({"uuid": u, "title": fb["title"],
                                  "from": pos_a[u], "to": pos_b[u]})
        added = [{"folio": pos_b[u], "uuid": u, "title": b["folios"][pos_b[u]]["title"]}
                 for u in sorted(set(pos_b) - set(pos_a), key=lambda u: pos_b[u])]
        removed = [{"folio": pos_a[u], "uuid": u, "title": a["folios"][pos_a[u]]["title"]}
                   for u in sorted(set(pos_a) - set(pos_b), key=lambda u: pos_a[u])]
    else:
        for n in sorted(set(a["folios"]) & set(b["folios"])):
            delta = {f: [a["folios"][n][f], b["folios"][n][f]] for f in _FOLIO_FIELDS
                     if a["folios"][n][f] != b["folios"][n][f]}
            if delta:
                changed.append({"folio": n, "changed": delta})
    out = {"before": len(a["folios"]), "after": len(b["folios"]),
           "keyed_by": "uuid" if by_uuid else "position", "changed": changed[:50]}
    if by_uuid:
        out.update(added=added[:50], removed=removed[:50], reordered=reordered[:50])
    elif len(a["folios"]) != len(b["folios"]) and changed:
        out["note"] = ("the folio count changed, so changes listed here may be "
                       "later folios shifting position rather than edits")
    return out


def _diff_element_texts(a: dict, b: dict) -> dict:
    """Element text fields, keyed by their own uuid when every field has one.

    Otherwise by element, what the field is bound to, and the nth such field
    -- which cannot tell a field that was removed from one that moved down
    the list. A field's own text is also compared ("shows"), so relabelling
    an element shows up here as well as in the element's information.
    """
    # A field's uuid is unique only within its symbol (copies keep them), so
    # a field is identified by its symbol's uuid and its own.
    ka = {k: (k[0], u) if k[0] and u else "" for k, u in a["element_text_uuids"].items()}
    kb = {k: (k[0], u) if k[0] and u else "" for k, u in b["element_text_uuids"].items()}
    by_uuid = _usable_ids(list(ka.values()), list(kb.values()))
    def label(k):
        return {"element": k[0], "source": k[1], "bound_to": k[2], "n": k[3]}
    if not by_uuid:
        out = _diff_keyed(a["element_texts"], b["element_texts"], label)
        out["keyed_by"] = "position"
        return out
    labels = {u: {**label(k), "uuid": u[1]} for side in (ka, kb) for k, u in side.items()}
    out = _diff_keyed({ka[k]: v for k, v in a["element_texts"].items()},
                      {kb[k]: v for k, v in b["element_texts"].items()},
                      lambda u: labels[u])
    out["keyed_by"] = "uuid"
    return out


def _diff_extras(before: ET.Element, after: ET.Element) -> dict:
    a, b = _extras(before), _extras(after)
    out = {}
    ta, tb = before.get("title", ""), after.get("title", "")
    out["project"] = {"changed": {"title": [ta, tb]} if ta != tb else {}}
    out["folios"] = _diff_folios(a, b)
    out["texts"] = _diff_items(a["texts"], b["texts"])
    out["shapes"] = _diff_items(a["shapes"], b["shapes"])
    out["images"] = _diff_items(a["images"], b["images"])
    out["tables"] = _diff_items(a["tables"], b["tables"])
    out["element_texts"] = _diff_element_texts(a, b)
    out["terminal_strips"] = _diff_keyed(
        a["strips"], b["strips"],
        lambda k: (lambda v: f"{v['installation']} {v['location']} {v['name']}".strip())(
            (b["strips"].get(k) or a["strips"].get(k))))
    return out


def tool_diff(before: str, after: str) -> dict:
    """Structural diff of two .qet files.

    This is the tool that answers "what did that edit actually change",
    which is the question a screenshot answers badly.
    """
    # Key on uuid where there is one. Files written before conductors and
    # elements carried persisted uuids fall back to a positional key, which
    # is why a move in such a file reads as remove+add rather than a move.
    a_el, b_el = {}, {}
    for i, e in _elements(_root(before)):
        r = _element_row(i, e)
        # Rotation is saved as "orientation", in quarter turns (0-3); it is
        # the only thing a rotation changes, so without it a rotated symbol
        # reads as untouched.
        r["orientation"] = e.get("orientation", "0")
        a_el[r["uuid"] or f"{i}:{r['x']},{r['y']}:{r['name']}"] = r
    for i, e in _elements(_root(after)):
        r = _element_row(i, e)
        r["orientation"] = e.get("orientation", "0")
        b_el[r["uuid"] or f"{i}:{r['x']},{r['y']}:{r['name']}"] = r

    moved, rotated, relabelled, changed_info = [], [], [], []
    for k, a in a_el.items():
        b = b_el.get(k)
        if b is None:
            continue
        if a["orientation"] != b["orientation"]:
            rotated.append({"uuid": k, "name": a["name"], "folio": a["folio"],
                            "orientation": [a["orientation"], b["orientation"]]})
        if (a["x"], a["y"]) != (b["x"], b["y"]):
            moved.append({
                "uuid": k, "name": a["name"], "folio": a["folio"],
                "from": [a["x"], a["y"]], "to": [b["x"], b["y"]],
                "delta": [_num(b["x"]) - _num(a["x"]),
                          _num(b["y"]) - _num(a["y"])],
            })
        if a["label"] != b["label"]:
            relabelled.append({"uuid": k, "name": a["name"],
                               "from": a["label"], "to": b["label"]})
        # An empty field and a missing one mean the same thing, and
        # QElectroTech drops empty ones when it saves, so compare only the
        # fields that hold a value.
        a_info = {n: v for n, v in a["info"].items() if v}
        b_info = {n: v for n, v in b["info"].items() if v}
        if a_info != b_info:
            changed_info.append({"uuid": k, "name": a["name"],
                                 "from": a_info, "to": b_info})

    a_rows = [_conductor_row(i, c, ix) for i, c, ix in _conductors(_root(before))]
    b_rows = [_conductor_row(i, c, ix) for i, c, ix in _conductors(_root(after))]
    # Keyed by the conductor's own uuid when every conductor on both sides
    # has one, so a rewired conductor is that conductor, changed ("ends").
    # QElectroTech keeps a uuid only on conductors that were loaded with one
    # or created since, so an older file keys on its two ends instead.
    co_by_uuid = _usable_ids([r["uuid"] for r in a_rows], [r["uuid"] for r in b_rows])
    co_id = (lambda r: r["uuid"]) if co_by_uuid else (lambda r: r["key"])
    a_co = {co_id(r): r for r in a_rows}
    b_co = {co_id(r): r for r in b_rows}
    # An end that could not be resolved to an element is keyed on the
    # folio-scoped integer id, which QElectroTech reassigns on every write.
    # Say so rather than presenting the result as if it were comparable:
    # in such a file an untouched conductor can read as removed and re-added.
    shaky = 0 if co_by_uuid else sum(1 for k in set(a_co) | set(b_co) if "#" in k)
    unstable = {} if not shaky else {
        "unstable_keys": shaky,
        "warning": "some conductors sit on elements with no persisted uuid, so "
                   "they are keyed on folio-scoped terminal ids that "
                   "QElectroTech renumbers on save; added/removed entries "
                   "marked with # may be the same conductor, not a change",
    }
    conductor_changes = []
    for k, a in a_co.items():
        b = b_co.get(k)
        if b is None:
            continue
        fields = {f: [a[f], b[f]] for f in
                  ("num", "formula", "cable", "bus", "color", "section",
                   "function", "type")
                  if a[f] != b[f]}
        if a["key"] != b["key"]:
            fields["ends"] = [a["key"], b["key"]]
        if fields:
            conductor_changes.append({"key": b["key"], **({"uuid": k} if co_by_uuid else {}),
                                      "changed": fields})

    deltas = sorted({tuple(m["delta"]) for m in moved})
    return {
        "elements": {
            "before": len(a_el), "after": len(b_el),
            "added": sorted(set(b_el) - set(a_el))[:50],
            "removed": sorted(set(a_el) - set(b_el))[:50],
            "moved": moved[:100],
            "moved_count": len(moved),
            "distinct_move_deltas": [list(d) for d in deltas],
            "relabelled": relabelled[:50],
            "info_changed": changed_info[:50],
            "rotated": rotated[:50],
        },
        "conductors": {
            "before": len(a_co), "after": len(b_co),
            "keyed_by": "uuid" if co_by_uuid else "ends",
            "added": sorted(b_co[k]["key"] for k in set(b_co) - set(a_co))[:50],
            "removed": sorted(a_co[k]["key"] for k in set(a_co) - set(b_co))[:50],
            "changed": conductor_changes[:100],
            "changed_count": len(conductor_changes),
            **unstable,
        },
        **_diff_extras(_root(before), _root(after)),
    }


def _num(v) -> float:
    try:
        return float(v)
    except (TypeError, ValueError):
        return 0.0


def tool_scan(directory: str, tag: str = "conductor",
              attribute: str = "cable", recursive: bool = True) -> dict:
    """Sweep every .qet in a directory, counting how many <tag> carry a
    non-empty `attribute`.

    This is the corpus question: "3190 conductors, 0 cable values across
    25 projects" is exactly one call to this tool.
    """
    d = Path(directory).expanduser()
    if not d.is_dir():
        raise ValueError(f"not a directory: {d}")
    files = sorted(d.rglob("*.qet") if recursive else d.glob("*.qet"))
    total = non_empty = 0
    per_file, values, unreadable = [], {}, []
    for f in files:
        try:
            root = ET.parse(f).getroot()
        except ET.ParseError as exc:
            unreadable.append({"file": f.name, "error": str(exc)})
            continue
        n = ne = 0
        for node in root.iter(tag):
            n += 1
            v = (node.get(attribute) or "").strip()
            if v:
                ne += 1
                values[v] = values.get(v, 0) + 1
        total += n
        non_empty += ne
        per_file.append({"file": f.name, tag: n, "non_empty": ne})
    return {
        "directory": str(d), "files": len(files), "unreadable": unreadable,
        "tag": tag, "attribute": attribute,
        "total": total, "non_empty": non_empty,
        "distinct_values": sorted(values.items(), key=lambda kv: -kv[1])[:25],
        "per_file": per_file,
    }


def _terminals_in_index_order(terminal_nodes) -> tuple:
    """Order an element's terminals the way QElectroTech indexes them.

    Not file order. Element::parseTerminal() re-sorts the terminals every
    time it adds one, top to bottom and then left to right, on each
    terminal's local (y, x) -- Terminal::dockConductor() is mapToScene() of
    its position, evaluated while the element still sits unrotated at the
    origin. So the terminal a script reaches as index 0 is the topmost one,
    whatever order the .elmt lists them in: bobine_ka_a_remanence.elmt
    writes A2 (y=20) before A1 (y=-20), and add_conductor's index 0 is A1.
    Getting this wrong wires the wrong end of a coil and nothing complains.

    Returns (nodes in index order, ambiguous). Two terminals at the same
    point tie, and the C++ sort is not stable, so which one is index 0 is
    not defined; ambiguous says so instead of pretending.
    """
    def key(t):
        try:
            return (float(t.get("y", 0)), float(t.get("x", 0)))
        except ValueError:
            return (0.0, 0.0)
    ordered = sorted(terminal_nodes, key=key)
    keys = [key(t) for t in ordered]
    return ordered, len(keys) != len(set(keys))


def tool_element_info(path: str) -> dict:
    """Introspect a .elmt: names, terminals, and which info fields it carries."""
    root = _root(path)
    names = {n.get("lang"): (n.text or "") for n in root.iter("name")}
    ordered, ambiguous = _terminals_in_index_order(list(root.iter("terminal")))
    terminals = [{"index": i, "x": t.get("x"), "y": t.get("y"),
                  "orientation": t.get("orientation"),
                  "name": t.get("name", ""), "type": t.get("type", ""),
                  "uuid": t.get("uuid", "")}
                 for i, t in enumerate(ordered)]
    info_fields = sorted({(i.text or "").strip()
                          for i in root.iter("info_name") if (i.text or "").strip()})
    parts = {}
    part_list = []
    desc = root.find("description")
    for child in (desc if desc is not None else []):
        parts[child.tag] = parts.get(child.tag, 0) + 1
        if child.tag != "terminal":
            # uuid is empty for a part saved before parts carried one; the
            # element editor gives it one on the next save.
            part_list.append({"type": child.tag, "uuid": child.get("uuid", "")})
    return {
        "file": str(Path(path).expanduser()),
        "type": root.get("type", ""), "link_type": root.get("link_type", ""),
        "width": root.get("width"), "height": root.get("height"),
        "names": names,
        "terminal_count": len(terminals), "terminals": terminals,
        "terminal_order": "index order: top to bottom then left to right, "
                          "not the file's order" + (
                              "; two terminals share a point, so their relative "
                              "index is undefined" if ambiguous else ""),
        "info_fields": info_fields,
        "parts": parts,
        "part_list": part_list,
    }


def _launch_executable(src: Path, sandbox: Path, windows: bool) -> Path:
    """The executable _run_qet() starts: a private copy, except on Windows.

    The copy gives each run its own SingleApplication key, which is derived
    from the executable's path. On Windows a program loads its DLLs from its
    own folder, so a copy on its own dies before main() (0xC0000135, DLL not
    found) and nothing could ever be exported or edited there. Run the
    original instead: every flag this server passes is a CLI export flag or
    --run, and QElectroTech handles both and returns before it constructs
    SingleApplication (main.cpp), so there is no instance to be handed to.
    The copy stays elsewhere for builds from before that early return.
    """
    if windows:
        return src
    exe = sandbox / f"qet-mcp-{os.getpid()}"
    shutil.copy2(src, exe)
    return exe


def _launch_env(env: dict, home: Path, windows: bool) -> dict:
    """The environment _run_qet() starts QElectroTech in.

    A private HOME and XDG directories, and QET_SETTINGS_DIR pointing into
    them. HOME and XDG move QElectroTech's settings only on Linux: Qt keeps
    them in the registry on Windows and in the user's preferences on
    macOS, so without QET_SETTINGS_DIR every run there read the user's own
    settings and never saw a collection path written for it
    (qelectrotech-source-mirror#1178). A QElectroTech that knows the
    variable keeps its settings in an INI file in that folder instead.

    Except on Windows, also Qt's offscreen platform so no display is
    needed. The Windows packages ship only the qwindows platform plugin:
    asked for "offscreen", Qt finds no plugin and stops at a message box
    nobody can close, so every call hung until its timeout. The export
    flags and --run open no window there, so the default platform is what
    they need.
    """
    env = dict(env,
               HOME=str(home),
               XDG_CONFIG_HOME=str(home / ".config"),
               XDG_DATA_HOME=str(home / ".local" / "share"),
               QET_SETTINGS_DIR=str(home / ".config"))
    if not windows:
        env["QT_QPA_PLATFORM"] = "offscreen"
    return env


def _collection_setting(collection: PurePath) -> str:
    """The settings file that points QElectroTech at @p collection.

    Forward slashes: Qt reads a backslash in these files as an escape, so a
    Windows path written as-is arrives mangled."""
    return ("[elements-collections]\n"
            f"common-collection-path={collection.as_posix()}\n")


def _run_qet(binary: str, args: list[str], timeout: int = 180,
             elements_dir: str | None = None,
             script: str | None = None, tail: int = 4000) -> dict:
    """Launch QElectroTech headlessly, carrying the known launch traps.

    SingleApplication keys its socket on applicationFilePath(), so a second
    launch of the same path forwards its request to an already-running
    instance and returns THAT process's answer with no error. Copying the
    binary to a unique path gives this run its own socket. A symlink will
    not do: applicationFilePath() resolves it back.

    The sandbox HOME that isolation buys also costs something, and it is
    not obvious: with no settings file, QETApp::commonElementsDir() falls
    back to the compiled-in QET_COMMON_COLLECTION_PATH, which on a machine
    that has never run `make install` does not exist. Every "common://..."
    path then fails to resolve and the only symptom is addElement()
    reporting "does not resolve to an element" for a file that is plainly
    there. elements_dir writes the one setting that fixes it. The file name
    is not free-choice: QSettings derives it from the organisation and
    application names main.cpp sets before this branch runs. It is written
    twice: QElectroTech/QElectroTech.ini is what a QElectroTech that knows
    QET_SETTINGS_DIR reads, on every system (see _launch_env()), and
    QElectroTech/QElectroTech.conf is what an older one reads on Linux.

    script, when given, is written into the sandbox and passed to --run.
    It lives inside the temporary directory so it cannot collide with a
    concurrent call, and it is returned to the caller on failure, because a
    generated script nobody can see is not debuggable.

    The environment is inherited, not rebuilt, so QET_ENABLE_SCRIPTING
    reaches QElectroTech from wherever this server was started -- normally
    the "env" block of the MCP client's own configuration. That is the
    consent: whoever configured this server and pointed it at a
    QElectroTech binary made the choice, and their interactive
    QElectroTech keeps whatever its own setting says. This server does not
    set the variable itself, because a switch a program turns on for
    itself is not a switch.
    """
    src = Path(binary).expanduser()
    if not src.is_file() or not os.access(src, os.X_OK):
        raise ValueError(f"not an executable: {src}")
    with tempfile.TemporaryDirectory(prefix="qet-mcp-") as tmp:
        sandbox = Path(tmp)
        exe = _launch_executable(src, sandbox, os.name == "nt")
        home = sandbox / "home"
        (home / ".config").mkdir(parents=True)
        (home / ".local" / "share").mkdir(parents=True)
        if elements_dir:
            coll = Path(elements_dir).expanduser()
            if not coll.is_dir():
                raise ValueError(f"no such elements directory: {coll}")
            cfg = home / ".config" / "QElectroTech"
            cfg.mkdir(parents=True, exist_ok=True)
            for name in ("QElectroTech.ini", "QElectroTech.conf"):
                (cfg / name).write_text(_collection_setting(coll), encoding="utf-8")
        if script is not None:
            script_path = sandbox / "qet-mcp-edit.js"
            script_path.write_text(script, encoding="utf-8")
            args = ["--run", str(script_path), *args]
        env = _launch_env(dict(os.environ), home, os.name == "nt")
        try:
            p = subprocess.run([str(exe), *args], env=env, timeout=timeout,
                               capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            return {"ok": False, "timed_out": True, "timeout_s": timeout,
                    "hint": "a modal dialog during load will hang a headless "
                            "run; check the project's format version"}
        result = {"ok": p.returncode == 0, "exit_code": p.returncode,
                  "stdout": p.stdout[-tail:], "stderr": p.stderr[-tail:]}
        # QElectroTech refuses --run when scripting is switched off, which
        # it is by default. Its own message is clear but French, and it
        # names a settings dialog that nobody driving this server is
        # looking at -- so say the thing that actually applies here. Keyed
        # on QElectroTech naming the variable, with exit 3 as a fallback
        # for a future build that words the refusal differently.
        if script is not None and not result["ok"] and (
                "QET_ENABLE_SCRIPTING" in p.stderr or p.returncode == 3):
            result["hint"] = (
                "this tool drives QElectroTech through a script, and this "
                "QElectroTech has scripting switched off. Add "
                "QET_ENABLE_SCRIPTING=1 to the environment this server is "
                "started in -- in an MCP client that is the \"env\" block of "
                "its entry in the client configuration. Only qet_query, "
                "qet_continuity, qet_check, qet_project_new, qet_edit, "
                "qet_script_api, qet_script_test, qet_script_install and "
                "qet_script_remove need it; every other tool either reads "
                "the file directly "
                "or uses a plain CLI flag.")
        return result


def tool_export(binary: str, project: str, format: str, output: str,
                timeout: int = 180) -> dict:
    if format not in EXPORT_FORMATS:
        raise ValueError(f"unknown format {format!r}; "
                         f"expected one of {', '.join(sorted(EXPORT_FORMATS))}")
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")
    # The CLI matches its flags by exact string (cli_export.cpp:828) and takes
    # the project and output as the two positional arguments after the flag
    # (:862, :882). A "--export-bom=out.csv" form is NOT recognised: it fails
    # the flag test, so the run is not treated as an export at all and the
    # application starts its GUI instead, which then hangs on an offscreen
    # platform. Order matters here.
    flag = EXPORT_FORMATS[format]
    result = _run_qet(binary, [flag, str(proj), output], timeout)
    out = Path(output).expanduser()
    result["output"] = str(out)
    result["output_exists"] = out.exists()
    if out.exists() and out.is_file():
        result["output_bytes"] = out.stat().st_size
    return result


# --------------------------------------------------------------------------
# qet_element_build: author a .elmt definition
# --------------------------------------------------------------------------

# Derived from the 6,918 shipped elements rather than from documentation:
# these are the attributes each part tag actually carries. Everything not
# listed here is refused, so a typo becomes an error instead of an
# attribute QElectroTech silently ignores.
PART_SCHEMA = {
    "line":    {"required": ("x1", "y1", "x2", "y2"),
                "optional": ("end1", "end2", "length1", "length2")},
    "rect":    {"required": ("x", "y", "width", "height"), "optional": ("rx", "ry")},
    "ellipse": {"required": ("x", "y", "width", "height"), "optional": ()},
    "circle":  {"required": ("x", "y", "diameter"), "optional": ()},
    "arc":     {"required": ("x", "y", "width", "height", "start", "angle"),
                "optional": ()},
    "polygon": {"required": ("points",), "optional": ("closed",)},
    "text":    {"required": ("x", "y", "text"), "optional": ("size", "rotation", "color")},
}

DEFAULT_STYLE = "line-style:normal;line-weight:normal;filling:none;color:black"

TERMINAL_ORIENTATIONS = ("n", "s", "e", "w")

# As used in the collection. "thumbnail" is included because it is the
# second most common value, not because this tool can build a good one.
LINK_TYPES = ("simple", "thumbnail", "master", "slave", "terminal",
              "next_report", "previous_report")


def _f(value, where: str) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        raise ValueError(f"{where}: expected a number, got {value!r}")


def _part_extent(kind: str, part: dict) -> list:
    """The x,y points a part reaches, for the bounding box."""
    g = lambda k: _f(part[k], f"{kind}.{k}")
    if kind == "line":
        return [(g("x1"), g("y1")), (g("x2"), g("y2"))]
    if kind in ("rect", "ellipse", "arc"):
        x, y, w, h = g("x"), g("y"), g("width"), g("height")
        return [(x, y), (x + w, y + h)]
    if kind == "circle":
        x, y, d = g("x"), g("y"), g("diameter")
        return [(x, y), (x + d, y + d)]
    if kind == "polygon":
        return [(_f(px, "polygon point"), _f(py, "polygon point"))
                for px, py in part["points"]]
    if kind == "text":
        return [(g("x"), g("y"))]
    return []


def _element_geometry(parts: list, terminals: list) -> dict:
    """Bounding box, then a declared box that contains it.

    The .elmt header carries width/height/hotspot_x/hotspot_y, and the
    relationship to the drawing is a containment constraint rather than a
    formula: the declared box runs from (-hotspot_x, -hotspot_y) to
    (width - hotspot_x, height - hotspot_y) in the element's own
    coordinates, and the drawing has to fit inside it. Checked against the
    shipped collection, where authors chose their own margins -- one
    element pads 2 units on the left and 3 on the right, another 8 and 2 --
    so there is nothing to copy, only an invariant to satisfy.

    Sizes are rounded out to multiples of 10, which is what every element
    sampled from the collection uses and what keeps terminals on the grid.
    """
    points = []
    for part in parts:
        points += _part_extent(part["type"], part)
    for t in terminals:
        points.append((_f(t["x"], "terminal.x"), _f(t["y"], "terminal.y")))
    if not points:
        raise ValueError("an element needs at least one part or terminal")

    min_x = min(x for x, _ in points)
    max_x = max(x for x, _ in points)
    min_y = min(y for _, y in points)
    max_y = max(y for _, y in points)

    import math
    pad = 5.0
    hotspot_x = int(math.ceil((-min_x + pad) / 10.0) * 10)
    hotspot_y = int(math.ceil((-min_y + pad) / 10.0) * 10)
    width = int(math.ceil((max_x + hotspot_x + pad) / 10.0) * 10)
    height = int(math.ceil((max_y + hotspot_y + pad) / 10.0) * 10)

    # The invariant, asserted rather than trusted: an element whose drawing
    # escapes its declared box is the classic way a hand-written .elmt
    # renders clipped in the collection panel while looking fine in XML.
    if not (-hotspot_x <= min_x and max_x <= width - hotspot_x
            and -hotspot_y <= min_y and max_y <= height - hotspot_y):
        raise ValueError(
            f"internal error: declared box ({-hotspot_x}, {-hotspot_y}) to "
            f"({width - hotspot_x}, {height - hotspot_y}) does not contain the "
            f"drawing ({min_x}, {min_y}) to ({max_x}, {max_y})")

    return {"width": width, "height": height,
            "hotspot_x": hotspot_x, "hotspot_y": hotspot_y,
            "bbox": [min_x, min_y, max_x, max_y]}


def _validate_part(index: int, part) -> str:
    if not isinstance(part, dict):
        raise ValueError(f"part {index} is not an object: {part!r}")
    kind = part.get("type")
    if kind not in PART_SCHEMA:
        raise ValueError(f"part {index}: unknown type {kind!r}; expected one of "
                         f"{', '.join(sorted(PART_SCHEMA))}")
    spec = PART_SCHEMA[kind]
    for key in spec["required"]:
        if key not in part:
            raise ValueError(f"part {index} ({kind}) is missing {key!r}")
    allowed = set(spec["required"]) | set(spec["optional"]) | {"type", "style", "antialias", "uuid"}
    for key in part:
        if key not in allowed:
            raise ValueError(f"part {index} ({kind}): unexpected {key!r}; "
                             f"allowed: {', '.join(sorted(allowed))}")
    if "uuid" in part and not (isinstance(part["uuid"], str)
                               and _UUID_RE.fullmatch(part["uuid"])):
        raise ValueError(f"part {index} ({kind}): uuid must look like "
                         f"{{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}}, got {part['uuid']!r}")
    if kind == "polygon":
        pts = part["points"]
        if not isinstance(pts, list) or len(pts) < 2:
            raise ValueError(f"part {index} (polygon) needs at least two points")
        for pt in pts:
            if not (isinstance(pt, (list, tuple)) and len(pt) == 2):
                raise ValueError(f"part {index} (polygon): each point is [x, y], got {pt!r}")
    return kind


def _part_uuid(part: dict) -> str:
    """The caller's uuid for this part, braced as QElectroTech writes it, or
    a new one. Every part carries one, as the element editor saves them."""
    given = part.get("uuid")
    if given:
        return given if given.startswith("{") else "{" + given + "}"
    return "{" + str(__import__("uuid").uuid4()) + "}"


def _part_element(part: dict, uuid: str) -> ET.Element:
    kind = part["type"]
    node = ET.Element(kind)
    node.set("uuid", uuid)
    if kind == "polygon":
        for n, (px, py) in enumerate(part["points"], start=1):
            node.set(f"x{n}", _fmt(px))
            node.set(f"y{n}", _fmt(py))
        node.set("closed", "true" if part.get("closed", True) else "false")
    elif kind == "text":
        node.set("x", _fmt(part["x"]))
        node.set("y", _fmt(part["y"]))
        node.set("text", str(part["text"]))
        node.set("rotation", _fmt(part.get("rotation", 0)))
        node.set("font", f"Sans Serif,{int(part.get('size', 9))},-1,5,50,0,0,0,0,0")
        node.set("color", str(part.get("color", "#000000")))
        return node
    else:
        for key in PART_SCHEMA[kind]["required"] + PART_SCHEMA[kind]["optional"]:
            if key in part:
                node.set(key, _fmt(part[key]))
    node.set("antialias", "true" if part.get("antialias", True) else "false")
    node.set("style", part.get("style", DEFAULT_STYLE))
    return node


def _fmt(v) -> str:
    """Numbers the way QElectroTech writes them: no trailing .0."""
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int, float)):
        f = float(v)
        return str(int(f)) if f == int(f) else repr(f)
    return str(v)


def tool_element_build(output: str, names: dict, parts: list,
                       terminals: list | None = None,
                       link_type: str = "simple",
                       informations: dict | None = None,
                       uuid: str | None = None) -> dict:
    """Write a .elmt element definition.

    Unlike a project, an element definition is not rewritten by
    QElectroTech on a round trip, so generating one here is safe in a way
    that generating a .qet would not be: there is no toXml() that will
    drop what this writer did not know to emit.

    What it will not do is invent geometry. The caller supplies the parts;
    this validates them against the schema the shipped collection actually
    uses, computes the width/height/hotspot header so the declared box
    contains the drawing, and refuses anything it cannot place.
    """
    terminals = terminals or []
    if not isinstance(names, dict) or not names:
        raise ValueError('names must be a non-empty object, e.g. {"en": "Coil", "fr": "Bobine"}')
    if link_type not in LINK_TYPES:
        raise ValueError(f"unknown link_type {link_type!r}; expected one of "
                         f"{', '.join(LINK_TYPES)}")
    if not isinstance(parts, list):
        raise ValueError("parts must be a list")
    for i, part in enumerate(parts):
        _validate_part(i, part)
    for i, t in enumerate(terminals):
        if not isinstance(t, dict):
            raise ValueError(f"terminal {i} is not an object: {t!r}")
        for key in ("x", "y", "orientation"):
            if key not in t:
                raise ValueError(f"terminal {i} is missing {key!r}")
        if t["orientation"] not in TERMINAL_ORIENTATIONS:
            raise ValueError(f"terminal {i}: orientation is one of "
                             f"{', '.join(TERMINAL_ORIENTATIONS)}, got {t['orientation']!r}")
    # A master with no terminal cannot be wired, and a slave with none
    # cannot be placed on a rail -- both are silent failures at use time.
    if link_type in ("master", "slave", "simple") and not terminals:
        raise ValueError(f"a {link_type} element with no terminals cannot be connected; "
                         "add terminals, or use link_type 'thumbnail' for a drawing-only element")

    geometry = _element_geometry(parts, terminals)

    root = ET.Element("definition", {
        "version": "0.100.0", "type": "element", "link_type": link_type,
        "width": str(geometry["width"]), "height": str(geometry["height"]),
        "hotspot_x": str(geometry["hotspot_x"]), "hotspot_y": str(geometry["hotspot_y"]),
    })
    ET.SubElement(root, "uuid", {"uuid": uuid or "{" + str(__import__("uuid").uuid4()) + "}"})
    names_node = ET.SubElement(root, "names")
    for lang in sorted(names):
        ET.SubElement(names_node, "name", {"lang": lang}).text = str(names[lang])
    if informations:
        kind = ET.SubElement(root, "kindInformations")
        for key in sorted(informations):
            ET.SubElement(kind, "kindInformation", {"name": key}).text = str(informations[key])
    ET.SubElement(root, "informations")
    description = ET.SubElement(root, "description")
    part_uuids = [_part_uuid(part) for part in parts]
    if len(set(part_uuids)) != len(part_uuids):
        raise ValueError("two parts were given the same uuid")
    for part, part_uuid in zip(parts, part_uuids):
        description.append(_part_element(part, part_uuid))
    for t in terminals:
        attrs = {"x": _fmt(t["x"]), "y": _fmt(t["y"]),
                 "orientation": t["orientation"],
                 "type": t.get("type", "Generic"),
                 "uuid": "{" + str(__import__("uuid").uuid4()) + "}"}
        if t.get("name"):
            attrs["name"] = str(t["name"])
        ET.SubElement(description, "terminal", attrs)

    out = Path(output).expanduser()
    out.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(root, space="    ")
    out.write_bytes(ET.tostring(root, encoding="utf-8", xml_declaration=True))

    # Read it back with the same reader every other tool here uses, rather
    # than reporting what was intended.
    check = tool_element_info(str(out))
    return {"ok": True, "output": str(out), "bytes": out.stat().st_size,
            **geometry,
            # The order add_conductor will use, which is not the order the
            # caller listed them in.
            "terminal_index_order": [t["name"] or f"({t['x']},{t['y']})"
                                    for t in check["terminals"]],
            # In the order the parts were given.
            "part_uuids": part_uuids,
            "verified": check}


# --------------------------------------------------------------------------
# qet_edit: drive the scripting API, then prove what it did
# --------------------------------------------------------------------------

# op name -> (qet method, argument spec). A spec entry is (json key, kind),
# where kind says how the value is turned into JavaScript and, for "folio"
# and "elmt", that it may be a "$name" reference to an earlier op's result.
OPS = {
    "add_folio":        (None,                  []),
    "set_folio_title":  ("setFolioTitle",       [("folio", "folio"), ("title", "str")]),
    "add_element":      ("addElement",          [("folio", "folio"), ("path", "str"),
                                                 ("x", "num"), ("y", "num")]),
    "set_position":     ("setElementPosition",  [("folio", "folio"), ("element", "elmt"),
                                                 ("x", "num"), ("y", "num")]),
    "move_element":     ("moveElement",         [("folio", "folio"), ("element", "elmt"),
                                                 ("dx", "num"), ("dy", "num")]),
    "rotate_element":   ("rotateElement",       [("folio", "folio"), ("element", "elmt"),
                                                 ("angle", "num")]),
    "set_label":        ("setElementLabel",     [("folio", "folio"), ("element", "elmt"),
                                                 ("label", "str")]),
    "set_info":         ("setElementInfo",      [("folio", "folio"), ("element", "elmt"),
                                                 ("key", "str"), ("value", "str")]),
    "add_conductor":    ("addConductor",        [("folio", "folio"),
                                                 ("from", "elmt"), ("from_terminal", "term"),
                                                 ("to", "elmt"), ("to_terminal", "term")]),
    "delete_element":   ("deleteElement",       [("folio", "folio"), ("element", "elmt")]),
    "set_conductor":    ("setConductorProperty", [("folio", "folio"), ("element", "elmt"),
                                                  ("terminal", "term"), ("property", "str"),
                                                  ("value", "str")]),
    "move_conductor_segment": ("moveConductorSegment", [("folio", "folio"), ("element", "elmt"),
                                                         ("terminal", "term"), ("segment", "num"),
                                                         ("dx", "num"), ("dy", "num")]),
    "link_elements":    ("linkElements",        [("folio", "folio"), ("element", "elmt"),
                                                 ("to_folio", "folio"), ("to", "elmt")]),
    "link_plc_io":      ("linkElements",        [("folio", "folio"), ("element", "elmt"),
                                                 ("to_folio", "folio"), ("to", "elmt"),
                                                 ("io_index", "folio")]),
    "unlink_element":   ("unlinkElement",       [("folio", "folio"), ("element", "elmt")]),
    "add_plc_io":       ("addPlcIO",            [("folio", "folio"), ("element", "elmt"),
                                                 ("type", "str"), ("address", "str"),
                                                 ("function", "str"), ("comment", "str")]),
    "set_plc_io":       ("setPlcIO",            [("folio", "folio"), ("element", "elmt"),
                                                 ("index", "folio"), ("property", "str"),
                                                 ("value", "str")]),
    "remove_plc_io":    ("removePlcIO",         [("folio", "folio"), ("element", "elmt"),
                                                 ("index", "folio")]),
    # Texts, shapes and images are addressed by "index": their index in a
    # position-sorted listing, which add_text/add_shape return, or their
    # uuid, which does not shift when another one is added. So
    # it can be named as "$id". Indexes shift when one is added or deleted.
    "delete_conductor": ("deleteConductor",     [("folio", "folio"), ("element", "elmt"),
                                                 ("terminal", "term")]),
    "remove_folio":     ("removeFolio",         [("folio", "folio")]),
    "set_folio":        ("setFolioProperty",    [("folio", "folio"), ("property", "str"),
                                                 ("value", "str")]),
    "add_terminal_strip":    ("addTerminalStrip",    [("installation", "str"), ("location", "str"),
                                                      ("name", "str")]),
    "remove_terminal_strip": ("removeTerminalStrip", [("strip", "folio")]),
    "add_to_strip":          ("addTerminalToStrip",  [("strip", "folio"), ("folio", "folio"),
                                                      ("element", "elmt")]),
    "group_terminals":       ("groupTerminals",       [("strip", "folio"), ("indices", "indices")]),
    "bridge_terminals":      ("bridgeTerminals",      [("strip", "folio"), ("indices", "indices")]),
    "sort_terminal_strip":   ("sortTerminalStrip",    [("strip", "folio")]),
    "add_autonum":      ("addAutoNum",          [("kind", "str"), ("name", "str"),
                                                 ("parts", "list")]),
    "remove_autonum":   ("removeAutoNum",       [("kind", "str"), ("name", "str")]),
    "use_conductor_autonum": ("useConductorAutoNum", [("folio", "folio"), ("name", "str")]),
    "use_element_autonum": ("useElementAutoNum", [("name", "str")]),
    "number_element":   ("numberElement",       [("folio", "folio"), ("element", "elmt")]),
    # The text fields drawn on a symbol. Indexed within the element's own
    # list, which follows its definition and shifts on delete (and undo of a
    # delete puts the field back at the end).
    "add_element_text":    ("addElementText",         [("folio", "folio"), ("element", "elmt"),
                                                       ("source", "str"), ("value", "str"),
                                                       ("x", "num"), ("y", "num")]),
    "set_element_text":    ("setElementTextProperty", [("folio", "folio"), ("element", "elmt"),
                                                       ("index", "element_text"), ("property", "str"),
                                                       ("value", "str")]),
    "delete_element_text": ("deleteElementText",      [("folio", "folio"), ("element", "elmt"),
                                                       ("index", "element_text")]),
    # Returns the uuids of the copies IN THE ORDER the elements were named,
    # so "$copies[0]" is the copy of the first one. Conductors between the
    # copied elements are copied with them; copies arrive without labels or
    # wire numbers, as they do on a paste in the application.
    "insert_folio":     ("insertFolio",         [("position", "folio")]),
    # Reads, not edits: the result is reported in the operations list, so a
    # follow-up call can lay something out relative to it.
    "element_geometry": ("elementGeometry",     [("folio", "folio"), ("element", "elmt")]),
    # Undo/redo act on QElectroTech's undo stack for this run, one command at
    # a time. Consecutive edits to the same property or information key merge
    # into one command, so one undo can revert several of them.
    "undo":             ("undo",                []),
    "redo":             ("redo",                []),
    "search_and_replace": ("searchAndReplace",  [("kind", "str"), ("field", "str"),
                                                 ("pattern", "str"), ("replacement", "str"),
                                                 ("regex", "bool"), ("case_sensitive", "bool")]),
    "set_project_title": ("setProjectTitle",    [("title", "str")]),
    "set_folio_border": ("setFolioBorder",      [("folio", "folio"), ("property", "str"),
                                                 ("value", "str")]),
    "embed_title_block_template": ("embedTitleBlockTemplate", [("name", "str")]),
    "duplicate_elements": ("duplicateElements", [("folio", "folio"), ("elements", "elmts"),
                                                 ("to_folio", "folio"), ("x", "num"), ("y", "num")]),
    "add_text":         ("addText",             [("folio", "folio"), ("text", "str"),
                                                 ("x", "num"), ("y", "num")]),
    "set_text":         ("setTextContent",      [("folio", "folio"), ("index", "text"),
                                                 ("text", "str")]),
    "set_text_color":   ("setTextColor",        [("folio", "folio"), ("index", "text"),
                                                 ("color", "str")]),
    "rotate_text":      ("setTextRotation",     [("folio", "folio"), ("index", "text"),
                                                 ("angle", "num")]),
    "delete_text":      ("deleteText",          [("folio", "folio"), ("index", "text")]),
    "add_shape":        ("addShape",            [("folio", "folio"), ("shape", "str"),
                                                 ("x1", "num"), ("y1", "num"),
                                                 ("x2", "num"), ("y2", "num")]),
    "set_shape":        ("setShapeProperty",    [("folio", "folio"), ("index", "shape"),
                                                 ("property", "str"), ("value", "str")]),
    "add_image":        ("addImage",            [("folio", "folio"), ("file", "str"),
                                                 ("x", "num"), ("y", "num")]),
    "add_pdf_page":     ("addPdfPage",           [("folio", "folio"), ("file", "str"),
                                                 ("page", "num"), ("dpi", "num"),
                                                 ("x", "num"), ("y", "num")]),
    "scale_image":      ("setImageScale",       [("folio", "folio"), ("index", "image"),
                                                 ("factor", "num")]),
    "rotate_image":     ("setImageRotation",    [("folio", "folio"), ("index", "image"),
                                                 ("angle", "num")]),
    "delete_image":     ("deleteImage",         [("folio", "folio"), ("index", "image")]),
    "delete_shape":     ("deleteShape",         [("folio", "folio"), ("index", "shape")]),
    "add_polygon":      ("addPolygon",          [("folio", "folio"), ("points", "points"),
                                                 ("closed", "bool")]),
    "set_shape_polygon": ("setShapePolygon",    [("folio", "folio"), ("index", "shape"),
                                                 ("points", "points")]),
    "add_path":         ("addPath",             [("folio", "folio"), ("nodes", "nodes"),
                                                 ("closed", "bool")]),
    "set_shape_path_nodes": ("setShapePathNodes", [("folio", "folio"), ("index", "shape"),
                                                   ("nodes", "nodes")]),
    "set_shape_closed": ("setShapeClosed",      [("folio", "folio"), ("index", "shape"),
                                                 ("closed", "bool")]),
    "add_table":        ("addTable",            [("folio", "folio"), ("kind", "str"),
                                                 ("name", "str"), ("query", "str")]),
    "set_table_position": ("setTablePosition",  [("folio", "folio"), ("table", "table"),
                                                 ("x", "num"), ("y", "num")]),
    "delete_table":     ("deleteTable",         [("folio", "folio"), ("table", "table")]),
}

SHAPES = ["line", "rectangle", "ellipse", "polygon"]
FOLIO_BORDER_PROPERTIES = ["columns", "column-width", "display-columns",
                           "rows", "row-height", "display-rows"]
ELEMENT_TEXT_SOURCES = ["text", "info", "composite"]
ELEMENT_TEXT_PROPERTIES = ["text", "source", "info", "composite", "frame", "size",
                           "x", "y", "rotation", "width"]
SHAPE_PROPERTIES = ["color", "fill", "width", "line-style", "rotation"]
AUTONUM_KINDS = ["conductor", "element", "folio"]
SEARCH_REPLACE_KINDS = ["element_info", "conductor", "text"]
FOLIO_PROPERTIES = ["title", "author", "filename", "plant", "locmach",
                    "indexrev", "folio", "template"]

# The ops that make a folio: their "$id" is an index the next insert_folio
# or remove_folio can shift, so the script also keeps the folio's uuid.
FOLIO_MAKING_OPS = ("add_folio", "insert_folio")

# The ops that address one conductor by element + terminal, and so also
# take "conductor": "{uuid}" (qet_conductors reports each one's uuid).
CONDUCTOR_UUID_OPS = ("set_conductor", "move_conductor_segment", "delete_conductor")

# Accepted by set_conductor. The names are the project file's own, so what
# a script sets is what qet_conductors reports back.
CONDUCTOR_PROPERTIES = ["num", "formula", "function", "bus", "cable",
                        "tension_protocol", "conductor_color",
                        "conductor_section", "color", "text_color",
                        # the conductor's look, under the file's own names
                        "color2", "bicolor", "style", "dash-size",
                        "condsize", "numsize", "displaytext"]

# Methods this tool needs that only exist in a build carrying the drawing
# verbs. Probed in the script rather than assumed, because the failure mode
# otherwise is a TypeError on line N of a generated file the caller never
# sees, reported as "the edit failed".
_REQUIRED_METHODS = sorted({m for m, _ in OPS.values() if m} |
                           {"save", "folioCount", "conductorCount", "elementCount"})

_MARKER = "QETEDIT "

_UUID_RE = re.compile(r"\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
                      r"[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\}?")


def _js(value) -> str:
    """A JSON literal is a JavaScript literal for every type used here."""
    return json.dumps(value)


def _build_script(operations: list, output: str) -> str:
    """Turn the operation list into a script, or raise on a bad operation.

    Every op is validated here, before QElectroTech is launched at all: a
    typo in an op name should cost nothing, not a process start and a
    JavaScript exception.
    """
    refs: set[str] = set()
    # "$name"s made by an op that creates a folio: held as the folio's uuid
    # too, since a later insert_folio or remove_folio shifts its index.
    folio_refs: set[str] = set()
    # Resolvers a uuid reference needs; required only when one is used, so
    # an index-only edit still runs on a build that predates them.
    uuid_methods: set[str] = set()
    folio_js = "0"
    element_js = None      # the op's element, for lookups scoped to it
    owner_js = None        # the element a terminal argument belongs to
    lines = [
        "// generated by qet-mcp; do not edit",
        "var R = {};",                       # $name -> value from an earlier op
        "var F = {};",                       # $name -> uuid of a folio an op made
        "var missing = [];",
        "var need = @NEED@;",
        "for (var i = 0; i < need.length; i++) {",
        "  if (typeof qet[need[i]] !== 'function') missing.push(need[i]);",
        "}",
        f"qet.log({_js(_MARKER)} + JSON.stringify("
        "{kind: 'capabilities', missing: missing}));",
        "var stop = false;",
        # A conductor named by uuid is turned into one of its ends, as the
        # conductor calls take it: an end whose terminal carries no other
        # conductor, so the call cannot pick the wrong one. null, with the
        # reason logged, if it is not on the folio or both ends are shared.
        "function qetMcpConductorEnd(index, folio, uuid) {",
        "  var ends = qet.conductorEnds(folio, uuid);",
        "  var why = 'no conductor ' + uuid + ' on folio ' + folio;",
        "  if (ends && ends.length === 2) {",
        "    var lines = qet.conductors(folio);",
        "    for (var k = 0; k < 2; k++) {",
        "      if (ends[k] === '?') continue;",
        "      var n = 0;",
        "      for (var j = 0; j < lines.length; j++) {",
        "        var p = lines[j].split(' : ')[0].split(' -- ');",
        "        if (p[0] === ends[k] || p[1] === ends[k]) n++;",
        "      }",
        "      if (n === 1) {",
        "        var m = ends[k].split(' terminal ');",
        "        return {element: m[0], terminal: parseInt(m[1], 10)};",
        "      }",
        "    }",
        "    why = 'conductor ' + uuid + ' shares both of its terminals with other '",
        "        + 'conductors; the conductor calls address one by a terminal carrying '",
        "        + 'only it';",
        "  }",
        f"  qet.log({_js(_MARKER)} + JSON.stringify("
        "{kind: 'op_note', index: index, note: why}));",
        "  return null;",
        "}",
        # A terminal named by uuid is turned into the index the calls take,
        # on its own element; -1, with the reason logged, if that element
        # has no terminal with it.
        "function qetMcpTerminal(index, key, folio, element, uuid) {",
        "  var t = qet.terminalIndex(folio, element, uuid);",
        "  if (t < 0) qet.log(" + _js(_MARKER) + " + JSON.stringify({kind: 'op_note', "
        "index: index, note: key + ': no terminal ' + uuid + ' on element ' + element + "
        "' (or two of its terminals carry it)'}));",
        "  return t;",
        "}",
        "if (missing.length === 0) {",
    ]

    def ref_or(value, kind: str, op_index: int, key: str) -> str:
        if kind == "elmts":
            if not isinstance(value, list) or not value or not all(isinstance(v, str) for v in value):
                raise ValueError(f"operation {op_index}: {key!r} must be a non-empty list of "
                                 f"elements (uuids or \"$id\" references), got {value!r}")
            return "[" + ", ".join(ref_or(v, "elmt", op_index, key) for v in value) + "]"
        if isinstance(value, str) and value.startswith("$"):
            indexed = re.fullmatch(r"\$([A-Za-z0-9_]+)\[(\d+)\]", value)
            if indexed:
                # one item of a list result, e.g. the second copy from
                # duplicate_elements
                name, n = indexed.group(1), int(indexed.group(2))
                if name not in refs:
                    raise ValueError(
                        f"operation {op_index} refers to {value!r}, which no earlier "
                        f"operation defined (set \"id\": {name!r} on the op that creates it)")
                return f"R[{_js(name)}][{n}]"
            name = value[1:]
            if name not in refs:
                raise ValueError(
                    f"operation {op_index} refers to {value!r}, which no earlier "
                    f"operation defined (set \"id\": {name!r} on the op that creates it)")
            if kind == "folio" and name in folio_refs:
                # the folio's index now, not when it was made; the stored
                # index on a build that cannot report folio uuids
                ref = _js(name)
                return f"(F[{ref}] ? qet.folioIndex(F[{ref}]) : R[{ref}])"
            return f"R[{_js(name)}]"
        if kind == "num":
            if not isinstance(value, (int, float)) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a number, "
                                 f"got {value!r}")
            return _js(value)
        if kind == "list":
            if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
                raise ValueError(f"operation {op_index}: {key!r} must be a list of strings, "
                                 f"got {value!r}")
            return _js(value)
        if kind == "indices":
            if (not isinstance(value, list) or not value
                    or not all(isinstance(x, int) and not isinstance(x, bool) for x in value)):
                raise ValueError(f"operation {op_index}: {key!r} must be a non-empty list of "
                                 f"integer indices, got {value!r}")
            return _js(value)
        if kind == "table":
            # As for texts below: a uuid is resolved to the current index at
            # run time, since deleting an earlier table shifts every index.
            if isinstance(value, str) and _UUID_RE.fullmatch(value):
                uuid_methods.add("tableIndex")
                return f"qet.tableIndex({folio_js}, {_js(value)})"
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a table index "
                                 f"or its uuid, got {value!r}")
            return _js(value)
        if kind == "term":
            # A terminal's uuid comes from its symbol's definition, so it
            # names a terminal only on its element: the op's element, or for
            # add_conductor the end's own. Turned into the index the call
            # takes at run time; unlike the index, it is defined between two
            # terminals at the same point.
            if isinstance(value, str) and _UUID_RE.fullmatch(value):
                uuid_methods.add("terminalIndex")
                return (f"qetMcpTerminal({op_index}, {_js(key)}, {folio_js}, {owner_js}, "
                        f"{_js(value)})")
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a terminal index "
                                 f"or its uuid, got {value!r}")
            return _js(value)
        if kind == "element_text":
            # A field's uuid is unique only within its element (copies keep
            # them), so the lookup takes the op's element too.
            if isinstance(value, str) and _UUID_RE.fullmatch(value):
                uuid_methods.add("elementTextIndex")
                return f"qet.elementTextIndex({folio_js}, {element_js}, {_js(value)})"
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a text field index "
                                 f"or its uuid, got {value!r}")
            return _js(value)
        if kind in ("text", "shape", "image"):
            # A uuid names the item for good; it is turned into the index
            # the call takes at run time, by the item's own folio.
            if isinstance(value, str) and _UUID_RE.fullmatch(value):
                method = {"text": "textIndex", "shape": "shapeIndex",
                          "image": "imageIndex"}[kind]
                uuid_methods.add(method)
                return f"qet.{method}({folio_js}, {_js(value)})"
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a {kind} index, "
                                 f"its uuid, or a \"$name\" reference, got {value!r}")
            return _js(value)
        if kind == "folio":
            # A uuid names the folio for good; it is turned into the index
            # the call takes at run time, since adding or removing a folio
            # shifts every index after it.
            if isinstance(value, str) and _UUID_RE.fullmatch(value):
                uuid_methods.add("folioIndex")
                return f"qet.folioIndex({_js(value)})"
            if value == "":
                raise ValueError(f"operation {op_index}: {key!r} is empty -- a folio saved "
                                 f"without a uuid has none in the file until the project is "
                                 f"saved once; give its index instead")
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a folio index, "
                                 f"its uuid, or a \"$name\" reference, got {value!r}")
            return _js(value)
        if kind == "bool":
            if not isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be true or false, "
                                 f"got {value!r}")
            return _js(value)
        if kind in ("points", "nodes"):
            def _num(v):
                return isinstance(v, (int, float)) and not isinstance(v, bool)
            if not isinstance(value, list) or len(value) < 2:
                raise ValueError(f"operation {op_index}: {key!r} must be a list of at "
                                 f"least 2 {'points' if kind == 'points' else 'nodes'}, "
                                 f"got {value!r}")
            for item in value:
                if not isinstance(item, dict) or not _num(item.get("x")) or not _num(item.get("y")):
                    raise ValueError(f"operation {op_index}: {key!r} entries must be "
                                     f"{{\"x\": num, \"y\": num, ...}}, got {item!r}")
                if kind == "nodes":
                    if "kind" in item and item["kind"] not in ("corner", "smooth", "symmetric"):
                        raise ValueError(f"operation {op_index}: {key!r} entry kind "
                                         f"{item['kind']!r} must be corner, smooth or symmetric")
                    for hkey in ("inHandle", "outHandle"):
                        if hkey in item and (not isinstance(item[hkey], dict)
                                             or not _num(item[hkey].get("x"))
                                             or not _num(item[hkey].get("y"))):
                            raise ValueError(f"operation {op_index}: {key!r} entry "
                                             f"{hkey!r} must be {{\"x\": num, \"y\": num}}")
            return _js(value)
        return _js("" if value is None else str(value))

    for i, op in enumerate(operations):
        if not isinstance(op, dict):
            raise ValueError(f"operation {i} is not an object: {op!r}")
        name = op.get("op")
        if name not in OPS:
            raise ValueError(f"operation {i}: unknown op {name!r}; "
                             f"expected one of {', '.join(sorted(OPS))}")
        method, spec = OPS[name]
        if name == "set_conductor" and op.get("property") not in CONDUCTOR_PROPERTIES:
            raise ValueError(f"operation {i}: unknown conductor property "
                             f"{op.get('property')!r}; expected one of "
                             f"{', '.join(CONDUCTOR_PROPERTIES)}")
        if name == "set_folio" and op.get("property") not in FOLIO_PROPERTIES:
            raise ValueError(f"operation {i}: unknown folio property "
                             f"{op.get('property')!r}; expected one of "
                             f"{', '.join(FOLIO_PROPERTIES)}")
        if name in ("add_autonum", "remove_autonum") and op.get("kind") not in AUTONUM_KINDS:
            raise ValueError(f"operation {i}: unknown kind {op.get('kind')!r}; "
                             f"expected one of {', '.join(AUTONUM_KINDS)}")
        if name == "search_and_replace":
            if op.get("kind") not in SEARCH_REPLACE_KINDS:
                raise ValueError(f"operation {i}: unknown kind {op.get('kind')!r}; "
                                 f"expected one of {', '.join(SEARCH_REPLACE_KINDS)}")
            if op.get("kind") == "conductor" and op.get("field") not in CONDUCTOR_PROPERTIES:
                raise ValueError(f"operation {i}: unknown conductor field "
                                 f"{op.get('field')!r}; expected one of "
                                 f"{', '.join(CONDUCTOR_PROPERTIES)}")
            if op.get("kind") == "element_info" and not op.get("field"):
                raise ValueError(f"operation {i}: element_info needs a non-empty "
                                 f"\"field\" (information key)")
        if name == "set_folio_border" and op.get("property") not in FOLIO_BORDER_PROPERTIES:
            raise ValueError(f"operation {i}: unknown folio border property "
                             f"{op.get('property')!r}; expected one of "
                             f"{', '.join(FOLIO_BORDER_PROPERTIES)}")
        if name == "add_element_text" and op.get("source") not in ELEMENT_TEXT_SOURCES:
            raise ValueError(f"operation {i}: unknown source {op.get('source')!r}; "
                             f"expected one of {', '.join(ELEMENT_TEXT_SOURCES)}")
        if name == "set_element_text" and op.get("property") not in ELEMENT_TEXT_PROPERTIES:
            raise ValueError(f"operation {i}: unknown element-text property "
                             f"{op.get('property')!r}; expected one of "
                             f"{', '.join(ELEMENT_TEXT_PROPERTIES)}")
        if name == "set_shape" and op.get("property") not in SHAPE_PROPERTIES:
            raise ValueError(f"operation {i}: unknown shape property "
                             f"{op.get('property')!r}; expected one of "
                             f"{', '.join(SHAPE_PROPERTIES)}")
        if name == "add_shape" and op.get("shape") not in SHAPES:
            raise ValueError(f"operation {i}: unknown shape {op.get('shape')!r}; "
                             f"expected one of {', '.join(SHAPES)}")
        # The conductor ops take "conductor": "{uuid}" in place of element +
        # terminal: a uuid names one conductor for good, where a terminal
        # can carry several.
        conductor_js = None
        if name in CONDUCTOR_UUID_OPS and "conductor" in op:
            if "element" in op or "terminal" in op:
                raise ValueError(f"operation {i} ({name}): give either \"conductor\" "
                                 f"(its uuid) or \"element\" + \"terminal\", not both")
            if op["conductor"] == "":
                raise ValueError(f"operation {i}: \"conductor\" is empty -- a conductor "
                                 f"from a project saved before conductors carried a uuid "
                                 f"has none in the file; name it by \"element\" + "
                                 f"\"terminal\" instead")
            if not isinstance(op["conductor"], str) or not _UUID_RE.fullmatch(op["conductor"]):
                raise ValueError(f"operation {i}: \"conductor\" must be a conductor uuid, "
                                 f"got {op['conductor']!r}")
            conductor_js = _js(op["conductor"])
            uuid_methods.add("conductorEnds")
            op = {**op, "element": "{00000000-0000-0000-0000-000000000000}", "terminal": 0}
        args = []
        for key, kind in spec:
            if key not in op:
                raise ValueError(f"operation {i} ({name}) is missing {key!r}")
            folio_js = args[0] if args else "0"
            element_js = args[1] if len(args) > 1 else None
            owner_js = args[-1] if args else None     # a terminal's element precedes it
            args.append(ref_or(op[key], kind, i, key))
        if conductor_js is not None:
            args[1], args[2] = f"e{i}.element", f"e{i}.terminal"

        ident = op.get("id")
        if ident is not None:
            if not isinstance(ident, str) or not ident or ident.startswith("$"):
                raise ValueError(f"operation {i}: \"id\" must be a non-empty name "
                                 f"without a leading $, got {ident!r}")
            if ident in refs:
                raise ValueError(f"operation {i}: \"id\" {ident!r} is already used")

        call = "qet.addFolio()" if method is None else f"qet.{method}({', '.join(args)})"
        lines.append("  if (!stop) {")
        if conductor_js is not None:
            lines.append(f"  var e{i} = qetMcpConductorEnd({i}, {args[0]}, {conductor_js});")
            call = f"(e{i} ? {call} : false)"
        lines.append(f"  var v{i} = {call};")
        if ident is not None:
            lines.append(f"  R[{_js(ident)}] = v{i};")
            refs.add(ident)
            if name in FOLIO_MAKING_OPS:
                lines.append(f"  F[{_js(ident)}] = (typeof qet.folioUuid === 'function' "
                             f"&& v{i} >= 0) ? qet.folioUuid(v{i}) : '';")
                folio_refs.add(ident)
        lines.append(
            f"  qet.log({_js(_MARKER)} + JSON.stringify("
            f"{{kind: 'op', index: {i}, op: {_js(name)}, "
            f"id: {_js(ident)}, result: v{i}}}));")
        # An op that failed usually invalidates the ones after it -- a
        # conductor to an element that was never placed is not a second,
        # independent finding, it is noise on top of the first one. The
        # three falsey returns are the three the API uses: false for a
        # refused edit, "" for an addElement that placed nothing, -1 for an
        # addFolio that added none.
        # An empty list is a failure too (duplicateElements returns [] when it
        # refuses). Duck-typed on .length, not Array.isArray: QJSEngine hands
        # an empty QStringList back as an array-like wrapper for which
        # Array.isArray is false, so that test never fired and the run went
        # on past the failed operation.
        lines.append(f"  if (v{i} === false || v{i} === '' || v{i} === -1 || "
                     f"(typeof v{i} === 'object' && v{i} !== null && "
                     f"(v{i}.length === 0 || (v{i}.length === undefined && "
                     f"Object.keys(v{i}).length === 0)))) "
                     "stop = true;")
        lines.append("  }")
    lines.append("  // ---- end of operations ----")

    # Save even after a failed op: a partial result that can be inspected
    # beats no result at all, and the diff is what says how far it got.
    lines.append(f"  var saved = qet.save({_js(output)});")
    lines.append(f"  qet.log({_js(_MARKER)} + JSON.stringify("
                 "{kind: 'save', result: saved, stopped_early: stop}));")
    lines.append("}")
    need = _js(sorted(set(_REQUIRED_METHODS) | uuid_methods))
    return "\n".join(line.replace("@NEED@", need) for line in lines) + "\n"


def _parse_script_output(text: str) -> dict:
    """Read the marker lines the generated script emits.

    They arrive on stderr, not stdout: QetScriptApi::log() is a
    QTextStream(stderr). Read both anyway rather than depending on that --
    the cost is nothing and the failure it prevents is silent (an edit that
    worked, reported as having run no operations at all, which is what the
    first version of this tool did)."""
    caps, ops, saved, stopped, notes = None, [], None, False, {}
    for line in text.splitlines():
        idx = line.find(_MARKER)
        if idx < 0:
            continue
        try:
            rec = json.loads(line[idx + len(_MARKER):])
        except json.JSONDecodeError:
            continue
        if rec.get("kind") == "capabilities":
            caps = rec.get("missing") or []
        elif rec.get("kind") == "op":
            rec.pop("kind", None)
            # Not "in (False, ...)": 0 == False in Python, and 0 is a valid
            # index/folio result. The script side already uses strict ===.
            r = rec.get("result")
            rec["succeeded"] = not (r is None or r is False or r == "" or r == [] or r == {} or
                                    (isinstance(r, int) and not isinstance(r, bool) and r == -1))
            ops.append(rec)
        elif rec.get("kind") == "op_note":
            # An op can log more than one (add_conductor, one per end):
            # keep them all rather than only the last.
            idx = rec.get("index")
            notes[idx] = (notes[idx] + "; " if idx in notes else "") + str(rec.get("note"))
        elif rec.get("kind") == "save":
            saved = bool(rec.get("result"))
            stopped = bool(rec.get("stopped_early"))
    for rec in ops:
        if rec.get("index") in notes:
            rec["note"] = notes[rec["index"]]
    return {"missing_methods": caps, "operations": ops, "saved": saved,
            "stopped_early": stopped}


def tool_query(binary: str, project: str, sql: str,
               elements_dir: str | None = None, timeout: int = 180) -> dict:
    """Run a read-only SELECT against the project's SQLite database.

    This is the surface the rest of this server has done without. Every
    other structural tool here re-derives its answer from the XML, because
    the database was unreachable from outside the application; it is
    reachable now, through the same guarded path QElectroTech's own
    "Requête SQL personnalisée" box uses, so a structural question can be
    asked of the database that already knows it.

    The three *_view names are the surface to depend on --
    element_nomenclature_view, project_summary_view, wiring_list_view.
    They exist to be queried. The underlying tables are how the cache is
    arranged today and a column may move; qet_query with sql omitted
    lists both.
    """
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")

    if sql:
        # Same first-word rule projectDataBase::isReadOnlySelect() applies,
        # checked here too so an obvious write is refused without paying
        # for a process launch. QET still enforces it; this is not the
        # guard, only an early one.
        head = sql.strip().lstrip("(").split(None, 1)[0].upper() if sql.strip() else ""
        if head not in ("SELECT", "WITH"):
            raise ValueError("only read-only queries are allowed: a statement must "
                             f"begin with SELECT or WITH, not {head or '(nothing)'}")

    script = ("var out = %s ? qet.query(%s) : qet.tables();\n"
              "qet.log(%s + JSON.stringify({kind: 'query', rows: out, "
              "error: qet.queryError ? qet.queryError() : ''}));\n"
              % (json.dumps(bool(sql)), json.dumps(sql or ""), json.dumps(_MARKER)))

    result = _run_qet(binary, [str(proj)], timeout=timeout,
                      elements_dir=elements_dir, script=script, tail=400_000)
    streams = result.get("stdout", "") + "\n" + result.get("stderr", "")
    rows, error = None, ""
    for line in streams.splitlines():
        idx = line.find(_MARKER)
        if idx < 0:
            continue
        try:
            rec = json.loads(line[idx + len(_MARKER):])
        except json.JSONDecodeError:
            continue
        if rec.get("kind") == "query":
            rows, error = rec.get("rows"), rec.get("error") or ""
    for key in ("stdout", "stderr"):
        kept = [ln for ln in result.get(key, "").splitlines() if _MARKER not in ln]
        result[key] = "\n".join(kept)[-4000:]

    if rows is None:
        result["ok"] = False
        result.setdefault("hint", "the query returned nothing at all -- this build's "
                                  "scripting API may predate qet.query()")
        return result
    if error:
        result["ok"] = False
        result["error"] = error
    result["rows"] = rows
    result["row_count"] = len(rows)
    result["listing"] = not sql
    return result


# --------------------------------------------------------------------------
# qet_continuity: electrical continuity / ERC-style structural checks
# --------------------------------------------------------------------------

def tool_continuity(binary: str, project: str, folio: int | None = None,
                    elements_dir: str | None = None, timeout: int = 180) -> dict:
    """Run qet.checkContinuity() and get its findings back.

    Three checks, against the live Terminal/Conductor object graph rather
    than a heuristic read of the XML (that is qet_check's job, and the two
    are complementary, not redundant -- qet_check looks at labels and
    numbering conventions, this looks at the electrical graph itself):
    unconnected_terminal (info -- routine, not necessarily a mistake),
    potential_mismatch (error -- two conductors QElectroTech's own
    setConductorProperty() would always keep identical, found disagreeing,
    which only happens from hand-edited XML, a legacy file, or an external
    tool), and report_link_mismatch (warning -- a next_report/
    previous_report folio-jump pair whose conductors disagree on colour,
    style, num, etc.; unlike potential_mismatch this one CAN happen through
    ordinary use, since LinkElementCommand::isLinkable() never checks
    conductor properties, only type and freedom -- see
    qelectrotech/qelectrotech-source-mirror#974, which this check
    reproduces exactly: one folio-link conductor drawn in two different
    colours on either side of the link). See qet.checkContinuity()'s own
    doc comment (qetscriptapi.cpp) for what this deliberately does not
    check: pin electrical direction/power conflicts and No/Nc/Common
    contact shorts, since QElectroTech's terminal data model does not
    carry the information either would need.
    """
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")
    if folio is not None:
        # qet.checkContinuity() answers an index it has no folio for with an
        # empty list, which reads exactly like a clean folio. The index counts
        # from 0 while qet_elements numbers folios from 1, so the likely
        # mistake -- passing the last folio's number -- would pass silently.
        count = len(list(_folios(_root(str(proj)))))
        if not 0 <= folio < count:
            raise ValueError(
                f"folio {folio} does not exist: the project has {count} folio(s), "
                f"indexed 0 to {count - 1} here. qet_continuity counts folios from 0; "
                "the folio qet_elements calls N is N - 1.")

    folio_arg = -1 if folio is None else folio
    script = ("var out = qet.checkContinuity(%s);\n"
              "qet.log(%s + JSON.stringify({kind: 'continuity', findings: out}));\n"
              % (json.dumps(folio_arg), json.dumps(_MARKER)))

    result = _run_qet(binary, [str(proj)], timeout=timeout,
                      elements_dir=elements_dir, script=script, tail=400_000)
    streams = result.get("stdout", "") + "\n" + result.get("stderr", "")
    findings = None
    for line in streams.splitlines():
        idx = line.find(_MARKER)
        if idx < 0:
            continue
        try:
            rec = json.loads(line[idx + len(_MARKER):])
        except json.JSONDecodeError:
            continue
        if rec.get("kind") == "continuity":
            findings = rec.get("findings")
    for key in ("stdout", "stderr"):
        kept = [ln for ln in result.get(key, "").splitlines() if _MARKER not in ln]
        result[key] = "\n".join(kept)[-4000:]

    if findings is None:
        result["ok"] = False
        result.setdefault("hint", "no findings came back at all -- this build's "
                                  "scripting API may predate qet.checkContinuity()")
        return result
    for f in findings:
        # "folio" is the 0-based index qet.checkContinuity() uses; add the
        # number qet_elements and the application show, so the two can be
        # matched without arithmetic.
        if isinstance(f, dict) and isinstance(f.get("folio"), int):
            f["folio_number"] = f["folio"] + 1
    result["findings"] = findings
    result["finding_count"] = len(findings)
    result["errors"] = sum(1 for f in findings if f.get("severity") == "error")
    result["warnings"] = sum(1 for f in findings if f.get("severity") == "warning")
    result["info"] = sum(1 for f in findings if f.get("severity") == "info")
    return result


# --------------------------------------------------------------------------
# qet_element_search: find a symbol in the collection
# --------------------------------------------------------------------------

_ELEMENT_INDEX: dict = {}


def _fold(text: str) -> str:
    """Case- and accent-insensitive form, so 'resistance' finds 'Résistance'."""
    import unicodedata
    return "".join(c for c in unicodedata.normalize("NFKD", text.lower())
                   if not unicodedata.combining(c))


def _collection_signature(root: Path):
    """Cheap change detector: file count and newest mtime, no parsing."""
    count, newest = 0, 0.0
    for f in root.rglob("*.elmt"):
        count += 1
        try:
            newest = max(newest, f.stat().st_mtime)
        except OSError:
            pass
    return count, newest


def _index_collection(root: Path) -> list:
    """Parse every .elmt under root once and keep what a search needs.

    Cached for the life of the process and rebuilt when the file count or
    the newest modification time changes -- which is what makes a symbol
    written by qet_element_build findable straight away, without the caller
    knowing there is an index at all.
    """
    key = str(root.resolve())
    sig = _collection_signature(root)
    cached = _ELEMENT_INDEX.get(key)
    if cached and cached["sig"] == sig:
        return cached["items"]

    items = []
    for f in sorted(root.rglob("*.elmt")):
        try:
            d = ET.parse(f).getroot()
        except (ET.ParseError, OSError):
            continue
        if d.tag != "definition":
            continue
        names = {n.get("lang", ""): (n.text or "").strip() for n in d.iter("name")}
        kind = ""
        for ki in d.iter("kindInformation"):
            if ki.get("name") == "type":
                kind = (ki.text or "").strip()
        ordered, ambiguous = _terminals_in_index_order(list(d.iter("terminal")))
        terminals = [t.get("name") or "" for t in ordered]
        rel = f.relative_to(root).as_posix()
        items.append({
            "path": "common://" + rel,
            "file": str(f),
            "name": names.get("en") or names.get("fr") or next(iter(names.values()), ""),
            "names": names,
            "link_type": d.get("link_type", "simple"),
            "kind": kind,
            "terminals": len(terminals),
            "terminal_names": terminals,       # index order, not file order
            "terminal_order_ambiguous": ambiguous,
            "width": d.get("width"), "height": d.get("height"),
            "haystack": _fold(" ".join([*names.values(), rel, kind])),
        })
    _ELEMENT_INDEX[key] = {"sig": sig, "items": items}
    return items


def tool_element_search(directory: str, query: str = "", link_type: str | None = None,
                        min_terminals: int | None = None, max_terminals: int | None = None,
                        kind: str | None = None, limit: int = 25) -> dict:
    """Search an element collection by name, type and terminal count.

    Matches every word of query against all the translated names, the
    element's path and its kind, ignoring case and accents -- so a French
    or German search finds the same symbol an English one does. Results
    carry a common:// path that qet_edit's add_element takes directly, and
    the terminal names in the order add_conductor indexes them -- which is
    top to bottom then left to right, not the order the file lists them.
    """
    root = Path(directory).expanduser()
    if not root.is_dir():
        raise ValueError(f"no such directory: {root}")
    if link_type is not None and link_type not in LINK_TYPES:
        raise ValueError(f"unknown link_type {link_type!r}; expected one of "
                         f"{', '.join(LINK_TYPES)}")
    if limit < 1:
        raise ValueError("limit must be >= 1")

    words = _fold(query).split()
    matches = []
    for it in _index_collection(root):
        if link_type and it["link_type"] != link_type:
            continue
        if kind and _fold(kind) not in _fold(it["kind"]):
            continue
        if min_terminals is not None and it["terminals"] < min_terminals:
            continue
        if max_terminals is not None and it["terminals"] > max_terminals:
            continue
        if not all(w in it["haystack"] for w in words):
            continue
        matches.append(it)

    # Whole-name hits before substring hits, then shorter names first: a
    # search for "coil" should offer "Coil" before "Remanence coil, latching".
    def rank(it):
        name = _fold(it["name"])
        exact = 0 if (words and name == " ".join(words)) else 1
        starts = 0 if (words and name.startswith(words[0])) else 1
        return (exact, starts, len(it["name"]), it["path"])
    matches.sort(key=rank)

    shown = [{k: v for k, v in it.items() if k not in ("haystack", "names", "file")}
             | {"languages": sorted(it["names"])} for it in matches[:limit]]
    return {"query": query, "total_matches": len(matches), "returned": len(shown),
            "indexed": len(_ELEMENT_INDEX[str(root.resolve())]["items"]),
            "results": shown}


# Design-rule checks, each one a read-only query over the project database.
#
# Every check is a SELECT that returns the offending rows, so "no rows" is a
# pass and the same query is what a human would write by hand. The severity
# and the note say how much to trust a hit, because these are heuristics
# tuned against the 24 shipped examples, not standards:
#
# - Every text comparison is COALESCE'd. A value that was never set is NULL
#   in the database when the element was placed in this session and an empty
#   string when it was loaded from a file, and `col = ''` matches only the
#   second -- which made the first version of these checks silently pass on
#   exactly the freshly-edited projects qet_edit produces. The JavaScript
#   side renders both as "", so the difference is invisible until a check
#   fails to fire. Likewise exclude_from_bom is text, not a number.
# - An unnumbered conductor is '' in some files and '_' in others: '_' is the
#   placeholder QElectroTech assigns when no numbering is configured, so
#   testing for '' alone passed on industrial.qet's 36 placeholder conductors
#   and on every project qet_edit builds without a numbering context.
# - Slaves and terminals are excluded from the duplicate-label check on
#   purpose. A slave contact carries its master coil's label by design, and
#   terminals repeat their numbers from one strip to the next; counting
#   either would bury the real findings.
# - "simple" elements are checked for duplicates too but only as a warning:
#   industrial.qet reuses V1..V6 across folios on purpose.
CHECKS = {
    "duplicate_master_labels": {
        "severity": "error",
        "note": "Two master elements with the same label are ambiguous in every "
                "report that keys on it (BOM, cross-references, wiring list).",
        "sql": "SELECT label, COUNT(*) AS n FROM element_nomenclature_view "
               "WHERE COALESCE(label,'') <> '' AND element_type = 'master' "
               "GROUP BY label HAVING n > 1 ORDER BY n DESC, label",
    },
    "duplicate_simple_labels": {
        "severity": "warning",
        "note": "Legitimate when a label is reused on purpose across folios "
                "(industrial.qet does); worth a look otherwise.",
        "sql": "SELECT label, COUNT(*) AS n FROM element_nomenclature_view "
               "WHERE COALESCE(label,'') <> '' AND element_type = 'simple' "
               "GROUP BY label HAVING n > 1 ORDER BY n DESC, label",
    },
    "unlabelled_masters": {
        "severity": "warning",
        "note": "A master with no label cannot be told apart from its slaves' "
                "cross-references.",
        "sql": "SELECT folio, diagram_position, element_sub_type FROM "
               "element_nomenclature_view WHERE element_type = 'master' AND COALESCE(label,'') = '' "
               "ORDER BY folio, diagram_position",
    },
    "unnumbered_conductors": {
        "severity": "info",
        "note": "Conductors with no wire number -- empty, or QElectroTech's own "
                "'_' placeholder, which is what a conductor gets when no "
                "numbering is configured. If every conductor is unnumbered the "
                "project simply does not use wire numbering; a few among many "
                "numbered ones is the finding.",
        "sql": "SELECT COUNT(*) AS unnumbered, (SELECT COUNT(*) FROM wiring_list_view) AS total "
               "FROM wiring_list_view WHERE COALESCE(wire_number,'') IN ('', '_') "
               "HAVING unnumbered > 0",
    },
    "empty_folios": {
        "severity": "info",
        "note": "Folios with no element on them. Often cover pages, sometimes "
                "left behind by a deleted drawing.",
        "sql": "SELECT p.pos AS position, p.title AS title FROM diagram d "
               "JOIN project_summary_view p ON p.pos = d.pos "
               "WHERE NOT EXISTS (SELECT 1 FROM element e WHERE e.diagram_uuid = d.uuid) "
               "ORDER BY p.pos",
    },
    "masters_without_manufacturer_reference": {
        "severity": "info",
        "note": "Masters that will show a blank article number in the BOM.",
        "sql": "SELECT label, folio, diagram_position FROM element_nomenclature_view "
               "WHERE element_type = 'master' AND COALESCE(manufacturer_reference,'') = '' "
               "AND COALESCE(exclude_from_bom,'') IN ('', '0', 'false') "
               "ORDER BY folio, diagram_position",
    },
}


def tool_check(binary: str, project: str, checks: list | None = None,
               sample: int = 10, elements_dir: str | None = None,
               timeout: int = 180) -> dict:
    """Run design-rule checks over a project in one QElectroTech launch.

    Every check is a read-only SELECT over the project database, so this is
    qet_query with the questions already written down. It exists because the
    useful questions are always the same handful and re-deriving them per
    conversation is where the mistakes creep in -- the first draft of the
    duplicate-label check counted slave contacts, which share their coil's
    label by design and flagged nearly every relay.
    """
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")
    chosen = list(CHECKS) if not checks else list(checks)
    for name in chosen:
        if name not in CHECKS:
            raise ValueError(f"unknown check {name!r}; expected one of "
                             f"{', '.join(sorted(CHECKS))}")
    if sample < 0:
        raise ValueError("sample must be >= 0")

    queries = {name: CHECKS[name]["sql"] for name in chosen}
    script = ("var Q = %s;\nfor (var k in Q) {\n"
              "  var rows = qet.query(Q[k]);\n"
              "  qet.log(%s + JSON.stringify({kind: 'check', name: k, rows: rows, "
              "error: qet.queryError()}));\n}\n" % (json.dumps(queries), json.dumps(_MARKER)))
    result = _run_qet(binary, [str(proj)], timeout=timeout,
                      elements_dir=elements_dir, script=script, tail=2_000_000)
    streams = result.get("stdout", "") + "\n" + result.get("stderr", "")

    got = {}
    for line in streams.splitlines():
        idx = line.find(_MARKER)
        if idx < 0:
            continue
        try:
            rec = json.loads(line[idx + len(_MARKER):])
        except json.JSONDecodeError:
            continue
        if rec.get("kind") == "check":
            got[rec["name"]] = rec

    findings, errors, passed = [], [], []
    for name in chosen:
        rec = got.get(name)
        if rec is None:
            errors.append({"check": name, "error": "no result came back"})
            continue
        if rec.get("error"):
            errors.append({"check": name, "error": rec["error"]})
            continue
        rows = rec.get("rows") or []
        if not rows:
            passed.append(name)
            continue
        findings.append({"check": name, "severity": CHECKS[name]["severity"],
                         "count": len(rows), "note": CHECKS[name]["note"],
                         "rows": rows[:sample]})

    order = {"error": 0, "warning": 1, "info": 2}
    findings.sort(key=lambda f: (order[f["severity"]], f["check"]))
    answer = {"ok": not errors and not any(f["severity"] == "error" for f in findings),
              "summary": {"errors": sum(f["severity"] == "error" for f in findings),
                          "warnings": sum(f["severity"] == "warning" for f in findings),
                          "info": sum(f["severity"] == "info" for f in findings),
                          "passed": len(passed), "check_failures": len(errors)},
              "findings": findings, "passed": passed, "check_failures": errors}
    # This answer is built fresh rather than layered onto the launch result,
    # so a reason the launch failed at all has to be carried across
    # explicitly. Without it every check reads "no result came back", which
    # is true and tells nobody why.
    if result.get("hint"):
        answer["ok"] = False
        answer["hint"] = result["hint"]
        answer["exit_code"] = result.get("exit_code")
    return answer


def tool_project_new(binary: str, output: str, title: str = "Untitled",
                     folios=1, author: str = "", overwrite: bool = False,
                     elements_dir: str | None = None, timeout: int = 180) -> dict:
    """Create a new, empty project so qet_edit has something to start from.

    Every other edit tool needs an existing .qet, which made building a
    schematic from nothing impossible. The obvious candidate,
    examples/Projet_vierge.qet, is not blank: it is a 600 KB real project
    with 23 elements and 15 conductors.

    So this writes the smallest project QElectroTech will open -- one
    element with a title, no folios -- and then has QElectroTech itself
    add the folios and save. What is left on disk is QElectroTech's own
    canonical output, not the hand-written skeleton, which is why this is
    not the "write .qet XML directly" route that was rejected: the
    skeleton never reaches the result, and the result is checked by
    reading it back.

    folios is a count, or a list of folio titles.
    """
    out = Path(output).expanduser()
    if out.exists() and not overwrite:
        raise ValueError(f"{out} already exists; pass overwrite=true to replace it")
    if isinstance(folios, bool) or not isinstance(folios, (int, list)):
        raise ValueError("folios must be a count or a list of titles")
    titles = ([""] * folios) if isinstance(folios, int) else [str(t) for t in folios]
    if not 0 <= len(titles) <= 200:
        raise ValueError("folios must be between 0 and 200")
    if not isinstance(title, str) or not title.strip():
        raise ValueError("title must be a non-empty string")

    from xml.sax.saxutils import quoteattr
    script = ["var t = %s;" % json.dumps(titles), "var made = [];",
              "for (var i = 0; i < t.length; i++) {",
              "  var f = qet.addFolio(); made.push(f);",
              "  if (f >= 0 && t[i]) qet.setFolioTitle(f, t[i]);",
              "  if (f >= 0 && %s) qet.setFolioProperty(f, 'author', %s);" %
              (json.dumps(bool(author)), json.dumps(author)),
              "}",
              "var saved = qet.save(%s);" % json.dumps(str(out)),
              "qet.log(%s + JSON.stringify({kind: 'new', folios: made, saved: saved}));"
              % json.dumps(_MARKER)]

    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="qet-mcp-new-") as tmp:
        skeleton = Path(tmp) / "skeleton.qet"
        skeleton.write_text('<project version="0.100.0" title=%s>\n</project>\n'
                            % quoteattr(title), encoding="utf-8")
        result = _run_qet(binary, [str(skeleton)], timeout=timeout,
                          elements_dir=elements_dir, script="\n".join(script), tail=200_000)

    rec = None
    for line in (result.get("stdout", "") + "\n" + result.get("stderr", "")).splitlines():
        idx = line.find(_MARKER)
        if idx >= 0:
            try:
                r = json.loads(line[idx + len(_MARKER):])
            except json.JSONDecodeError:
                continue
            if r.get("kind") == "new":
                rec = r
    for key in ("stdout", "stderr"):
        result[key] = "\n".join(l for l in result.get(key, "").splitlines()
                                if _MARKER not in l)[-2000:]

    if rec is None or not rec.get("saved") or not out.is_file():
        result["ok"] = False
            #setdefault: _run_qet() may already have said something more
            #specific than this guess -- notably that scripting is switched
            #off, in which case "your build is too old" sends the reader
            #looking for the wrong thing entirely.
        result.setdefault("hint",
                          "QElectroTech did not write the project; this build's scripting "
                          "API may predate addFolio()/save()")
        return result
    if any(f < 0 for f in rec["folios"]):
        result["ok"] = False
        result["hint"] = "a folio could not be added"
        return result

    # Read back what is actually on disk rather than report what was asked for.
    info = tool_project_info(str(out))
    if info["title"] != title or info["folio_count"] != len(titles):
        result["ok"] = False
        result["hint"] = (f"the file on disk has title {info['title']!r} and "
                          f"{info['folio_count']} folio(s), not what was requested")
    result["output"] = str(out)
    result["project"] = info
    return result


def _wrong_folio(project: str, op: dict) -> str:
    """Explain a failed op that named an element on the wrong folio.

    qet_elements and qet_project_info number folios from 1, as the UI does;
    qet_edit passes "folio" straight to the scripting API, which counts from
    0. Passing the number qet_elements showed therefore addresses the next
    folio, and the op fails with nothing but "false". When the element the
    op names is in the project on some other folio, say which index to use.
    """
    folio = op.get("folio")
    spec = OPS.get(op.get("op"), (None, []))[1]
    uuids = [op[key] for key, kind in spec
             if kind == "elmt" and isinstance(op.get(key), str)
             and not op[key].startswith("$")]
    if not isinstance(folio, int) or not uuids:
        return ""
    try:
        where = {el.get("uuid"): i for i, el in _elements(_root(project))}
    except (OSError, ET.ParseError):
        return ""
    for uuid in uuids:
        number = where.get(uuid)
        if number is not None and number - 1 != folio:
            return (f"Element {uuid} is on folio {number} as qet_elements numbers "
                    f"it, which is \"folio\": {number - 1} here: qet_edit counts "
                    "folios from 0.")
    return ""


def tool_edit(binary: str, project: str, operations: list, output: str,
              elements_dir: str | None = None, timeout: int = 180) -> dict:
    """Apply edits through the scripting API and report what actually changed.

    The point is the last part. The scripting API returns a bool per call,
    which says the call was accepted, not that the file came out the way
    anyone intended -- so this runs qet_diff between the input project and
    the saved result and puts that in the answer. A caller that trusts
    "addConductor -> true" and stops there is back to trusting the
    screenshot.

    The project is never written in place: output is a separate file, and
    the original is what the diff is taken against.
    """
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")
    if not isinstance(operations, list) or not operations:
        raise ValueError("operations must be a non-empty list")
    out = Path(output).expanduser()
    if out.resolve() == proj.resolve():
        raise ValueError("output must differ from project; this tool does not "
                         "edit a project in place")

    script = _build_script(operations, str(out))
    # QET interrupts a script at 30 s (kScriptTimeoutMs in qetscripting.cpp),
    # independently of this timeout. Leaving room above it means a script
    # that hits the engine's limit comes back as a script error we can
    # report, rather than as our own opaque process timeout.
    result = _run_qet(binary, [str(proj)], timeout=timeout,
                      elements_dir=elements_dir, script=script, tail=200_000)
    streams = result.get("stdout", "") + "\n" + result.get("stderr", "")
    result.update(_parse_script_output(streams))
    # The marker lines have been parsed into "operations"; leaving them in
    # the reported streams as well just doubles the size of the answer.
    for key in ("stdout", "stderr"):
        kept = [ln for ln in result.get(key, "").splitlines() if _MARKER not in ln]
        result[key] = "\n".join(kept)[-4000:]
    result["output"] = str(out)
    result["output_exists"] = out.exists()

    if result.get("missing_methods") is None and not result.get("timed_out"):
        # The script's first act is to report which methods exist. No report
        # means the script never ran -- a binary with no --run support, one
        # that exited early, or the wrong executable -- and exit code 0 from
        # something that did nothing is not success.
        result["ok"] = False
            #setdefault, for the same reason as in tool_project_new(): a
            #refusal to run scripts at all also produces no capability
            #report, and "is it a build with --run support?" is then the
            #wrong question.
        result.setdefault("hint",
                          "the binary never ran the script (no capability report came "
                          "back), so nothing was changed. Is it a QElectroTech build with "
                          "--run support?")
        result["script"] = script
        return result

    missing = result.get("missing_methods")
    if missing:
        result["ok"] = False
        result["hint"] = (
            "this build's scripting API lacks " + ", ".join(missing) +
            " -- it predates the drawing verbs, so nothing was changed")
        result["script"] = script
        return result

    for record in result.get("operations", []):
        if not record["succeeded"]:
            result["ok"] = False
            hint = (f"operation {record['index']} ({record['op']}) returned "
                    f"{record['result']!r}; later operations were skipped. "
                    "qet.log lines in stderr/stdout say why.")
            if 0 <= record["index"] < len(operations):
                wrong = _wrong_folio(str(proj), operations[record["index"]])
                if wrong:
                    hint += " " + wrong
            result.setdefault("hint", hint)
            break

    if result.get("saved") is False:
        result["ok"] = False
        result.setdefault("hint", "the edits were made but save() failed")

    if out.is_file():
        result["output_bytes"] = out.stat().st_size
        try:
            result["diff"] = tool_diff(str(proj), str(out))
        except ET.ParseError as exc:          # a truncated or unwritten save
            result["ok"] = False
            result["diff_error"] = str(exc)
    if not result.get("ok"):
        result["script"] = script
    return result


# --------------------------------------------------------------------------
# Stored scripts: the buttons in Projet > Scripts and on the Scripts toolbar
# --------------------------------------------------------------------------
#
# QElectroTech turns every .js file in one folder into a command with an
# icon, read from a // ==QETScript== header at the top of the file. The
# folder is the whole contract: a person writing a script by hand and an
# assistant using these tools both end with a file there, and QElectroTech
# notices it without a restart. So installing is writing a file, and these
# tools never talk to a running QElectroTech.
#
# The folder is not the client's to choose -- scripts_dir() finds it the
# way QElectroTech does -- and writing to it needs the same consent as
# editing a project: QET_ENABLE_SCRIPTING=1 in this server's environment.
# A stored script runs, with the user's rights, when they click its button.

_SCRIPT_CONTEXTS = ("canvas", "selection", "conductor")
_SCRIPT_ID = re.compile(r"^[a-z0-9][a-z0-9_-]{0,63}$")
_SCRIPT_MAX_BYTES = 256 * 1024
_ICON_MAX_BYTES = 64 * 1024

SCRIPT_HEADER_HELP = (
    "A stored script is one .js file; its first lines say how its button "
    "looks:\n"
    "// ==QETScript==\n"
    "// @name     Add revision note          (required)\n"
    "// @icon     <id>.svg                   (optional: a file next to the "
    "script, or builtin:<icon theme name>; a tile with the initials otherwise)\n"
    "// @tooltip  Puts a note on the folio on screen\n"
    "// @shortcut Ctrl+Alt+R                 (optional default shortcut)\n"
    "// @context  canvas                     (canvas: always enabled; "
    "selection: something selected; conductor: a conductor selected)\n"
    "// @api      1\n"
    "// ==/QETScript==\n"
    "The script sees one global, qet. qet.currentFolio() is the folio on "
    "screen; folio indexes count from 0. A click is one undo step, so do "
    "not call qet.undo() in a stored script. Report with qet.log(); "
    "qet.showMessage() opens a dialog the user has to close.")


def default_scripts_dir(os_name: str, platform: str, env, home) -> PurePath:
    """QETApp::dataDir() + "/scripts" for a given platform.

    dataDir() is Qt's AppDataLocation with organisation and application
    both "QElectroTech" (main.cpp). Pure, so the Windows and macOS answers
    are tested on any machine.
    """
    if os_name == "nt":
        appdata = env.get("APPDATA")
        base = (PureWindowsPath(appdata) if appdata
                else PureWindowsPath(home, "AppData", "Roaming"))
    elif platform == "darwin":
        base = PurePosixPath(home, "Library", "Application Support")
    else:
        base = PurePosixPath(env.get("XDG_DATA_HOME") or PurePosixPath(home, ".local", "share"))
    return base / "QElectroTech" / "QElectroTech" / "scripts"


def assistant_info_file() -> Path:
    """Where QElectroTech writes qet-assistant.json.

    QET_MCP_INFO_FILE if set; next to a QET_MCP_SCRIPTS_DIR if that is set
    (one folder up, as in QElectroTech's own layout); else the platform's
    standard data folder, where QElectroTech always writes it, even when
    --data-dir moves the rest.
    """
    explicit = os.environ.get("QET_MCP_INFO_FILE", "").strip()
    if explicit:
        return Path(explicit).expanduser()
    scripts = os.environ.get("QET_MCP_SCRIPTS_DIR", "").strip()
    if scripts:
        return Path(scripts).expanduser().parent / "qet-assistant.json"
    default = default_scripts_dir(os.name, sys.platform, os.environ, str(Path.home()))
    return Path(str(default.parent)) / "qet-assistant.json"


def assistant_info() -> dict | None:
    """qet-assistant.json as QElectroTech last wrote it, or None.

    QElectroTech rewrites it whenever an editor opens and whenever its
    stored scripts, settings or live channel change: its folders, features,
    the calls a script can make, the stored scripts, and the live channel
    while one is open.
    """
    path = assistant_info_file()
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def scripts_dir() -> Path:
    """The folder QElectroTech reads stored scripts from.

    QET_MCP_SCRIPTS_DIR first (a test, or a deliberate choice); then the
    folder qet-assistant.json names; else the platform's default.
    """
    env = os.environ.get("QET_MCP_SCRIPTS_DIR", "").strip()
    if env:
        return Path(env).expanduser()
    info = assistant_info() or {}
    named = (info.get("folders") or {}).get("scripts")
    if isinstance(named, str) and named:
        return Path(named)
    return Path(str(default_scripts_dir(os.name, sys.platform, os.environ, str(Path.home()))))


SERVER_INSTRUCTIONS = (
    "This server works with QElectroTech, a free editor for electrical "
    "diagrams. A project (.qet) holds folios (sheets) of symbols "
    "(elements) joined by wires (conductors). Folio indexes count from 0.\n"
    "Start with qet_about: where QElectroTech keeps things, what is "
    "switched on, and the stored scripts.\n"
    "Two ways of working. HEADLESS (qet_* and qet_script_*): read, check "
    "and edit .qet files and store script buttons; nothing the user has "
    "open is touched. To make a button: qet_script_api for the calls, "
    "qet_script_test on a copy until the diff is right, then "
    "qet_script_install with test_project set. LIVE (qet_live_*): act on "
    "the project open in the user's QElectroTech while they watch; only "
    "when they switched live mode on and accepted its warning at this "
    "start, each action one undo step, scripts written on the spot shown "
    "to them first.\n"
    "Verify edits by reading the result (qet_diff, qet_elements), not by "
    "assuming them.")


def tool_about() -> dict:
    """What this server and the QElectroTech it works with look like now."""
    info = assistant_info()
    binary = resolve_binary()
    out = {
        "server": {
            "version": SERVER_VERSION,
            "qelectrotech_binary": str(binary) if binary else None,
            "workspace": [str(r) for r in workspace_roots()] or "any path (QET_MCP_ALLOW_ANY_PATH=1)",
            "scripting_allowed_here": os.environ.get("QET_ENABLE_SCRIPTING") == "1",
        },
        "info_file": str(assistant_info_file()),
        "scripts_folder": str(scripts_dir()),
    }
    if info is None:
        out["found"] = False
        out["note"] = ("QElectroTech writes this file when an editor window opens; "
                       "start it once (a version with script buttons) to fill it in. "
                       "Until then folders are this server's own guess.")
        return out
    live = info.get("live")
    out.update({
        "found": True,
        "qelectrotech": {k: info.get(k) for k in
                         ("qelectrotech_version", "program", "running", "written")},
        "folders": info.get("folders"),
        "features": info.get("features"),
        "stored_scripts": info.get("stored_scripts"),
        "refused_scripts": info.get("refused_scripts"),
        "script_api": info.get("script_api"),
        # Never the token: it is for the live tools, not the conversation.
        "live": {"open": bool(live), "pid": (live or {}).get("pid")},
    })
    return out


def parse_script_header(text: str, script_id: str) -> dict:
    """The same rules as QElectroTech's ScriptHeader::parse().

    Returns the header's fields, with "error" set when QElectroTech would
    refuse it (and so show no button for it).
    """
    h = {"id": script_id, "name": "", "icon": "", "tooltip": "", "shortcut": "",
         "context": "canvas", "api": 1,
         "action_id": "diagrameditor.script." + script_id}
    m = re.search(r"//\s*==QETScript==\s*\n(.*?)//\s*==/QETScript==", text, re.S)
    if not m:
        h["error"] = "no // ==QETScript== header"
        return h
    for line in m.group(1).split("\n"):
        lm = re.match(r"^\s*//\s*@(\w+)\s+(.*?)\s*$", line)
        if not lm:
            continue
        key, value = lm.groups()
        if key == "api":
            try:
                h["api"] = int(value)
            except ValueError:
                h["api"] = 0
        elif key in ("name", "icon", "tooltip", "shortcut", "context"):
            h[key] = value
        else:
            h["error"] = f"unknown header key @{key}"
            return h
    if not h["name"]:
        h["error"] = "@name is required"
    elif h["context"] not in _SCRIPT_CONTEXTS:
        h["error"] = "@context must be one of: " + ", ".join(_SCRIPT_CONTEXTS)
    elif h["api"] != 1:
        h["error"] = f"@api {h['api']} is not supported by this version (1 is)"
    return h


def _script_id(script_id) -> str:
    if not isinstance(script_id, str) or not _SCRIPT_ID.match(script_id):
        raise ValueError("'id' must be 1-64 characters of a-z, 0-9, '-' and '_', "
                         "starting with a letter or digit: it is the file name")
    return script_id


def _require_script_consent() -> None:
    if os.environ.get("QET_ENABLE_SCRIPTING") != "1":
        raise ValueError(
            "storing a script needs the same consent as editing a project: "
            "QET_ENABLE_SCRIPTING=1 in the environment this server is started "
            "in. A stored script runs with the user's rights when they click it.")


def _check_icon_svg(svg: str) -> None:
    if not isinstance(svg, str) or len(svg.encode("utf-8")) > _ICON_MAX_BYTES:
        raise ValueError(f"'icon_svg' must be SVG text under {_ICON_MAX_BYTES // 1024} KB")
    try:
        root = ET.fromstring(svg)
    except ET.ParseError as exc:
        raise ValueError(f"'icon_svg' is not well-formed XML: {exc}") from None
    if root.tag.rsplit("}", 1)[-1] != "svg":
        raise ValueError("'icon_svg' must have <svg> as its root element")


def _write_atomic(path: Path, data: str) -> None:
    """Write, then rename into place, so the watcher never reads half a file."""
    tmp = path.with_name("." + path.name + ".tmp")
    tmp.write_text(data, encoding="utf-8")
    os.replace(tmp, path)


def tool_script_api(binary: str, timeout: int = 120) -> dict:
    """The calls a script can make, asked of the QElectroTech that will run it."""
    script = ("var sigs = typeof qet.apiSignatures === 'function' ? qet.apiSignatures()"
              " : null;\n"
              "var names = []; for (var k in qet) if (typeof qet[k] === 'function') "
              "names.push(k);\n"
              "qet.log(%s + JSON.stringify({kind: 'api', signatures: sigs, names: names}));\n"
              % json.dumps(_MARKER))
    with tempfile.TemporaryDirectory(prefix="qet-mcp-api-") as tmp:
        proj = Path(tmp) / "api.qet"
        proj.write_text('<project version="0.100.0" title="api">\n</project>\n',
                        encoding="utf-8")
        result = _run_qet(binary, [str(proj)], timeout=timeout, script=script,
                          tail=400_000)
    rec = None
    for line in (result.get("stdout", "") + "\n" + result.get("stderr", "")).splitlines():
        idx = line.find(_MARKER)
        if idx >= 0:
            try:
                rec = json.loads(line[idx + len(_MARKER):])
            except json.JSONDecodeError:
                pass
    if rec is None:
        result["ok"] = False
        result["stdout"] = result.get("stdout", "")[-4000:]
        result["stderr"] = result.get("stderr", "")[-4000:]
        return result
    out = {"ok": True, "header_format": SCRIPT_HEADER_HELP}
    if rec.get("signatures"):
        out["calls"] = rec["signatures"]
    else:
        out["calls"] = sorted(n for n in rec.get("names", [])
                              if not n.endswith("Changed") and n != "deleteLater")
        out["note"] = ("this QElectroTech predates qet.apiSignatures(): names only, "
                       "see the JavaScript Scripting wiki page for parameters")
    out["call_count"] = len(out["calls"])
    return out


def tool_script_test(binary: str, project: str, source: str,
                     elements_dir: str | None = None, timeout: int = 180) -> dict:
    """Run a script on a copy of a project and say what it would change.

    What clicking its button would do, without touching the project: the
    script runs headless on a copy, the copy is saved, and qet_diff compares
    it with the original. Headless there is no folio on screen, so
    qet.currentFolio() is the first folio.
    """
    proj = Path(project).expanduser()
    if not proj.is_file():
        raise ValueError(f"no such project: {proj}")
    if not isinstance(source, str) or not source.strip():
        raise ValueError("'source' must be the script's text")
    header = parse_script_header(source, "test")
    with tempfile.TemporaryDirectory(prefix="qet-mcp-script-") as tmp:
        copy = Path(tmp) / proj.name
        shutil.copy2(proj, copy)
        # Older builds have no currentFolio(); the first folio stands in, as
        # it does headless in builds that have it. On the script's own first
        # line, so the line numbers in its errors are its own.
        script = ("if (typeof qet.currentFolio !== 'function') "
                  "qet.currentFolio = function () { return qet.folioCount() ? 0 : -1; }; "
                  + source + "\n"
                  "qet.log(%s + JSON.stringify({kind: 'save', result: qet.save(%s)}));\n"
                  % (json.dumps(_MARKER), json.dumps(str(copy))))
        result = _run_qet(binary, [str(copy)], timeout=timeout,
                          elements_dir=elements_dir, script=script, tail=400_000)
        streams = result.get("stdout", "") + "\n" + result.get("stderr", "")
        saved = None
        for line in streams.splitlines():
            idx = line.find(_MARKER)
            if idx >= 0:
                try:
                    saved = json.loads(line[idx + len(_MARKER):]).get("result")
                except json.JSONDecodeError:
                    pass
        errors = [ln.strip() for ln in streams.splitlines() if "Script error:" in ln]
        log = [ln for ln in streams.splitlines()
               if ln.strip() and _MARKER not in ln and "Script error:" not in ln]
        out = {"ok": bool(result.get("ok")) and saved is True and not errors,
               "header": header, "errors": errors, "log": log[-60:]}
        if result.get("hint"):
            out["hint"] = result["hint"]
        if result.get("timed_out"):
            out["timed_out"] = True
        if saved is True:
            out["diff"] = tool_diff(str(proj), str(copy))
        elif not errors:
            out["errors"] = ["the script did not finish: it threw before the "
                             "copy could be saved, or never ran"]
        if header.get("error"):
            out["header_warning"] = (f"QElectroTech would show no button for this "
                                     f"script: {header['error']}")
        return out


def tool_script_install(script_id: str, source: str, icon_svg: str | None = None,
                        overwrite: bool = False, test_project: str | None = None,
                        binary: str | None = None, elements_dir: str | None = None,
                        timeout: int = 180) -> dict:
    """Store a script so QElectroTech shows it as a button."""
    _require_script_consent()
    sid = _script_id(script_id)
    if not isinstance(source, str) or not source.strip():
        raise ValueError("'source' must be the script's text")
    if len(source.encode("utf-8")) > _SCRIPT_MAX_BYTES:
        raise ValueError(f"'source' is over {_SCRIPT_MAX_BYTES // 1024} KB")
    header = parse_script_header(source, sid)
    if header.get("error"):
        raise ValueError(f"QElectroTech would refuse this header: {header['error']}. "
                         + SCRIPT_HEADER_HELP)
    folder = scripts_dir()
    icon = header["icon"]
    if icon_svg is not None:
        _check_icon_svg(icon_svg)
        if icon != f"{sid}.svg":
            raise ValueError(f"with 'icon_svg', the header must say '// @icon {sid}.svg'")
    elif icon and not icon.startswith("builtin:") and not (folder / icon).is_file():
        raise ValueError(f"the header names icon file {icon!r}, which is not in "
                         f"{folder}: pass its SVG as 'icon_svg', use builtin:<name>, "
                         "or leave @icon out for an initials tile")

    target = folder / f"{sid}.js"
    if target.exists() and not overwrite:
        raise ValueError(f"a script with id {sid!r} is already stored: {target}. "
                         "Pass \"overwrite\": true to replace it.")

    test = None
    if test_project:
        test = tool_script_test(binary, test_project, source, elements_dir, timeout)
        if not test.get("ok"):
            return {"ok": False, "installed": None,
                    "reason": "the test run failed, so nothing was stored",
                    "test": test}

    folder.mkdir(parents=True, exist_ok=True)
    if icon_svg is not None:
        _write_atomic(folder / f"{sid}.svg", icon_svg)
    _write_atomic(target, source)
    out = {"ok": True, "installed": str(target), "header": header,
           "where": "Projet > Scripts, the Scripts toolbar, command search "
                    "(Ctrl+Shift+M) and the shortcut bar's Customise list; an "
                    "open QElectroTech picks it up without a restart"}
    if test is not None:
        out["test"] = {"ok": True, "diff": test.get("diff")}
    return out


def tool_script_list() -> dict:
    folder = scripts_dir()
    scripts, refused = [], []
    if folder.is_dir():
        for path in sorted(folder.glob("*.js")):
            try:
                text = path.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError) as exc:
                refused.append({"file": path.name, "error": str(exc)})
                continue
            h = parse_script_header(text, path.stem)
            if h.get("error"):
                refused.append({"file": path.name, "error": h["error"]})
            else:
                scripts.append(h)
    return {"folder": str(folder), "exists": folder.is_dir(),
            "scripts": scripts, "refused": refused}


def tool_script_read(script_id: str) -> dict:
    sid = _script_id(script_id)
    folder = scripts_dir()
    path = folder / f"{sid}.js"
    if not path.is_file():
        raise ValueError(f"no stored script {sid!r} in {folder}")
    source = path.read_text(encoding="utf-8")
    out = {"id": sid, "path": str(path), "source": source,
           "header": parse_script_header(source, sid)}
    icon = folder / f"{sid}.svg"
    if icon.is_file():
        out["icon_svg"] = icon.read_text(encoding="utf-8")
    return out


def tool_script_remove(script_id: str) -> dict:
    _require_script_consent()
    sid = _script_id(script_id)
    folder = scripts_dir()
    path = folder / f"{sid}.js"
    if not path.is_file():
        raise ValueError(f"no stored script {sid!r} in {folder}")
    icon = parse_script_header(path.read_text(encoding="utf-8"), sid).get("icon", "")
    path.unlink()
    removed = [str(path)]
    # The icon goes too unless another script still names it.
    if icon and not icon.startswith("builtin:") and "/" not in icon and "\\" not in icon:
        others = {parse_script_header(p.read_text(encoding="utf-8"), p.stem).get("icon")
                  for p in folder.glob("*.js")}
        if icon not in others and (folder / icon).is_file():
            (folder / icon).unlink()
            removed.append(str(folder / icon))
    return {"removed": removed}


# --------------------------------------------------------------------------
# Live mode: act on the project open in a running QElectroTech
# --------------------------------------------------------------------------
#
# Everything above is headless: it reads and writes files and launches its
# own QElectroTech. These tools instead talk to the QElectroTech the user
# has open, which only listens when three things are true: scripting is
# allowed, its "mode direct" setting is on (off by default), and the user
# accepted the warning it shows at every start. It then puts the socket
# name and token in the "live" part of qet-assistant.json, and clears it
# when the channel closes. Each action is one undo step in front of the user.

def _live_session() -> dict:
    """The live channel QElectroTech advertises in qet-assistant.json."""
    info = assistant_info()
    path = assistant_info_file()
    if info is None:
        raise ValueError(
            "QElectroTech has not written qet-assistant.json yet (looked for "
            f"{path}): start QElectroTech, a version with live mode, first.")
    live = info.get("live")
    if not info.get("running") or not live:
        features = info.get("features") or {}
        if not info.get("running"):
            why = "QElectroTech is not running"
        elif not features.get("live_mode_setting"):
            why = ("live mode is off: in QElectroTech, Configurer QElectroTech > "
                   "Général > \"Autoriser un assistant IA à agir sur le projet "
                   "ouvert\", then restart it")
        else:
            why = ("live mode is on but not open for this session: answer "
                   "\"Continuer\" in the warning QElectroTech shows at start, or "
                   "restart it if \"Pas pour cette session\" or Arrêter was chosen")
        raise ValueError(f"no QElectroTech is listening for an assistant: {why}.")
    return live


def _live_call(request: dict, timeout: float = 60.0) -> dict:
    session = _live_session()
    request = dict(request, token=session.get("token", ""), id=1)
    line = (json.dumps(request) + "\n").encode("utf-8")
    name = session.get("socket", "")
    try:
        if os.name == "nt":
            # QLocalServer is a named pipe on Windows; fullServerName() is
            # already \\.\pipe\<name>.
            with open(name, "r+b", buffering=0) as pipe:
                pipe.write(line)
                data = b""
                while not data.endswith(b"\n"):
                    chunk = pipe.read(1)
                    if not chunk:
                        break
                    data += chunk
        else:
            import socket
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.settimeout(timeout)
                sock.connect(name)
                sock.sendall(line)
                data = b""
                while not data.endswith(b"\n"):
                    chunk = sock.recv(65536)
                    if not chunk:
                        break
                    data += chunk
    except OSError as exc:
        raise ValueError(f"could not reach QElectroTech's live channel ({exc}); it "
                         "may have been stopped, or QElectroTech closed") from None
    if not data.strip():
        raise ValueError("QElectroTech closed the live channel without answering")
    answer = json.loads(data.decode("utf-8"))
    answer.pop("id", None)
    return answer


def tool_live_status() -> dict:
    return _live_call({"cmd": "status"})


def tool_live_run_script(source: str, name: str = "", timeout: int = 300) -> dict:
    _require_script_consent()
    if not isinstance(source, str) or not source.strip():
        raise ValueError("'source' must be the script's text")
    return _live_call({"cmd": "run_script", "source": source, "name": name or "script"},
                      timeout)


def tool_live_run_stored(script_id: str, timeout: int = 60) -> dict:
    _require_script_consent()
    return _live_call({"cmd": "run_stored", "script": _script_id(script_id)}, timeout)


def tool_live_command(action: str) -> dict:
    _require_script_consent()
    if not isinstance(action, str) or not action:
        raise ValueError("'action' must be a command id, e.g. diagrameditor.zoom_fit")
    return _live_call({"cmd": "command", "action": action})


def tool_live_show_folio(folio: int) -> dict:
    if not isinstance(folio, int) or isinstance(folio, bool):
        raise ValueError("'folio' must be an index counted from 0")
    return _live_call({"cmd": "show_folio", "folio": folio})


def tool_live_undo_last() -> dict:
    _require_script_consent()
    return _live_call({"cmd": "undo_last"})


def tool_live_screenshot() -> dict:
    answer = _live_call({"cmd": "screenshot"})
    data = answer.pop("png_base64", None)
    if data:
        answer["_image_png_base64"] = data
    return answer


TOOLS = [
    {
        "name": "qet_project_info",
        "description": "Summarise a .qet project: title, format version, folios "
                       "with their uuids, and element/conductor counts per folio. A "
                       "folio saved without a uuid shows it empty; QElectroTech gives "
                       "it one on load and writes it on the next save. Reads the file "
                       "directly; does not launch QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string", "description": "path to a .qet file"}},
            "required": ["path"],
        },
        "handler": lambda a: tool_project_info(a["path"]),
    },
    {
        "name": "qet_items",
        "description": "List the drawn items that are not symbols or wires -- free texts, "
                       "shapes, pictures, tables and the text fields of symbols -- with "
                       "each one's uuid, folio (counted from 1) and main fields. Use the "
                       "uuid to address an item in qet_edit or to find it in qet_diff. "
                       "Reads the file directly; does not launch QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "folio": {"type": "integer", "description": "folio number counted from 1"},
                "kind": {"type": "string", "enum": ITEM_KINDS},
                "limit": {"type": "integer", "default": 500},
            },
            "required": ["path"],
        },
        "handler": lambda a: tool_items(a["path"], a.get("folio"), a.get("kind"),
                                        a.get("limit", 500)),
    },
    {
        "name": "qet_elements",
        "description": "List placed elements with uuid, type, position, label and "
                       "their elementInformations bag. Optionally filter by folio "
                       "or by element name substring.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "folio": {"type": "integer", "description": "1-based folio number"},
                "name_contains": {"type": "string"},
                "limit": {"type": "integer", "default": 200},
            },
            "required": ["path"],
        },
        "handler": lambda a: tool_elements(a["path"], a.get("folio"),
                                           a.get("name_contains"),
                                           a.get("limit", 200)),
    },
    {
        "name": "qet_conductors",
        "description": "List conductors with their documentation fields (num, "
                       "formula, cable, bus, function, colour, section). Set "
                       "attribute+non_empty to find only conductors that carry a "
                       "value for one attribute.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "folio": {"type": "integer",
                          "description": "folio number counted from 1, as qet_elements and the application show it"},
                "attribute": {"type": "string",
                              "description": "an XML attribute of <conductor>, e.g. cable"},
                "non_empty": {"type": "boolean", "default": False},
                "limit": {"type": "integer", "default": 200},
            },
            "required": ["path"],
        },
        "handler": lambda a: tool_conductors(a["path"], a.get("folio"),
                                             a.get("attribute"),
                                             a.get("non_empty", False),
                                             a.get("limit", 200)),
    },
    {
        "name": "qet_diff",
        "description": "Structurally diff two .qet files: which elements moved and "
                       "by what delta, which were rotated (orientation in quarter "
                       "turns, 0-3), which were added, removed or relabelled, and "
                       "which conductor fields changed; also folio fields, texts, shapes, "
                       "pictures, tables, symbol text fields and terminal strips. Items are "
                       "matched by their uuid when every one of a kind has one (each "
                       "section says so in \"keyed_by\"), otherwise by position or ends. "
                       "Use this to verify what an edit actually did, rather than reading "
                       "a screenshot.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "before": {"type": "string"},
                "after": {"type": "string"},
            },
            "required": ["before", "after"],
        },
        "handler": lambda a: tool_diff(a["before"], a["after"]),
    },
    {
        "name": "qet_scan",
        "description": "Sweep every .qet in a directory and count how many nodes of "
                       "a given tag carry a non-empty attribute, with the distinct "
                       "values found. For corpus questions such as how many "
                       "conductors in the shipped examples have a cable value.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "directory": {"type": "string"},
                "tag": {"type": "string", "default": "conductor"},
                "attribute": {"type": "string", "default": "cable"},
                "recursive": {"type": "boolean", "default": True},
            },
            "required": ["directory"],
        },
        "handler": lambda a: tool_scan(a["directory"], a.get("tag", "conductor"),
                                       a.get("attribute", "cable"),
                                       a.get("recursive", True)),
    },
    {
        "name": "qet_element_info",
        "description": "Introspect a .elmt element definition: translated names, "
                       "terminals, which dynamic-text info fields it carries, and a "
                       "count of its drawing parts.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string", "description": "path to a .elmt file"}},
            "required": ["path"],
        },
        "handler": lambda a: tool_element_info(a["path"]),
    },
    {
        "name": "qet_export",
        "description": "Run a QElectroTech export headlessly (pdf, png, svg, dxf, bom, "
                       "cables, wires, wiring, nets, links, info). Launches the "
                       "binary in an isolated sandbox so it cannot be captured by, "
                       "or capture, a running QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string"},
                "format": {"type": "string", "enum": sorted(EXPORT_FORMATS)},
                "output": {"type": "string"},
                "overwrite": {"type": "boolean", "default": False,
                              "description": "replace \"output\" if it already exists; "
                                             "without this an existing file is never clobbered"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project", "format", "output"],
        },
        "handler": lambda a: tool_export(a["binary"], a["project"], a["format"],
                                         a["output"], a.get("timeout", 180)),
    },
    {
        "name": "qet_edit",
        "description": "Edit a project through QElectroTech's own scripting API "
                       "and report what actually changed. Places, moves, rotates, "
                       "labels and deletes elements, wires two terminals together, "
                       "and adds folios -- each through the same undo command the "
                       "GUI uses, so the result is undoable and reaches the project "
                       "database. Writes a new file, never the input, and returns a "
                       "qet_diff of the two. Needs a build whose scripting API "
                       "carries the drawing verbs; says so plainly if it does not.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string", "description": "the .qet to start from; not modified"},
                "output": {"type": "string", "description": "where to write the edited project"},
                "overwrite": {"type": "boolean", "default": False,
                              "description": "replace \"output\" if it already exists; "
                                             "without this an existing file is never clobbered"},
                "operations": {
                    "type": "array",
                    "minItems": 1,
                    "description":
                        "Operations applied in order. Each is an object with \"op\" "
                        "and that op's arguments. Ops: " + ", ".join(sorted(OPS)) + ". "
                        "Give an op an \"id\" to name what it produced, then refer to "
                        "it later as \"$id\" -- that is how an element placed by "
                        "add_element gets wired by add_conductor, and how a folio made "
                        "by add_folio is addressed. A \"folio\" given as a number is "
                        "an index counted from 0: the folio qet_elements and "
                        "qet_project_info call 1 is \"folio\": 0 here. A folio can "
                        "be given as its uuid instead (qet_project_info lists them), "
                        "which still names the same folio after an earlier op adds or "
                        "removes one; so does the \"$id\" of an add_folio or "
                        "insert_folio. A terminal (\"terminal\", \"from_terminal\", "
                        "\"to_terminal\") is given by its index -- qet_element_info "
                        "lists terminals in index order, top to bottom then left to "
                        "right -- or by its uuid, which qet_element_info also lists and "
                        "which, unlike the index, tells apart two terminals at the same "
                        "point; a terminal uuid names a terminal of the op's own "
                        "element (for add_conductor, of that end's element). "
                        "set_conductor addresses a conductor as the one on a given "
                        "terminal and applies the change to its whole electrical "
                        "potential, so name a terminal carrying exactly one conductor, "
                        "or give \"conductor\": its uuid from qet_conductors in place "
                        "of \"element\" + \"terminal\" (set_conductor, "
                        "move_conductor_segment and delete_conductor all accept it; it "
                        "needs one of the conductor's two terminals to carry only it, "
                        "and a conductor qet_conductors lists with an empty uuid has "
                        "none to give). A uuid names one wire, but set_conductor still "
                        "changes its whole potential, as it does by terminal. "
                        "set_conductor's \"property\" is one of " + ", ".join(CONDUCTOR_PROPERTIES) +
                        ". move_conductor_segment reroutes the drawn path itself rather "
                        "than a property of the potential -- addressed the same way (a "
                        "terminal carrying exactly one conductor), plus a segment index "
                        "into that conductor's own path. A segment only moves "
                        "perpendicular to its own direction, the same as dragging its "
                        "handle in the GUI: dx moves a vertical segment, dy moves a "
                        "horizontal one, the other of the pair is silently ignored, and "
                        "the two segments touching a terminal are static (refused, no "
                        "handle exists on them either). There is no query op to list "
                        "segments or their indexes first -- a freshly auto-routed "
                        "conductor between two terminals is a static segment, one or two "
                        "movable ones, then a static segment, in that order from the "
                        "first terminal; call with a guessed index and read \"succeeded\" "
                        "to check it landed on a movable one. "
                        "link_elements takes a folio for each end, since a master "
                        "and its slave are usually on different ones. "
                        "delete_conductor removes only the conductor on the named "
                        "terminal (which must carry exactly one). remove_folio shifts "
                        "later folio indexes down. set_folio takes one of " +
                        ", ".join(FOLIO_PROPERTIES) + ". "
                        "Auto-numbering: add_autonum defines a named context of kind "
                        "conductor, element or folio from parts written "
                        "\"type[:value[:increase]]\" (e.g. [\"string:W\", \"unit:1:1\"]); "
                        "use_conductor_autonum then makes new conductors on a folio "
                        "take their number from it, so define and select it BEFORE the "
                        "add_conductor ops it should number. For elements, "
                        "use_element_autonum selects the context and number_element applies "
                        "it to one element AFTER it is placed (add_element does not number "
                        "what it places); slaves and reports are refused, since they take "
                        "their label from their master. "
                        "Terminal strips: add_terminal_strip returns an index (name "
                        "it \"$id\"); add_to_strip puts a terminal-type element on "
                        "it, and refuses any other kind. group_terminals/bridge_terminals "
                        "take \"indices\" (at least two) into that strip's real-terminal "
                        "listing -- group merges onto whichever named position already has "
                        "the most terminals, not necessarily the first index given; bridge "
                        "refuses terminals that are not all at the same level. A group() call "
                        "can fully reorder the listing, not just shift indices after it -- "
                        "always re-list before addressing one by index again. "
                        "sort_terminal_strip reorders it canonically. "
                        "Images: add_image takes a file path (over 10 MB is refused) "
                        "and returns an index; the pixels are embedded in the saved "
                        "project. scale_image/rotate_image can change an image's sort "
                        "index, so rely on the \"$id\" only until the next scale or "
                        "rotate. "
                        "add_pdf_page renders one page of a PDF file to an image and "
                        "places it, through the same code path as the \"add image\" "
                        "toolbar action's own PDF support: \"page\" is 1-based, \"dpi\" "
                        "is the render resolution (the GUI dialog defaults to 150), and "
                        "the result is an ordinary image afterwards -- scale_image, "
                        "rotate_image and delete_image all apply to it same as any other. "
                        "Only reachable in a build with the QtPdf module (Qt >= 6.4); "
                        "some Qt6 distributions omit it, and the op is refused with a "
                        "clear reason rather than being absent, so check the op's own "
                        "\"succeeded\"/result rather than assuming a missing method. "
                        "insert_folio puts a new folio at a position (0 = first, the "
                        "folio count = last) and returns its index; element_geometry reads "
                        "an element's x, y, rotation and the box it occupies "
                        "(left/top/right/bottom) and reports it in the result -- use it to "
                        "lay things out relative to each other across calls; undo/redo step "
                        "QElectroTech's undo stack (consecutive edits to one property merge, "
                        "so one undo can revert several) and fail if there is nothing to "
                        "undo. search_and_replace finds and replaces a substring or (with "
                        "\"regex\": true) a regular expression within one text field, "
                        "across every folio, as a single undo step -- unlike doing the "
                        "same with a read op and set_conductor/set_info/set_text in a "
                        "loop, which would leave one undo entry per item touched. \"kind\" "
                        "is element_info (\"field\" is an information key such as "
                        "\"label\"), conductor (\"field\" is one of " +
                        ", ".join(CONDUCTOR_PROPERTIES) + " -- replacing on one conductor "
                        "of a potential updates the whole potential, the same as "
                        "set_conductor always does) or text (independent texts; \"field\" "
                        "is ignored). This is NOT QElectroTech's own \"Search and replace\" "
                        "panel: that one is a batch overwrite-with-sentinel template built "
                        "for picking items from a tree interactively, a poor fit for a "
                        "script that can already say precisely which items it means. This "
                        "does what the name says instead -- an actual substring/regex "
                        "replace within each item's current value, touching only items "
                        "where it is found. Returns the number of items changed; never "
                        "matches an empty field. set_project_title renames the project. "
                        "set_folio_border sets one "
                        "of the folio frame's " + ", ".join(FOLIO_BORDER_PROPERTIES) +
                        " (counts 1-99, sizes 1-1000, display-* true/false). "
                        "embed_title_block_template copies a template into the project from "
                        "the common/company/custom collection that has it (only reachable if "
                        "the binary's compiled-in template path resolves to something real -- "
                        "typically a make install'd QET; there is no per-run override for this "
                        "one the way elements_dir is for elements, since QElectroTech reads "
                        "--common-tbt-dir before --run's own argument handling ever sees it, "
                        "so this tool cannot pass it through). set_folio's \"template\" property "
                        "then embeds-if-needed and applies it in one call; a template literally "
                        "named \"default\" reads back as \"\" afterwards, since QElectroTech "
                        "treats the two as the same thing. "
                        "duplicate_elements copies elements, with the conductors between "
                        "them, to a position (the top-left of the copied group's bounding "
                        "box; (0,0) keeps the source coordinates) on the same or another "
                        "folio: \"elements\" is a list of uuids or \"$id\" references, "
                        "and the result lists the copies in that same order, so "
                        "\"$copies[0]\" is the copy of the first. Copies come without "
                        "labels or wire numbers, as on a paste in the application. "
                        "Symbol text fields (the label, terminal names, values drawn on a "
                        "symbol): add_element_text (source text|info|composite; value is "
                        "the string, an information key such as \"label\", or a formula; "
                        "x/y are in the element's own coordinates) returns an index within "
                        "that element; set_element_text takes " +
                        ", ".join(ELEMENT_TEXT_PROPERTIES) + ". A field bound with source "
                        "\"info\" follows set_label/set_info. Indexes shift on delete. "
                        "Texts, shapes and images: add_text/add_shape/add_image return an "
                        "index you can name as \"$id\", and the other text/shape/image "
                        "ops take it as \"index\". Indexes shift when one is added or "
                        "deleted; \"index\" also accepts the item's uuid (from the "
                        "project database's drawing_item_view, or a saved file), which "
                        "does not. Shapes: " + ", ".join(SHAPES) + "; set_shape takes " + ", ".join(SHAPE_PROPERTIES) +
                        " (fill accepts a colour or \"none\"). "
                        "add_shape's own \"polygon\" is always the degenerate two-point "
                        "form (it shares add_shape's p1/p2 shape); add_polygon takes as "
                        "many points as wanted instead, as [{\"x\":.., \"y\":..}, ...] "
                        "in scene coordinates (at least 2), plus \"closed\"; "
                        "set_shape_polygon replaces an existing one's points the same "
                        "way. add_path places a curved shape -- a polygon's points plus, "
                        "per node, an optional \"kind\" (corner, the default; smooth; or "
                        "symmetric) and optional \"inHandle\"/\"outHandle\" bezier "
                        "control points, the same model the pen tool and node-edit mode "
                        "build; set_shape_path_nodes replaces an existing path's nodes. "
                        "set_shape_closed opens or closes a polygon or path (a no-op on "
                        "any other shape). set_shape_polygon/set_shape_path_nodes refuse "
                        "a shape of the wrong kind -- a shape made by add_shape is never "
                        "a valid target for either, and vice versa. A shape's index can "
                        "shift on any edit that moves it, not only an add or delete: "
                        "shapes are listed by current on-folio position, so changing one "
                        "shape's points can reorder it relative to the others -- re-list "
                        "before addressing one by index again if more than one is being "
                        "edited in the same run, or address it by uuid. "
                        "set_table_position/delete_table take a table's index or its uuid, "
                        "and set_element_text/delete_element_text a text field's index or "
                        "its uuid (the field's own, looked up within the op's element) "
                        "-- a uuid still names the right item after an "
                        "earlier one is deleted. "
                        "Tables: add_table places a BOM/nomenclature or summary table "
                        "(kind is \"nomenclature\" or \"summary\") built from a query "
                        "against a project database view -- run qet.query() (the "
                        "query op) against element_nomenclature_view or "
                        "project_summary_view first to find one that returns real "
                        "columns; an empty query is refused, since each query widget "
                        "defaults to zero selected columns and produces a table with "
                        "no rows. Returns an index you can name as \"$id\". Every new "
                        "table lands at the same fixed (50, 50), so a folio getting "
                        "more than one must reposition all but the first with "
                        "set_table_position or they stack exactly on top of each "
                        "other. delete_table removes one; indexes shift afterwards. "
                        "PLC IO: a PLC master (elementData type Master, masterType "
                        "PLC) carries an IO table -- add_plc_io appends a row (type "
                        "is one of entree_digitale, sortie_digitale, "
                        "entree_analogique, sortie_analogique, entree_universelle, "
                        "sortie_universelle) and returns its index as \"$id\"; "
                        "set_plc_io changes one field (type, address, function or "
                        "comment) of an existing row; remove_plc_io deletes one and "
                        "shifts the indexes after it. None of the three are "
                        "undoable -- MasterPropertiesWidget's own PLC IO editor "
                        "isn't either, since it manages PLC linking through the "
                        "table rather than the ordinary link-tree undo path. "
                        "link_plc_io is link_elements plus an io_index: it links a "
                        "PLC slave onto one specific row of a PLC master's IO table "
                        "(io_index into that table, from add_plc_io's return or a "
                        "count of prior add_plc_io calls) rather than leaving which "
                        "row unspecified the way a plain link_elements call would. "
                        "If an op fails the rest are skipped, since they usually "
                        "depend on it.",
                    "items": {"type": "object"},
                },
                "elements_dir": {
                    "type": "string",
                    "description": "the common elements collection, e.g. a checkout's "
                                   "elements/ directory. Required for \"common://\" "
                                   "paths: the sandboxed run has no settings of its "
                                   "own and would not find the collection otherwise. "
                                   "An absolute .elmt path works without it.",
                },
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project", "output", "operations"],
        },
        "handler": lambda a: tool_edit(a["binary"], a["project"], a["operations"],
                                       a["output"], a.get("elements_dir"),
                                       a.get("timeout", 180)),
    },
    {
        "name": "qet_query",
        "description": "Run a read-only SQL SELECT against the project's SQLite "
                       "database and get rows back. Prefer the views "
                       "(element_nomenclature_view, project_summary_view, "
                       "wiring_list_view) over the raw tables. Omit sql to list what "
                       "is queryable. Only SELECT and WITH are permitted -- "
                       "QElectroTech enforces this itself, the same way it does for "
                       "the custom-query box in its own interface.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string", "description": "the .qet to query; never modified"},
                "sql": {"type": "string",
                        "description": "a single SELECT or WITH...SELECT. "
                                       "Omit to list the tables and views instead."},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project"],
        },
        "handler": lambda a: tool_query(a["binary"], a["project"], a.get("sql", ""),
                                        a.get("elements_dir"), a.get("timeout", 180)),
    },
    {
        "name": "qet_continuity",
        "description": "Electrical continuity / ERC-style checks against the live "
                       "Terminal/Conductor object graph, not a heuristic read of the "
                       "XML (that is qet_check; the two are complementary). "
                       "unconnected_terminal (info -- routine, not necessarily wrong) "
                       "and potential_mismatch (error -- two conductors on the same "
                       "electrical potential disagreeing on num/colour/section/"
                       "function/bus/cable, which QElectroTech's own edits never "
                       "produce, so it means hand-edited XML, a legacy file, or an "
                       "external tool); and report_link_mismatch (warning -- a "
                       "next_report/previous_report folio-jump pair whose conductors "
                       "disagree, which CAN happen through ordinary use since linking "
                       "two report elements never checks or syncs conductor "
                       "properties -- reproduces qelectrotech/qelectrotech-source-"
                       "mirror#974). Does NOT check pin electrical direction/power "
                       "conflicts or No/Nc/Common contact shorts -- QElectroTech's "
                       "terminal data model carries neither. One QElectroTech "
                       "launch; read-only.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string", "description": "the .qet to check; never modified"},
                "folio": {"type": "integer", "description":
                          "check one folio only; omit for the whole project. An index "
                          "counted from 0, like qet_edit: the folio qet_elements calls 1 "
                          "is 0 here. An index with no folio is refused, not reported "
                          "clean. Each finding carries both \"folio\" (this index) and "
                          "\"folio_number\" (counted from 1)."},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project"],
        },
        "handler": lambda a: tool_continuity(a["binary"], a["project"], a.get("folio"),
                                             a.get("elements_dir"), a.get("timeout", 180)),
    },
    {
        "name": "qet_project_new",
        "description": "Create a new, empty project to start a schematic from: a "
                       "title and any number of folios, written by QElectroTech "
                       "itself and read back to check. qet_edit needs an existing "
                       "project, and the shipped 'blank' example is not blank, so "
                       "this is the way to begin from nothing. Refuses to overwrite "
                       "unless told to.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "output": {"type": "string", "description": "where to write the new .qet"},
                "title": {"type": "string", "description": "the project title"},
                "folios": {"description": "how many empty folios, or a list of folio titles",
                           "oneOf": [{"type": "integer", "minimum": 0, "maximum": 200},
                                     {"type": "array", "items": {"type": "string"}}],
                           "default": 1},
                "author": {"type": "string", "description": "set on every folio's title block"},
                "overwrite": {"type": "boolean", "default": False},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["output", "title"],
        },
        "handler": lambda a: tool_project_new(
            a["binary"], a["output"], a["title"], a.get("folios", 1), a.get("author", ""),
            a.get("overwrite", False), a.get("elements_dir"), a.get("timeout", 180)),
    },
    {
        "name": "qet_element_search",
        "description": "Find a symbol in an element collection by name (any "
                       "language, ignoring case and accents), link type, kind or "
                       "terminal count. Results carry a common:// path that "
                       "qet_edit's add_element takes directly, and the terminal "
                       "names in add_conductor's index order (top-to-bottom, then "
                       "left-to-right; not file order). Indexes the "
                       "collection on first use and re-indexes when it changes, so "
                       "a symbol written by qet_element_build is found at once.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "directory": {"type": "string",
                              "description": "the collection root, e.g. a checkout's elements/ directory"},
                "query": {"type": "string",
                          "description": "words to find; every word must match some name, the path or the kind"},
                "link_type": {"type": "string", "enum": list(LINK_TYPES)},
                "kind": {"type": "string", "description": "the element's type information, e.g. coil, protection"},
                "min_terminals": {"type": "integer"},
                "max_terminals": {"type": "integer"},
                "limit": {"type": "integer", "default": 25},
            },
            "required": ["directory"],
        },
        "handler": lambda a: tool_element_search(
            a["directory"], a.get("query", ""), a.get("link_type"),
            a.get("min_terminals"), a.get("max_terminals"), a.get("kind"),
            a.get("limit", 25)),
    },
    {
        "name": "qet_check",
        "description": "Run design-rule checks over a project and report findings by "
                       "severity: duplicate master labels (error), duplicate simple "
                       "labels and unlabelled masters (warning), unnumbered "
                       "conductors, empty folios and masters missing a manufacturer "
                       "reference (info). One QElectroTech launch; read-only. "
                       "These are heuristics tuned against QElectroTech's shipped "
                       "examples, not standards -- each finding carries a note "
                       "saying how far to trust it.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string"},
                "checks": {"type": "array", "items": {"type": "string", "enum": sorted(CHECKS)},
                           "description": "which checks to run; omit for all"},
                "sample": {"type": "integer", "default": 10,
                           "description": "how many offending rows to return per check"},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project"],
        },
        "handler": lambda a: tool_check(a["binary"], a["project"], a.get("checks"),
                                        a.get("sample", 10), a.get("elements_dir"),
                                        a.get("timeout", 180)),
    },
    {
        "name": "qet_element_build",
        "description": "Write a .elmt element definition: named in one or more "
                       "languages, drawn from lines, rectangles, ellipses, circles, "
                       "arcs, polygons and text, with terminals to wire it by. "
                       "Computes the width/height/hotspot header so the declared box "
                       "contains the drawing, validates every part against the schema "
                       "the shipped collection uses, and reads the result back. "
                       "Writes the file directly; does not launch QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "output": {"type": "string", "description": "path to write, ending .elmt"},
                "overwrite": {"type": "boolean", "default": False,
                              "description": "replace \"output\" if it already exists; "
                                             "without this an existing file is never clobbered"},
                "names": {"type": "object",
                          "description": 'translated names by language code, e.g. '
                                         '{"en": "Coil", "fr": "Bobine"}. French is '
                                         "QElectroTech's source language; give it if you can."},
                "parts": {
                    "type": "array",
                    "description":
                        'the drawing. Each part is {"type": ...} plus its own keys: '
                        'line x1,y1,x2,y2; rect/ellipse/arc x,y,width,height '
                        "(arc also start,angle); circle x,y,diameter; polygon "
                        'points:[[x,y],...] and closed; text x,y,text with optional '
                        "size, rotation, color. Any part may carry style and antialias, "
                        "and a uuid to name it by; parts without one get a new uuid, "
                        "returned in part_uuids. "
                        "Coordinates are the element's own, with (0,0) at its origin.",
                    "items": {"type": "object"},
                },
                "terminals": {
                    "type": "array",
                    "description": 'where conductors attach: {"x","y","orientation"} '
                                   "with orientation n, s, e or w, plus an optional "
                                   'name such as "A1". Their order here is the order '
                                   "qet_edit indexes terminals by position, not by this order: "
                                   "top to bottom, then left to right. qet_element_build "
                                   "returns the resulting index order.",
                    "items": {"type": "object"},
                },
                "link_type": {"type": "string", "enum": list(LINK_TYPES),
                              "description": "simple for an ordinary symbol, master/slave "
                                             "for a cross-referenced pair, thumbnail for "
                                             "a drawing with no terminals"},
                "informations": {"type": "object",
                                 "description": "kindInformation entries, e.g. {\"type\": \"coil\"}"},
                "uuid": {"type": "string", "description": "reuse an existing uuid; "
                                                          "omit to generate one"},
            },
            "required": ["output", "names", "parts"],
        },
        "handler": lambda a: tool_element_build(
            a["output"], a["names"], a["parts"], a.get("terminals"),
            a.get("link_type", "simple"), a.get("informations"), a.get("uuid")),
    },
    {
        "name": "qet_script_api",
        "description": "List every call a QElectroTech script can make (the global "
                       "'qet'), asked of the QElectroTech that will run it, plus the "
                       "header format that turns a script into a button. Read this "
                       "before writing a script for qet_script_install. One launch; "
                       "needs QET_ENABLE_SCRIPTING=1.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "timeout": {"type": "integer", "default": 120},
            },
        },
        "handler": lambda a: tool_script_api(a["binary"], a.get("timeout", 120)),
    },
    {
        "name": "qet_script_test",
        "description": "Run a script's text on a COPY of a project and return what it "
                       "would change (a qet_diff), what it logged, and any error with "
                       "its line. The project is never modified. Headless there is no "
                       "folio on screen: qet.currentFolio() is the first folio. Also "
                       "says if the header would get no button. Needs "
                       "QET_ENABLE_SCRIPTING=1.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "the qelectrotech executable; leave it out to use the one this server is configured with. Any other is refused unless its configuration allows it"},
                "project": {"type": "string", "description": "the .qet to try it on; never modified"},
                "source": {"type": "string", "description": "the script's full text"},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["project", "source"],
        },
        "handler": lambda a: tool_script_test(a["binary"], a["project"], a["source"],
                                              a.get("elements_dir"), a.get("timeout", 180)),
    },
    {
        "name": "qet_script_install",
        "description": "Store a script so QElectroTech shows it as a button with an "
                       "icon: Projet > Scripts, the Scripts toolbar, command search "
                       "and the shortcut bar. A running QElectroTech picks it up "
                       "without a restart. The text must start with a "
                       "// ==QETScript== header (see qet_script_api); the file is "
                       "<id>.js in the user's scripts folder, which this server "
                       "chooses. Give 'icon_svg' to store an icon as <id>.svg (the "
                       "header must then say '// @icon <id>.svg'). Give "
                       "'test_project' to run qet_script_test first and store "
                       "nothing if it fails -- recommended. Does not run the "
                       "script: the user clicks it. Needs QET_ENABLE_SCRIPTING=1.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "file name without .js: a-z, 0-9, '-', '_'"},
                "source": {"type": "string", "description": "the script's full text, header first"},
                "icon_svg": {"type": "string", "description": "optional SVG for the button, stored as <id>.svg"},
                "overwrite": {"type": "boolean", "default": False},
                "test_project": {"type": "string", "description": "optional .qet to test on first; never modified"},
                "binary": {"type": "string", "description": "the qelectrotech executable for the test; leave it out to use the one this server is configured with"},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["id", "source"],
        },
        "handler": lambda a: tool_script_install(
            a["id"], a["source"], a.get("icon_svg"), bool(a.get("overwrite")),
            a.get("test_project"), a.get("binary"), a.get("elements_dir"),
            a.get("timeout", 180)),
    },
    {
        "name": "qet_script_list",
        "description": "List the stored scripts QElectroTech shows as buttons: each "
                       "one's id, name, icon, tooltip, shortcut and context, and the "
                       "files it ignores with the reason. Reads files only.",
        "inputSchema": {"type": "object", "properties": {}},
        "handler": lambda a: tool_script_list(),
    },
    {
        "name": "qet_script_read",
        "description": "The text (and stored SVG icon, if any) of one stored script, "
                       "to change it and store it again with qet_script_install "
                       "overwrite=true.",
        "inputSchema": {
            "type": "object",
            "properties": {"id": {"type": "string"}},
            "required": ["id"],
        },
        "handler": lambda a: tool_script_read(a["id"]),
    },
    {
        "name": "qet_script_remove",
        "description": "Delete a stored script, and its icon if no other script uses "
                       "it; its button goes from a running QElectroTech. Needs "
                       "QET_ENABLE_SCRIPTING=1.",
        "inputSchema": {
            "type": "object",
            "properties": {"id": {"type": "string"}},
            "required": ["id"],
        },
        "handler": lambda a: tool_script_remove(a["id"]),
    },
    {
        "name": "qet_live_status",
        "description": "LIVE MODE. Ask the QElectroTech the user has open what is on "
                       "screen: the project, the folio shown (index and title), the "
                       "selected elements, the last undo step and the stored scripts. "
                       "Works only if the user switched live mode on in QElectroTech "
                       "and accepted its warning at this start; the error says which "
                       "step is missing. Changes nothing.",
        "inputSchema": {"type": "object", "properties": {}},
        "handler": lambda a: tool_live_status(),
    },
    {
        "name": "qet_live_run_script",
        "description": "LIVE MODE. Run script text on the project the user has open, "
                       "in front of them, as one undo step named after 'name'. "
                       "qet.currentFolio() is the folio on screen. Returns what the "
                       "script logged, its error with the line if it threw, and the "
                       "undo step (empty if nothing changed). Try a new script with "
                       "qet_script_test on a copy first when you can. Needs "
                       "QET_ENABLE_SCRIPTING=1 and live mode on in QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "source": {"type": "string", "description": "the script's text"},
                "name": {"type": "string", "description": "what the user sees in the undo step"},
                "timeout": {"type": "integer", "default": 300,
                            "description": "seconds; the user may be reading the script before saying yes"},
            },
            "required": ["source"],
        },
        "handler": lambda a: tool_live_run_script(a["source"], a.get("name", ""),
                                                  a.get("timeout", 300)),
    },
    {
        "name": "qet_live_run_stored",
        "description": "LIVE MODE. Press a stored script's button (see "
                       "qet_script_list) in the QElectroTech the user has open: one "
                       "undo step. Needs QET_ENABLE_SCRIPTING=1 and live mode on in "
                       "QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {"id": {"type": "string"},
                           "timeout": {"type": "integer", "default": 60}},
            "required": ["id"],
        },
        "handler": lambda a: tool_live_run_stored(a["id"], a.get("timeout", 60)),
    },
    {
        "name": "qet_live_command",
        "description": "LIVE MODE. Trigger one editor command in the QElectroTech the "
                       "user has open, by id. Only commands that open no dialog are "
                       "allowed: diagrameditor.select_all, select_nothing, "
                       "select_invert, select_all_conductors, select_all_text_fields, "
                       "zoom_in, zoom_out, zoom_content, zoom_fit, zoom_reset, "
                       "rotate_selection, rotate_texts, snap_selection_to_grid, "
                       "group_selection, ungroup_selection, conductor_reset (all "
                       "prefixed diagrameditor.). Anything else -- saving, deleting, "
                       "exporting -- is refused; use a script for edits.",
        "inputSchema": {
            "type": "object",
            "properties": {"action": {"type": "string"}},
            "required": ["action"],
        },
        "handler": lambda a: tool_live_command(a["action"]),
    },
    {
        "name": "qet_live_show_folio",
        "description": "LIVE MODE. Show another folio of the open project (index "
                       "from 0), so qet.currentFolio() and qet_live_screenshot "
                       "follow it.",
        "inputSchema": {
            "type": "object",
            "properties": {"folio": {"type": "integer"}},
            "required": ["folio"],
        },
        "handler": lambda a: tool_live_show_folio(a["folio"]),
    },
    {
        "name": "qet_live_undo_last",
        "description": "LIVE MODE. Undo the newest step in the open project, only if "
                       "the assistant made it (its name starts \"Assistant :\"); "
                       "the user's own steps are never undone this way.",
        "inputSchema": {"type": "object", "properties": {}},
        "handler": lambda a: tool_live_undo_last(),
    },
    {
        "name": "qet_live_screenshot",
        "description": "LIVE MODE. An image of the folio on screen in the user's "
                       "QElectroTech, as they see it. Changes nothing.",
        "inputSchema": {"type": "object", "properties": {}},
        "handler": lambda a: tool_live_screenshot(),
    },
    {
        "name": "qet_about",
        "description": "Start here. What QElectroTech last wrote about itself in "
                       "qet-assistant.json: version, every folder (data, settings, "
                       "scripts, element and title block collections), which "
                       "features are on (scripting, live mode), every call a script "
                       "can make, the stored scripts and the ones refused with why, "
                       "and whether a live session is open; plus this server's own "
                       "setup. Reads one file; changes nothing.",
        "inputSchema": {"type": "object", "properties": {}},
        "handler": lambda a: tool_about(),
    },
]

_BY_NAME = {t["name"]: t for t in TOOLS}


# --------------------------------------------------------------------------
# Filesystem policy
# --------------------------------------------------------------------------
#
# Every path in a tool call arrives from the model, so without a policy this
# server is a read/write primitive for anything the OS lets the process
# touch: read any .qet or .elmt, export a project's contents somewhere else,
# overwrite an unrelated file, embed an arbitrary local image or PDF. The
# sandboxed HOME each QElectroTech launch gets isolates *settings*, not the
# filesystem.
#
# So data paths are confined to a workspace, and the program the server
# launches is not the client's to choose:
#
#   data          chosen by the client per call -- the projects, directories,
#                 images and outputs below. Confined to the workspace.
#   executable    "binary". Resolved by the server itself (resolve_binary());
#                 a client may name it only when it is that same file or one
#                 whoever configured the server listed in QET_MCP_BINARIES.
#                 It once counted as configuration and went unchecked, but it
#                 is a per-call argument: a model steered by text in a
#                 project could run any program on the machine with it.
#   collection    "elements_dir". Normally outside the workspace (in /usr or
#                 a build tree), so allowed there, in the collection of the
#                 resolved install, or in a directory listed in
#                 QET_MCP_ELEMENTS.
#
# Enforced here, at the dispatcher, because this is the trust boundary --
# the point where model-supplied arguments enter. Calling the tool_* helpers
# directly from Python is not confined and is not meant to be: that is the
# server's own code calling itself.
_DATA_PATHS = {
    "qet_project_info":   {"read": ("path",)},
    "qet_elements":       {"read": ("path",)},
    "qet_items":          {"read": ("path",)},
    "qet_conductors":     {"read": ("path",)},
    "qet_diff":           {"read": ("before", "after")},
    "qet_scan":           {"read": ("directory",)},
    "qet_element_info":   {"read": ("path",)},
    "qet_element_search": {"read": ("directory",)},
    "qet_export":         {"read": ("project",), "write": ("output",)},
    "qet_edit":           {"read": ("project",), "write": ("output",)},
    "qet_query":          {"read": ("project",)},
    "qet_continuity":     {"read": ("project",)},
    "qet_check":          {"read": ("project",)},
    "qet_project_new":    {"write": ("output",)},
    "qet_element_build":  {"write": ("output",)},
    # The scripts folder is chosen by scripts_dir(), never by the client,
    # so only the project a script is tried on is a data path here.
    "qet_script_api":     {},
    "qet_script_test":    {"read": ("project",)},
    "qet_script_install": {"read": ("test_project",)},
}

# Tools that launch QElectroTech, and so take "binary" and "elements_dir".
_LAUNCHES_QET = {"qet_export", "qet_edit", "qet_query", "qet_continuity",
                 "qet_check", "qet_project_new", "qet_script_api", "qet_script_test"}

# Tools that launch QElectroTech only when given this argument.
_LAUNCHES_QET_WITH = {"qet_script_install": "test_project"}

# Tools whose "overwrite" guards a file the server names itself (the
# stored script, in scripts_dir()), not a client-chosen output path.
_OVERWRITE_OWN_FILE = {"qet_script_install"}

# qet_edit operations that name a file of their own.
_DATA_PATH_OPS = {"add_image": "file", "add_pdf_page": "file"}


def workspace_roots() -> list:
    """The directories tool calls may read and write.

    QET_MCP_WORKSPACE, os.pathsep-separated, or the process's working
    directory when unset -- a real confinement either way, and the working
    directory is what an MCP host normally starts the server in. Set
    QET_MCP_ALLOW_ANY_PATH=1 to turn confinement off entirely, which is
    equivalent to granting the client local filesystem access with this
    process's privileges; it exists so that is a deliberate, visible choice
    rather than the default.
    """
    if os.environ.get("QET_MCP_ALLOW_ANY_PATH") == "1":
        return []
    raw = os.environ.get("QET_MCP_WORKSPACE", "")
    parts = [p for p in raw.split(os.pathsep) if p.strip()] or [os.getcwd()]
    roots = []
    for part in parts:
        try:
            roots.append(Path(part).expanduser().resolve())
        except OSError:
            continue
    return roots


def _within_workspace(path: Path, roots: list) -> bool:
    for root in roots:
        try:
            if path == root or path.is_relative_to(root):
                return True
        except ValueError:
            continue
    return False


def _env_paths(name: str) -> list:
    """An os.pathsep-separated list of paths from the environment, resolved."""
    out = []
    for part in os.environ.get(name, "").split(os.pathsep):
        if part.strip():
            try:
                out.append(Path(part).expanduser().resolve())
            except OSError:
                continue
    return out


def _installation() -> tuple | None:
    """(program directory, element collection) of the QElectroTech this
    script was installed with, or None when it runs from anywhere else.

    Two layouts, both put there by QElectroTech's own packaging:
      <prefix>/share/qelectrotech/mcp/  -> <prefix>/bin, <prefix>/share/qelectrotech/elements
                                           (make install: Linux, snap, flatpak, macOS)
      <root>/mcp/                       -> <root>/bin, <root>/elements
                                           (the Windows installers and portable folder)
    """
    here = Path(__file__).resolve().parent
    if here.name != "mcp":
        return None
    if here.parent.name == "qelectrotech" and here.parent.parent.name == "share":
        prefix = here.parent.parent.parent
        return prefix / "bin", here.parent / "elements"
    if (here.parent / "bin").is_dir():
        return here.parent / "bin", here.parent / "elements"
    return None


def resolve_binary() -> Path | None:
    """The QElectroTech this server launches, found without asking the client.

    QET_BINARY first, then the install this script ships in, then
    qelectrotech on PATH. None when there is none; the tools that launch
    QElectroTech then say how to set it.
    """
    env = os.environ.get("QET_BINARY", "").strip()
    if env:
        return Path(env).expanduser().resolve()
    install = _installation()
    if install is not None:
        # The Windows build names it QElectroTech.exe; only a case-sensitive
        # file system tells the spellings apart.
        for name in ("qelectrotech", "qelectrotech.exe", "QElectroTech.exe"):
            cand = install[0] / name
            if cand.is_file():
                return cand.resolve()
    found = shutil.which("qelectrotech")
    return Path(found).resolve() if found else None


def default_elements_dir() -> Path | None:
    """The element collection of the install this script ships in, if any."""
    install = _installation()
    if install is not None and install[1].is_dir():
        return install[1].resolve()
    return None


def _check_binary(arguments: dict) -> None:
    """Fill in "binary", or refuse one that is not the server's own choice."""
    if os.environ.get("QET_MCP_ALLOW_ANY_BINARY") == "1" and arguments.get("binary"):
        return
    default = resolve_binary()
    raw = arguments.get("binary")
    if not raw:
        if default is None:
            raise ValueError(
                "no QElectroTech found: set QET_BINARY to the qelectrotech "
                "executable in the environment this server is started in")
        arguments["binary"] = str(default)
        return
    if not isinstance(raw, str):
        raise ValueError("'binary' must be a path")
    given = Path(raw).expanduser().resolve()
    allowed = ([default] if default else []) + _env_paths("QET_MCP_BINARIES")
    if given not in allowed:
        raise ValueError(
            f"'binary' is not an allowed QElectroTech: {given}. Leave it out "
            "to use " + (str(default) if default else "QET_BINARY")
            + "; whoever configured this server can list others in "
              "QET_MCP_BINARIES, or set QET_MCP_ALLOW_ANY_BINARY=1 to "
              "disable this check (which lets the client run any program).")
    arguments["binary"] = str(given)


def _check_elements_dir(arguments: dict, roots: list) -> None:
    """Fill in "elements_dir" from the install, or confine a given one."""
    raw = arguments.get("elements_dir")
    if not raw:
        default = default_elements_dir()
        if default is not None:
            arguments["elements_dir"] = str(default)
        return
    if not isinstance(raw, str):
        raise ValueError("'elements_dir' must be a path")
    given = Path(raw).expanduser().resolve()
    extra = _env_paths("QET_MCP_ELEMENTS")
    default = default_elements_dir()
    if default is not None:
        extra.append(default)
    if roots and not _within_workspace(given, roots + extra):
        raise ValueError(
            f"'elements_dir' is outside the workspace: {given}. Leave it out "
            "to use the installed collection, or list the directory in "
            "QET_MCP_ELEMENTS.")


def _check_path(raw, arg: str, mode: str, roots: list) -> Path:
    """Resolve one path and refuse it if it leaves the workspace.

    resolve() follows symlinks, so a link planted inside the workspace is
    judged by where it actually points, not by where it sits. A path that
    does not exist yet still resolves (its parents do), which is what makes
    this usable for an output file.
    """
    if not isinstance(raw, str) or not raw:
        raise ValueError(f"{arg!r} must be a non-empty path")
    resolved = Path(raw).expanduser().resolve()
    if roots and not _within_workspace(resolved, roots):
        raise ValueError(
            f"{arg!r} is outside the workspace: {resolved}. Allowed: "
            + os.pathsep.join(str(r) for r in roots)
            + ". Set QET_MCP_WORKSPACE to widen it, or "
              "QET_MCP_ALLOW_ANY_PATH=1 to disable this check "
              "(which grants this client local filesystem access)."
        )
    return resolved


def enforce_path_policy(tool_name: str, arguments: dict) -> None:
    """Apply the workspace, executable and overwrite policy to one tool call.

    For a tool that launches QElectroTech this also fills in "binary" and,
    when the install has one, "elements_dir", so a client need not know them.
    """
    spec = _DATA_PATHS.get(tool_name)
    if spec is None:
        return
    roots = workspace_roots()

    if tool_name in _LAUNCHES_QET or arguments.get(_LAUNCHES_QET_WITH.get(tool_name, "")):
        _check_binary(arguments)
        _check_elements_dir(arguments, roots)

    for arg in spec.get("read", ()):
        if arg in arguments:
            _check_path(arguments[arg], arg, "read", roots)

    for arg in spec.get("write", ()):
        if arg not in arguments:
            continue
        out = _check_path(arguments[arg], arg, "write", roots)
        # Writing over something that is already there is the one step this
        # server cannot undo, so it is the one step it will not take on its
        # own. qet_project_new already had this flag; the others now match it.
        if out.exists() and not arguments.get("overwrite"):
            raise ValueError(
                f"{arg!r} already exists: {out}. Pass \"overwrite\": true to "
                "replace it, or choose another name."
            )

    if tool_name == "qet_edit":
        for i, op in enumerate(arguments.get("operations") or []):
            if not isinstance(op, dict):
                continue
            key = _DATA_PATH_OPS.get(op.get("op"))
            if key and key in op:
                _check_path(op[key], f"operations[{i}].{key}", "read", roots)


# --------------------------------------------------------------------------
# JSON-RPC / MCP plumbing
# --------------------------------------------------------------------------

def _public(tool: dict) -> dict:
    return {k: v for k, v in tool.items() if k != "handler"}


def handle(msg: dict) -> dict | None:
    method = msg.get("method")
    mid = msg.get("id")

    if method == "initialize":
        want = (msg.get("params") or {}).get("protocolVersion")
        return _ok(mid, {
            "protocolVersion": want or DEFAULT_PROTOCOL,
            "capabilities": {"tools": {}},
            "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
            "instructions": SERVER_INSTRUCTIONS,
        })

    if method in ("notifications/initialized", "initialized"):
        return None  # notification: no reply

    if method == "ping":
        return _ok(mid, {})

    if method == "tools/list":
        return _ok(mid, {"tools": [_public(t) for t in TOOLS]})

    if method == "tools/call":
        params = msg.get("params") or {}
        name = params.get("name")
        tool = _BY_NAME.get(name)
        if tool is None:
            return _err(mid, -32602, f"unknown tool: {name}")
        try:
            arguments = params.get("arguments") or {}
            enforce_path_policy(name, arguments)
            result = tool["handler"](arguments)
            content = []
            # A tool may return a picture (qet_live_screenshot): sent as an
            # MCP image so the assistant can look at it, not as a string.
            image = result.pop("_image_png_base64", None) if isinstance(result, dict) else None
            if image:
                content.append({"type": "image", "data": image, "mimeType": "image/png"})
            text = json.dumps(result, indent=2, ensure_ascii=False)
            content.append({"type": "text", "text": text})
            return _ok(mid, {"content": content})
        except Exception as exc:  # surfaced to the model, not the transport
            return _ok(mid, {
                "isError": True,
                "content": [{"type": "text",
                             "text": f"{type(exc).__name__}: {exc}"}],
            })

    if mid is None:
        return None
    return _err(mid, -32601, f"method not found: {method}")


def _ok(mid, result):
    return {"jsonrpc": "2.0", "id": mid, "result": result}


def _err(mid, code, message):
    return {"jsonrpc": "2.0", "id": mid, "error": {"code": code, "message": message}}


def serve(stdin=sys.stdin, stdout=sys.stdout) -> None:
    for line in stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError as exc:
            print(json.dumps(_err(None, -32700, f"parse error: {exc}")),
                  file=stdout, flush=True)
            continue
        reply = handle(msg)
        if reply is not None:
            print(json.dumps(reply, ensure_ascii=False), file=stdout, flush=True)


def call_once(argv: list[str], stdin=sys.stdin, stdout=sys.stdout,
              stderr=sys.stderr) -> int:
    """--call <tool> [arguments]: one tools/call, the result's text on stdout.

    arguments is a JSON object, or "-" to read it from stdin (which spares
    the caller from quoting JSON for a shell). Exit status: 0 the tool
    succeeded, 1 the tool reported an error, 2 the call itself was malformed.
    """
    if not argv or len(argv) > 2:
        print("usage: qet_mcp.py --call <tool> ['<json arguments>' | -]",
              file=stderr)
        return 2
    name, raw = argv[0], (argv[1] if len(argv) == 2 else "{}")
    if raw == "-":
        raw = stdin.read()
    try:
        arguments = json.loads(raw) if raw.strip() else {}
    except json.JSONDecodeError as exc:
        print(f"arguments are not valid JSON: {exc}", file=stderr)
        return 2
    if not isinstance(arguments, dict):
        print("arguments must be a JSON object", file=stderr)
        return 2

    reply = handle({"jsonrpc": "2.0", "id": 1, "method": "tools/call",
                    "params": {"name": name, "arguments": arguments}})
    if "error" in reply:
        print(reply["error"]["message"], file=stderr)
        return 2
    result = reply["result"]
    for part in result["content"]:
        if part.get("type") == "image":
            # A picture has no text; print it whole, as a data: URI a
            # browser or a script can use, rather than drop it.
            print(f"data:{part['mimeType']};base64,{part['data']}", file=stdout)
        else:
            print(part["text"], file=stdout)
    return 1 if result.get("isError") else 0


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] in ("--list", "-l"):
        for t in TOOLS:
            print(f"{t['name']}\n    {t['description']}\n")
        return 0
    if len(sys.argv) > 1 and sys.argv[1] == "--call":
        return call_once(sys.argv[2:])
    serve()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
