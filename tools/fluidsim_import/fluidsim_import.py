"""Import FluidSIM 6 circuits (.circ) as a QElectroTech project (.qet).

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

A .circ is gzip-packed XML; every circuit becomes a folio. Imported are:
- every part with connectors (pneumatic, electric, hydraulic, digital) as an embedded QElectroTech element with
  OWN graphics after ISO 1219-1 / IEC 60617 (own_graphics.py): only the connector positions and the data are
  taken from the file, never the FluidSIM symbols, so the result holds no FluidSIM graphics and the import works
  without FluidSIM; with the rotation, mirroring and scale of the part in the circuit;
- the part's English description as its name, and the name in another language from FluidSIM's own translation
  when an installation is found (--fluidsim, --language);
- the label (-MM1, -QM1, -MB1 ...), and the values FluidSIM shows at the part (24 V, p = 6 bar ...) as texts at the
  element and in its comment; valves, cylinders and manual switches without a label get the next free one after
  IEC 81346-2 (-QM1, -MM1, -SF1 ...);
- every wire as a QElectroTech conductor between the same terminals (QElectroTech lays out the path); branches
  (T distributors) become conductors of the same net; shafts between a motor and its load as a line;
- silencers and blind plugs drawn at the port they close;
- valve solenoid <-> valve (reference at the valve connector, or the solenoid label at the actuation) and relay
  coil <-> contacts (same label) as master/slave links, also across folios;
- GRAFCET charts (steps, transitions, actions, branches), see fluidsim_grafcet.py;
- the program of a LOGO module on a folio of its own;
- older files with packed coordinates.

Not imported: frame and title block, tables, state diagrams, parts lists, distance rules (only their marks would
be data), simulation values. FluidSIM stores no article data (manufacturer, order number).

    python3 fluidsim_import.py CIRCUIT.circ [MORE.circ ...] OUTPUT.qet [--qet /path/to/qelectrotech]
    python3 fluidsim_import.py FOLDER_WITH_CIRC OUTPUT.qet          # one folio per .circ, sorted by name
    python3 fluidsim_import.py CIRCUIT.circ OUTPUT.qet --report report.json

--stop-on-eof (QElectroTech's import passes it): when standard input closes (Cancel), stop the QElectroTech runs
this program started, remove the temporary folders and exit with code 3.

In QElectroTech: File > Import FluidSIM circuit (sources/import/fluidsim/fluidsimimport.cpp).

QElectroTech itself writes the project, through its headless scripting (--run, the same way as the MCP server's
qet_edit in misc/qet-mcp); for this one call the tool sets QET_ENABLE_SCRIPTING=1. Needs Python 3.8 or later and
nothing beyond its standard library.
"""
import argparse
import gzip
import hashlib
import json
import os
import re
import sys
import tempfile
import threading
import time
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
import fluidsim_elements as fe  # noqa: E402
import fluidsim_grafcet  # noqa: E402
import own_graphics  # noqa: E402


def _first(*paths):
    """The first existing path: in the source tree (tools/..., misc/...) or next to an installed QElectroTech."""
    return next((p for p in paths if p.exists()), paths[0])


MCP = _first(REPO / "misc" / "qet-mcp", HERE.parent / "mcp")
GRAFCET_SYMBOLS = HERE / "grafcet"
if sys.platform == "win32":
    FLUIDSIM = Path("C:/Program Files (x86)/Didactic/FluidSIM6")
else:
    FLUIDSIM = Path.home() / ".wine" / "drive_c" / "Program Files (x86)" / "Didactic" / "FluidSIM6"
SKIP = ("Grafcet.",)   # object kinds that are not parts (GRAFCET: fluidsim_grafcet.py)
BRANCH = ("Interface.MultiPin", "Digital.MultiPin")   # T distributor: no element in QElectroTech, only a net


def read(path: Path):
    raw = path.read_bytes()
    if raw[:2] == b"\x1f\x8b":
        raw = gzip.decompress(raw)
    root = fe.unpack_coordinates(ET.fromstring(raw))
    # some files pack their sheet in a section "circuits" with a <file>; in all samples exactly one. Its
    # sections are those of an ordinary circuit.
    group = next((s for s in root.findall("section") if s.get("type") == "circuits"), None)
    if group is not None and section(root, "circuit-objects") is None:
        sheet = group.find("file")
        if sheet is not None:
            return sheet
    return root


def shaft_connectors(symbols, obj):
    """Idents of the shaft connectors (FluidSIM type "rotational") of an object."""
    symbol = symbols.get(obj.get("symbol-ref"))
    if symbol is None:
        return set()
    shaft = {(layer.get("ident", ""), c.get("ident")) for layer in symbol.findall("symbol-layer")
             for c in layer.findall("connector") if c.get("type") == "rotational"}
    # the connections name the part's connector (MR); con-ref points to layer.ident in the symbol
    out = {c.get("ident") for c in obj.findall("connector")
           if tuple(fe.connector_key(obj, c.get("ident"))) in shaft}
    return out or {i for _, i in shaft}


