"""Own graphics for the parts of FluidSIM circuits, instead of the FluidSIM symbols.

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

Only data is taken from the .circ: position and names of the connectors, the layer names (which actuation, which
position), the flow paths of the positions, the extent of the part, its classification and values (spring left/
right). Everything is drawn anew after ISO 1219-1 or IEC 60617 from simple primitives: valve boxes with arrows and
blocked ports, actuations (solenoid, push button, roller, pilot, spring), cylinders, sources, tank, gauges,
contacts, coils, lamps, potentials, two-terminal parts (resistor, capacitor, diode, LED, fuse, DC source), bipolar
transistor, signal generator/oscilloscope, multi-pole circuit breakers, machines (M 3~, G ...), rectifier,
transformer, inductor, operational amplifier, potentials L1 ... PE; every other class as a box with its sign after
IEC 60617 (PID, SUM, p ...) or empty, never with its name. So an imported project holds no FluidSIM graphics, and
anyone can import a circuit, also without FluidSIM.

replace() returns a <symbol> in the FluidSIM format: the same layers (names, visibility, position) with only their
connectors, plus a layer "OWN" with the new graphics. fluidsim_elements.convert_component() draws it like any
symbol, with the rotation, mirroring and terminal UUIDs of the circuit.
"""
import math
import re
import xml.etree.ElementTree as ET

BLACK = "#FF000000"
ACTUATIONS = ("AEM", "AMAN", "AMECH", "APNEU", "PLT")

# Order of undirected connections: from the supply to the consumer or to the exhaust
RANK = {"P": 0, "1": 0, "X": 1, "Y": 1, "A": 2, "B": 2, "2": 2, "4": 2, "C": 2, "D": 2,
        "T": 4, "R": 4, "S": 4, "3": 4, "5": 4, "L": 5}


def rank(p):
    return RANK.get(p, 3), p


def _n(e, name, default=0.0):
    v = e.get(name)
    try:
        return float(v) if v not in (None, "") else default
    except ValueError:
        return default


class Pen:
    """Collects primitives in the coordinate system of the symbol (FluidSIM units, y downwards)."""

    def __init__(self):
        self.layer = ET.Element("symbol-layer", {"ident": "OWN"})

    def line(self, x1, y1, x2, y2, end=None, dash=None):
        a = {"x1": repr(float(x1)), "y1": repr(float(y1)), "x2": repr(float(x2)), "y2": repr(float(y2))}
        if end:
            a["end-cap"] = end
        if dash:
            a["style"] = dash
        ET.SubElement(self.layer, "line", a)

    def path(self, points, filled=False):
        a = {"points": " ".join(f"{float(x)!r},{float(y)!r}" for x, y in points)}
        if filled:
            a["fill-color"] = BLACK
        ET.SubElement(self.layer, "polyline", a)

    def box(self, x, y, w, h):
        ET.SubElement(self.layer, "rect", {"x1": repr(float(x)), "y1": repr(float(y)),
                                           "width": repr(float(w)), "height": repr(float(h))})

    def circle(self, cx, cy, r, filled=False):
        a = {"cx": repr(float(cx)), "cy": repr(float(cy)), "r": repr(float(r))}
        if filled:
            a["fill-color"] = BLACK
        ET.SubElement(self.layer, "circle", a)

    def text(self, x, y, content, height=6):
        t = ET.SubElement(self.layer, "text", {"x1": repr(float(x)), "y1": repr(float(y)), "height": repr(float(height))})
        t.text = content

    def spring(self, x1, y, x2, h=4):
        n = 6
        points = [(x1, y)] + [(x1 + (x2 - x1) * i / n, y + (h if i % 2 else -h)) for i in range(1, n)] + [(x2, y)]
        self.path(points)


def connectors(symbol):
    """[(layer, connector element, x, y)] in symbol coordinates, only real connectors (no labels)."""
    out = []
    for layer in symbol.findall("symbol-layer"):
        if layer.get("visible") == "false":
            continue
        ox, oy = _n(layer, "ref-x"), _n(layer, "ref-y")
        sx, sy = _n(layer, "scale-x", 1.0), _n(layer, "scale-y", 1.0)
        for c in layer.iter("connector"):
            if c.get("type") in ("pneumatic", "hydraulic", "electric", "digital"):
                out.append((layer, c, ox + sx * _n(c, "cx"), oy + sy * _n(c, "cy")))
    return out


def outline(symbol):
    """Extent of the original graphics (only its bounds) as (x0, y0, x1, y1)."""
    xs, ys = [], []
    for layer in symbol.findall("symbol-layer"):
        if layer.get("visible") == "false":
            continue
        ox, oy = _n(layer, "ref-x"), _n(layer, "ref-y")
        sx, sy = _n(layer, "scale-x", 1.0), _n(layer, "scale-y", 1.0)

        def p(x, y):
            xs.append(ox + sx * x)
            ys.append(oy + sy * y)
        for e in layer:
            if e.tag == "line":
                x1, y1 = _n(e, "x1"), _n(e, "y1")
                p(x1, y1)
                p(_n(e, "x2", x1), _n(e, "y2", y1))
            elif e.tag == "rect":
                p(_n(e, "x1"), _n(e, "y1"))
                p(_n(e, "x1") + _n(e, "width"), _n(e, "y1") + _n(e, "height"))
            elif e.tag == "circle":
                r = _n(e, "r")
                p(_n(e, "cx") - r, _n(e, "cy") - r)
                p(_n(e, "cx") + r, _n(e, "cy") + r)
            elif e.tag == "polyline":
                for pk in (e.get("points") or "").split():
                    a, b = pk.split(",")
                    p(float(a), float(b))
            elif e.tag == "connector":
                p(_n(e, "cx"), _n(e, "cy"))
    if not xs:
        return (-12.0, -12.0, 12.0, 12.0)
    return (min(xs), min(ys), max(xs), max(ys))


def _stubs(s, conn, x0, y0, x1, y1):
    """Port line from the connector to the box (x0..x1, y0..y1)."""
    for _, _, cx, cy in conn:
        if cy >= y1:
            s.line(cx, cy, cx, y1)
        elif cy <= y0:
            s.line(cx, cy, cx, y0)
        elif cx <= x0:
            s.line(cx, cy, x0, cy)
        elif cx >= x1:
            s.line(cx, cy, x1, cy)


