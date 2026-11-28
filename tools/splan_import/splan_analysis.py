"""sPlan sheet -> content: parts with terminals, nets, potential names, free
texts, images.

sPlan has no terminals and no nets (help topic "What is a component?": a
part is a group of drawing elements with a designator and a value; junction
dots show whether crossing lines are connected). The connections are
therefore derived from the geometry (FORMAT_SPL8.md, section 12):

- wire = a polyline at the top level (not part of a part);
- two wires are connected when an end of one lies on the other (T joint,
  corner) or a junction dot lies on both; a plain crossing without a dot
  is not connected;
- terminal = a wire end on the drawing of a part. Name: the contact of the
  part (an sPlan "contact"), else the nearest text in the part, else its
  position (top/bottom);
- potential name = the text at the arrow end of a wire (an arrow as line
  end, section 6, or as a small triangle, section 7).
Whatever is ambiguous goes to `questions` (open_questions.txt).
"""
import math
import re

from splan8 import walk

TOL = 3.0                     # 0.3 mm: a point lies on a line
# drawing area: the inner frame is at 7.2 mm, the title block from 175 mm (0.1 mm)
FRAME = (75, 75, 2865, 1752)


def _seg_distance(p, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    t = 0 if l2 == 0 else max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / l2))
    return math.hypot(p[0] - ax - t * dx, p[1] - ay - t * dy)


def _segments(o):
    p = o["pts"]
    if o["kind"] in ("line", "bezier"):
        return list(zip(p, p[1:]))
    if o["kind"] in ("rectangle", "polygon", "ellipse"):
        return list(zip(p, p[1:] + p[:1]))
    return []


def bbox(pts):
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    return (min(xs), min(ys), max(xs), max(ys))


def center(pts):
    b = bbox(pts)
    return ((b[0] + b[2]) / 2, (b[1] + b[3]) / 2)


def in_frame(o):
    """The frame, its column and row marks and the title block lie outside
    the drawing area."""
    pts = [q for k in walk(o) for q in k["pts"]]
    if not pts:
        return False

    def outside(p):
        return (p[0] <= FRAME[0] + 5 or p[0] >= FRAME[2] - 5 or p[1] <= FRAME[1] + 5
                or p[1] >= FRAME[3] - 5)
    if o["kind"] in ("line", "polygon"):
        # frame lines (also the zigzag of the column marks) lie in the border
        # strip together with the middles of their segments
        middles = [((a[0] + b[0]) / 2, (a[1] + b[1]) / 2) for a, b in zip(pts, pts[1:])]
        return all(outside(p) for p in pts + middles)
    # areas (image, rectangle, group, text): frame only when the centre is
    # in the border strip
    return outside(center(pts))


def clean(s):
    """Control characters (sPlan stores e.g. byte 07 as a library name) are
    not allowed in XML."""
    return re.sub(r"[\x00-\x08\x0b-\x1f]", "", s or "").strip()


class Part:
    def __init__(self, o, nr):
        self.o, self.nr = o, nr
        self.tag = clean(o.get("tag"))
        self.value = clean(o.get("value"))
        self.symbol = clean((o.get("symbol") or "").split("\r")[0])
        self.id, self.parent = o.get("id", 0), o.get("parent", 0)
        self.is_parent = bool(o.get("is_parent"))
        self.is_group = False
        self.is_image = False
        self.segs = [s for k in walk(o) if k is not o for s in _segments(k)]
        pts = [q for k in walk(o) if k is not o and k["kind"] not in ("text", "contact")
               for q in k["pts"]]
        self.bbox = bbox(pts) if pts else bbox(o["tag_text"]["pts"] if "tag_text" in o
                                               else [(0, 0)])
        self.contacts = [(k.get("name", ""), k["text"], center(k["pts"])) for k in walk(o)
                         if k["kind"] == "contact"]
        self.texts = []
        for k in walk(o):
            if k["kind"] in ("text", "multiline_text") and k is not o:
                self.texts += _text_words(k)
        self.extra = [z for z in o.get("extra", []) if z]
        self.terminals = []       # [Terminal]
        self.description = []     # free texts next to it
        self.strip = None         # terminal strip (of terminals)

    def distance(self, p):
        """Distance of the point to the drawing of the part (0 = on a line or
        inside its frame)."""
        d = min((_seg_distance(p, a, b) for a, b in self.segs), default=1e9)
        x0, y0, x1, y1 = self.bbox
        if x0 - 1 <= p[0] <= x1 + 1 and y0 - 1 <= p[1] <= y1 + 1:
            d = min(d, 1.0)
        return d

    @property
    def center(self):
        return ((self.bbox[0] + self.bbox[2]) / 2, (self.bbox[1] + self.bbox[3]) / 2)

    def __repr__(self):
        return f"<{self.tag or '?'} {self.symbol!r} {len(self.terminals)}T>"