def shaft_ops(folio, symbols, by_id, connections, here, dx, dy):
    """Mechanical couplings (shaft motor <-> load) as a line between the shaft connectors. QElectroTech has no
    shaft terminal; a conductor would be an electrical net."""
    ops = []
    for v in connections:
        ends = []
        for ref in (v.get("con1", ""), v.get("con2", "")):
            oid, _, x = ref.partition(".")
            o = by_id.get(oid)
            if o is None or oid not in here or x not in shaft_connectors(symbols, o):
                break
            pos = connector_position(symbols, o, x)
            if pos is None:
                break
            ends.append((round(pos[0] * fe.SCALE + dx, 1), round(pos[1] * fe.SCALE + dy, 1)))
        if len(ends) == 2 and ends[0] != ends[1]:
            (x1, y1), (x2, y2) = ends
            ops.append({"op": "add_shape", "folio": folio, "shape": "line", "x1": x1, "y1": y1, "x2": x2, "y2": y2})
    return ops


class Sheet:
    """A QElectroTech folio from a FluidSIM circuit: its main window or a LOGO program."""

    def __init__(self, path: Path, root, suffix=""):
        self.path = path
        self.name = path.name + (f" ({suffix})" if suffix else "")
        self.stem = path.stem + (f" {suffix}" if suffix else "")
        self.root = root


def split_logo(root):
    """FluidSIM shows the program of a LOGO module (Digital.Logo.Logobox) in a window of its own with its own
    coordinates; its objects carry logo-box-ref. On a folio with the circuit they would lie over valves and
    cylinders. @return [(name, root)] per LOGO module; the objects are removed from root."""
    co = section(root, "circuit-objects")
    if co is None:
        return []
    per_box = {}
    for c in list(co):
        if c.get("logo-box-ref"):
            per_box.setdefault(c.get("logo-box-ref"), []).append(c)
    if not per_box:
        return []
    boxes = {o.get("ident", "").lstrip("O"): o for o in co.iter("component")
             if (o.get("object-ref") or "").endswith("Logobox")}
    out = []
    for n, (ref, parts) in enumerate(sorted(per_box.items()), 1):
        ids = {e.get("ident") for t in parts for e in t.iter() if e.get("ident")}
        links = [v for v in co.findall("connect")
                 if v.get("con1", "").partition(".")[0] in ids and v.get("con2", "").partition(".")[0] in ids]
        new = ET.Element(root.tag, root.attrib)
        for sec in root.findall("section"):
            if sec is co:
                sec2 = ET.SubElement(new, "section", sec.attrib)
                sec2.extend(parts + links)
            else:
                new.append(sec)
        for t in parts + links:
            co.remove(t)
        box = boxes.get(ref)
        label = box.find("dki-data/designation") if box is not None else None
        name = "LOGO"
        if label is not None and label.get("value") and not label.get("value").startswith("?"):
            name += " " + label.get("value")
        elif len(per_box) > 1:
            name += f" {n}"
        out.append((name, new))
    return out


def sheets(circuits):
    """The folios for the circuits: per circuit its main window, followed by its LOGO programs."""
    out = []
    for p in circuits:
        root = read(p)
        logo = split_logo(root)
        out.append(Sheet(p, root))
        out += [Sheet(p, lr, name) for name, lr in logo]
    return out


def section(root, kind):
    return next((s for s in root.findall("section") if s.get("type") == kind), None)


def circuit_objects(root):
    """All circuit objects with a symbol, also inside groups."""
    co = section(root, "circuit-objects")
    if co is None:
        return []
    return [o for o in co.iter() if o.tag in ("component", "terminal") and o.get("symbol-ref")]


def connector_position(symbols, obj, connector):
    """Position of a connector of an object on the sheet (FluidSIM units), or None."""
    layer_name, ident = fe.connector_key(obj, connector)
    symbol = symbols.get(obj.get("symbol-ref"))
    if symbol is None:
        return None
    for layer in symbol.findall("symbol-layer"):
        if layer.get("ident", "") != layer_name:
            continue
        for con in layer.findall("connector"):
            if con.get("ident") == ident:
                lx = fe.number(layer, "ref-x") + fe.number(layer, "scale-x", 1.0) * fe.number(con, "cx")
                ly = fe.number(layer, "ref-y") + fe.number(layer, "scale-y", 1.0) * fe.number(con, "cy")
                q = fe.Transform.of(obj).point(lx, ly)
                m = fe.centre(obj)
                return (m[0] + q[0], m[1] + q[1])
    return None


def file_name(name, seed):
    """A readable name of the embedded element file: the part's name and a piece of the checksum."""
    short = re.sub(r"[^\w\-]+", "_", name, flags=re.UNICODE).strip("_")[:40] or "part"
    return f"{short}_{seed[-8:]}.elmt"