# ------------------------------------------------------------------------------------- flow paths of a valve
def block_paths(layer, ports):
    """Flow paths of one position block (symbol layer B<ports>_<no>) of a FluidSIM directional valve.

    The block is drawn from lines with an arrow or a blocking end (end-cap "arrow1" / "ortho1"), polylines and
    joints; that tells which port is connected with which, and in which direction. Only this data is used, not
    the drawing. ports: {(x, top): name} in block coordinates.
    @return list of (from, to) and the set of blocked ports"""
    segments, arrows, bars = [], set(), set()
    for e in layer:
        if e.tag == "line":
            x1, y1 = float(e.get("x1", 0)), float(e.get("y1", 0))
            x2, y2 = float(e.get("x2", e.get("x1", 0))), float(e.get("y2", e.get("y1", 0)))
            if e.get("end-cap") == "ortho1":
                bars.add((x1, y1))
                continue
            segments.append(((x1, y1), (x2, y2)))
            if e.get("end-cap") == "arrow1":
                arrows.add((x2, y2))
            if e.get("start-cap") == "arrow1":
                arrows.add((x1, y1))
        elif e.tag == "polyline":
            pk = [tuple(float(v) for v in p.split(",")) for p in e.get("points", "").split()]
            segments += list(zip(pk, pk[1:]))

    def on(p, s):
        (ax, ay), (bx, by) = s
        if min(ax, bx) - 1e-6 <= p[0] <= max(ax, bx) + 1e-6 and min(ay, by) - 1e-6 <= p[1] <= max(ay, by) + 1e-6:
            return abs((bx - ax) * (p[1] - ay) - (by - ay) * (p[0] - ax)) < 1e-6
        return False
    n = len(segments)
    parent = list(range(n))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for i in range(n):
        for j in range(n):
            if i != j and any(on(p, segments[j]) for p in segments[i]):
                parent[root(i)] = root(j)

    def port_at(p):
        if abs(abs(p[1]) - 12) > 0.5:
            return None
        return ports.get((round(p[0]), p[1] < 0))
    groups = {}
    for i, s in enumerate(segments):
        for p in s:
            a = port_at(p)
            if a:
                groups.setdefault(root(i), {})[a] = p
    paths = []
    for g in groups.values():
        if len(g) < 2:
            continue
        targets = [a for a, p in g.items() if p in arrows]
        sources = [a for a in g if a not in targets]
        if targets and sources:
            for q in sorted(sources, key=rank):
                for z in sorted(targets, key=rank):
                    paths.append((q, z))
        else:   # undirected or both ways: from the supply, else towards the exhaust
            names = sorted(g, key=rank)
            if rank(names[0])[0] == 0 or rank(names[-1])[0] < 4:
                paths += [(names[0], z) for z in names[1:]]
            else:
                paths += [(q, names[-1]) for q in names[:-1]]
    connected = {a for w in paths for a in w}
    blocked = {ports[(round(x), y < 0)] for x, y in bars if (round(x), y < 0) in ports}
    blocked |= set(ports.values()) - connected
    return paths, blocked - connected


def valve_positions(symbol):
    """Flow paths per position from the position blocks B<n>_<no> of a valve symbol, left to right, or None."""
    layers = [l for l in symbol.findall("symbol-layer") if re.match(r"^B\d+_\d+$", l.get("ident") or "")]
    con = next((l for l in symbol.findall("symbol-layer") if re.match(r"^CON\d+$", l.get("ident") or "")), None)
    if not layers or con is None:
        return None
    layers.sort(key=lambda l: float(l.get("ref-x", 0)))
    ports = {(round(float(c.get("cx"))), float(c.get("cy")) < 0): c.get("designation") or c.get("ident")
             for c in con.iter("connector")}
    return [block_paths(e, ports) for e in layers]


# ------------------------------------------------------------------------------------------ directional valve
def _valve(s, symbol, component, positions):
    blocks = sorted((l for l in symbol.findall("symbol-layer") if re.match(r"^B\d+_\d+$", l.get("ident") or "")),
                    key=lambda l: _n(l, "ref-x"))
    con = next((l for l in symbol.findall("symbol-layer") if re.match(r"^CON\d+$", l.get("ident") or "")), None)
    if not blocks or con is None:
        return False
    ports = {}      # name -> (x in the block, at the top?)
    for c in con.iter("connector"):
        ports[c.get("designation") or c.get("ident")] = (_n(c, "cx") + _n(con, "ref-x"), _n(c, "cy") < 0)
    width, height = 36.0, 24.0
    for layer in blocks:
        r = layer.find("rect")
        if r is not None:
            width, height = _n(r, "width", 36.0), _n(r, "height", 24.0)
            break
    half = height / 2
    oy = _n(blocks[0], "ref-y")
    left = min(_n(l, "ref-x") for l in blocks)
    right = max(_n(l, "ref-x") for l in blocks) + width
    # the ports sit at the normal position: its block is the reference of the ports' x positions
    try:
        normal = int(float(next((e.get("value") for e in component if e.get("ident") == "posNoInit"), "1")))
    except ValueError:
        normal = 1
    reference = _n(blocks[min(max(normal, 1), len(blocks)) - 1], "ref-x")
    for i, layer in enumerate(blocks):
        bx = _n(layer, "ref-x")
        dx = bx - reference
        s.box(bx, oy - half, width, height)
        paths, blocked = positions[i] if positions and i < len(positions) else ([], set())
        for a_name, b_name in paths:
            if a_name not in ports or b_name not in ports:
                continue
            (xa, ta), (xb, tb) = ports[a_name], ports[b_name]
            ya = oy - half if ta else oy + half
            yb = oy - half if tb else oy + half
            if ta == tb:            # both on one side: over the middle of the box
                s.path([(dx + xa, ya), (dx + xa, oy), (dx + xb, oy)])
                s.line(dx + xb, oy, dx + xb, yb, end="arrow1")
            else:
                s.line(dx + xa, ya, dx + xb, yb, end="arrow1")
        for name in blocked:
            if name in ports:
                x, top = ports[name]
                y = oy - half if top else oy + half
                s.line(dx + x, y, dx + x, y + (5 if top else -5), end="ortho1")
    for c in con.iter("connector"):
        cx, cy = _n(con, "ref-x") + _n(c, "cx"), _n(con, "ref-y") + _n(c, "cy")
        s.line(cx, cy, cx, oy - half if cy < oy else oy + half)
    # pilot ports (12, 14 ...) on the actuation layers: a line up to the actuation
    for layer, c, cx, cy in connectors(symbol):
        name = layer.get("ident") or ""
        if layer is con or re.match(r"^B\d+_\d+$", name):
            continue
        edge = left if cx < (left + right) / 2 else right
        target_x = edge - 6 if edge == left else edge + 6
        s.path([(cx, cy), (cx, oy), (target_x, oy)] if abs(cy - oy) > 0.5 else [(cx, cy), (target_x, oy)])
    # actuations left and right, stacked outwards
    names = [l.get("ident") or "" for l in symbol.findall("symbol-layer") if l.get("visible") != "false"]
    values = {e.get("ident"): e.get("value") for e in component if e.tag in ("const", "parameter")}
    for side, edge, r in (("L", left, -1), ("R", right, 1)):
        x = edge
        for n in names:
            prefix = next((v for v in ACTUATIONS if n.startswith(v)), None)
            if prefix is None or n[len(prefix):len(prefix) + 1] != side:
                continue
            if prefix == "AEM":
                s.box(min(x, x + 8 * r), oy - 6, 8, 12)
                s.line(min(x, x + 8 * r), oy + 6, max(x, x + 8 * r), oy - 6)
                x += 8 * r
            elif prefix == "AMAN":
                s.line(x, oy, x + 8 * r, oy)
                s.line(x + 8 * r, oy - 5, x + 8 * r, oy + 5)
                x += 8 * r
            elif prefix == "AMECH":
                s.line(x, oy, x + 5 * r, oy)
                s.circle(x + 8 * r, oy, 3)
                x += 11 * r
            else:                   # pneumatic / piloted
                s.path([(x, oy), (x + 6 * r, oy - 4), (x + 6 * r, oy + 4), (x, oy)], filled=True)
                x += 6 * r
        if values.get("spring" + side) == "True":
            s.spring(x, oy, x + 12 * r)
    return True


