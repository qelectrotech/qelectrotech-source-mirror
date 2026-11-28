"""Turn the parts of a FluidSIM 6 circuit into QElectroTech element definitions (.elmt).

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

A part of a circuit is drawn from its <symbol>: the layers, their lines, polylines, rectangles, circles, arcs,
texts and connectors. The import never draws the symbols FluidSIM ships: own_graphics.replace() first swaps the
graphics of every symbol for its own drawing after ISO 1219-1 / IEC 60617 and keeps only the connectors, so what
this module draws is that own drawing, placed with the rotation, mirroring and scale the part has in the circuit.

Kept from the circuit:
- every connector as a QElectroTech terminal with its name (port number 1, 2, 3 ...) and direction;
- a silencer or blind plug sitting on a port, drawn at the end of that port;
- directional valves with solenoids become masters (the valve solenoids link to them), valve solenoids and
  relay contacts slaves, relay coils masters;
- the label as a text to the right of the symbol.

Scale 10/12: FluidSIM's connector grid of 12 falls on QElectroTech's grid of 10.
"""
import gzip
import math
import re
import struct
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path
from xml.sax.saxutils import escape, quoteattr

SCALE = 10.0 / 12.0          # FluidSIM unit -> QElectroTech pixel
ARROW_LENGTH = 4.5           # arrow head, FluidSIM units
ARROW_HALF_WIDTH = 2.0       # half width of the arrow head
BAR_HALF_WIDTH = 3.0         # half width of the blocking bar (end cap "ortho1")
LINE_STYLES = {"dash": "dashed", "dot": "dotted", "dashdot": "dashdotted"}
# Parts that only close a port: drawn at that port of the part they sit on
TERMINATIONS = {"Pneu.Supply.Silencer": "silencer", "Pneu.Supply.BlindPlug": "plug"}
# FluidSIM classification -> state of the QElectroTech contact (slave)
CONTACTS = {"make-contact": "NO", "break-contact": "NC", "changeover-contact": "SW"}


def number(e, name, default=0.0):
    v = e.get(name)
    return float(v) if v not in (None, "") else default