def visible_values(obj, text_boxes):
    """The parameters FluidSIM shows at the part (^#..., e.g. "p0 = 6 bar") with their text box."""
    out = []
    for a in obj.iter("attribute"):
        name, ref = a.get("name", ""), a.get("display-ref")
        if not name.startswith("^#") or not ref:
            continue
        if "FTPVar" in (a.get("comment") or ""):
            continue    # a simulated quantity (P_out = 0 kW ...), not a value of the plan
        tb = text_boxes.get(ref)
        value = (tb.get("value") if tb is not None else None) or a.get("value") or ""
        numbers = re.findall(r"\d+(?:[.,]\d+)?", value)
        if numbers and all(float(z.replace(",", ".")) == 0 for z in numbers):
            continue    # a set point of 0 says nothing in the plan
        if value.strip() and tb is not None and tb.get("visible") != "false":
            out.append((value.strip(), tb))
    return out


def solenoids(symbol, obj):
    """Valve solenoids at the valve: [(side, label)] from the layers AEML.../AEMR... (connector electric-label).
    With a mirrored or 180 degrees rotated valve the sides are swapped as in the drawing."""
    swap = fe.Transform.of(obj).swaps_sides()
    out = []
    for layer in symbol.findall("symbol-layer"):
        name = layer.get("ident") or ""
        if not name.startswith(("AEML", "AEMR")):
            continue
        side = "left" if name.startswith("AEML") else "right"
        if swap:
            side = "right" if side == "left" else "left"
        for con in layer.iter("connector"):
            if con.get("type") == "electric-label" and con.get("designation"):
                out.append((side, con.get("designation")))
    return out


def manual_switch(obj_ref, cls):
    """True for a push button or a switch operated by hand."""
    return cls == "contact-manual" and "Changeover" not in obj_ref


# paper, smallest first (QElectroTech template: frame 96 dpi, header row/column 20, title block at the bottom 50)
PAPER = (("a4-landscape", 297, 210), ("a4-portrait", 210, 297), ("a3-landscape", 420, 297),
         ("a3-portrait", 297, 420), ("a2-landscape", 594, 420), ("a1-landscape", 841, 594),
         ("a0-landscape", 1189, 841))
MARGIN = 50.0        # distance of the circuit from the edge of the folio (with the header)
TITLE_BLOCK = 52.0   # height of the title block at the bottom


def folio_placement(objects, element_ops):
    """(dx, dy, paper): the shift of the elements (a multiple of 10, so straight wires stay straight) that makes
    the circuit start at MARGIN, and the smallest paper it then fits on."""
    boxes = [(fe.number(o, "x1") * fe.SCALE, fe.number(o, "y1") * fe.SCALE,
              (fe.number(o, "x1") + fe.number(o, "width")) * fe.SCALE,
              (fe.number(o, "y1") + fe.number(o, "height")) * fe.SCALE)
             for o in objects if fe.number(o, "width") > 0]
    x0 = min([k[0] for k in boxes] + [op["x"] for op in element_ops])
    y0 = min([k[1] for k in boxes] + [op["y"] for op in element_ops])
    x1 = max([k[2] for k in boxes] + [op["x"] for op in element_ops])
    y1 = max([k[3] for k in boxes] + [op["y"] for op in element_ops])
    dx = 10.0 * round((MARGIN - x0) / 10.0)
    dy = 10.0 * round((MARGIN - y0) / 10.0)
    width, height = x1 + dx + MARGIN, y1 + dy + MARGIN + TITLE_BLOCK
    for name, w, h in PAPER:
        if width <= w / 25.4 * 96 and height <= h / 25.4 * 96:
            return dx, dy, name
    return dx, dy, PAPER[-1][0]


