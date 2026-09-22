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
from pathlib import Path

SERVER_NAME = "qet-mcp"
SERVER_VERSION = "0.1.0"
DEFAULT_PROTOCOL = "2025-06-18"

EXPORT_FORMATS = {
    "pdf": "--export-pdf",
    "png": "--export-png",
    "svg": "--export-svg",
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


def _conductors(root: ET.Element):
    for i, d in _folios(root):
        index = _terminal_index(d)
        for c in d.iter("conductor"):
            yield i, c, index


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


def _terminal_index(diagram: ET.Element) -> dict:
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
        for t in el.iter("terminal"):
            tid = t.get("id")
            if tid is None:
                continue
            index[tid] = (f"{uuid}@{t.get('x','?')},{t.get('y','?')}"
                          f",{t.get('orientation','?')}")
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
            ends.append(f"{owner}/{tid or c.get(name_attr, '?')}")
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
            "title": d.get("title", ""),
            "elements": sum(1 for _ in d.iter("element")),
            "conductors": sum(1 for _ in d.iter("conductor")),
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


def _extras(root: ET.Element) -> dict:
    """Everything a folio holds besides elements and conductors.

    Independent texts, shapes and images have no uuid, so nothing here can
    say "this is the same text, edited". A change to one therefore reads as
    the old one removed and a new one added, with both shown -- accurate,
    if less tidy than a move. Position is the identity they have.
    """
    folios, texts, shapes, images = {}, {}, {}, {}
    for n, d in _folios(root):
        folios[n] = {f: d.get(f, "") for f in _FOLIO_FIELDS}
        for t in d.iter("input"):
            key = (n, t.get("x", ""), t.get("y", ""), _plain_text(t.get("text", "")))
            texts[key] = {"rotation": t.get("rotation", "0"),
                          "font": t.get("font", ""), "color": t.get("color", "")}
        for sh in d.iter("shape"):
            pen, brush = sh.find("pen"), sh.find("brush")
            key = (n, sh.get("type", ""), sh.get("x1", ""), sh.get("y1", ""),
                   sh.get("x2", ""), sh.get("y2", ""))
            shapes[key] = {
                "line_color": pen.get("color", "") if pen is not None else "",
                "line_style": pen.get("style", "") if pen is not None else "",
                "line_width": pen.get("widthF", "") if pen is not None else "",
                "fill": (brush.get("color", "") if brush is not None and
                         brush.get("style", "") != "NoBrush" else "none"),
                "rotation": sh.get("rotation", "0")}
        for im in d.iter("image"):
            key = (n, im.get("x", ""), im.get("y", ""))
            images[key] = {"scale": im.get("size", ""), "rotation": im.get("rotation", "")}

    element_texts = {}
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
                element_texts[base + (seen[base],)] = {
                    "x": t.get("x", ""), "y": t.get("y", ""), "size": fs,
                    "frame": t.get("frame", ""), "rotation": t.get("rotation", ""),
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
    return {"folios": folios, "texts": texts, "shapes": shapes,
            "images": images, "strips": strips, "element_texts": element_texts}


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


def _diff_extras(before: ET.Element, after: ET.Element) -> dict:
    a, b = _extras(before), _extras(after)
    out = {}
    ta, tb = before.get("title", ""), after.get("title", "")
    out["project"] = {"changed": {"title": [ta, tb]} if ta != tb else {}}
    # Folios are keyed by position. A reorder or a removal in the middle
    # shifts every later index, so a folio "changing" its title alongside a
    # folio count change can just be the shift -- the count says which.
    folio_changes = []
    for n in sorted(set(a["folios"]) & set(b["folios"])):
        delta = {f: [a["folios"][n][f], b["folios"][n][f]] for f in _FOLIO_FIELDS
                 if a["folios"][n][f] != b["folios"][n][f]}
        if delta:
            folio_changes.append({"folio": n, "changed": delta})
    out["folios"] = {"before": len(a["folios"]), "after": len(b["folios"]),
                     "changed": folio_changes[:50]}
    if len(a["folios"]) != len(b["folios"]) and folio_changes:
        out["folios"]["note"] = ("the folio count changed, so changes listed here may be "
                                 "later folios shifting position rather than edits")
    out["texts"] = _diff_keyed(a["texts"], b["texts"],
                               lambda k: {"folio": k[0], "x": k[1], "y": k[2], "text": k[3]})
    out["shapes"] = _diff_keyed(a["shapes"], b["shapes"],
                                lambda k: {"folio": k[0], "type": k[1],
                                           "from": [k[2], k[3]], "to": [k[4], k[5]]})
    out["images"] = _diff_keyed(a["images"], b["images"],
                                lambda k: {"folio": k[0], "x": k[1], "y": k[2]})
    # Keyed by element, what the field is bound to, and the nth such field.
    # A field's own text is also compared ("shows"), so relabelling an
    # element shows up here as well as in the element's information.
    out["element_texts"] = _diff_keyed(
        a["element_texts"], b["element_texts"],
        lambda k: {"element": k[0], "source": k[1], "bound_to": k[2], "n": k[3]})
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
        a_el[r["uuid"] or f"{i}:{r['x']},{r['y']}:{r['name']}"] = r
    for i, e in _elements(_root(after)):
        r = _element_row(i, e)
        b_el[r["uuid"] or f"{i}:{r['x']},{r['y']}:{r['name']}"] = r

    moved, relabelled, changed_info = [], [], []
    for k, a in a_el.items():
        b = b_el.get(k)
        if b is None:
            continue
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
        if a["info"] != b["info"]:
            changed_info.append({"uuid": k, "name": a["name"],
                                 "from": a["info"], "to": b["info"]})

    a_co = {r["key"]: r for i, c, ix in _conductors(_root(before))
            for r in [_conductor_row(i, c, ix)]}
    b_co = {r["key"]: r for i, c, ix in _conductors(_root(after))
            for r in [_conductor_row(i, c, ix)]}
    # An end that could not be resolved to an element is keyed on the
    # folio-scoped integer id, which QElectroTech reassigns on every write.
    # Say so rather than presenting the result as if it were comparable:
    # in such a file an untouched conductor can read as removed and re-added.
    shaky = sum(1 for k in set(a_co) | set(b_co) if "#" in k)
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
        if fields:
            conductor_changes.append({"key": k, "changed": fields})

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
        },
        "conductors": {
            "before": len(a_co), "after": len(b_co),
            "added": sorted(set(b_co) - set(a_co))[:50],
            "removed": sorted(set(a_co) - set(b_co))[:50],
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
                  "name": t.get("name", ""), "type": t.get("type", "")}
                 for i, t in enumerate(ordered)]
    info_fields = sorted({(i.text or "").strip()
                          for i in root.iter("info_name") if (i.text or "").strip()})
    parts = {}
    desc = root.find("description")
    for child in (desc if desc is not None else []):
        parts[child.tag] = parts.get(child.tag, 0) + 1
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
    }


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
    application names main.cpp sets before this branch runs, so it must be
    QElectroTech/QElectroTech.conf and nothing else.

    script, when given, is written into the sandbox and passed to --run.
    It lives inside the temporary directory so it cannot collide with a
    concurrent call, and it is returned to the caller on failure, because a
    generated script nobody can see is not debuggable.
    """
    src = Path(binary).expanduser()
    if not src.is_file() or not os.access(src, os.X_OK):
        raise ValueError(f"not an executable: {src}")
    with tempfile.TemporaryDirectory(prefix="qet-mcp-") as tmp:
        sandbox = Path(tmp)
        exe = sandbox / f"qet-mcp-{os.getpid()}"
        shutil.copy2(src, exe)
        home = sandbox / "home"
        (home / ".config").mkdir(parents=True)
        (home / ".local" / "share").mkdir(parents=True)
        if elements_dir:
            coll = Path(elements_dir).expanduser()
            if not coll.is_dir():
                raise ValueError(f"no such elements directory: {coll}")
            cfg = home / ".config" / "QElectroTech"
            cfg.mkdir(parents=True, exist_ok=True)
            (cfg / "QElectroTech.conf").write_text(
                "[elements-collections]\n"
                f"common-collection-path={coll}\n", encoding="utf-8")
        if script is not None:
            script_path = sandbox / "qet-mcp-edit.js"
            script_path.write_text(script, encoding="utf-8")
            args = ["--run", str(script_path), *args]
        env = dict(os.environ,
                   HOME=str(home),
                   XDG_CONFIG_HOME=str(home / ".config"),
                   XDG_DATA_HOME=str(home / ".local" / "share"),
                   QT_QPA_PLATFORM="offscreen")
        try:
            p = subprocess.run([str(exe), *args], env=env, timeout=timeout,
                               capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            return {"ok": False, "timed_out": True, "timeout_s": timeout,
                    "hint": "a modal dialog during load will hang a headless "
                            "run; check the project's format version"}
        return {"ok": p.returncode == 0, "exit_code": p.returncode,
                "stdout": p.stdout[-tail:], "stderr": p.stderr[-tail:]}


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
    allowed = set(spec["required"]) | set(spec["optional"]) | {"type", "style", "antialias"}
    for key in part:
        if key not in allowed:
            raise ValueError(f"part {index} ({kind}): unexpected {key!r}; "
                             f"allowed: {', '.join(sorted(allowed))}")
    if kind == "polygon":
        pts = part["points"]
        if not isinstance(pts, list) or len(pts) < 2:
            raise ValueError(f"part {index} (polygon) needs at least two points")
        for pt in pts:
            if not (isinstance(pt, (list, tuple)) and len(pt) == 2):
                raise ValueError(f"part {index} (polygon): each point is [x, y], got {pt!r}")
    return kind


def _part_element(part: dict) -> ET.Element:
    kind = part["type"]
    node = ET.Element(kind)
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
    for part in parts:
        description.append(_part_element(part))
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
                                                 ("from", "elmt"), ("from_terminal", "num"),
                                                 ("to", "elmt"), ("to_terminal", "num")]),
    "delete_element":   ("deleteElement",       [("folio", "folio"), ("element", "elmt")]),
    "set_conductor":    ("setConductorProperty", [("folio", "folio"), ("element", "elmt"),
                                                  ("terminal", "num"), ("property", "str"),
                                                  ("value", "str")]),
    "move_conductor_segment": ("moveConductorSegment", [("folio", "folio"), ("element", "elmt"),
                                                         ("terminal", "num"), ("segment", "num"),
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
    # Texts and shapes have no uuid; they are addressed by index into a
    # position-sorted listing, and add_text/add_shape return that index so
    # it can be named as "$id". Indexes shift when one is added or deleted.
    "delete_conductor": ("deleteConductor",     [("folio", "folio"), ("element", "elmt"),
                                                 ("terminal", "num")]),
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
                                                       ("index", "folio"), ("property", "str"),
                                                       ("value", "str")]),
    "delete_element_text": ("deleteElementText",      [("folio", "folio"), ("element", "elmt"),
                                                       ("index", "folio")]),
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
    "set_text":         ("setTextContent",      [("folio", "folio"), ("index", "folio"),
                                                 ("text", "str")]),
    "set_text_color":   ("setTextColor",        [("folio", "folio"), ("index", "folio"),
                                                 ("color", "str")]),
    "rotate_text":      ("setTextRotation",     [("folio", "folio"), ("index", "folio"),
                                                 ("angle", "num")]),
    "delete_text":      ("deleteText",          [("folio", "folio"), ("index", "folio")]),
    "add_shape":        ("addShape",            [("folio", "folio"), ("shape", "str"),
                                                 ("x1", "num"), ("y1", "num"),
                                                 ("x2", "num"), ("y2", "num")]),
    "set_shape":        ("setShapeProperty",    [("folio", "folio"), ("index", "folio"),
                                                 ("property", "str"), ("value", "str")]),
    "add_image":        ("addImage",            [("folio", "folio"), ("file", "str"),
                                                 ("x", "num"), ("y", "num")]),
    "add_pdf_page":     ("addPdfPage",           [("folio", "folio"), ("file", "str"),
                                                 ("page", "num"), ("dpi", "num"),
                                                 ("x", "num"), ("y", "num")]),
    "scale_image":      ("setImageScale",       [("folio", "folio"), ("index", "folio"),
                                                 ("factor", "num")]),
    "rotate_image":     ("setImageRotation",    [("folio", "folio"), ("index", "folio"),
                                                 ("angle", "num")]),
    "delete_image":     ("deleteImage",         [("folio", "folio"), ("index", "folio")]),
    "delete_shape":     ("deleteShape",         [("folio", "folio"), ("index", "folio")]),
    "add_polygon":      ("addPolygon",          [("folio", "folio"), ("points", "points"),
                                                 ("closed", "bool")]),
    "set_shape_polygon": ("setShapePolygon",    [("folio", "folio"), ("index", "folio"),
                                                 ("points", "points")]),
    "add_path":         ("addPath",             [("folio", "folio"), ("nodes", "nodes"),
                                                 ("closed", "bool")]),
    "set_shape_path_nodes": ("setShapePathNodes", [("folio", "folio"), ("index", "folio"),
                                                   ("nodes", "nodes")]),
    "set_shape_closed": ("setShapeClosed",      [("folio", "folio"), ("index", "folio"),
                                                 ("closed", "bool")]),
    "add_table":        ("addTable",            [("folio", "folio"), ("kind", "str"),
                                                 ("name", "str"), ("query", "str")]),
    "set_table_position": ("setTablePosition",  [("folio", "folio"), ("table", "folio"),
                                                 ("x", "num"), ("y", "num")]),
    "delete_table":     ("deleteTable",         [("folio", "folio"), ("table", "folio")]),
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
    lines = [
        "// generated by qet-mcp; do not edit",
        "var R = {};",                       # $name -> value from an earlier op
        "var missing = [];",
        f"var need = {_js(_REQUIRED_METHODS)};",
        "for (var i = 0; i < need.length; i++) {",
        "  if (typeof qet[need[i]] !== 'function') missing.push(need[i]);",
        "}",
        f"qet.log({_js(_MARKER)} + JSON.stringify("
        "{kind: 'capabilities', missing: missing}));",
        "var stop = false;",
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
        if kind == "folio":
            if not isinstance(value, int) or isinstance(value, bool):
                raise ValueError(f"operation {op_index}: {key!r} must be a folio index "
                                 f"or a \"$name\" reference, got {value!r}")
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
        args = []
        for key, kind in spec:
            if key not in op:
                raise ValueError(f"operation {i} ({name}) is missing {key!r}")
            args.append(ref_or(op[key], kind, i, key))

        ident = op.get("id")
        if ident is not None:
            if not isinstance(ident, str) or not ident or ident.startswith("$"):
                raise ValueError(f"operation {i}: \"id\" must be a non-empty name "
                                 f"without a leading $, got {ident!r}")
            if ident in refs:
                raise ValueError(f"operation {i}: \"id\" {ident!r} is already used")

        call = "qet.addFolio()" if method is None else f"qet.{method}({', '.join(args)})"
        lines.append("  if (!stop) {")
        lines.append(f"  var v{i} = {call};")
        if ident is not None:
            lines.append(f"  R[{_js(ident)}] = v{i};")
            refs.add(ident)
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
    return "\n".join(lines) + "\n"


def _parse_script_output(text: str) -> dict:
    """Read the marker lines the generated script emits.

    They arrive on stderr, not stdout: QetScriptApi::log() is a
    QTextStream(stderr). Read both anyway rather than depending on that --
    the cost is nothing and the failure it prevents is silent (an edit that
    worked, reported as having run no operations at all, which is what the
    first version of this tool did)."""
    caps, ops, saved, stopped = None, [], None, False
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
        elif rec.get("kind") == "save":
            saved = bool(rec.get("result"))
            stopped = bool(rec.get("stopped_early"))
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
    return {"ok": not errors and not any(f["severity"] == "error" for f in findings),
            "summary": {"errors": sum(f["severity"] == "error" for f in findings),
                        "warnings": sum(f["severity"] == "warning" for f in findings),
                        "info": sum(f["severity"] == "info" for f in findings),
                        "passed": len(passed), "check_failures": len(errors)},
            "findings": findings, "passed": passed, "check_failures": errors}


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
        result["hint"] = ("QElectroTech did not write the project; this build's scripting "
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
        result["hint"] = ("the binary never ran the script (no capability report came "
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
            result.setdefault("hint",
                              f"operation {record['index']} ({record['op']}) returned "
                              f"{record['result']!r}; later operations were skipped. "
                              "qet.log lines in stderr/stdout say why.")
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


TOOLS = [
    {
        "name": "qet_project_info",
        "description": "Summarise a .qet project: title, format version, folios, "
                       "and element/conductor counts per folio. Reads the file "
                       "directly; does not launch QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string", "description": "path to a .qet file"}},
            "required": ["path"],
        },
        "handler": lambda a: tool_project_info(a["path"]),
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
                "folio": {"type": "integer"},
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
                       "by what delta, which were added, removed or relabelled, and "
                       "which conductor fields changed. Use this to verify what an "
                       "edit actually did, rather than reading a screenshot.",
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
        "description": "Run a QElectroTech export headlessly (pdf, png, svg, bom, "
                       "cables, wires, wiring, nets, links, info). Launches the "
                       "binary in an isolated sandbox so it cannot be captured by, "
                       "or capture, a running QElectroTech.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
                "project": {"type": "string"},
                "format": {"type": "string", "enum": sorted(EXPORT_FORMATS)},
                "output": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["binary", "project", "format", "output"],
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
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
                "project": {"type": "string", "description": "the .qet to start from; not modified"},
                "output": {"type": "string", "description": "where to write the edited project"},
                "operations": {
                    "type": "array",
                    "minItems": 1,
                    "description":
                        "Operations applied in order. Each is an object with \"op\" "
                        "and that op's arguments. Ops: " + ", ".join(sorted(OPS)) + ". "
                        "Give an op an \"id\" to name what it produced, then refer to "
                        "it later as \"$id\" -- that is how an element placed by "
                        "add_element gets wired by add_conductor, and how a folio made "
                        "by add_folio is addressed. Terminals are numbered by their "
                        "index in the element definition; qet_element_info lists them. "
                        "set_conductor addresses a conductor as the one on a given "
                        "terminal and applies the change to its whole electrical "
                        "potential, so name a terminal carrying exactly one conductor; "
                        "its \"property\" is one of " + ", ".join(CONDUCTOR_PROPERTIES) +
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
                        "Texts and shapes have no uuid: add_text/add_shape return an "
                        "index you can name as \"$id\", and the other text/shape ops "
                        "take it as \"index\". Indexes shift when one is added or "
                        "deleted. Shapes: " + ", ".join(SHAPES) + "; set_shape takes " + ", ".join(SHAPE_PROPERTIES) +
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
                        "edited in the same run. "
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
            "required": ["binary", "project", "output", "operations"],
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
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
                "project": {"type": "string", "description": "the .qet to query; never modified"},
                "sql": {"type": "string",
                        "description": "a single SELECT or WITH...SELECT. "
                                       "Omit to list the tables and views instead."},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["binary", "project"],
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
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
                "project": {"type": "string", "description": "the .qet to check; never modified"},
                "folio": {"type": "integer", "description": "check one folio only; omit for the whole project"},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["binary", "project"],
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
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
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
            "required": ["binary", "output", "title"],
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
                "binary": {"type": "string", "description": "path to the qelectrotech executable"},
                "project": {"type": "string"},
                "checks": {"type": "array", "items": {"type": "string", "enum": sorted(CHECKS)},
                           "description": "which checks to run; omit for all"},
                "sample": {"type": "integer", "default": 10,
                           "description": "how many offending rows to return per check"},
                "elements_dir": {"type": "string"},
                "timeout": {"type": "integer", "default": 180},
            },
            "required": ["binary", "project"],
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
                        "size, rotation, color. Any part may carry style and antialias. "
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
]

_BY_NAME = {t["name"]: t for t in TOOLS}


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
            result = tool["handler"](params.get("arguments") or {})
            text = json.dumps(result, indent=2, ensure_ascii=False)
            return _ok(mid, {"content": [{"type": "text", "text": text}]})
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


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] in ("--list", "-l"):
        for t in TOOLS:
            print(f"{t['name']}\n    {t['description']}\n")
        return 0
    serve()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