def _text_words(t):
    """Splits the text of a part into terminal names: "PE I L I N" (the
    terminal marking of a power supply) becomes PE, L, N at their position
    in the text; a single word stays one word (position = centre of the
    text)."""
    txt = t["text"]
    words = [(m.start(), m.group(0)) for m in re.finditer(r"\S+", txt)]
    words = [(k, w) for k, w in words if w not in ("I", "|")]
    if len(words) <= 1 or "\n" in txt or len(words) > 12:
        return [(txt, center(t["pts"]))]
    p0, p1, p3 = t["pts"][0], t["pts"][1], t["pts"][3]
    n = max(1, len(txt))
    out = []
    for k, w in words:
        f = (k + len(w) / 2) / n
        out.append((w, (p0[0] + (p1[0] - p0[0]) * f + (p3[0] - p0[0]) / 2,
                        p0[1] + (p1[1] - p0[1]) * f + (p3[1] - p0[1]) / 2)))
    return out


def side(part, p):
    cx, cy = part.center
    x0, y0, x1, y1 = part.bbox
    dx, dy = p[0] - cx, p[1] - cy
    rx, ry = abs(dx) / max(1, x1 - x0), abs(dy) / max(1, y1 - y0)
    return ("bottom" if dy > 0 else "top") if ry >= rx else ("right" if dx > 0 else "left")


# sPlan's own libraries are German: "Klemme" is a terminal, "Klemmen" a
# multi-level terminal, as the library names of the parts read
def is_terminal(part):
    return bool(re.search(r"klemme|terminal", part.symbol, re.I))


def is_multi_level_terminal(part):
    return bool(re.search(r"klemmen|terminals", part.symbol, re.I))


def _same_terminal(part, t, e):
    """Terminals: every wire on one side ends on the same screw; multi-level
    terminals per column."""
    if is_terminal(part):
        if (t.pos[1] < part.center[1]) != (e[1] < part.center[1]):   # only top/bottom
            return False
        if is_multi_level_terminal(part):
            return abs(t.pos[0] - e[0]) <= 8
        return True
    return math.dist(t.pos, e) <= 4


class Terminal:
    def __init__(self, part, pos):
        self.part, self.pos, self.name, self.net = part, pos, "", None
        self.side = ""            # top/bottom/left/right of the part
        self.wire = None
        self.level = 1

    def __repr__(self):
        return f"{self.part.tag}:{self.name}"


class Net:
    def __init__(self, nr):
        self.nr, self.wires, self.terminals, self.names, self.arrows = nr, [], [], [], []

    @property
    def name(self):
        return self.names[0] if self.names else ""


class Content:
    """The result of one sheet."""

    def __init__(self, number, title):
        self.number, self.title = number, title
        self.parts, self.nets, self.texts, self.images, self.graphics = [], [], [], [], []
        self.questions = []


def _union(parent, a, b):
    a, b = _root(parent, a), _root(parent, b)
    if a != b:
        parent[b] = a


def _root(parent, a):
    while parent[a] != a:
        parent[a] = parent[parent[a]]
        a = parent[a]
    return a


def _crosses(p1, p2, a, b):
    def orient(u, v, w):
        return (v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])
    return (orient(p1, p2, a) * orient(p1, p2, b) < 0
            and orient(a, b, p1) * orient(a, b, p2) < 0)


def _turning_points(pts):
    """Inner points where the polyline runs back on itself: that is how a
    bridge is drawn in sPlan that touches a device and returns (checking
    only the ends missed that terminal)."""
    for a, m, b in zip(pts, pts[1:], pts[2:]):
        v1, v2 = (a[0] - m[0], a[1] - m[1]), (b[0] - m[0], b[1] - m[1])
        l1, l2 = math.hypot(*v1), math.hypot(*v2)
        if l1 > 1 and l2 > 1 and (v1[0] * v2[0] + v1[1] * v2[1]) / (l1 * l2) > 0.98:
            yield m


TAG = re.compile(r"^-?[A-Z]{1,3}\d+(\.\d+)?$")
STRIP_NAME = re.compile(r"^-?X\d+[\w.]*$")