def plan(circuits, collection: Path, translation, language="en", route="default"):
    """circuits: the folios (sheets()). Writes the elements into collection.
    @return the operations for qet_edit and a summary."""
    ops = []
    count = {"parts": 0, "wires": 0, "links": 0, "texts": 0, "skipped": 0, "skipped_objects": [],
             "labels_added": 0, "steps": 0, "transitions": 0, "actions": 0}
    not_imported = []
    buttons, manual_valves = [], []
    # all labels from FluidSIM first, so a label given here never equals a later one
    taken = set()
    for circ in circuits:
        for o in circuit_objects(circ.root):
            k = o.find("dki-data/designation")
            if k is not None and k.get("value") and not k.get("value").startswith("?"):
                taken.add(k.get("value"))
    parts = []   # dict: folio, id, label, link, classification, targets [(comp-ref, label)], ident

    for folio, circ in enumerate(circuits):
        root = circ.root
        graphics = section(root, "symbol-graphics")
        if graphics is None:
            continue
        border = {"op": "set_folio_border", "folio": folio, "property": "preset", "value": "a3-landscape"}
        ops.append(border)
        # do not show empty conductor texts (else every wire carries a dash)
        ops.append({"op": "set_conductor_default", "folio": folio, "property": "displaytext", "value": "false"})
        symbols = {s.get("ident"): s for s in graphics.findall("symbol")}
        objects = circuit_objects(root)
        connections = list(root.iter("connect"))
        by_id = {o.get("ident"): o for o in objects}
        co = section(root, "circuit-objects")
        text_boxes = {t.get("ident"): t for t in co.iter("text-box")} if co is not None else {}
        here = {}       # FluidSIM ident -> part of this folio

        for o in objects:
            obj_ref = o.get("object-ref") or ""
            if o.tag == "component" and (not obj_ref or obj_ref in fe.TERMINATIONS or obj_ref.startswith(SKIP)
                                         or obj_ref.endswith(BRANCH) or obj_ref.startswith("Mech.")):
                # branches become conductors of the same net, distance rules are only marks
                continue
            descr = o.find("dki-data/description")
            en = (descr.get("value") if descr is not None else "") or obj_ref
            if not en:
                # the terminals of a terminal strip have no description
                en = "Terminal" if o.tag == "terminal" else "FluidSIM part"
            local = translation.translate(en, language) if language != "en" else en
            shown = local or en
            # own graphics instead of the FluidSIM symbols; only the connectors and the data remain
            source = symbols
            if o.get("symbol-ref") in symbols:
                source = dict(symbols)
                source[o.get("symbol-ref")] = own_graphics.replace(
                    symbols[o.get("symbol-ref")], o, fe.classification(o),
                    own_graphics.valve_positions(symbols[o.get("symbol-ref")]))
            # two passes: the UUIDs follow the content, so equal parts become one element
            names = (en, local, language)
            trial = fe.convert_component(source, o, objects, connections, names, circ.name, "trial", placed=True)
            if trial is None or (not trial["uuids"] and not shaft_connectors(symbols, o)):
                # parts without a fluid or electric connector are animations or hardware couplings (EasyPort);
                # with a shaft connector (load at a motor) they stay, shaft_ops draws the shaft
                count["skipped"] += 1
                count["skipped_objects"].append(obj_ref or o.tag)
                continue
            seed = "plan/" + hashlib.sha1(trial["text"].encode("utf-8")).hexdigest()
            res = fe.convert_component(source, o, objects, connections, names, circ.name, seed, placed=True)
            path = collection / file_name(en, seed)
            path.write_text(res["text"], encoding="utf-8", newline="\n")
            m = fe.centre(o)
            x = round((m[0] + res["anchor"][0]) * fe.SCALE, 1)
            y = round((m[1] + res["anchor"][1]) * fe.SCALE, 1)
            oid = f"b{folio}_{o.get('ident')}"
            ops.append({"op": "add_element", "id": oid, "folio": folio, "path": f"common://{path.name}",
                        "x": x, "y": y})
            el = "$" + oid
            k = o.find("dki-data/designation")
            label = k.get("value", "") if k is not None else ""
            if label.startswith("?"):
                label = ""
            kind = res["kind"]
            semi_rotary = "SemiRotaryMotor" in obj_ref
            manual = manual_switch(obj_ref, res["classification"])
            # roller lever valves carry their label (the distance mark) at the actuation (mechanical-label)
            mark = next((c.get("designation") for c in symbols[o.get("symbol-ref")].iter("connector")
                         if c.get("type") == "mechanical-label" and c.get("designation")), "")
            if not label and kind == "valve" and mark:
                label = mark
            proportional = obj_ref.startswith("Hyd.Proportional.") and "Valve" in obj_ref
            if not label and (kind in ("valve", "cylinder") or semi_rotary or proportional or manual):
                # the next free label after IEC 81346-2 (-QM1, -MM1, -SF1 ...)
                prefix = ("-QM" if kind == "valve" or proportional else "-MM" if kind == "cylinder" or semi_rotary
                          else "-SF")
                n = 1
                while f"{prefix}{n}" in taken:
                    n += 1
                label = f"{prefix}{n}"
                count["labels_added"] += 1
            if label:
                taken.add(label)
                ops.append({"op": "set_label", "folio": folio, "element": el, "label": label})
            ops.append({"op": "set_info", "folio": folio, "element": el, "key": "description", "value": shown})
            if label and manual:
                buttons.append(label)
            if label and kind == "valve" and res["manual"]:
                manual_valves.append(label)
            # a value that reads like the label ("24 V" at the voltage source) is there already
            values = [(v, tb) for v, tb in visible_values(o, text_boxes) if v.replace(" ", "") != label.replace(" ", "")]
            if values:
                ops.append({"op": "set_info", "folio": folio, "element": el, "key": "comment",
                            "value": "; ".join(v for v, _ in values)})
            for value, tb in values:
                # in element coordinates: the element stands with its origin at (x, y)
                ops.append({"op": "add_element_text", "folio": folio, "element": el, "source": "text",
                            "value": value, "x": round(fe.number(tb, "x1") * fe.SCALE - x, 1),
                            "y": round(fe.number(tb, "y1") * fe.SCALE - y, 1)})
                count["texts"] += 1
            targets = []      # (comp-ref, label)
            for con in o.findall("connector"):
                link_kind = con.find("link-props/type")
                target = con.find("link-props/target")
                if (link_kind is not None and target is not None
                        and link_kind.get("value") == "solenoid-valve-coil-target"):
                    targets.append((target.get("comp-ref", ""), target.get("value", "")))
            for _, lbl in solenoids(symbols[o.get("symbol-ref")], o):
                if not any(t[1] == lbl for t in targets):
                    targets.append(("", lbl))
            b = {"folio": folio, "id": oid, "label": label, "link": res["link"], "uuids": res["uuids"],
                 "classification": res["classification"], "targets": targets, "ident": o.get("ident")}
            parts.append(b)
            here[o.get("ident")] = b
            count["parts"] += 1

        # GRAFCET of this folio
        g_ops, g_count, g_open = fluidsim_grafcet.convert(folio, objects, connections, collection, fe.SCALE,
                                                          fe.centre)
        ops += g_ops
        for k in ("steps", "transitions", "actions"):
            count[k] += g_count[k]
        not_imported += [f"{circ.name}: {x}" for x in g_open]

        # shift the whole folio so it starts at the top left of the frame, and choose the smallest paper it fits
        # on: FluidSIM draws its samples small in a corner of an A4 sheet (or at negative coordinates), on A3
        # they would be a spot in the corner. Texts are relative to their element.
        element_ops = [op for op in ops if op["op"] == "add_element" and op["folio"] == folio]
        if element_ops:
            dx, dy, border["value"] = folio_placement(objects + list(text_boxes.values()), element_ops)
            for op in element_ops:
                op["x"] = round(op["x"] + dx, 1)
                op["y"] = round(op["y"] + dy, 1)
            ops += shaft_ops(folio, symbols, by_id, connections, here, dx, dy)

        # the wires of this folio
        def terminal_uuid(ref):
            oid, _, x = ref.partition(".")
            b = here.get(oid)
            if b is None:
                return None, None
            return b["id"], b["uuids"].get(fe.connector_key(by_id[oid], x))

        # branches (Interface.MultiPin) are no element in QElectroTech: all connectors joined through one or
        # more branches form a net and are connected one after the other
        def is_branch(ref):
            o = by_id.get(ref.partition(".")[0])
            return o is not None and (o.get("object-ref") or "").endswith(BRANCH)

        parent = {}

        def root_of(k):
            while parent.setdefault(k, k) != k:
                parent[k] = parent[parent[k]]
                k = parent[k]
            return k

        in_net = set()
        pairs = []
        for v in connections:
            a, b2 = v.get("con1", ""), v.get("con2", "")
            va, vb = is_branch(a), is_branch(b2)
            if not va and not vb:
                pairs.append((a, b2))
                continue
            ka = a.partition(".")[0] if va else a
            kb = b2.partition(".")[0] if vb else b2
            parent[root_of(ka)] = root_of(kb)
            in_net.update((ka, kb))

        def position(ref):
            oid, _, x = ref.partition(".")
            return connector_position(symbols, by_id[oid], x) or (0.0, 0.0)

        nets = {}
        for k in in_net:
            if k not in by_id:          # not the branch itself
                nets.setdefault(root_of(k), []).append(k)
        for members in nets.values():
            # only connectors of imported parts: helper points without a part (empty object-ref, measuring
            # points) must not break the chain
            members = [g for g in members if all(terminal_uuid(g))]
            members.sort(key=lambda r: (round(position(r)[1]), position(r)[0]))
            pairs += list(zip(members, members[1:]))

        for a, b2 in pairs:
            ea, ta = terminal_uuid(a)
            eb, tb = terminal_uuid(b2)
            if ea and eb and ta and tb:
                ops.append({"op": "add_conductor", "folio": folio, "from": "$" + ea,
                            "from_terminal": "{" + ta + "}", "to": "$" + eb, "to_terminal": "{" + tb + "}",
                            "route": route})   # see reroute_wires
                count["wires"] += 1

    # links, also across folios: first through the reference on the same folio, else through the label
    linked = set()

    def link(master, slave):
        if slave["id"] in linked:
            return
        linked.add(slave["id"])
        # from the slave: QElectroTech's linkElements sets the links of the first element to exactly the second;
        # from the master every further coil (second solenoid, further contacts) would replace the previous one
        ops.append({"op": "link_elements", "folio": slave["folio"], "element": "$" + slave["id"],
                    "to_folio": master["folio"], "to": "$" + master["id"]})
        count["links"] += 1

    coils = [b for b in parts if b["link"] == "slave" and b["classification"] == "solenoid-valve-coil"]
    for valve in (b for b in parts if b["link"] == "master" and b["targets"]):
        for comp_ref, target_label in valve["targets"]:
            coil = next((s for s in coils if s["folio"] == valve["folio"] and comp_ref and s["ident"] == comp_ref),
                        None)
            if coil is None and target_label:
                coil = next((s for s in coils if s["label"] == target_label and s["id"] not in linked
                             and s["folio"] == valve["folio"]), None) or next(
                    (s for s in coils if s["label"] == target_label and s["id"] not in linked), None)
            if coil is not None:
                link(valve, coil)
    relays = [b for b in parts if b["link"] == "master" and b["classification"] == "solenoid-coil" and b["label"]]
    for contact in (b for b in parts if b["link"] == "slave" and b["classification"] in fe.CONTACTS
                    and b["label"]):
        coil = next((r for r in relays if r["label"] == contact["label"]), None)
        if coil is not None:
            link(coil, contact)
    count["not_imported"] = not_imported
    count["push_buttons"] = sorted(set(buttons))
    count["manual_valves"] = sorted(set(manual_valves))
    return ops, count