def unpack_coordinates(root):
    """Rewrite the packed coordinates of older FluidSIM files as attributes, in place.

    Older circuits store lines and polylines not as attributes but as text: doubles (8 bytes, little endian) in
    hexadecimal, first all x values, then all y values. A line has four numbers (x1, x2, y1, y2), a polyline
    2 * count. Afterwards everything reads them like newer files."""
    for e in root.iter():
        if e.tag not in ("line", "polyline"):
            continue
        text = (e.text or "").strip()
        if not text or len(text) % 16 or not re.fullmatch(r"[0-9A-Fa-f]+", text):
            continue
        raw = bytes.fromhex(text)
        values = list(struct.unpack("<%dd" % (len(raw) // 8), raw))
        n = len(values) // 2
        xs, ys = values[:n], values[n:]
        if e.tag == "line" and n == 2:
            e.set("x1", repr(xs[0]))
            e.set("x2", repr(xs[1]))
            e.set("y1", repr(ys[0]))
            e.set("y2", repr(ys[1]))
        elif e.tag == "polyline":
            e.set("points", " ".join(f"{x!r},{y!r}" for x, y in zip(xs, ys)))
        else:
            continue
        e.text = None
    return root


def fmt(v):
    return ("%.2f" % v).rstrip("0").rstrip(".").replace("-0", "0") if abs(v) > 1e-9 else "0"


def style(e, filling=None):
    line = LINE_STYLES.get(e.get("style", ""), "normal")
    fill = "none"
    colour = (e.get("fill-color") or "").upper()
    if filling:
        fill = filling
    elif colour == "#FF000000":
        fill = "black"
    elif colour == "#FFFFFFFF":
        fill = "white"
    return f"line-style:{line};line-weight:normal;filling:{fill};color:black"


class Translation:
    """FluidSIM's own translation of its part descriptions (db/lang.db of an installation), when there is one."""

    def __init__(self, path: Path = None):
        self.english_to = {}
        if path is None or not path.is_file():
            return
        try:
            root = ET.fromstring(gzip.decompress(path.read_bytes()))
        except (OSError, ET.ParseError):
            return
        for t in root.iter("text"):
            en = (t.get("ENUS") or t.get("ENGB") or "").strip().lower()
            if not en:
                continue
            for lang in ("DE", "FR", "ES", "IT", "NL", "PT", "RU", "ZH", "JA"):
                if t.get(lang):
                    self.english_to.setdefault(lang.lower(), {}).setdefault(en, t.get(lang))

    def translate(self, text, lang):
        """The text in the language lang ("de", "fr" ...), or the text itself."""
        return self.english_to.get(lang, {}).get((text or "").strip().lower(), text)


def unique_names(names):
    """Make names unique: two "1" become "1.1" and "1.2"; empty names stay empty.

    A shuttle valve has two inputs "1" after ISO 1219. It stays drawn like that, but QElectroTech wants terminal
    names that can be told apart (wiring list, IEC 61666)."""
    count = {}
    for n in names:
        if n:
            count[n] = count.get(n, 0) + 1
    seen = {}
    out = []
    for n in names:
        if n and count[n] > 1:
            seen[n] = seen.get(n, 0) + 1
            out.append(f"{n}.{seen[n]}")
        else:
            out.append(n)
    return out


def stable_uuid(*parts):
    """A UUID that is the same on every run, derived from the given parts."""
    return uuid.uuid5(uuid.NAMESPACE_URL, "qet-fluidsim/" + "/".join(str(t) for t in parts))


class Drawer:
    """Collects the graphics of a symbol in FluidSIM coordinates and writes them as QElectroTech XML."""

    def __init__(self, seed="", transform=None):
        self.seed = seed           # for the UUIDs
        self.transform = transform or Transform()
        self.parts = []            # (kind, data)
        self.terminals = []        # dict
        self.lines = []            # end points of all lines, for the direction of the terminals
        self.anchor = (0.0, 0.0)

    # --- primitives ----------------------------------------------------------------------------------------
    def line(self, a, b, e, start=None, end=None):
        self.parts.append(("line", (a, b, style(e))))
        self.lines.append((a, b))
        for cap, tip, origin in ((end, b, a), (start, a, b)):
            if cap in ("arrow1", "arrow2"):
                self.arrow(tip, origin)
            elif cap == "ortho1":
                self.bar(tip, origin)

    def arrow(self, tip, origin):
        dx, dy = tip[0] - origin[0], tip[1] - origin[1]
        length = math.hypot(dx, dy) or 1.0
        ux, uy = dx / length, dy / length
        bx, by = tip[0] - ux * ARROW_LENGTH, tip[1] - uy * ARROW_LENGTH
        points = [tip, (bx - uy * ARROW_HALF_WIDTH, by + ux * ARROW_HALF_WIDTH),
                  (bx + uy * ARROW_HALF_WIDTH, by - ux * ARROW_HALF_WIDTH)]
        self.parts.append(("polygon", (points, True, "line-style:normal;line-weight:thin;filling:black;color:black")))

    def bar(self, tip, origin):
        dx, dy = tip[0] - origin[0], tip[1] - origin[1]
        length = math.hypot(dx, dy) or 1.0
        ux, uy = dx / length, dy / length
        a = (tip[0] - uy * BAR_HALF_WIDTH, tip[1] + ux * BAR_HALF_WIDTH)
        b = (tip[0] + uy * BAR_HALF_WIDTH, tip[1] - ux * BAR_HALF_WIDTH)
        self.parts.append(("line", (a, b, "line-style:normal;line-weight:normal;filling:none;color:black")))

    def polyline(self, points, e, closed=False):
        if len(points) < 2:
            return
        if e.get("fill-color"):
            closed = True
        self.parts.append(("polygon", (points, closed, style(e))))
        for a, b in zip(points, points[1:]):
            self.lines.append((a, b))
        if e.get("start-cap") in ("arrow1", "arrow2"):
            self.arrow(points[0], points[1])
        if e.get("end-cap") in ("arrow1", "arrow2"):
            self.arrow(points[-1], points[-2])

    def rectangle(self, x, y, w, h, e):
        self.parts.append(("rect", ((x, y), (x + w, y + h), style(e))))
        for a, c in (((x, y), (x + w, y)), ((x + w, y), (x + w, y + h)),
                     ((x + w, y + h), (x, y + h)), ((x, y + h), (x, y))):
            self.lines.append((a, c))

    def circle(self, m, r, e, sx=1.0):
        self.parts.append(("ellipse", (m, r * sx, r, style(e))))

    def arc(self, m, r, start, end, e):
        # FluidSIM: start/end in degrees, counter-clockwise on screen like Qt. The arcs of the own graphics are
        # symmetric to their axis, so a wrong sense would not show there.
        sweep = (end - start) % 360 or 360
        self.parts.append(("arc", (m, r, start, sweep, style(e))))
        for cap, angle, sense in ((e.get("end-cap"), end, 1), (e.get("start-cap"), start, -1)):
            if cap in ("arrow1", "arrow2"):
                w = math.radians(angle)
                tip = (m[0] + r * math.cos(w), m[1] - r * math.sin(w))
                # tangent in the running direction of the arc
                t = (-math.sin(w) * sense, -math.cos(w) * sense)
                self.arrow(tip, (tip[0] - t[0], tip[1] - t[1]))

    def text(self, p, content, height):
        self.parts.append(("text", (p, content, height)))

    # --- output --------------------------------------------------------------------------------------------
    def direction(self, p):
        """n/s/e/w: away from the line that ends at the terminal; else away from the middle of the symbol."""
        for a, b in self.lines:
            for end, other in ((a, b), (b, a)):
                if math.hypot(end[0] - p[0], end[1] - p[1]) < 0.6:
                    dx, dy = end[0] - other[0], end[1] - other[1]
                    if abs(dx) < 1e-6 and abs(dy) < 1e-6:
                        continue
                    if abs(dy) >= abs(dx):
                        return "s" if dy > 0 else "n"
                    return "e" if dx > 0 else "w"
        xs = [q[0] for a, b in self.lines for q in (a, b)] or [p[0]]
        ys = [q[1] for a, b in self.lines for q in (a, b)] or [p[1]]
        mx, my = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
        dx, dy = p[0] - mx, p[1] - my
        if abs(dy) >= abs(dx):
            return "s" if dy > 0 else "n"
        return "e" if dx > 0 else "w"

    @staticmethod
    def termination(p, direction, kind):
        """XML lines of a silencer (box) or a blind plug (bar) at the end of a port, outwards (QET pixels)."""
        dx, dy = {"n": (0, -1), "s": (0, 1), "e": (1, 0), "w": (-1, 0)}[direction]
        plain = "line-style:normal;line-weight:normal;filling:none;color:black"
        out = []
        if kind == "plug":
            cx, cy = p[0] + 2 * dx, p[1] + 2 * dy
            ax, ay = cx - 4 * dy, cy - 4 * dx
            bx, by = cx + 4 * dy, cy + 4 * dx
            out.append(f'<line x1="{fmt(p[0])}" y1="{fmt(p[1])}" x2="{fmt(cx)}" y2="{fmt(cy)}" end1="none" '
                       f'end2="none" length1="1.5" length2="1.5" style="{plain}" antialias="true"/>')
            out.append(f'<line x1="{fmt(ax)}" y1="{fmt(ay)}" x2="{fmt(bx)}" y2="{fmt(by)}" end1="none" '
                       f'end2="none" length1="1.5" length2="1.5" style="{plain}" antialias="true"/>')
            return out
        # silencer: a short stub, then a box across the port (ISO 1219-1)
        sx, sy = p[0] + 2 * dx, p[1] + 2 * dy
        out.append(f'<line x1="{fmt(p[0])}" y1="{fmt(p[1])}" x2="{fmt(sx)}" y2="{fmt(sy)}" end1="none" '
                   f'end2="none" length1="1.5" length2="1.5" style="{plain}" antialias="true"/>')
        cx, cy = sx + 2.5 * dx, sy + 2.5 * dy
        w, h = (5.0, 8.0) if dx else (8.0, 5.0)
        out.append(f'<rect x="{fmt(cx - w / 2)}" y="{fmt(cy - h / 2)}" width="{fmt(w)}" height="{fmt(h)}" '
                   f'rx="0" ry="0" style="{plain}" antialias="true"/>')
        return out

    def xml(self, label_text=True):
        # origin: the first fluid terminal, so the terminals fall on the QElectroTech grid
        anchor = next((a["p"] for a in self.terminals), None)
        if anchor is None:
            points = [q for _, d in self.parts for q in _points(d)] or [(0.0, 0.0)]
            anchor = (min(q[0] for q in points), min(q[1] for q in points))
        ox, oy = anchor
        self.anchor = anchor

        def t(p):
            return ((p[0] - ox) * SCALE, (p[1] - oy) * SCALE)

        lines = []
        extent = []
        for kind, d in self.parts:
            if kind == "line":
                a, b, s = d
                a, b = t(a), t(b)
                extent += [a, b]
                lines.append(f'<line x1="{fmt(a[0])}" y1="{fmt(a[1])}" x2="{fmt(b[0])}" y2="{fmt(b[1])}" '
                             f'end1="none" end2="none" length1="1.5" length2="1.5" style="{s}" antialias="true"/>')
            elif kind == "polygon":
                points, closed, s = d
                points = [t(p) for p in points]
                extent += points
                attr = " ".join(f'x{i}="{fmt(p[0])}" y{i}="{fmt(p[1])}"' for i, p in enumerate(points, 1))
                open_attr = "" if closed else ' closed="false"'
                lines.append(f'<polygon {attr}{open_attr} style="{s}" antialias="true"/>')
            elif kind == "rect":
                a, b, s = d
                a, b = t(a), t(b)
                x, y = min(a[0], b[0]), min(a[1], b[1])
                w, h = abs(b[0] - a[0]), abs(b[1] - a[1])
                extent += [a, b]
                lines.append(f'<rect x="{fmt(x)}" y="{fmt(y)}" width="{fmt(w)}" height="{fmt(h)}" rx="0" ry="0" '
                             f'style="{s}" antialias="true"/>')
            elif kind == "ellipse":
                m, rx, ry, s = d
                m = t(m)
                rx, ry = rx * SCALE, ry * SCALE
                extent += [(m[0] - rx, m[1] - ry), (m[0] + rx, m[1] + ry)]
                lines.append(f'<ellipse x="{fmt(m[0] - rx)}" y="{fmt(m[1] - ry)}" width="{fmt(2 * rx)}" '
                             f'height="{fmt(2 * ry)}" style="{s}" antialias="true"/>')
            elif kind == "arc":
                m, r, start, sweep, s = d
                m = t(m)
                r *= SCALE
                extent += [(m[0] - r, m[1] - r), (m[0] + r, m[1] + r)]
                lines.append(f'<arc x="{fmt(m[0] - r)}" y="{fmt(m[1] - r)}" width="{fmt(2 * r)}" '
                             f'height="{fmt(2 * r)}" start="{fmt(start)}" angle="{fmt(sweep)}" style="{s}" '
                             f'antialias="true"/>')
            elif kind == "text":
                p, content, height = d
                p = t(p)
                size = max(4, round(height * SCALE * 0.75))
                extent.append(p)
                # FluidSIM: x1/y1 is the top left corner; QElectroTech: y is the base line
                lines.append(f'<text x="{fmt(p[0])}" y="{fmt(p[1] + height * SCALE * 0.8)}" size="{size}" '
                             f'color="#000000" text={quoteattr(content)}/>')

        for a in self.terminals:
            p = t(a["p"])
            extent.append(p)
            if a.get("display"):
                # the port number as in FluidSIM: beside the port line, between the port and the body; a fixed
                # text (part of the drawing, y = base line), so QElectroTech does not move it off the symbol
                r = a["direction"]
                width = 2.6 * len(a["display"])
                tx, ty = {"n": (p[0] + 1.5, p[1] + 5), "s": (p[0] + 1.5, p[1] - 2),
                          "w": (p[0] + 1, p[1] - 1.5), "e": (p[0] - 1 - width, p[1] - 1.5)}[r]
                lines.append(f'<text x="{fmt(tx)}" y="{fmt(ty)}" size="4" color="#000000" '
                             f'text={quoteattr(a["display"])}/>')
            if a.get("termination"):
                lines += self.termination(p, a["direction"], a["termination"])
            attr = [f'uuid="{{{stable_uuid(self.seed, "terminal", a["key"])}}}"', f'name={quoteattr(a["name"])}',
                    f'x="{fmt(round(p[0], 1))}"', f'y="{fmt(round(p[1], 1))}"', f'orientation="{a["direction"]}"',
                    'type="Generic"']
            lines.append("<terminal " + " ".join(attr) + "/>")

        xs = [p[0] for p in extent] or [0.0]
        ys = [p[1] for p in extent] or [0.0]
        if label_text:
            # to the right of the symbol, at the top: QElectroTech puts the cross reference of a slave under the
            # label, so it does not cover the symbol. The label does not count for the extent of the element:
            # otherwise the boxes of contacts side by side overlap and wires "run through" them.
            lx, ly = max(xs) + 4, min(ys) - 2
            lines.insert(0, (
                f'<dynamic_text x="{fmt(lx)}" y="{fmt(ly)}" z="3" text_width="-1" '
                f'Halignment="AlignLeft" Valignment="AlignTop" frame="false" rotation="0" '
                f'keep_visual_rotation="false" text_from="ElementInfo" uuid="{{{stable_uuid(self.seed, "label")}}}" '
                f'font="Liberation Sans,7,-1,5,0,0,0,0,0,0,Regular"><text></text><info_name>label</info_name>'
                f'</dynamic_text>'))
        x0 = math.floor(min(xs) / 10) * 10
        y0 = math.floor(min(ys) / 10) * 10
        x1 = math.ceil(max(xs) / 10) * 10
        y1 = math.ceil(max(ys) / 10) * 10
        return lines, (max(10, x1 - x0), max(10, y1 - y0), -x0, -y0)


def _points(d):
    out = []
    for v in d if isinstance(d, tuple) else ():
        if isinstance(v, tuple) and len(v) == 2 and all(isinstance(z, float) for z in v):
            out.append(v)
        elif isinstance(v, list):
            out += [q for q in v if isinstance(q, tuple)]
    return out


class Transform:
    """Placement of an object in a FluidSIM circuit: first scale and mirroring (scale-x/-y, -1 mirrors), then the
    rotation (rotation, multiples of 90 degrees) about the middle of the object.

    SENSE: -1, FluidSIM rotates counter-clockwise (on screen, y downwards). Found on the sample circuits through
    their wires (fluidsim_import.py --check-rotation: every wire at a rotated part fits)."""
    SENSE = -1

    def __init__(self, sx=1.0, sy=1.0, rot=0.0):
        self.sx, self.sy = sx or 1.0, sy or 1.0
        self.rot = (round(rot / 90.0) * 90) % 360

    @classmethod
    def of(cls, obj):
        return cls(number(obj, "scale-x", 1.0), number(obj, "scale-y", 1.0), number(obj, "rotation", 0.0))

    def point(self, x, y):
        x, y = x * self.sx, y * self.sy
        w = math.radians(self.rot * self.SENSE)
        c, s = round(math.cos(w)), round(math.sin(w))
        return (x * c - y * s, x * s + y * c)

    def angle(self, a):
        """Arc angle (degrees, counter-clockwise on screen) after the placement."""
        if self.sx < 0:
            a = 180 - a
        if self.sy < 0:
            a = -a
        return (a - self.rot * self.SENSE) % 360

    def mirrors(self):
        return (self.sx < 0) != (self.sy < 0)

    def sideways(self):
        return self.rot in (90, 270)

    def swaps_sides(self):
        """True when the left of the symbol ends up on the right of the drawing."""
        return (self.sx < 0) != (self.rot == 180)


def read_layer(d: Drawer, layer, terminations, graphics_only=False):
    """Draw a <symbol-layer> into d. terminations: {(layer, ident): "silencer" | "plug"}.
    graphics_only: also draw a hidden layer, but without its connectors."""
    if layer.get("visible") == "false" and not graphics_only:
        return
    ox, oy = number(layer, "ref-x"), number(layer, "ref-y")
    sx = number(layer, "scale-x", 1.0)
    sy = number(layer, "scale-y", 1.0)
    name = layer.get("ident", "")
    t = d.transform

    def p(x, y):
        return t.point(ox + sx * x, oy + sy * y)

    for e in layer:
        if e.tag == "line":
            x1, y1 = number(e, "x1"), number(e, "y1")
            x2 = number(e, "x2", x1) if e.get("x2") is not None else x1
            y2 = number(e, "y2", y1) if e.get("y2") is not None else y1
            d.line(p(x1, y1), p(x2, y2), e, e.get("start-cap"), e.get("end-cap"))
        elif e.tag == "polyline":
            points = []
            for pair in (e.get("points") or "").split():
                a, b = pair.split(",")
                points.append(p(float(a), float(b)))
            d.polyline(points, e)
        elif e.tag == "rect":
            a = p(number(e, "x1"), number(e, "y1"))
            b = p(number(e, "x1") + number(e, "width"), number(e, "y1") + number(e, "height"))
            d.rectangle(min(a[0], b[0]), min(a[1], b[1]), abs(b[0] - a[0]), abs(b[1] - a[1]), e)
        elif e.tag == "circle":
            rx = number(e, "r") * abs(sx * t.sx)
            ry = number(e, "r") * abs(sy * t.sy)
            if t.sideways():
                rx, ry = ry, rx
            d.circle(p(number(e, "cx"), number(e, "cy")), ry, e, rx / ry if ry else 1.0)
        elif e.tag == "arc":
            start, end = t.angle(number(e, "start")), t.angle(number(e, "end"))
            if t.mirrors():
                start, end = end, start
            d.arc(p(number(e, "cx"), number(e, "cy")), number(e, "r") * abs(sy * t.sy), start, end, e)
        elif e.tag == "text":
            content = (e.text or "").strip()
            # "~..." are FluidSIM translation keys; the own graphics (layer OWN) may write "3~"
            if content and not content.startswith("?") and ("~" not in content or layer.get("ident") == "OWN"):
                d.text(p(number(e, "x1"), number(e, "y1")), content, number(e, "height", 8))
        elif e.tag == "connector":
            if graphics_only:
                continue
            kind = e.get("type")
            if e.get("visible") == "false" or kind not in ("pneumatic", "electric", "hydraulic", "digital"):
                continue
            d.terminals.append({
                "p": p(number(e, "cx"), number(e, "cy")),
                "name": e.get("designation") or "",
                "medium": "electric" if kind == "digital" else kind,
                "termination": terminations.get((name, e.get("ident"))),
                "key": (name, e.get("ident")),
            })


def component_values(component):
    out = {}
    for c in component:
        if c.tag in ("const", "parameter") and c.get("ident"):
            out[c.get("ident")] = c.get("value", "")
    return out


def classification(component):
    """FluidSIM's classification of a part (classification-data/type), e.g. solenoid-coil, make-contact."""
    t = component.find("classification-data/type")
    return t.get("value", "") if t is not None else ""


def part_kind(obj_ref, values, layer_names):
    """("valve", number of solenoids), ("cylinder", 0), ("solenoid", 0) or ("", 0)."""
    if obj_ref.startswith(("Pneu.DirectionalValve", "Hyd.DirectionalValve", "Pneu.Valve.Directional")):
        try:
            positions = int(float(values.get("posNoMax", "0")))
        except ValueError:
            return "", 0
        if not 1 <= positions <= 4:
            return "", 0
        solenoids = sum(1 for side in ("L", "R") if any(n.startswith("AEM" + side) for n in layer_names))
        return "valve", solenoids
    if obj_ref.startswith(("Pneu.Actuator", "Hyd.Actuator")) and "CYL" in obj_ref.upper():
        return "cylinder", 0
    if obj_ref.startswith("ET.Relay.ValveSolenoid"):
        return "solenoid", 0
    return "", 0


def manual_actuation(layer_names):
    """True when a valve has a manual actuation (layers AMANL..., AMANR...)."""
    return any(n.startswith(("AMANL", "AMANR")) for n in layer_names)


def centre(c):
    """The middle of a circuit object; symbol coordinates are relative to it."""
    return (number(c, "x1") + number(c, "width") / 2, number(c, "y1") + number(c, "height") / 2)


def connector_key(obj, connector):
    """(layer, ident) of the symbol connector a connector of a circuit object points to.

    A part names its connectors ("C4") and refers with con-ref to the symbol connector ("CON5.4"); objects without
    a <connector> list (terminals of a terminal strip) are addressed directly with "layer.ident"."""
    for c in obj.findall("connector"):
        if c.get("ident") == connector:
            connector = c.get("con-ref", connector)
            break
    layer, _, ident = connector.rpartition(".")
    return (layer, ident)


def convert_component(symbols, main, surroundings, connections, names, source_name, seed, placed=False):
    """One part of a FluidSIM circuit as .elmt.

    symbols: {ident: <symbol>} of the circuit, main: the part, surroundings: all circuit objects (for the
    silencers and blind plugs at its ports), connections: the <connect> of the circuit. names: (English name,
    name in another language or "", its language). seed makes the UUIDs: same symbol, same seed, same file.
    placed: take the rotation, mirroring and scale of the part in the circuit into the geometry.

    @return dict with text, anchor (origin of the .elmt in symbol coordinates), uuids ({(layer, ident): terminal
            UUID}), link (simple, master, slave), classification, kind and manual; or None
    """
    symbol = symbols.get(main.get("symbol-ref"))
    if symbol is None:
        return None

    # terminations through a wire
    termination_of = {c.get("ident"): TERMINATIONS[c.get("object-ref")]
                      for c in surroundings if c.get("object-ref") in TERMINATIONS}
    terminations = {}
    for v in connections:
        ends = [v.get("con1", ""), v.get("con2", "")]
        for a, b in (ends, ends[::-1]):
            part_a, _, port_a = a.partition(".")
            part_b = b.partition(".")[0]
            if part_a == main.get("ident") and part_b in termination_of:
                terminations[connector_key(main, port_a)] = termination_of[part_b]

    # most of the time the termination sits on the port without a wire: its connector lies on that of the part
    hm = centre(main)
    termination_points = []
    for c in surroundings:
        if c.get("object-ref") not in TERMINATIONS or c.get("symbol-ref") not in symbols:
            continue
        cm = centre(c)
        tc = Transform.of(c) if placed else Transform()
        for layer in symbols[c.get("symbol-ref")].findall("symbol-layer"):
            for con in layer.findall("connector"):
                q = tc.point(number(layer, "ref-x") + number(con, "cx"), number(layer, "ref-y") + number(con, "cy"))
                termination_points.append((cm[0] + q[0] - hm[0], cm[1] + q[1] - hm[1],
                                           TERMINATIONS[c.get("object-ref")]))

    d = Drawer(seed, Transform.of(main) if placed else None)
    layer_names = []
    for layer in symbol.findall("symbol-layer"):
        if layer.get("visible") != "false":
            layer_names.append(layer.get("ident", ""))
        read_layer(d, layer, terminations)
    if not d.parts and d.terminals:
        # some symbols (the coil in a ladder diagram) only draw in a hidden layer
        for layer in symbol.findall("symbol-layer"):
            if layer.get("visible") == "false":
                read_layer(d, layer, terminations, graphics_only=True)
    for a in d.terminals:
        if not a.get("termination"):
            for px, py, kind in termination_points:
                if math.hypot(a["p"][0] - px, a["p"][1] - py) < 1.5:
                    a["termination"] = kind
    if not d.parts:
        return None
    # cylinders, sources, coils and the GRAFCET PLC carry no designation on the symbol; the part names its
    # connectors though ("A", "B" at the cylinder, "I0", "Q0" at the PLC): that name goes to the terminal
    own = {connector_key(main, c.get("ident")): c.get("ident") for c in main.findall("connector")}
    for a in d.terminals:
        a["display"] = a["name"]
        if not a["name"]:
            a["name"] = own.get(a["key"], "")
    for a, name in zip(d.terminals, unique_names([a["name"] for a in d.terminals])):
        a["direction"] = d.direction(a["p"])
        a["name"] = name
    # fluid terminals first: they set the grid
    d.terminals.sort(key=lambda a: a["medium"] == "electric")

    obj_ref = main.get("object-ref", "")
    cls = classification(main)
    kind, solenoids = part_kind(obj_ref, component_values(main), layer_names)
    if not kind and cls == "solenoid-valve-coil":
        kind = "solenoid"
    lines, (width, height, hx, hy) = d.xml()

    def slave(state):
        return ('    <kindInformations>\n        <kindInformation name="type">simple</kindInformation>\n'
                f'        <kindInformation name="state">{state}</kindInformation>\n'
                '        <kindInformation name="number">1</kindInformation>\n    </kindInformations>\n')

    link = "simple"
    kind_info = ""
    if solenoids:
        link = "master"
        kind_info = ('    <kindInformations>\n        <kindInformation name="type">coil</kindInformation>\n'
                     f'        <kindInformation name="max_slaves">{solenoids}</kindInformation>\n'
                     '    </kindInformations>\n')
    elif kind == "solenoid":
        link, kind_info = "slave", slave("Other")
    elif cls == "solenoid-coil":
        # coil (relay, comparator card ...): master of the contacts with the same label
        link = "master"
        kind_info = ('    <kindInformations>\n        <kindInformation name="type">coil</kindInformation>\n'
                     '    </kindInformations>\n')
    elif cls in CONTACTS:
        link, kind_info = "slave", slave(CONTACTS[cls])

    en, other, lang = names
    name_lines = f'        <name lang="en">{escape(en)}</name>\n'
    if other and lang and lang != "en" and other != en:
        name_lines = f'        <name lang="{lang}">{escape(other)}</name>\n' + name_lines
    text = (f'<definition version="0.90" type="element" link_type="{link}" width="{width}" height="{height}" '
            f'hotspot_x="{hx}" hotspot_y="{hy}">\n'
            f'    <uuid uuid="{{{stable_uuid(seed)}}}"/>\n'
            f'    <names>\n{name_lines}    </names>\n'
            f'{kind_info}'
            f'    <informations>Drawn by the FluidSIM import of QElectroTech (own graphics after ISO 1219-1 / '
            f'IEC 60617); terminals and values from the circuit {escape(source_name)} ({escape(obj_ref)}).'
            f'</informations>\n'
            f'    <description>\n' + "\n".join("        " + zl for zl in lines) + "\n    </description>\n"
            f'</definition>\n')
    uuids = {a["key"]: str(stable_uuid(seed, "terminal", a["key"])) for a in d.terminals}
    return {"text": text, "anchor": d.anchor, "uuids": uuids, "link": link, "classification": cls,
            "kind": kind, "manual": kind == "valve" and manual_actuation(layer_names),
            "roller": kind == "valve" and any(n.startswith(("AMECHL", "AMECHR")) for n in layer_names)}