def analyse(sheet, number):
    c = Content(number, sheet.name)
    wires, small_triangles, dots, free_texts = [], [], [], []
    for o in sheet.objects:
        if in_frame(o):
            continue
        k = o["kind"]
        if k == "part":
            c.parts.append(Part(o, len(c.parts)))
        elif k == "line":
            wires.append(o)
        elif k == "polygon" and max(bbox(o["pts"])[2] - bbox(o["pts"])[0],
                                    bbox(o["pts"])[3] - bbox(o["pts"])[1]) <= 30:
            small_triangles.append(o)
        elif k == "dot":
            dots.append(o["pts"][0])
        elif k in ("text", "multiline_text"):
            if o["text"].strip():
                free_texts.append(o)
        elif k == "image":
            c.images.append(o)
        else:                                      # groups (clips), rectangles, ellipses, big polygons
            c.graphics.append(o)

    # groups without part data (clips) count as an unnamed part
    for g in [g for g in c.graphics if g["kind"] == "group"]:
        part = Part(dict(g, tag="", value="", symbol="Group"), len(c.parts))
        part.is_group = True
        c.parts.append(part)
    c.graphics = [g for g in c.graphics if g["kind"] != "group"]

    # pasted images (photos of devices) on which wires end are devices
    # without data
    for b in c.images:
        part = Part(dict(b, children=[dict(kind="rectangle", pts=b["pts"] + b["pts"][:1],
                                           children=[])],
                         tag="", value="", symbol="Image"), len(c.parts))
        part.is_image = True
        b["_part"] = part
        c.parts.append(part)

    # --- connect the wires ------------------------------------------------
    n = len(wires)
    parent = list(range(n))
    segs = [_segments(o) for o in wires]
    ends = [(o["pts"][0], o["pts"][-1]) for o in wires]
    for i in range(n):
        for e in ends[i]:
            for j in range(n):
                if i != j and any(_seg_distance(e, a, b) <= TOL for a, b in segs[j]):
                    _union(parent, i, j)
    # overshoot: a wire end that crosses another wire and ends at most
    # 1.5 mm behind it is a tap drawn by hand past the rail (seen in a
    # drawing where two taps ended 0.5 and 0.8 mm above a 24 V rail)
    for i in range(n):
        pts = wires[i]["pts"]
        for e, before, arrow in ((pts[0], pts[1], wires[i].get("arrow_start")),
                                 (pts[-1], pts[-2], wires[i].get("arrow_end"))):
            if arrow:
                continue
            for j in range(n):
                if i != j and _root(parent, i) != _root(parent, j) and any(
                        TOL < _seg_distance(e, a, b) <= 15 and _crosses(e, before, a, b)
                        for a, b in segs[j]):
                    _union(parent, i, j)
                    c.questions.append(f"The wire end at {e[0] / 10:.0f}/{e[1] / 10:.0f} mm "
                                       f"overshoots the wire it crosses; taken as connected.")
    for p in dots:
        on = [i for i in range(n) if any(_seg_distance(p, a, b) <= TOL for a, b in segs[i])]
        for i in on[1:]:
            _union(parent, on[0], i)
        if not on:
            c.questions.append(f"The junction dot at {p[0] / 10:.0f}/{p[1] / 10:.0f} mm is on "
                               f"no wire.")

    # --- arrow ends -------------------------------------------------------
    arrow_ends = []            # (wire, point, point before)
    for i, o in enumerate(wires):
        if o.get("arrow_start"):
            arrow_ends.append((i, o["pts"][0], o["pts"][1]))
        if o.get("arrow_end"):
            arrow_ends.append((i, o["pts"][-1], o["pts"][-2]))
    for d in small_triangles:
        m = center(d["pts"])
        best = min(((math.dist(m, e), i, k) for i in range(n) for k, e in enumerate(ends[i])),
                   default=None)
        if best and best[0] <= 25:
            i, k = best[1], best[2]
            if not any(j == i and math.dist(p, ends[i][k]) < 1 for j, p, _ in arrow_ends):
                arrow_ends.append((i, ends[i][k], wires[i]["pts"][1 if k == 0 else -2]))

    # --- terminals --------------------------------------------------------
    def on_wire(e, i):
        return any(j != i and any(_seg_distance(e, a, b) <= TOL for a, b in segs[j])
                   for j in range(n))

    for i in range(n):
        for e in list(ends[i]) + list(_turning_points(wires[i]["pts"])):
            near = sorted(((part.distance(e), part.nr, part) for part in c.parts),
                          key=lambda t: t[:2])
            if not near or near[0][0] > TOL:
                continue
            part = near[0][2]
            t = next((t for t in part.terminals if _same_terminal(part, t, e)), None)
            if t is None:
                t = Terminal(part, e)
                part.terminals.append(t)
            if t.wire is None:
                t.wire = i
            else:
                _union(parent, t.wire, i)

    # --- nets -------------------------------------------------------------
    groups = {}
    for i in range(n):
        groups.setdefault(_root(parent, i), []).append(i)
    for k, idx in enumerate(groups.values()):
        net = Net(k)
        net.wires = [wires[i] for i in idx]
        c.nets.append(net)
        for i in idx:
            wires[i]["_net"] = net
    for part in c.parts:
        for t in part.terminals:
            t.net = wires[t.wire]["_net"]
            t.net.terminals.append(t)

    # --- texts: arrow names, terminal names, descriptions, free ------------
    used = set()

    # wire ends that can carry a name: arrow ends and free ends (no part,
    # no wire)
    name_ends = [(i, p, before, True) for i, p, before in arrow_ends]
    for i in range(n):
        for k, e in enumerate(ends[i]):
            if on_wire(e, i) or any(math.dist(e, p) < 2 for _, p, _ in arrow_ends) or any(
                    math.dist(t.pos, e) <= 4 for part in c.parts for t in part.terminals):
                continue
            name_ends.append((i, e, wires[i]["pts"][1 if k == 0 else -2], False))
    # assigned globally by distance (with arrows close together, every text
    # belongs to exactly one wire, section 12).
    # Texts along the wire: with arrows close together a text is often nearer
    # to the neighbouring wire than to its own. But whoever drew it put all
    # texts of a group next to their wire the same way (the same offset
    # across the wire; side and distance differ from sheet to sheet). So the
    # offset of every text is the one most neighbouring texts have to one of
    # their wires too, and it is measured from there.
    pairs, along = [], []
    for ni, (i, p, before, arrow) in enumerate(name_ends):
        vertical = abs(p[0] - before[0]) < abs(p[1] - before[1])
        for ti, t in enumerate(free_texts):
            if t["kind"] != "text" or (not arrow and STRIP_NAME.match(t["text"].strip())):
                continue                           # a strip name (X1) is no potential name
            q = t["pts"]
            d = min(_seg_distance(corner, p, before) for corner in q)
            d = min(d, min(_seg_distance(x, a, b) for x in (p, before)
                           for a, b in zip(q, q[1:] + q[:1])))
            parallel = (abs(t["rotation"]) == 90) == vertical
            limit = (140 if arrow else 40) if parallel else 40
            if d > limit:
                continue
            if not parallel:
                pairs.append((d + 30, ni, ti))
                continue
            k = 0 if vertical else 1                # the axis across the wire
            m = center(q)
            lo, hi = sorted((p[1 - k], before[1 - k]))
            along.append((ni, ti, p[k] - m[k], max(0.0, lo - m[1 - k], m[1 - k] - hi), d))
    TOL_OFFSET = 12           # 1.2 mm: the same offset text <-> wire
    offset = {}
    for ti in {ti for _, ti, _, _, _ in along}:
        c0 = center(free_texts[ti]["pts"])
        neighbours = {tj for _, tj, _, _, _ in along
                      if free_texts[tj]["rotation"] == free_texts[ti]["rotation"]
                      and math.dist(center(free_texts[tj]["pts"]), c0) <= 300}
        own = [s for _, tj, s, _, _ in along if tj == ti]
        best = None
        for s0 in own:
            hits = sum(1 for tj in neighbours
                       if any(abs(s - s0) <= TOL_OFFSET for _, tk, s, _, _ in along if tk == tj))
            key = (-hits, abs(s0))
            if best is None or key < best[0]:
                best = (key, s0)
        offset[ti] = best[1]
    for ni, ti, s, beside, d in along:
        pairs.append((abs(s - offset[ti]) + beside + 0.01 * d, ni, ti))
    pairs.sort()
    given_e, given_t = set(), set()
    for d, ni, ti in pairs:
        if ni in given_e or ti in given_t:
            continue
        given_e.add(ni)
        given_t.add(ti)
        i, p, before, arrow = name_ends[ni]
        t = free_texts[ti]
        used.add(id(t))
        net = wires[i]["_net"]
        net.names.append(t["text"].strip())
        net.arrows.append((p, before, t["text"].strip()))
    for ni, (i, p, before, arrow) in enumerate(name_ends):
        if arrow and ni not in given_e:
            net = wires[i]["_net"]
            net.arrows.append((p, before, ""))
            c.questions.append(f"The arrow at {p[0] / 10:.0f}/{p[1] / 10:.0f} mm has no name "
                               f"(net on {', '.join(map(str, net.terminals)) or 'no part'}).")

    c.parts = [part for part in c.parts if not part.is_image or part.terminals]
    # a part without a designator but with one as a text inside it: taken over
    for part in c.parts:
        if not part.tag and not part.is_image:
            t = next((t for t, _ in part.texts if TAG.match(t.strip())), None)
            if t:
                part.tag = t.strip()
                c.questions.append(f"The part \"{part.symbol}\" has no designator in sPlan; "
                                   f"{part.tag} was taken from the text inside it.")
    for b in c.images:
        if b["_part"] in c.parts:
            b["_device"] = True
    for part in c.parts:
        if part.is_image:
            x0, y0, x1, y1 = part.bbox
            near = []
            for t in free_texts:
                if id(t) in used or not TAG.match(t["text"].strip()):
                    continue
                m = center(t["pts"])
                near.append((math.hypot(max(x0 - m[0], 0, m[0] - x1),
                                        max(y0 - m[1], 0, m[1] - y1)), id(t), t))
            near.sort(key=lambda k: k[:2])
            if near and near[0][0] <= 80:
                part.tag = near[0][2]["text"].strip()
                used.add(id(near[0][2]))
            c.questions.append(f"{part.tag or 'A device'} is only an image in sPlan: its terminal "
                               f"names are in the picture and were numbered by position; "
                               f"please check.")
    for part in c.parts:
        _terminal_names(part, c)
    # descriptions (device name, strip name) close to a part
    for t in free_texts:
        if id(t) in used:
            continue
        m = center(t["pts"])
        near = []
        for part in c.parts:
            x0, y0, x1, y1 = part.bbox
            dx = max(x0 - m[0], 0, m[0] - x1)
            dy = max(y0 - m[1], 0, m[1] - y1)
            near.append((math.hypot(dx, dy), part.nr, part))
        near.sort(key=lambda k: k[:2])
        if near and near[0][0] <= 80:
            near[0][2].description.append(t["text"].strip())
            used.add(id(t))
        else:
            c.texts.append(t)
    return c