# Move labels clear: when the label of an element lies on a wire, on another element or on another label, it is
# put with placeElementTexts on the first free of the places around the element (like Edit > Place symbol texts).
# Same geometry as qet_layout_check.
LABEL_JS = r"""
var M = @MARKER@, moved = 0, stuck = 0;
var PLACES = ['right', 'left', 'top-right', 'bottom-right', 'top-left', 'bottom-left', 'top', 'bottom'];
function labelIndex(f, u) {
  var t = qet.elementTexts(f, u);
  for (var i = 0; i < t.length; i++)
    if (qet.elementTextProperty(f, u, i, 'source') === 'info'
        && qet.elementTextProperty(f, u, i, 'info') === 'label') return i;
  return -1;
}
function box(f, u, i) {
  var g = qet.elementTextGeometry(f, u, i);
  return (g && g.right > g.left) ? g : null;
}
function overlaps(a, b) {
  return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}
function segmentHits(p, q, g) {
  var l = g.left + 1, t = g.top + 1, r = g.right - 1, b = g.bottom - 1;
  if (Math.abs(p.y - q.y) < 1e-6) return t <= p.y && p.y <= b && Math.min(p.x, q.x) <= r && Math.max(p.x, q.x) >= l;
  if (Math.abs(p.x - q.x) < 1e-6) return l <= p.x && p.x <= r && Math.min(p.y, q.y) <= b && Math.max(p.y, q.y) >= t;
  return false;
}
for (var f = 0; f < qet.folioCount(); f++) {
  var els = qet.elementUuids(f), geo = {}, lab = {};
  for (var e = 0; e < els.length; e++) {
    geo[els[e]] = qet.elementGeometry(f, els[e]);
    lab[els[e]] = labelIndex(f, els[e]);
  }
  var segs = [], cu = qet.conductorUuids(f);
  for (var c = 0; c < cu.length; c++) {
    var p = qet.conductorPath(f, cu[c]);
    if (p) for (var k = 0; k + 1 < p.length; k++) segs.push([p[k], p[k + 1]]);
  }
  function hits(u, g) {
    var n = 0;
    for (var k = 0; k < segs.length; k++) if (segmentHits(segs[k][0], segs[k][1], g)) n++;
    for (var e2 = 0; e2 < els.length; e2++) {
      var v = els[e2];
      if (v !== u && overlaps(g, geo[v])) n++;
      if (v !== u && lab[v] >= 0) { var b2 = box(f, v, lab[v]); if (b2 && overlaps(g, b2)) n++; }
    }
    return n;
  }
  for (var e = 0; e < els.length; e++) {
    var u = els[e], i = lab[u];
    if (i < 0 || qet.elementTextProperty(f, u, i, 'shows') === 'false') continue;
    var g = box(f, u, i), eg = geo[u];
    if (!g || hits(u, g) === 0) continue;
    // a label inside its own symbol (the number in a GRAFCET step) belongs there
    if (g.left >= eg.left - 1 && g.right <= eg.right + 1 && g.top >= eg.top - 1 && g.bottom <= eg.bottom + 1) continue;
    var best = null, bestN = hits(u, g);
    for (var s = 0; s < PLACES.length; s++) {
      if (!qet.placeElementTexts(f, [u], PLACES[s])) continue;
      var g2 = box(f, u, i), n2 = g2 ? hits(u, g2) : 99;
      if (n2 < bestN) { best = PLACES[s]; bestN = n2; }
      if (n2 === 0) break;
    }
    if (best !== null && best !== PLACES[s]) qet.placeElementTexts(f, [u], best);
    if (best !== null) moved++;
    if (bestN > 0) stuck++;
  }
}
qet.log(M + JSON.stringify({kind: 'labels', moved: moved, stuck: stuck, save: qet.save(@OUTPUT@)}));
"""