# ------------------------------------------------------------------------------------------- other classes
def _cylinder(s, conn, u, single_acting):
    x0, y0, x1, y1 = u
    middle = (y0 + y1) / 2
    body = x0 + (x1 - x0) * 0.72
    high = min(y1 - y0, 24.0)
    top, bottom = middle - high / 2, middle + high / 2
    s.box(x0, top, body - x0, high)
    piston = x0 + (body - x0) * 0.25
    s.line(piston, top, piston, bottom)
    s.box(piston, middle - 1.5, x1 - piston, 3)
    if single_acting:
        s.spring(piston + 2, middle - high / 4, body - 2, 3)
    for _, _, cx, cy in conn:
        s.line(cx, cy, cx, bottom if cy > middle else top)


def _source(s, conn, u, hydraulic):
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    r = max(min(x1 - x0, y1 - y0) / 2 - 2, 5)
    s.circle(cx, cy, r)
    triangle = [(cx, cy - r + 1), (cx - 3, cy - r + 6), (cx + 3, cy - r + 6), (cx, cy - r + 1)]
    s.path(triangle, filled=hydraulic)
    for _, _, ax, ay in conn:
        if ay < cy - r:
            s.line(ax, ay, ax, cy - r)
        elif ay > cy + r:
            s.line(ax, ay, ax, cy + r)
        elif ax < cx:
            s.line(ax, ay, cx - r, ay)
        else:
            s.line(ax, ay, cx + r, ay)


def _tank(s, conn, u):
    x0, y0, x1, y1 = u
    s.path([(x0, y0), (x0, y1), (x1, y1), (x1, y0)])
    for _, _, ax, ay in conn:
        s.line(ax, ay, ax, y1 - 3)


def _gauge(s, conn, u):
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    r = max(min(x1 - x0, y1 - y0) / 2 - 1, 5)
    s.circle(cx, cy, r)
    s.line(cx - r * 0.6, cy + r * 0.6, cx + r * 0.6, cy - r * 0.6, end="arrow1")
    for _, _, ax, ay in conn:
        _lead(s, ax, ay, cx, cy, r)


def _box_with_sign(s, conn, u, sign):
    x0, y0, x1, y1 = u
    if len(conn) >= 2:
        # a box between the connectors instead of the whole original extent
        xs, ys = [a[2] for a in conn], [a[3] for a in conn]
        x0, x1 = max(x0, min(xs) + 4), min(x1, max(xs) - 4)
        y0, y1 = max(y0, min(ys) + 4), min(y1, max(ys) - 4)
        if x1 - x0 < 16:
            x0, x1 = (min(xs) + max(xs)) / 2 - 12, (min(xs) + max(xs)) / 2 + 12
        if y1 - y0 < 16:
            y0, y1 = (min(ys) + max(ys)) / 2 - 8, (min(ys) + max(ys)) / 2 + 8
    if x1 - x0 < 12:
        x0, x1 = (x0 + x1) / 2 - 6, (x0 + x1) / 2 + 6
    if y1 - y0 < 12:
        y0, y1 = (y0 + y1) / 2 - 6, (y0 + y1) / 2 + 6
    s.box(x0, y0, x1 - x0, y1 - y0)
    if sign:
        h = max(min(7.0, (y1 - y0) * 0.5, (x1 - x0) / max(len(sign), 1) * 1.6), 4.0)
        s.text((x0 + x1) / 2 - len(sign) * h * 0.28, (y0 + y1) / 2 + h * 0.4, sign, h)
    _stubs(s, conn, x0, y0, x1, y1)


def _contact(s, conn, kind, actuated):
    """Contact between the upper and the lower connector (IEC 60617), changeover with a third connector."""
    if len(conn) < 2:
        return False
    pk = sorted(conn, key=lambda a: a[3])
    upper, lower = pk[0], pk[-1]
    x, xl = upper[2], lower[2]
    ya, yb = upper[3], lower[3]
    m1, m2 = ya + (yb - ya) * 0.35, ya + (yb - ya) * 0.65
    s.line(x, ya, x, m1)
    s.line(xl, yb, xl, m2)
    if kind == "break":
        s.line(x, m1, x + 6, m1)
        s.line(xl, m2, x + 7, m1 - 2)
    else:
        s.line(xl, m2, xl - 7, m1 - 1)
    for a in pk[1:-1]:
        s.line(a[2], a[3], a[2], m1)
    if actuated:
        m = (m1 + m2) / 2
        s.line(xl - 4, m, xl - 12, m, dash="dash")
        s.line(xl - 12, m - 4, xl - 12, m + 4)
    return True


def _coil(s, conn, valve):
    if len(conn) < 2:
        return False
    pk = sorted(conn, key=lambda a: a[3])
    x = pk[0][2]
    m = (pk[0][3] + pk[-1][3]) / 2
    s.box(x - 8, m - 5, 16, 10)
    if valve:
        s.line(x - 8, m + 5, x + 8, m - 5)
    s.line(x, pk[0][3], x, m - 5)
    s.line(pk[-1][2], pk[-1][3], pk[-1][2], m + 5)
    return True


def _lamp(s, conn):
    if len(conn) < 2:
        return False
    pk = sorted(conn, key=lambda a: a[3])
    x = pk[0][2]
    m = (pk[0][3] + pk[-1][3]) / 2
    s.circle(x, m, 6)
    s.line(x - 4.2, m - 4.2, x + 4.2, m + 4.2)
    s.line(x - 4.2, m + 4.2, x + 4.2, m - 4.2)
    s.line(x, pk[0][3], x, m - 6)
    s.line(pk[-1][2], pk[-1][3], pk[-1][2], m + 6)
    for a in pk[1:-1]:                  # e.g. PE of a mains lamp
        _lead(s, a[2], a[3], x, m, 6)
    return True


def _potential(s, conn, text, u=None):
    """Potential (24V, 0V, L1 ... PE): a stub from the connector into the part, a bar, the name well readable.
    When the FluidSIM part lies sideways (connector at the end of a horizontal extent), the stub is horizontal
    too; else potentials stacked on top of each other reach into the wires of their neighbours."""
    if not conn:
        return False
    _, _, x, y = conn[0]
    h = 8.0
    if u is not None and (u[2] - u[0]) > (u[3] - u[1]) + 2 and abs(x - (u[0] + u[2]) / 2) > 2:
        xb = x - 8 if x > (u[0] + u[2]) / 2 else x + 8
        s.line(x, y, xb, y)
        s.line(xb, y - 6, xb, y + 6)
        width = len(text) * h * 0.6
        s.text(xb - 2 - width if xb < x else xb + 2, y + h * 0.35, text, h)
        return True
    upper = y >= ((u[1] + u[3]) / 2 if u is not None else 0)
    yb = y - 8 if upper else y + 8
    s.line(x, y, x, yb)
    s.line(x - 6, yb, x + 6, yb)
    s.text(x + 8, yb + (2 if upper else h), text, h)
    return True


