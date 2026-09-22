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
    QElectroTech -- which is the common case for "what did that edit change",
    so the conductor half of the diff was noise precisely when it was needed.

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


def tool_element_info(path: str) -> dict:
    """Introspect a .elmt: names, terminals, and which info fields it carries."""
    root = _root(path)
    names = {n.get("lang"): (n.text or "") for n in root.iter("name")}
    terminals = [{"x": t.get("x"), "y": t.get("y"),
                  "orientation": t.get("orientation"),
                  "name": t.get("name", ""), "type": t.get("type", "")}
                 for t in root.iter("terminal")]
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
        "info_fields": info_fields,
        "parts": parts,
    }


def _run_qet(binary: str, args: list[str], timeout: int = 180) -> dict:
    """Launch QElectroTech headlessly, carrying the known launch traps.

    SingleApplication keys its socket on applicationFilePath(), so a second
    launch of the same path forwards its request to an already-running
    instance and returns THAT process's answer with no error. Copying the
    binary to a unique path gives this run its own socket. A symlink will
    not do: applicationFilePath() resolves it back.
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
                "stdout": p.stdout[-4000:], "stderr": p.stderr[-4000:]}


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