def wires_through_symbols(qet_mcp, binary, path: Path, collection: Path):
    """The wires running through a foreign symbol, as sets of their two elements (UUID without braces). The
    layout record names the terminal by number only; several wires between the same two elements are therefore
    rerouted together."""
    js = qet_mcp._LAYOUT_JS.replace("@FOLIO@", "-1").replace("@MARKER@", json.dumps(qet_mcp._MARKER))
    try:
        r = qet_mcp._run_qet(str(binary), [str(path)], timeout=600, elements_dir=str(collection), script=js,
                             tail=20_000_000)
    except Exception:
        return set()
    out = set()
    for line in (r.get("stdout", "") + "\n" + r.get("stderr", "")).splitlines():
        if qet_mcp._MARKER not in line:
            continue
        try:
            rec = json.loads(line.split(qet_mcp._MARKER, 1)[1])
        except ValueError:
            continue
        if rec.get("kind") != "layout":
            continue
        findings = qet_mcp._layout_folio(rec, 40.0)
        through = {x["conductor"] for x in findings["findings"] if x["rule"] == "wire_through_symbol"}
        for w in rec.get("conductors") or []:
            if w.get("uuid") in through:
                out.add(frozenset(e.partition(" terminal ")[0].strip("{}") for e in w.get("ends") or []))
    return out