# --------------------------------------------------------------------------------------------- electrical
TWO_TERMINAL = ("Resistor", "Potentiometer", "Capacitor", "CapacitorPol", "Diode", "LED", "Fuse", "ConstantVoltage")
CIRCLE_SOURCE = ("WaveformGenerator", "Oscilloscope")


def _two_terminal(s, conn, obj_ref):
    """Two-terminal part after IEC 60617 between the connectors p and n: resistor, potentiometer, capacitor,
    diode/LED, fuse, DC source; leads up to the body."""
    el = [a for a in conn if a[1].get("type") == "electric"]
    if len(el) < 2:
        return False
    plus = next((a for a in el if a[1].get("designation") == "p"), el[0])
    minus = next((a for a in el if a[1].get("designation") == "n" and a is not plus),
                 next(a for a in el if a is not plus))
    ax = Axis((plus[2], plus[3]), (minus[2], minus[3]))
    part = obj_ref.split(".")[-1]
    if part.startswith(("Capacitor", "ConstantVoltage")):
        L = 1.5
    elif part.startswith(("Diode", "LED")):
        L = min(4.0, ax.half - 1)
    else:
        L = max(min(9.0, ax.half - 2), 3.0)
    s.line(plus[2], plus[3], *ax.p(-L))
    s.line(*ax.p(L), minus[2], minus[3])
    if part.startswith(("Resistor", "Potentiometer", "Fuse")):
        s.path([ax.p(-L, -3.5), ax.p(L, -3.5), ax.p(L, 3.5), ax.p(-L, 3.5), ax.p(-L, -3.5)])
        if part.startswith("Fuse"):
            s.line(*ax.p(-L), *ax.p(L))
        if part.startswith("Potentiometer"):
            wiper = [a for a in el if a is not plus and a is not minus]
            if wiper:
                w = wiper[0]
                target = min((ax.p(0, -3.5), ax.p(0, 3.5)), key=lambda q: (q[0] - w[2]) ** 2 + (q[1] - w[3]) ** 2)
                s.line(w[2], w[3], *target, end="arrow1")
    elif part.startswith("Capacitor"):
        for u in (-1.5, 1.5):
            s.line(*ax.p(u, -6), *ax.p(u, 6))
        if part == "CapacitorPol":
            q = ax.p(-5, -7)
            s.text(q[0] - 2, q[1] + 2, "+", 5)
    elif part.startswith("ConstantVoltage"):
        s.line(*ax.p(-1.5, -7), *ax.p(-1.5, 7))
        s.line(*ax.p(1.5, -3.5), *ax.p(1.5, 3.5))
        q = ax.p(-5, -8)
        s.text(q[0] - 2, q[1] + 2, "+", 5)
    else:   # diode, LED: triangle in the forward direction p -> n, bar at the cathode
        s.path([ax.p(-L, -4.5), ax.p(-L, 4.5), ax.p(L, 0), ax.p(-L, -4.5)])
        s.line(*ax.p(L, -4.5), *ax.p(L, 4.5))
        if part.startswith("LED"):
            for u in (-1.5, 1.5):
                a, b = ax.p(u, -6), ax.p(u + 4, -11)
                s.line(*a, *b, end="arrow1")
    return True


def _circle_source(s, conn, u):
    """Signal generator or oscilloscope: a circle with a sine (IEC 60617), leads up to the circle."""
    el = [a for a in conn if a[1].get("type") == "electric"]
    if not el:
        return False
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    r = max(min(x1 - x0, y1 - y0) / 2 * 0.55, 6)
    s.circle(cx, cy, r)
    s.path([(cx - r * 0.6 + r * 1.2 * i / 12, cy - r * 0.35 * math.sin(i / 12 * 2 * math.pi))
            for i in range(13)])
    for _, _, x, y in el:
        dx, dy = x - cx, y - cy
        length = (dx * dx + dy * dy) ** 0.5 or 1.0
        if length > r:
            s.line(x, y, cx + dx / length * r, cy + dy / length * r)
    return True


def _transistor(s, conn, pnp):
    """Bipolar transistor (IEC 60617): base bar, collector and emitter slanted, arrow at the emitter (NPN out,
    PNP in), circle; connectors B, C, E."""
    el = {a[1].get("designation"): a for a in conn if a[1].get("type") == "electric"}
    if not all(k in el for k in ("B", "C", "E")):
        return False
    b, c, e = el["B"], el["C"], el["E"]
    bx = b[2] + (c[2] - b[2]) * 0.55
    cy, ey = c[3], e[3]
    my = (cy + ey) / 2
    h = abs(ey - cy) * 0.22
    s.line(b[2], b[3], bx, b[3])
    s.line(bx, my - h, bx, my + h)
    kx = c[2]
    s.line(bx, my - h * 0.5, kx, my - h * 1.6)
    s.line(kx, my - h * 1.6, kx, cy)
    if pnp:
        s.line(kx, my + h * 1.6, bx, my + h * 0.5, end="arrow1")
    else:
        s.line(bx, my + h * 0.5, kx, my + h * 1.6, end="arrow1")
    s.line(kx, my + h * 1.6, kx, ey)
    r = max(abs(kx - bx), h * 1.8) * 0.95
    s.circle((bx + kx) / 2, my, r)
    return True


def _electric(conn):
    return [a for a in conn if a[1].get("type") == "electric"]


def _lead(s, x, y, cx, cy, r):
    """A lead from (x, y), vertical or horizontal, up to a circle about (cx, cy)."""
    dx, dy = x - cx, y - cy
    if (dx * dx + dy * dy) ** 0.5 <= r:
        return
    if abs(dx) < r * 0.95:
        h = (r * r - dx * dx) ** 0.5
        s.line(x, y, x, cy + (h if dy > 0 else -h))
    elif abs(dy) < r * 0.95:
        h = (r * r - dy * dy) ** 0.5
        s.line(x, y, cx + (h if dx > 0 else -h), y)
    else:                       # corner: first vertical up to 0.7 r, then horizontal to the circle
        yy = cy + (0.7 * r if dy > 0 else -0.7 * r)
        h = (r * r - (yy - cy) ** 2) ** 0.5
        s.line(x, y, x, yy)
        s.line(x, yy, cx + (h if dx > 0 else -h), yy)