def _terminal_names(part, c):
    for t in part.terminals:
        t.side = side(part, t.pos)
        if is_terminal(part):
            t.side = "top" if t.pos[1] < part.center[1] else "bottom"
    if is_terminal(part):
        # a terminal: 1 on top, 2 at the bottom; multi-level terminals:
        # column 1, 2, 3 ... from the left, the side as suffix
        if is_multi_level_terminal(part):
            xs = sorted({round(t.pos[0] / 8) for t in part.terminals})
            for t in part.terminals:
                t.level = xs.index(round(t.pos[0] / 8)) + 1
                t.name = f"{t.level}.{'1' if t.side in ('top', 'left') else '2'}"
        else:
            for t in part.terminals:
                t.name = "1" if t.side in ("top", "left") else "2"
        return
    given = set()
    for t in sorted(part.terminals, key=lambda t: t.pos):
        cands = [(math.dist(t.pos, p), k, txt) for k, (nm, txt, p) in enumerate(part.contacts)
                 if ("c", k) not in given]
        cands += [(math.dist(t.pos, p) + 5, 1000 + k, txt) for k, (txt, p) in enumerate(part.texts)
                  if ("t", k) not in given and 0 < len(txt.strip()) <= 8]
        cands.sort(key=lambda x: x[:2])
        if cands and cands[0][0] <= 90:
            t.name = cands[0][2].strip()
            k = cands[0][1]
            given.add(("c", k) if k < 1000 else ("t", k - 1000))
    unnamed = [t for t in part.terminals if not t.name]
    if unnamed:
        top = sorted([t for t in unnamed if t.side in ("top", "left")], key=lambda t: t.pos)
        bottom = sorted([t for t in unnamed if t.side in ("bottom", "right")], key=lambda t: t.pos)
        for k, t in enumerate(top + bottom, 1):
            t.name = str(k)
        if not part.is_group and not part.is_image and len(part.terminals) > 2:
            c.questions.append(f"{part.tag or part.symbol}: {len(unnamed)} terminals without a "
                               f"name in the sPlan part, numbered by the import "
                               f"({', '.join(t.name for t in top + bottom)}).")
    seen = {}
    for t in part.terminals:
        t.name = clean(" ".join(t.name.split()))
        if t.name in seen:
            seen[t.name] += 1
            t.name = f"{t.name}.{seen[t.name]}"
        else:
            seen[t.name] = 1