def reroute_wires(qet_mcp, binary, empty: Path, ops, result, output: Path, collection: Path, rounds=2):
    """Lead only the wires that run through a foreign symbol around the symbols.

    Laying every wire with "avoid" keeps a distance to every symbol and makes detours and crossings where
    QElectroTech's straight path (two, three segments) would be free. So the straight path first, then the
    layout check; the wires of its findings "wire_through_symbol" get "avoid" in the next pass. route_conductor
    alone is not enough: it does not reach a wire whose two terminals carry further wires (nets).
    @return (result, number of wires with "avoid")"""
    def bare(u):
        return (u or "").strip("{}")

    rerouted = 0
    for _ in range(rounds):
        through = wires_through_symbols(qet_mcp, binary, output, collection)
        if not through:
            break
        # addConductor returns no UUID: find the wire through its two ends (element, terminal)
        element = {o.get("id"): bare(o.get("result")) for o in result.get("operations", [])
                   if o.get("op") == "add_element" and o.get("succeeded")}
        new = []
        for k, op in enumerate(ops):
            if op["op"] != "add_conductor" or op.get("route") == "avoid":
                continue
            ends = frozenset((element.get(op["from"][1:]), element.get(op["to"][1:])))
            if ends in through:
                new.append(k)
        if not new:
            break
        for k in new:
            ops[k]["route"] = "avoid"
        # a file of its own next to the target (never one the user has): replaced onto the target only when saved
        fd, name = tempfile.mkstemp(dir=output.parent, prefix="." + output.stem + ".", suffix=".rerouted.qet")
        os.close(fd)
        between = Path(name)
        try:
            result2 = qet_mcp.tool_edit(str(binary), str(empty), ops, str(between), elements_dir=str(collection),
                                        timeout=600)
            if not result2.get("saved") or not between.stat().st_size:
                for k in new:
                    ops[k]["route"] = "default"
                break
            os.replace(between, output)
        finally:
            between.unlink(missing_ok=True)
        result = result2
        rerouted += len(new)
    return result, rerouted


def place_labels(qet_mcp, binary, path: Path, collection: Path):
    """Move labels off wires, elements and other labels; saves the project."""
    js = (LABEL_JS.replace("@MARKER@", json.dumps(qet_mcp._MARKER))
          .replace("@OUTPUT@", json.dumps(str(path))))
    try:
        r = qet_mcp._run_qet(str(binary), [str(path)], timeout=600, elements_dir=str(collection), script=js,
                             tail=200_000)
    except Exception as e:   # the project is saved already; without this step it stays valid
        return {"error": str(e)[:300]}
    for line in (r.get("stdout", "") + "\n" + r.get("stderr", "")).splitlines():
        if qet_mcp._MARKER in line:
            try:
                d = json.loads(line.split(qet_mcp._MARKER, 1)[1])
            except ValueError:
                continue
            if d.get("kind") == "labels":
                return d
    return {"error": "no answer"}


def stop_on_eof():
    """--stop-on-eof: when standard input closes, end the run like Ctrl+C -- the QElectroTech runs started since are
    killed, the temporary folders removed as their "with" blocks unwind -- and exit with code 3 (see the end of this
    file). If the program does not end within 20 s, it exits anyway."""
    import _thread
    import subprocess
    children = []
    popen_init = subprocess.Popen.__init__

    def init(self, *a, **k):
        popen_init(self, *a, **k)
        children.append(self)
    subprocess.Popen.__init__ = init

    def watch():
        try:
            while os.read(0, 4096):      # not sys.stdin: its lock would block the shutdown
                pass
        except Exception:
            pass
        _thread.interrupt_main()
        for p in list(children):
            try:
                p.kill()
            except Exception:
                pass
        time.sleep(20)
        os._exit(3)
    threading.Thread(target=watch, daemon=True).start()