def _circle_with_text(s, conn, u, rows, factor=0.75):
    """Machine or meter: a circle with its sign (M 3~, G, cos phi, Ohm ...), leads to the circle."""
    el = _electric(conn)
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    r = max(min(x1 - x0, y1 - y0) / 2 * factor, 7)
    if el and min(((a[2] - cx) ** 2 + (a[3] - cy) ** 2) ** 0.5 for a in el) < r + 3:
        # the connectors would lie inside the circle (FluidSIM draws some inside): look for a middle and a
        # radius that keep every connector at least 4 units outside while the circle stays as large as can be
        best = (0.0, cx, cy)
        for i in range(25):
            for j in range(25):
                px, py = x0 + (x1 - x0) * i / 24, y0 + (y1 - y0) * j / 24
                rr = min(min(((a[2] - px) ** 2 + (a[3] - py) ** 2) ** 0.5 for a in el) - 4,
                         px - x0, x1 - px, py - y0, y1 - py, r)
                if rr > best[0]:
                    best = (rr, px, py)
        # rather centred under/over the rows of connectors (windings at the top, excitation at the bottom), so
        # every lead reaches the circle vertically, when the circle does not get much smaller there
        count = {}
        for a in el:
            count[round(a[3])] = count.get(round(a[3]), 0) + 1
        main = max(count, key=lambda y: (count[y], -abs(y - cy)))            # the row with most connectors
        opposite = [y for y in count if count[y] >= 2 and (y - cy) * (main - cy) < 0]
        upper = [a for a in el if round(a[3]) == main]
        lower = [a for a in el if opposite and round(a[3]) == max(opposite, key=lambda y: abs(y - main))]
        if len(upper) >= 2:
            px = sorted(a[2] for a in upper)[len(upper) // 2]
            py = (main + lower[0][3]) / 2 if lower else cy
            rr = min(min(((a[2] - px) ** 2 + (a[3] - py) ** 2) ** 0.5 for a in el) - 4, r)
            if rr >= 0.8 * best[0] and all(abs(a[2] - px) < rr for a in upper + lower):
                best = (rr, px, py)
        if best[0] >= 7:
            r, cx, cy = best
    s.circle(cx, cy, r)
    h = min(r * 0.55, 9)
    for i, z in enumerate(rows):
        yy = cy + h * 0.4 + (i - (len(rows) - 1) / 2) * h * 1.15
        s.text(cx - len(z) * h * 0.28, yy, z, h)
    for _, _, ax, ay in el:
        _lead(s, ax, ay, cx, cy, r)
    return True


def _multi_pole(s, conn, obj_ref):
    """Multi-pole switch (IEC 60617): one make contact per pole between 1-2, 3-4, 5-6; the poles coupled by a
    dashed mechanical link; circuit breakers with a cross at the fixed contact, motor protection switches also
    with a release (a box on the link)."""
    el = {a[1].get("designation") or a[1].get("ident"): a for a in _electric(conn)}
    poles = []
    for n in range(1, 12, 2):
        a, b = el.get(str(n)), el.get(str(n + 1))
        if a is not None and b is not None:
            poles.append((a, b))
    if not poles:
        return False
    middles = []
    for a, b in poles:
        ax = Axis((a[2], a[3]), (b[2], b[3]))
        L = min(7.0, ax.half - 2)
        s.line(a[2], a[3], *ax.p(-L))
        s.line(*ax.p(L), b[2], b[3])
        s.line(*ax.p(L), *ax.p(-L - 1, -6))           # contact blade, open
        if "Breaker" in obj_ref or "Protection" in obj_ref:
            s.line(*ax.p(-L - 2.5, -2.5), *ax.p(-L + 2.5, 2.5))
            s.line(*ax.p(-L - 2.5, 2.5), *ax.p(-L + 2.5, -2.5))
        middles.append(ax.p(0, -3.5))
    if len(middles) > 1:
        s.line(*middles[0], *middles[-1], dash="dash")
    if "MotorCircuitBreaker" in obj_ref or "MotorProtection" in obj_ref:
        x, y = middles[0]
        s.line(x, y, x - 14, y, dash="dash")
        s.box(x - 24, y - 5, 10, 10)
        s.text(x - 23.5, y + 2.5, "I>", 5)
    return True


def _converter(s, conn, u, left_sign, right_sign):
    """Rectifier, power supply, converter: a box with a diagonal, the kind of current top left and bottom right."""
    el = _electric(conn)
    x0, y0, x1, y1 = u
    xs = [a[2] for a in el]
    if xs:
        x0, x1 = min(xs) + 8, max(xs) - 8
    if x1 - x0 < 16:
        m = (x0 + x1) / 2
        x0, x1 = m - 10, m + 10
    s.box(x0, y0, x1 - x0, y1 - y0)
    s.line(x0, y1, x1, y0)
    h = max(min(8.0, (y1 - y0) * 0.4), 5.0)
    s.text(x0 + 1.5, y0 + h + 0.5, left_sign, h)
    s.text(x1 - h * 0.7 - 1.5, y1 - 1.5, right_sign, h)
    _stubs(s, el, x0, y0, x1, y1)
    return True


def _transformer(s, conn, u):
    el = _electric(conn)
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    r = max(min(x1 - x0, y1 - y0) / 2 * 0.5, 6)
    vertical = (y1 - y0) >= (x1 - x0)
    k1 = (cx, cy - r * 0.6) if vertical else (cx - r * 0.6, cy)
    k2 = (cx, cy + r * 0.6) if vertical else (cx + r * 0.6, cy)
    s.circle(*k1, r)
    s.circle(*k2, r)
    for _, _, ax, ay in el:
        k = k1 if (ay < cy if vertical else ax < cx) else k2
        _lead(s, ax, ay, k[0], k[1], r)
    return True


def _inductor(s, conn):
    """Inductor: four half circles between p and n."""
    el = _electric(conn)
    if len(el) < 2:
        return False
    a, b = el[0], el[1]
    ax = Axis((a[2], a[3]), (b[2], b[3]))
    L = min(10.0, ax.half - 1)
    s.line(a[2], a[3], *ax.p(-L))
    s.line(*ax.p(L), b[2], b[3])
    pk = []
    for i in range(4):
        u0 = -L + i * L / 2
        for j in range(9):
            t = math.pi * j / 8
            pk.append(ax.p(u0 + L / 4 - math.cos(t) * L / 4, -math.sin(t) * L / 4))
    s.path(pk)
    return True


def _diode_kind(s, conn, obj_ref):
    """Zener diode (bent bar), thyristor (gate), photo diode (arrows in)."""
    if not _two_terminal(s, conn, "ET.Semiconductor.Diode"):
        return False
    el = _electric(conn)
    plus = next((a for a in el if a[1].get("designation") in ("p", "A")), el[0])
    minus = next((a for a in el if a is not plus and a[1].get("designation") in ("n", "K")), el[1])
    ax = Axis((plus[2], plus[3]), (minus[2], minus[3]))
    L = min(4.0, ax.half - 1)
    if "ZDiode" in obj_ref:
        s.line(*ax.p(L, -4.5), *ax.p(L - 2, -4.5))
        s.line(*ax.p(L, 4.5), *ax.p(L + 2, 4.5))
    elif "Thyristor" in obj_ref:
        gate = [a for a in el if a is not plus and a is not minus]
        if gate:
            s.line(gate[0][2], gate[0][3], *ax.p(L, 4.5))
    elif "Photo" in obj_ref:
        for u0 in (-1.5, 1.5):
            s.line(*ax.p(u0 + 4, -11), *ax.p(u0, -6), end="arrow1")
    return True


def _amplifier(s, conn, u):
    """Operational amplifier (IEC 60617): a triangle towards the output, inputs + and -."""
    el = _electric(conn)
    if len(el) < 3:
        return False
    x0, y0, x1, y1 = u
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    out = max(el, key=lambda a: a[2])
    inputs = [a for a in el if a[2] < cx - 2]
    xa, xb = cx - 12, cx + 12
    h = 14
    s.path([(xa, cy - h), (xa, cy + h), (xb, cy), (xa, cy - h)])
    s.text(xa + 2, cy - h / 2 + 2, "−", 6)
    s.text(xa + 2, cy + h / 2 + 2, "+", 6)
    if abs(out[3] - cy) < 1:
        s.line(out[2], out[3], xb, cy)
    else:
        s.path([(out[2], out[3]), (xb + 3, out[3]), (xb + 3, cy), (xb, cy)])
    for a in inputs:
        yy = min(max(a[3], cy - h + 3), cy + h - 3)
        s.path([(a[2], a[3]), (xa - 3, a[3]), (xa - 3, yy), (xa, yy)])
    for a in el:
        if a is not out and a not in inputs:     # supply at the top/bottom
            s.line(a[2], a[3], a[2], cy - h * 0.5 if a[3] < cy else cy + h * 0.5)
    return True


def _distributor(s, conn):
    """Distributor: all connectors on one joint."""
    pk = [(a[2], a[3]) for a in conn]
    if not pk:
        return False
    cx = sum(p[0] for p in pk) / len(pk)
    cy = sum(p[1] for p in pk) / len(pk)
    for x, y in pk:
        if abs(x - cx) > 1:
            s.line(x, y, x, cy)
            s.line(x, cy, cx, cy)
        else:
            s.line(x, y, cx, cy)
    s.circle(cx, cy, 1.5, filled=True)
    return True


def _terminals(s, conn):
    el = _electric(conn)
    for _, _, x, y in el:
        s.circle(x, y, 2.5)
    return bool(el)


# Signs after IEC 60617 for machines, meters and function blocks (never a text box with the name)
MACHINES = (("Synchronous", ["MS", "3~"]), ("SquirrelCage", ["M", "3~"]), ("ThreePhaseAlternator", ["G", "3~"]),
            ("DC_Motor", ["M", "="]), ("DCMotor", ["M", "="]), ("UniversalMotor", ["M", "1~"]),
            ("CapacitorMotor", ["M", "1~"]), ("EngineTestBench", ["G"]), ("LinearSolenoid", ["M"]),
            ("Generator", ["G"]), ("Motor", ["M"]))
METERS = (("Voltmeter", ["V"]), ("AmpereMeter", ["A"]), ("PowerFactorMeter", ["cos φ"]), ("WattMeter", ["W"]),
          ("OhmMeter", ["Ω"]), ("Stopwatch", ["t"]),
          ("FrequencyMeter", ["Hz"]), ("Tachometer", ["n"]))
SIGNS = (("ControllerPID", "PID"), ("ControllerPT1", "PT1"), ("ControllerPT2", "PT2"),
         ("ControllerGain", "K"), ("ControllerIntegrator", "∫"), ("ControllerDerivator", "d/dt"),
         ("ControllerHysteresis", "Hyst"), ("ControllerS", "S"), ("Add3Amplifier", "Σ"),
         ("DifferentialAmplifier", "Δ"), ("PropAmplifier", "▷"), ("GrafcetPLC", "PLC"),
         ("GrafcetIO", "PLC"), ("DDEIn", "DDE"), ("DDEOut", "DDE"), ("SetPointGenerator", "w"),
         ("PulseWidthGenerator", "PWM"), ("DC_DC_Transformer", "=/="), ("SignalConverter", "U/I"),
         ("PositionSensor", "s"), ("PositionTransmitter", "s"), ("PressureSensor", "p"), ("FlowSensor", "Q"),
         ("Flowmeter", "Q"), ("PressureSwitch", "p"), ("SafetyRelay", "PNOZ"), ("Injector", "Inj"),
         ("Heater", "+ϑ"), ("Cooler", "−ϑ"), ("ConveyorModule", "M"), ("Timer", "t"),
         ("PneumaticCounter", "Z"))


def sign(obj_ref):
    part = obj_ref.split(".")[-1]
    return next((k for n, k in SIGNS if part.startswith(n) or n in part), "")


def _other_electric(s, conn, u, obj_ref):
    """Draws the remaining electrical parts; @return False when nothing fits."""
    part = obj_ref.split(".")[-1]
    if obj_ref.startswith("ET.Source.") and part in ("L1", "L2", "L3", "N", "PE"):
        return _potential(s, conn, part, u)
    if obj_ref.startswith("ET.Switch.") and re.search(r"CircuitBreaker|MotorProtection", part):
        return _multi_pole(s, conn, obj_ref)
    for n, z in MACHINES:
        if n in part:
            return _circle_with_text(s, conn, u, z)
    for n, z in METERS:
        if part.startswith(n):
            return _circle_with_text(s, conn, u, z, 0.6)
    if part.startswith("AC_DC_Transformer"):
        return _converter(s, conn, u, "~", "=")
    if part == "Transformer":
        return _transformer(s, conn, u)
    if part.startswith("Inductor"):
        return _inductor(s, conn)
    if re.match(r"ZDiode|Thyristor|Photodiode", part):
        return _diode_kind(s, conn, obj_ref)
    if part.startswith("OpAmp"):
        return _amplifier(s, conn, u)
    if part.startswith(("IndicatorLight", "BlinkingIndicator")):
        return _lamp(s, conn)
    if obj_ref.startswith("ET.Misc.DistributorPlate") or part in ("MultiPin", "ProtectiveContactSocket"):
        return _terminals(s, conn)
    return False


def _reach_connectors(s, conn, u):
    """Every connector must hang on the graphics (else the wire ends in nothing): a connector without any line
    gets a stub towards the middle of the part, up to the nearest drawn point."""
    points = []
    for e in s.layer:
        if e.tag == "line":
            points += [(_n(e, "x1"), _n(e, "y1")), (_n(e, "x2"), _n(e, "y2"))]
        elif e.tag == "polyline":
            for pk in (e.get("points") or "").split():
                a, b = pk.split(",")
                points.append((float(a), float(b)))
        elif e.tag == "rect":
            x, y, b, h = _n(e, "x1"), _n(e, "y1"), _n(e, "width"), _n(e, "height")
            points += [(x + b * i / 8, y) for i in range(9)] + [(x + b * i / 8, y + h) for i in range(9)]
            points += [(x, y + h * i / 8) for i in range(9)] + [(x + b, y + h * i / 8) for i in range(9)]
        elif e.tag == "circle":
            cx, cy, r = _n(e, "cx"), _n(e, "cy"), _n(e, "r")
            points += [(cx + r * math.cos(t / 8 * math.pi), cy + r * math.sin(t / 8 * math.pi)) for t in range(16)]
    if not points:
        return
    x0, y0, x1, y1 = u
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    for _, _, ax, ay in conn:
        if min(abs(px - ax) + abs(py - ay) for px, py in points) < 1.0:
            continue
        target = min(points, key=lambda q: (q[0] - ax) ** 2 + (q[1] - ay) ** 2)
        if abs(target[0] - ax) < 1.0 or abs(target[1] - ay) < 1.0:
            s.line(ax, ay, *target)
        elif abs(ax - mx) >= abs(ay - my):   # connector at the side: horizontal first
            s.line(ax, ay, target[0], ay)
            s.line(target[0], ay, *target)
        else:
            s.line(ax, ay, ax, target[1])
            s.line(ax, target[1], *target)


# ------------------------------------------------------------------------------- digital (IEC 60617-12)
# Sign in the box; negated = negation circle at the output. LOGO blocks with the names after IEC 61131-3.
LOGIC = (("NAND", "&", True), ("NOR", "≥1", True), ("XOR", "=1", False), ("Xor", "=1", False),
         ("AND", "&", False), ("And", "&", False), ("OR", "≥1", False), ("Or", "≥1", False),
         ("Nand", "&", True), ("Nor", "≥1", True), ("Inverter", "1", True), ("Not", "1", True),
         ("SchmittTriggerInverting", "1", True), ("SwitchOnOffDelay", "TONOF", False),
         ("SwitchSaveOnDelay", "TONR", False), ("SwitchOnDelay", "TON", False), ("SwitchOffDelay", "TOF", False),
         ("WipingRelay", "TP", False), ("SelfMaintainingRelay", "RS", False), ("ImpulseRelay", "T", False),
         ("ImpulseGenerator", "G", False), ("ForAndBackCounter", "CTUD", False), ("TimeSwitch", "TS", False),
         ("BufferD", "D", False), ("Lo", "0", False), ("Hi", "1", False), ("RS_Flipflop", "RS", False),
         ("Synchronous_RS", "RS", False), ("D_Flipflop", "D", False), ("JK", "JK", False), ("T_Flipflop", "T", False),
         ("Monoflop", "1⊓", False), ("CTR4", "CTR4", False), ("ShiftRegister", "SRG8", False),
         ("HalfAdder", "Σ", False), ("FullAdder", "Σ", False), ("SevenSegment", "8", False),
         ("HEX_switch", "HEX", False), ("IC06_74HC74", "D", False), ("IC06_74HC109", "JK", False),
         ("Logobox", "PLC", False), ("Switch", "I", False), ("OutputQ", "Q", False))


def _logic(s, conn, u, obj_ref):
    """Logic block: a box with its sign (&, >=1, =1, 1, TON, RS ...), inputs left, outputs right, a negation
    circle before the output; connectors at the top/bottom (supply, clock) with a stub."""
    part = obj_ref.split(".")[-1]
    mark, negated = next(((z, n) for name, z, n in LOGIC if part.startswith(name)), ("", False))
    if not conn:
        return False
    x0, y0, x1, y1 = u
    xs = [a[2] for a in conn]
    left = [a for a in conn if a[2] <= min(xs) + 0.5 and a[2] < (x0 + x1) / 2]
    right = [a for a in conn if a[2] >= max(xs) - 0.5 and a[2] > (x0 + x1) / 2]
    bx0 = (min(xs) + 6) if left else x0
    bx1 = (max(xs) - 6 - (4 if negated else 0)) if right else x1
    if bx1 - bx0 < 14:
        m = (bx0 + bx1) / 2
        bx0, bx1 = m - 7, m + 7
    s.box(bx0, y0, bx1 - bx0, y1 - y0)
    if mark:
        h = max(min(8.0, (y1 - y0) * 0.45, (bx1 - bx0) / max(len(mark), 1) * 1.4), 4.0)
        s.text((bx0 + bx1) / 2 - len(mark) * h * 0.3, y0 + h + 1.5, mark, h)
    for a in conn:
        ax, ay = a[2], a[3]
        if a in left:
            s.line(ax, ay, bx0, ay)
        elif a in right:
            if negated:
                s.circle(bx1 + 2, ay, 2)
                s.line(ax, ay, bx1 + 4, ay)
            else:
                s.line(ax, ay, bx1, ay)
        elif ay <= y0:
            s.line(ax, ay, ax, y0)
        elif ay >= y1:
            s.line(ax, ay, ax, y1)
    return True


class Axis:
    """Local coordinates along the connection of two connectors: u along it (from a to b), v across."""

    def __init__(self, a, b):
        self.m = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = (dx * dx + dy * dy) ** 0.5 or 1.0
        self.d = (dx / length, dy / length)
        self.n = (-self.d[1], self.d[0])
        self.half = length / 2

    def p(self, u, v=0.0):
        return (self.m[0] + u * self.d[0] + v * self.n[0], self.m[1] + u * self.d[1] + v * self.n[1])


def _check(s, ax, u0, v=0.0):
    """A ball with its seat; free in the +u direction (from the tip of the seat to the ball)."""
    k = ax.p(u0 + 2, v)
    s.circle(k[0], k[1], 3)
    s.path([ax.p(u0 - 2, v - 4), ax.p(u0 - 5, v), ax.p(u0 - 2, v + 4)])


def _throttle(s, ax, u0=0.0, v=0.0):
    s.path([ax.p(u0 - 5, v - 3), ax.p(u0, v - 1), ax.p(u0 + 5, v - 3)])
    s.path([ax.p(u0 - 5, v + 3), ax.p(u0, v + 1), ax.p(u0 + 5, v + 3)])


def _inline(s, conn, obj_ref):
    """Parts between two connectors: check, throttle, one-way flow control, shut-off valve, filter."""
    if len(conn) != 2 and not ("PilotToOpen" in obj_ref and len(conn) == 3):
        return False
    by_ident = {a[1].get("ident"): a for a in conn}
    if "PilotToOpen" in obj_ref:
        a, b = by_ident.get("1"), by_ident.get("3")       # A -> B free, X pilots
    else:
        a, b = by_ident.get("2"), by_ident.get("1")       # free from connector 2 to 1
    if a is None or b is None:
        a, b = conn[0], conn[1]
    ax = Axis((a[2], a[3]), (b[2], b[3]))
    r = min(14.0, ax.half - 1)
    s.line(a[2], a[3], *ax.p(-r))
    s.line(b[2], b[3], *ax.p(r))
    if "ThrottleCheck" in obj_ref:
        s.line(*ax.p(-r), *ax.p(-6))
        s.line(*ax.p(6), *ax.p(r))
        _throttle(s, ax)
        s.path([ax.p(-r + 2), ax.p(-r + 2, 9), ax.p(-6, 9)])
        s.path([ax.p(6, 9), ax.p(r - 2, 9), ax.p(r - 2)])
        _check(s, ax, 0, 9)
        s.line(*ax.p(-6, -6), *ax.p(6, 6), end="arrow1")
    elif "CheckValve" in obj_ref:
        s.line(*ax.p(-r), *ax.p(-5))
        _check(s, ax, 0)
        s.line(*ax.p(5), *ax.p(r))
        if "PilotToOpen" in obj_ref:
            x = next((c for c in conn if c[1].get("ident") == "2"), None)
            if x is not None:
                s.line(x[2], x[3], *ax.p(0, 8), dash="dash")
    elif "ShutoffValve" in obj_ref:
        s.path([ax.p(-r), ax.p(-r, -4), ax.p(r, 4), ax.p(r), ax.p(r, -4), ax.p(-r, 4), ax.p(-r)])
    elif "Filter" in obj_ref:
        s.path([ax.p(-r), ax.p(0, -r * 0.8), ax.p(r), ax.p(0, r * 0.8), ax.p(-r)])
        s.line(*ax.p(0, -r * 0.8), *ax.p(0, r * 0.8), dash="dash")
    else:                           # throttle / flow control valve
        s.line(*ax.p(-r), *ax.p(-5))
        _throttle(s, ax)
        s.line(*ax.p(5), *ax.p(r))
        s.line(*ax.p(-6, -6), *ax.p(6, 6), end="arrow1")
    return True


def _pressure_valve(s, conn, regulator):
    """Pressure relief or pressure regulating valve: a box with an arrow and a spring, ports in/out."""
    if len(conn) < 2:
        return False
    xs, ys = [a[2] for a in conn], [a[3] for a in conn]
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    s.box(cx - 9, cy - 9, 18, 18)
    if regulator:
        s.line(cx - 3, cy + 9, cx - 3, cy - 9, end="arrow1")
    else:
        s.line(cx - 3, cy + 7, cx + 3, cy - 7, end="arrow1")
    s.spring(cx + 9, cy, cx + 19, 3)
    s.line(cx - 9, cy + 6, cx - 15, cy + 6, dash="dash")
    for _, _, x, y in conn:
        if abs(y - cy) >= abs(x - cx):
            s.line(x, y, x, cy - 9 if y < cy else cy + 9)
        else:
            s.line(x, y, cx - 9 if x < cx else cx + 9, y)
    return True


def _logic_valve(s, conn, both):
    """Shuttle (OR) or dual pressure valve (AND): a box between the inputs, the output in the middle."""
    if len(conn) != 3:
        return False
    out = next((a for a in conn if (a[1].get("designation") or "") == "2"), None) or conn[-1]
    inputs = [a for a in conn if a is not out]
    x0, x1 = min(a[2] for a in inputs), max(a[2] for a in inputs)
    y = sum(a[3] for a in inputs) / 2
    s.box(x0 + 6, y - 6, x1 - x0 - 12, 12)
    m = (x0 + x1) / 2
    s.circle(m, y, 3, filled=both)
    s.line(x0, y, x0 + 6, y)
    s.line(x1 - 6, y, x1, y)
    for a in inputs:
        if abs(a[3] - y) > 0.5:
            s.line(a[2], a[3], a[2], y)
    s.path([(out[2], out[3]), (out[2], y - 6 if out[3] < y else y + 6)])
    return True


# ------------------------------------------------------------------------------------------------ entry point
def replace(symbol, component, cls="", positions=None):
    """A new <symbol>: the layers with only their connectors, plus the own graphics in the layer OWN."""
    obj_ref = component.get("object-ref") or ""
    new = ET.Element("symbol", dict(symbol.attrib))
    for layer in symbol.findall("symbol-layer"):
        copy = ET.SubElement(new, "symbol-layer", dict(layer.attrib))
        for c in layer.iter("connector"):
            copy.append(c)
    conn = connectors(symbol)
    u = outline(symbol)
    s = Pen()
    drawn = False
    if obj_ref.startswith("Digital."):
        drawn = _logic(s, conn, u, obj_ref)
    elif obj_ref.startswith("ET.") and _other_electric(s, conn, u, obj_ref):
        drawn = True
    elif "DirectionalValve" in obj_ref or "Valve.Directional" in obj_ref:
        drawn = _valve(s, symbol, component, positions)
    elif "Actuator" in obj_ref and "CYL" in obj_ref.upper():
        fluid = [a for a in conn if a[1].get("type") != "electric"]
        _cylinder(s, fluid, u, len(fluid) == 1)
        drawn = True
    elif obj_ref == "Pneu.Supply.AirSupply" or obj_ref.endswith(("SimpleFlowSource", "PumpBlock01")):
        _source(s, conn, u, obj_ref.startswith("Hyd."))
        drawn = True
    elif obj_ref.endswith("Supply.Tank"):
        _tank(s, conn, u)
        drawn = True
    elif ".Sensor.Manometer" in obj_ref or obj_ref.endswith(("Voltmeter", "AmpereMeter")) or ".Sensor.Flowmeter" in obj_ref:
        _gauge(s, conn, u)
        drawn = True
    elif obj_ref.startswith(("ET.Source.Pot", "ET.Source.Ground")):
        drawn = _potential(s, conn, "0V" if ("0V" in obj_ref or "Ground" in obj_ref) else "24V", u)
    elif cls in ("make-contact", "break-contact", "changeover-contact", "contact-manual", "proximity-switch"):
        kind = "break" if ("Break" in obj_ref or cls == "break-contact") else "make"
        drawn = _contact(s, conn, kind, cls == "contact-manual")
    elif cls in ("solenoid-coil", "solenoid-valve-coil", "solenoid-coil-clpc"):
        drawn = _coil(s, conn, cls != "solenoid-coil")
    elif cls == "indicator-light":
        drawn = _lamp(s, conn)
    elif re.search(r"CheckValve|ThrottleValve|FlowControlValve2Way|FlowValve\.ShutoffValve|Supply\.Filter", obj_ref):
        drawn = _inline(s, conn, obj_ref)
    elif re.search(r"PressureReliefValve|PressureReducingValve|PressureRegulator", obj_ref):
        drawn = _pressure_valve(s, conn, "Relief" not in obj_ref)
    elif "ShuttleValve" in obj_ref or "TwoPressureValve" in obj_ref:
        drawn = _logic_valve(s, conn, "TwoPressure" in obj_ref)
    elif obj_ref.startswith("ET.") and obj_ref.split(".")[-1].startswith(TWO_TERMINAL):
        drawn = _two_terminal(s, conn, obj_ref)
    elif obj_ref.startswith("ET.Semiconductor.") and obj_ref.split(".")[-1].startswith(("NPN", "PNP")):
        drawn = _transistor(s, conn, obj_ref.split(".")[-1].startswith("PNP"))
    elif obj_ref.startswith("ET.") and obj_ref.split(".")[-1].startswith(CIRCLE_SOURCE):
        drawn = _circle_source(s, conn, u)
    elif "Distributor" in obj_ref:
        drawn = _distributor(s, conn)
    if not drawn:
        # no text box with a (cut) name: a box with its sign after IEC 60617, or empty
        _box_with_sign(s, conn, u, sign(obj_ref))
    # shaft connector (motor, load): the shaft starts at the symbol, not in nothing
    shaft = [(layer, c, _n(layer, "ref-x") + _n(layer, "scale-x", 1.0) * _n(c, "cx"),
              _n(layer, "ref-y") + _n(layer, "scale-y", 1.0) * _n(c, "cy"))
             for layer in symbol.findall("symbol-layer") if layer.get("visible") != "false"
             for c in layer.iter("connector") if c.get("type") == "rotational"]
    _reach_connectors(s, conn + shaft, u)
    new.append(s.layer)
    return new