def default_qet():
    """QElectroTech next to this program (an installation) or in a build folder of the source tree."""
    names = ("QElectroTech.exe", "qelectrotech.exe") if sys.platform == "win32" else ("qelectrotech",)
    # <prefix>/fluidsim_import (Windows), <prefix>/share/qelectrotech/fluidsim_import, Contents/Resources (macOS)
    places = [HERE.parent, HERE.parent / "bin", HERE.parent.parent / "MacOS"]
    if len(HERE.parents) > 2:
        places.append(HERE.parents[2] / "bin")
    places += sorted(REPO.glob("build*"))
    for d in places:
        for n in names:
            if (d / n).is_file():
                return d / n
    return Path(names[0])


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="*", type=Path, help=".circ files or a folder, OUTPUT.qet last")
    ap.add_argument("--qet", type=Path, default=None, help="the QElectroTech executable that writes the project")
    ap.add_argument("--fluidsim", type=Path, default=FLUIDSIM,
                    help="a FluidSIM installation, only for its translation of the part names (db/lang.db)")
    ap.add_argument("--language", default="en",
                    help="language of the part names in the project (de, fr ...) when --fluidsim has it")
    ap.add_argument("--title", default=None)
    ap.add_argument("--route", choices=("default", "avoid"), default="default",
                    help="default: QElectroTech's straight path, then only the wires through symbols around them; "
                         "avoid: every wire around all symbols (more detours and crossings)")
    ap.add_argument("--stop-on-eof", action="store_true", help=argparse.SUPPRESS)
    ap.add_argument("--dry-run", action="store_true", help="print the operations as JSON, write nothing")
    ap.add_argument("--report", type=Path, help="a summary as JSON (counts, push buttons, what was not imported)")
    a = ap.parse_args(argv)

    if a.stop_on_eof:
        stop_on_eof()
    paths = list(a.paths)
    output = None
    if paths and paths[-1].suffix.lower() == ".qet":
        output = paths.pop()
    circuits = []
    for p in paths:
        circuits += sorted(p.glob("*.circ")) if p.is_dir() else [p]
    if not circuits or (output is None and not a.dry_run):
        ap.error("give CIRCUIT.circ (or a folder) and OUTPUT.qet")
    missing = [c for c in circuits if not c.is_file()]
    if missing:
        ap.error(f"no such circuit: {missing[0]}")
    qet = (a.qet or default_qet()).resolve()   # QElectroTech runs under qet_mcp in another working folder
    language = a.language.lower()
    translation = fe.Translation(a.fluidsim / "db" / "lang.db") if language != "en" else fe.Translation()

    with tempfile.TemporaryDirectory(prefix="fluidsim-import-") as tmp:
        collection = Path(tmp) / "elements"
        target = collection / fluidsim_grafcet.G
        target.mkdir(parents=True)
        (target / "qet_directory").write_text(
            '<qet-directory>\n    <names>\n        <name lang="en">GRAFCET (FluidSIM import)</name>\n'
            '    </names>\n</qet-directory>\n', encoding="utf-8", newline="\n")
        for f in GRAFCET_SYMBOLS.glob("*.elmt"):
            (target / f.name).write_bytes(f.read_bytes())
        folios = sheets(circuits)
        ops, count = plan(folios, collection, translation, language, a.route)
        if a.dry_run:
            print(json.dumps(ops, indent=1, ensure_ascii=False))
            print(json.dumps(count, ensure_ascii=False))
            return
        sys.path.insert(0, str(MCP))
        try:
            import qet_mcp  # noqa: E402
        except (ImportError, SyntaxError) as e:
            sys.exit(f"the MCP server qet_mcp.py (misc/qet-mcp, or mcp/ next to QElectroTech) cannot be loaded: {e}")
        os.environ["QET_ENABLE_SCRIPTING"] = "1"
        # QElectroTech stops a script after 30 s; large plans need longer for the layout check, the rerouting
        # and the labels, and a stopped pass only reports "0 moved"
        os.environ.setdefault("QET_SCRIPT_TIMEOUT_MS", "600000")
        empty = Path(tmp) / "empty.qet"
        title = a.title or (circuits[0].stem if len(circuits) == 1 else circuits[0].parent.name)
        qet_mcp.tool_project_new(str(qet), str(empty), title=title, folios=[b.stem for b in folios],
                                 elements_dir=str(collection))
        if ops:
            result = qet_mcp.tool_edit(str(qet), str(empty), ops, str(output), elements_dir=str(collection),
                                       timeout=600)
        else:
            # nothing to import: the empty project with its folios
            Path(output).write_bytes(empty.read_bytes())
            result = {"saved": True, "operations": []}
            count["not_imported"].append("no part could be imported")
        if ops and result.get("saved") and a.route == "default":
            result, count["rerouted"] = reroute_wires(qet_mcp, qet, empty, ops, result, Path(output), collection)
        failed = [o for o in result.get("operations", []) if not o.get("succeeded")]
        print(f"{output}: {len(folios)} folio(s), {count['parts']} parts, {count['wires']} wires, "
              f"{count['links']} links, {count['texts']} values as text, {count['labels_added']} labels added, "
              f"GRAFCET {count['steps']} steps/{count['transitions']} transitions/{count['actions']} actions, "
              f"{count['skipped']} objects without a connector skipped; {len(failed)} operations failed.")
        for o in failed[:10]:
            print("  FAILED", json.dumps(o, ensure_ascii=False))
        for x in count["not_imported"]:
            print("  not imported:", x)
        if not result.get("saved"):
            sys.exit("project not saved: " + json.dumps(result, ensure_ascii=False)[:2000])
        if "rerouted" in count:
            print(f"  wires led around symbols: {count['rerouted']}")
        if ops:
            labels = place_labels(qet_mcp, qet, Path(output), collection)
            count["labels"] = labels
            print(f"  labels: {labels.get('moved', 0)} moved, {labels.get('stuck', 0)} without a free place")
        print(f"done: {output}")
        if a.report:
            a.report.write_text(json.dumps(dict(count, failed_operations=len(failed), failures=failed[:10]),
                                           ensure_ascii=False, indent=1), encoding="utf-8")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("stopped", flush=True)
        sys.exit(3)
