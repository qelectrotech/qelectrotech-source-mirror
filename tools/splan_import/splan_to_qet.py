"""sPlan 8 schematic (.spl8) -> QElectroTech project (.qet), drawn anew.

Per sheet:
1. `splan8.read` reads the file (FORMAT_SPL8.md),
2. `splan_analysis.analyse` makes parts with terminals, nets and potential
   names of it,
3. here: the function of every part is recognised (contact, coil, terminal,
   circuit breaker, motor, device ...), a symbol chosen, the parts laid out
   in rows (rails on top and at the bottom, devices in the order of the
   original), the wires drawn as real QET conductors between terminals,
   potentials as rails or cross-reference arrows, coil <-> contacts as
   master/slave (by the sPlan parent id or the same designator).
The position in sPlan decides the order only, not the coordinates.

    python splan_to_qet.py <schematic.spl8> [<target.qet>] [--pdf]
                           [--compare <original.pdf>] [--mapping <file.json>]
                           [--mapping-template]

Without a target: <folder of the schematic>/QET/<name>.qet, and next to it
<name>_open_questions.txt. "PROGRESS <percent> <step>" lines feed the
progress bar of QElectroTech's import dialog; the last line is
"done: <project>".
"""
import hashlib
import io
import os
import re
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, ".."))

import splan8                                    # noqa: E402
import splan_analysis as A                       # noqa: E402
import part_mapping as M                         # noqa: E402
from qet_sheet import qet_runner                 # noqa: E402
from qet_sheet import symbols as S               # noqa: E402
from qet_sheet import symbol_builder             # noqa: E402
from qet_sheet.sheet import Plan, grid           # noqa: E402

MAPPING = M.Mapping()                            # sPlan part -> element (part_mapping.py), per run
X_LEFT, X_RIGHT = 110, 1480                      # usable width (room for the rail arrows)
RAIL_BOTTOM_Y = 900

# potentials drawn as a rail at the bottom; the others go on top
POT_BOTTOM = re.compile(r"^(GND|0\s?V|M|N|PE|X\d+:\s?M|\d?M)\b", re.I)
POT_NAME = re.compile(r"^(\+?\d+([.,]\d+)?\s?V(DC|AC)?|L[123+]?|N|PE|M|GND|0\s?V|L\d?\s?\d+\s?V|"
                      r"X\d+:\s?M)\b", re.I)


def _progress(percent, step):
    print(f"PROGRESS {int(percent)} {step}", flush=True)


# --- recognise the function ---------------------------------------------------
def classify(part):
    """The function of the part from its library name, value and contacts.
    sPlan's libraries are German, so are the names matched here
    ("Schliesser" = normally open contact, "Öffner" = normally closed,
    "Schütz" = contactor, "Leistungsschalter" = circuit breaker, ...)."""
    sym, value = part.symbol.lower(), part.value.lower()
    names = {t.name for t in part.terminals}
    n = len(part.terminals)
    if part.is_image or part.is_group:
        return "box"
    if A.is_multi_level_terminal(part):
        return "terminals"
    if A.is_terminal(part):
        return "terminal"
    if sym.startswith("schliesser") or sym.startswith("schließer"):
        return "no" if n <= 2 else "box"
    if sym.startswith("öffner") or sym.startswith("oeffner"):
        return "nc" if n <= 2 else "box"
    if ("schütz" in value or "relais" in value) and n <= 2 and names <= {"A1", "A2"}:
        return "coil"
    if "leistungsschalter" in sym and n <= 6:
        return "cb3"
    if ((sym.startswith("abb fi") or "fi-schutz" in " ".join(t for t, _ in part.texts).lower())
            and n <= 4):
        return "rcbo"
    if "kippschalter" in sym and n <= 2:
        return "switch"
    if "ntc" in sym and n <= 2:
        return "ntc"
    if "wechselstrom" in sym and n <= 2:
        return "motor_ac"
    if "gleichstrom" in sym and n <= 2:
        return "motor_dc"
    return "box"


STANDARD = {"no": S.NO_CONTACT, "nc": S.NC_CONTACT, "switch": S.NO_CONTACT, "coil": S.COIL,
            "terminal": S.TERMINAL, "cb3": S.CIRCUIT_BREAKER_3P, "rcbo": S.RCBO, "ntc": S.NTC,
            "motor_ac": S.MOTOR_AC, "motor_dc": S.MOTOR_DC}
# kinds that get the terminal numbers of sPlan written next to the symbol
NUMBERED = ("no", "nc", "switch", "cb3", "rcbo", "ntc", "motor_ac", "motor_dc", "coil")
BOX_FOLDER = "sPlan"                              # custom://<import folder>/sPlan/...


def safe_name(s):
    return re.sub(r"[^\w.+-]+", "_", s).strip("_") or "device"


def box_symbol(part):
    """A device box with the terminals of the sPlan part: on top or at the
    bottom as in the original, in order from left to right. The same list of
    terminals gives the same file."""
    top = sorted([t for t in part.terminals if t.side in ("top", "left")],
                 key=lambda t: (t.pos[0], t.pos[1]))
    bottom = sorted([t for t in part.terminals if t.side in ("bottom", "right")],
                    key=lambda t: (t.pos[0], t.pos[1]))
    kind = (part.value if part.value and not part.value.startswith("<")
            else (part.symbol if part.symbol not in ("Image", "Group") else "Device"))
    if part.is_image:
        kind = "Device (image in sPlan)"
    kind = kind[:28]
    key = "|".join([kind, ",".join(t.name for t in top), ",".join(t.name for t in bottom)])
    h = hashlib.md5(key.encode("utf-8")).hexdigest()[:8]
    rel = f"{BOX_FOLDER}/{safe_name(part.symbol or 'device')}_{h}.elmt"
    path = symbol_builder.device_box(rel, kind, [(t.name, "") for t in top],
                                     [(t.name, "") for t in bottom], description=part.symbol,
                                     pitch=30)
    return path, {id(t): t.name for t in top + bottom}


def _by_side(part, sym):
    """Maps the terminals of a standard symbol to the sPlan terminals: the
    same name, else the same side in order from left to right."""
    mapped, free = {}, list(sym.terminals)
    for t in part.terminals:
        s = next((s for s in free if s.name and s.name == t.name), None)
        if s:
            mapped[id(t)] = s.index
            free.remove(s)
    for sides, orientation in ((("top", "left"), "n"), (("bottom", "right"), "s")):
        rest = sorted([t for t in part.terminals if id(t) not in mapped and t.side in sides],
                      key=lambda t: t.pos)
        cands = sorted([s for s in free if s.orientation == orientation], key=lambda s: s.x)
        for t, s in zip(rest, cands):
            mapped[id(t)] = s.index
            free.remove(s)
    for t in part.terminals:                      # the rest (more on one side than the symbol has)
        if id(t) not in mapped and free:
            mapped[id(t)] = free.pop(0).index
    return mapped


def symbol_for(part, kind):
    """(path, {id(terminal): terminal of the symbol (name or index)})"""
    if kind in STANDARD:
        path = STANDARD[kind]
        mapped = _by_side(part, S.symbol(path))
        if len(mapped) == len(part.terminals):
            return path, mapped
    return box_symbol(part)


def _symbol_box(path):
    """Extent of the symbol relative to its insertion point: (x0, y0, x1,
    y1), from what is really drawn. The declared size (width/height) of a
    manufacturer's element is often much larger and would cost a third of
    the sheet height per row."""
    r = ET.parse(S.file(path)).getroot()
    hx, hy = float(r.get("hotspot_x", 0)), float(r.get("hotspot_y", 0))
    declared = (-hx, -hy, float(r.get("width", 0)) - hx, float(r.get("height", 0)) - hy)
    xs, ys = [], []

    def f(e, k, v=0.0):
        try:
            return float(e.get(k, v))
        except (TypeError, ValueError):
            return v
    for e in r.iter():
        t = e.tag
        if t == "line":
            xs += [f(e, "x1"), f(e, "x2")]
            ys += [f(e, "y1"), f(e, "y2")]
        elif t in ("rect", "ellipse", "arc"):
            xs += [f(e, "x"), f(e, "x") + f(e, "width")]
            ys += [f(e, "y"), f(e, "y") + f(e, "height")]
        elif t == "circle":
            xs += [f(e, "x"), f(e, "x") + f(e, "diameter")]
            ys += [f(e, "y"), f(e, "y") + f(e, "diameter")]
        elif t == "polygon":
            k = 1
            while e.get(f"x{k}") is not None:
                xs.append(f(e, f"x{k}"))
                ys.append(f(e, f"y{k}"))
                k += 1
        elif t == "text":
            xs += [f(e, "x"), f(e, "x") + 4 * len(e.get("text", ""))]
            ys += [f(e, "y") - 8, f(e, "y")]
        elif t == "dynamic_text":
            w = f(e, "text_width", -1)
            xs += [f(e, "x"), f(e, "x") + (w if w > 0 else 30)]
            ys += [f(e, "y"), f(e, "y") + 12]
        elif t == "terminal":
            xs.append(f(e, "x"))
            ys.append(f(e, "y"))
    if not xs:
        return declared
    g = (min(xs) - 4, min(ys) - 4, max(xs) + 4, max(ys) + 4)
    # never larger than declared (texts at the edge would be overestimated)
    return (max(g[0], declared[0]), max(g[1], declared[1]), min(g[2], declared[2]),
            min(g[3], declared[3]))


# --- draw a sheet --------------------------------------------------------------
class Device:
    def __init__(self, part, kind, path, mapped, label):
        self.part, self.kind, self.path, self.mapped, self.label = part, kind, path, mapped, label
        self.box = _symbol_box(path)
        self.placed, self.strip, self.x, self.y = None, None, 0, 0
        self.info, self.addresses = {}, {}  # article data of the mapping; channel: id(terminal) -> address
        self.page, self.sheet, self.orientation = 0, None, None

    def terminal(self, t):
        return self.mapped[id(t)]

    def pin_dx(self, t):
        return S.symbol(self.path).terminal(self.mapped[id(t)]).x


def _is_rail(net):
    if not net.name:
        return False
    long_ = any(abs(p[0] - q[0]) > 1500 for w in net.wires for p, q in zip(w["pts"], w["pts"][1:]))
    return bool(long_ or (POT_NAME.match(net.name) and len(net.terminals) >= 3))


def _rail_at_bottom(net):
    if POT_BOTTOM.match(net.name):
        return True
    ys = [p[1] for w in net.wires for p in w["pts"]]
    return bool(ys) and sum(ys) / len(ys) > 1100


def _copy_part(part, terminals):
    """A part of `part` with only these terminals."""
    p = A.Part.__new__(A.Part)
    p.__dict__.update(part.__dict__)
    p.terminals = terminals
    return p


def draw_sheet(plan, c, number, questions, all_devices):
    s0 = plan.sheet(c.title or f"Sheet {number}", "", "", number)
    devices = []
    for part in c.parts:
        if not part.terminals and not part.tag and not part.value:
            continue                                # a drawing without terminals (frame, logo ...)
        kind = classify(part)
        rule = MAPPING.find(part, int(number))
        if rule and rule.channels:
            # elements of the mapping: one per group of terminals (a PLC: an
            # element per channel); all elements of a device stay together
            # like a terminal strip
            grouped, rest = M.groups(part, rule, questions, number)
            for path, items in grouped:
                sub = _copy_part(part, [t for t, _, _ in items])
                sub.id = sub.parent = 0
                d = Device(sub, "channel", path, {id(t): et for t, et, _ in items}, part.tag)
                d.strip, d.info = ("PLC", part.tag, id(part)), rule.info()
                d.addresses = {id(t): adr for t, _, adr in items}
                devices.append(d)
            if rest:
                questions.append(f"Sheet {number}: {part.tag or part.symbol}: terminals without an "
                                 f"element in the mapping ({', '.join(t.name for t in rest)}), "
                                 f"drawn as a box.")
                sub = _copy_part(part, rest)
                path, mapped = box_symbol(sub)
                d = Device(sub, "box", path, mapped, part.tag)
                d.info = rule.info()
                devices.append(d)
            continue
        if kind == "terminals":                    # multi-level: a terminal per level
            for level in sorted({t.level for t in part.terminals}):
                sub = _copy_part(part, [t for t in part.terminals if t.level == level])
                path, mapped = symbol_for(sub, "terminal")
                d = Device(sub, "terminal", path, mapped, str(level))
                d.strip = _strip_of(part)
                devices.append(d)
            continue
        path, mapped = symbol_for(part, kind)
        if path not in STANDARD.values():
            kind = "box"
        d = Device(part, kind, path, mapped, part.tag)
        if rule:                                    # a standard symbol or box, with article data
            d.info = rule.info()
        if kind == "terminal":
            d.strip = _strip_of(part)
        devices.append(d)

    # rails: on top (+ potentials), at the bottom (0 V, M, N, PE)
    rails = [net for net in c.nets if _is_rail(net)]
    seen_names = {}
    for net in rails:                               # the same name twice on a sheet: a suffix
        seen_names[net.name] = seen_names.get(net.name, 0) + 1
        if seen_names[net.name] > 1:
            net.names.insert(0, f"{net.name} ({seen_names[net.name]})")
    top = [net for net in rails if not _rail_at_bottom(net)]
    bottom = [net for net in rails if _rail_at_bottom(net)]
    for k, net in enumerate(top):
        s0.rail(net.name, y=80 + 20 * k)
    for k, net in enumerate(bottom):
        s0.rail(net.name, y=RAIL_BOTTOM_Y - 20 * k)
    y_start = 80 + 20 * len(top) + 70
    y_end = (RAIL_BOTTOM_Y - 20 * len(bottom)) - 40

    placed_at = {}                                  # id(terminal) -> (x, y)
    # rows in the order of the original (top to bottom): what overlaps in
    # height in sPlan goes into one row; a terminal strip is always one unit.
    # That keeps the signal flow of the drawing, the position itself comes
    # from the grid.
    units = {}
    for d in devices:
        row = getattr(d.part, "_row", None)
        if d.kind == "channel":                    # the elements of a device stay together
            key = ("K", d.strip)
        else:
            key = (("L", d.strip or row) if d.kind == "terminal" and (d.strip or row)
                   else ("G", id(d)))
        units.setdefault(key, []).append(d)
    units = list(units.values())
    for u in units:
        u.sort(key=lambda d: (_num(d.part.tag), _num(d.label)) if d.kind == "terminal" else 0)

    def _orientation(d):
        return S.symbol(d.path).terminal(d.mapped[id(d.part.terminals[0])]).orientation

    # the channel elements of a device: split into pieces that fit a row
    # (else a CPU with 20 channels runs over the edge of the sheet); inputs
    # (wire upwards) and outputs (downwards) in separate rows, as a mixed
    # row needs room for arrows above and below
    split = []
    for u in units:
        if u[0].kind != "channel":
            split.append(u)
            continue
        u = sorted(u, key=lambda d: 0 if _orientation(d) == "n" else 1)
        piece, width = [], 0
        for d in u:
            d.orientation = _orientation(d)
            w = d.box[2] - d.box[0] + 20
            if piece and (width + w > X_RIGHT - X_LEFT - 40
                          or d.orientation != piece[-1].orientation):
                split.append(piece)
                piece, width = [], 0
            piece.append(d)
            width += w
        split.append(piece)
    units = split

    def y_range(u):
        return min(d.part.bbox[1] for d in u), max(d.part.bbox[3] for d in u)
    units.sort(key=lambda u: (sum(y_range(u)) / 2, min(d.part.bbox[0] for d in u)))
    raw = []
    for u in units:
        y0, y1 = y_range(u)
        if raw and y0 < raw[-1][1] - 0.25 * (y1 - y0):     # overlaps the current row
            raw[-1][0].append(u)
            raw[-1][1] = max(raw[-1][1], y1)
        else:
            raw.append([[u], y1])

    def centroid(u):
        """the mean position of the partners placed already (fewer
        crossings), else the position in the original"""
        xs = [placed_at[id(o)][0] for d in u for t in d.part.terminals
              for o in (t.net.terminals if t.net else [])
              if id(o) in placed_at and o.part is not d.part]
        return sum(xs) / len(xs) if xs else X_LEFT + min(d.part.bbox[0] for d in u) * 0.5

    rows = []
    for group, _ in raw:
        group.sort(key=lambda u: min(d.part.bbox[0] for d in u))
        if rows:
            group.sort(key=centroid)
        row, x = [], X_LEFT
        widths = [sum(d.box[2] - d.box[0] + ({"terminal": 10, "channel": 20}.get(d.kind, 60))
                      for d in u) + 30 for u in group]
        for k, u in enumerate(group):
            width = widths[k]
            rest = sum(widths[k + 1:])             # what still needs room in this row
            other_direction = u[0].kind == "channel" and any(
                z.kind == "channel" and z.orientation != u[0].orientation for z in row)
            if row and (x + width > X_RIGHT or other_direction):
                rows.append(row)
                row, x = [], X_LEFT
            if u[0].kind == "terminal" and u[0].strip:
                x += 50                             # room for the strip name on the left
            for d in u:
                gap = {"terminal": 10, "channel": 20}.get(d.kind) or 60 + (
                    int(len(d.part.value) * 4.5) if d.kind != "box" and d.part.value else 0)
                target = None                       # under/over its partner (a vertical wire)
                if d.kind not in ("terminal", "channel"):
                    for t in d.part.terminals:
                        for o in (t.net.terminals if t.net else []):
                            if id(o) in placed_at and o.part is not d.part:
                                target = placed_at[id(o)][0] - d.pin_dx(t)
                                break
                        if target is not None:
                            break
                gx = x - d.box[0]
                if target is not None and target >= gx and target + d.box[2] + rest <= X_RIGHT:
                    gx = target
                d.x = gx = grid(gx)
                row.append(d)
                x = gx + d.box[2] + gap
                for t in d.part.terminals:
                    placed_at[id(t)] = (gx + d.pin_dx(t), 0)
            x += 30
        if row:
            rows.append(row)

    row_of = {}
    for r, row in enumerate(rows):
        for d in row:
            row_of[id(d)] = r
    device_of = {}
    for d in devices:
        for t in d.part.terminals:
            device_of[id(t)] = d

    def orientation(t):
        d = device_of[id(t)]
        return S.symbol(d.path).terminal(d.terminal(t)).orientation

    def free_right(t, row):
        """Room right of the terminal up to the next terminal of the same
        orientation in the row (px)"""
        tx, o = placed_at[id(t)][0], orientation(t)
        xs = [placed_at[id(u)][0] for d in row for u in d.part.terminals
              if id(u) in placed_at and u is not t and orientation(u) == o
              and placed_at[id(u)][0] > tx + 1]
        return (min(xs) if xs else X_RIGHT + 60) - tx - 14

    def arrow_room(row, o):
        """Room for arrows above/below: the arrow and its cross reference;
        the name stands horizontally right of it (wrapped to the room
        there), upright only where there is no room (costs much height)."""
        upright, lines = 0, 0
        for d in row:
            for t in d.part.terminals:
                if t.net and t.net.names and t.net not in rails and orientation(t) == o:
                    z = _wrap(t.net.name, free_right(t, row))
                    if z is None:
                        upright = max(upright, len(t.net.name))
                    else:
                        lines = max(lines, len(z))
        if upright:
            return 60 + 7 * min(upright, 20)
        return 0 if not lines else 110 + 10 * max(0, lines - 4)

    # every net as a shortest tree (not a chain); then the channels between
    # the rows: every wire gets a height of its own (a track), so that
    # parallel wires never lie on each other
    pairs = []
    for net in c.nets:
        if net in rails:
            continue
        pins = [t for t in net.terminals if id(t) in device_of]
        if len(pins) < 2:
            continue

        def estimate(t):
            r = row_of[id(device_of[id(t)])]
            return placed_at[id(t)][0], r * 400 + (300 if orientation(t) == "s" else 0)
        in_tree, rest = [pins[0]], pins[1:]
        while rest:
            _, p, q = min(((abs(estimate(p)[0] - estimate(q)[0])
                            + abs(estimate(p)[1] - estimate(q)[1]), p, q)
                           for p in in_tree for q in rest), key=lambda t: t[0])
            pairs.append((p, q))
            in_tree.append(q)
            rest.remove(q)
    channel_of, tracks = {}, {}
    for p, q in pairs:
        rp, rq = row_of[id(device_of[id(p)])], row_of[id(device_of[id(q)])]
        dp, dq = orientation(p), orientation(q)
        if rp > rq:
            p, q, rp, rq, dp, dq = q, p, rq, rp, dq, dp
        k = None
        if dp == "s" and dq == "n" and rq == rp + 1:
            k = rp
        elif dp == dq == "s" and rp == rq:
            k = rp
        elif dp == dq == "n" and rp == rq:
            k = rp - 1
        x1, x2 = placed_at[id(p)][0], placed_at[id(q)][0]
        if k is None or (abs(x1 - x2) < 1 and dp != dq):
            continue
        lo, hi = min(x1, x2) - 5, max(x1, x2) + 5
        taken = tracks.setdefault(k, [])
        t = 0
        while any(t == s and not (hi < l2 or lo > h2) for s, l2, h2 in taken):
            t += 1
        taken.append((t, lo, hi))
        channel_of[(id(p), id(q))] = (k, t)

    def n_tracks(k):
        return 1 + max((t for t, _, _ in tracks.get(k, [])), default=-1)

    # rows from top to bottom; a row that does not fit on the sheet goes on
    # a continuation sheet (nothing is pushed together, connections to the
    # continuation sheet run over cross-reference arrows)
    y = y_start + n_tracks(-1) * 10
    channel_y = {-1: y_start}
    page_of, page = {}, 0
    for r, row in enumerate(rows):
        h_top = max(-d.box[1] for d in row)
        h_bottom = max(d.box[3] for d in row)
        for attempt in (0, 1):
            gy = grid(y + h_top + 30 + arrow_room(row, "n"))
            ky = grid(gy + h_bottom + 20 + arrow_room(row, "s"))
            if (attempt or ky + n_tracks(r) * 10 <= y_end
                    or not any(v == page for v in page_of.values())):
                break
            page += 1                               # a continuation sheet
            y = y_start + n_tracks(r - 1) * 10
        page_of[r] = page
        if os.environ.get("SPLAN_DEBUG"):
            print("ROW", number, r, page, len(row), "y", y, "top", h_top, arrow_room(row, "n"),
                  "bottom", h_bottom, arrow_room(row, "s"), "tracks", n_tracks(r), "end", y_end)
        for d in row:
            d.y, d.page = gy, page
        channel_y[r] = ky
        y = channel_y[r] + n_tracks(r) * 10 + 20
    sheets = [s0] + [plan.sheet(f"{c.title or 'Sheet ' + number} ({k + 1})", "", "",
                                f"{number}.{k}")
                     for k in range(1, page + 1)]
    if page:
        questions.append(f"Sheet {number}: the content does not fit on one sheet; spread over "
                         f"{page + 1} sheets (connections between them as cross-reference "
                         f"arrows).")
        for s in sheets[1:]:                        # the rails on every continuation sheet
            for j, net in enumerate(top):
                s.rail(net.name, y=80 + 20 * j)
            for j, net in enumerate(bottom):
                s.rail(net.name, y=RAIL_BOTTOM_Y - 20 * j)

    def track_y(k, r):
        """y of channel k as seen from row r (the top channel of a
        continuation sheet lies below the rails)"""
        return channel_y[k] if k >= 0 and page_of.get(k) == page_of.get(r) else y_start

    # place
    for d in devices:
        info = {"description": " ".join(d.part.description)[:120]} if d.part.description else {}
        if d.part.extra:
            info["designation"] = " ".join(d.part.extra)[:80]
        info.update(d.info)
        fields = None
        if d.kind == "channel":
            # a channel element: address, symbolic address (sPlan has
            # none), function = where the wire goes
            t = d.part.terminals[0]
            target = ""
            if t.net:
                target = t.net.name or ", ".join(str(x) for x in t.net.terminals
                                                 if x.part.tag != d.part.tag)
            adr = " ".join(v for v in (d.addresses.get(id(x), "") for x in d.part.terminals) if v)
            fields = [adr, "", target if len(d.part.terminals) == 1 else ""]
            info.pop("description", None)
        s = sheets[d.page]
        d.sheet = s
        d.placed = s.place(d.path, d.x, d.y, label=d.label, info=info, fields=fields)
        for t in d.part.terminals:
            placed_at[id(t)] = d.placed.pos(d.terminal(t))
        if d.kind in NUMBERED:
            # the terminal numbers of sPlan next to the standard symbol
            # (where the symbol does not show them itself)
            for t in d.part.terminals:
                st = S.symbol(d.path).terminal(d.terminal(t))
                if st.name == t.name or not t.name or re.match(r"^.+\.\d+$", t.name):
                    continue
                px, py = d.placed.pos(d.terminal(t))
                s.text(t.name, px + 3, py + (-14 if st.orientation == "n" else 2), "terminal")
        if (d.part.value and d.kind not in ("box", "channel")
                and d.part.value.lower() not in ("schütz", "motor", "schliesser", "öffner")):
            s.text(d.part.value, d.x + d.box[2] + 4, d.y + d.box[1] + 12, "data")
        if d.part.description and d.kind not in ("terminal", "channel"):
            s.text(" ".join(d.part.description)[:40], d.x + d.box[0], d.y + d.box[3] + 6, "data")
        if d.part.id:
            all_devices.append((s, d))
    # the strip name left of the first terminal of every strip
    seen = set()
    for d in sorted([d for d in devices if d.kind == "terminal"], key=lambda d: (d.y, d.x)):
        if (d.strip, d.y) not in seen:
            seen.add((d.strip, d.y))
            if d.strip:
                d.sheet.text_right(d.strip, d.x - 14, d.y - 8, "label")
            else:
                questions.append(f"Sheet {number}: terminals {d.part.tag}... without a strip "
                                 f"name.")

    def arrow_at(d, t, name, extras=()):
        s = d.sheet
        px, py = d.placed.pos(d.terminal(t))
        upwards = S.symbol(d.path).terminal(d.terminal(t)).orientation == "n"
        direction = "from_above" if upwards else "down"
        ay = py - 40 if upwards else py + 40
        z = _wrap(name, free_right(t, rows[row_of[id(d)]]))
        a = s.arrow(name, px, ay, direction, text=z is None)
        if z is not None:                           # horizontally right of the arrow
            y0 = ay - 12 - 10 * (len(z) - 1) if upwards else ay + 4
            for k, line in enumerate(z):
                s.text(line, px + 6, y0 + 10 * k, "reference")
        s.wire(a, 0, d.placed, d.terminal(t))
        for k, extra in enumerate(extras):
            if z is not None:                       # below (or above) the lines of the name
                s.text(extra, px + 6, (y0 - 10 * (k + 1)) if upwards else (y0 + 10 * (len(z) + k)),
                       "reference")
            else:
                s.text(extra, px + 6, ay + (-30 if upwards else 20), "reference")

    # wires
    for net in c.nets:
        pins = [t for t in net.terminals if id(t) in device_of]
        if net in rails:
            for t in pins:
                d = device_of[id(t)]
                px, py = d.placed.pos(d.terminal(t))
                o = S.symbol(d.path).terminal(d.terminal(t)).orientation
                side = "above" if net in bottom else "below"
                if (net in bottom and o == "n") or (net not in bottom and o == "s"):
                    # the terminal points away from the rail: past the box,
                    # not through the device
                    xr = grid(d.x + d.box[2] + 10)
                    yb = py - 20 if o == "n" else py + 20
                    p, pt = d.sheet.tap(net.name, xr, side)
                    d.sheet.route(p, pt, d.placed, d.terminal(t), via=[(xr, yb), (px, yb)])
                    continue
                p, pt = d.sheet.tap(net.name, px, side)
                d.sheet.wire(p, pt, d.placed, d.terminal(t))
            others = [x for x in dict.fromkeys(net.names[1:]) if x != net.name]
            s = sheets[0]
            if others:
                r = s.rails[net.name]
                x0 = min([px for px, _ in r.taps] + [r.x1]) if r.start is False else r.x1
                s.text("= " + ", ".join(others), x0 + 8 + 9 * len(net.name), r.y - 20, "reference")
            continue
        if not pins:
            if net.names:
                questions.append(f"Sheet {number}: the potential \"{net.name}\" is drawn but "
                                 f"leads to no part.")
            continue
        pins.sort(key=lambda t: placed_at[id(t)])
        if net.names:
            # an arrow at the first terminal: upwards when the terminal is on
            # top, else downwards
            t = pins[0]
            arrow_at(device_of[id(t)], t, net.name, net.names[1:])

    for p, q in pairs:
        d1, d2 = device_of[id(p)], device_of[id(q)]
        if d1.sheet is not d2.sheet:
            # across sheets: an arrow of the same name on both (linked when
            # the plan is generated)
            name = p.net.name if p.net and p.net.name else min(str(p), str(q))
            arrow_at(d1, p, name)
            arrow_at(d2, q, name)
            continue
        s = d1.sheet
        k = channel_of.get((id(p), id(q))) or channel_of.get((id(q), id(p)))
        if k is None:
            s.wire(d1.placed, d1.terminal(p), d2.placed, d2.terminal(q))
            continue
        yt = track_y(k[0], row_of[id(d1)]) + 10 * k[1]
        x1, x2 = placed_at[id(p)][0], placed_at[id(q)][0]
        if abs(x1 - x2) < 1:
            s.wire(d1.placed, d1.terminal(p), d2.placed, d2.terminal(q))
        else:
            s.route(d1.placed, d1.terminal(p), d2.placed, d2.terminal(q),
                    via=[(x1, yt), (x2, yt)])

    # free texts without a part: at the bottom as a note (nothing is lost)
    s = sheets[0]
    rest = [t["text"].replace("\n", " ").strip() for t in c.texts if t["text"].strip()]
    if rest:
        line, yy = "", 948
        for t in rest:
            if line and len(line) + len(t) > 170:
                s.text(line, 40, yy, "data")
                line, yy = "", yy + 11
            line = (line + "  ·  " if line else "Texts from sPlan: ") + t
        if line:
            s.text(line, 40, yy, "data")
    return s0, devices, [im for im in c.images if not im.get("_device")]


CHAR_PX = 4.3                                      # width of a character of the "reference" size (7 pt)


def _wrap(text, width):
    """Wraps text to lines of `width` px; None when not even 6 characters fit
    side by side."""
    n = int(width / CHAR_PX)
    if n < 6:
        return None
    lines, z = [], ""
    for w in text.split():
        while len(w) > n:                           # split overlong words
            if z:
                lines.append(z)
                z = ""
            lines.append(w[:n])
            w = w[n:]
        if z and len(z) + 1 + len(w) > n:
            lines.append(z)
            z = w
        else:
            z = (z + " " + w) if z else w
    if z:
        lines.append(z)
    return lines or [""]


def _num(s):
    m = re.match(r"\d+", s or "")
    return int(m.group(0)) if m else 0


def _strip_of(part):
    for t in part.description:
        m = re.match(r"^-?(X\d+[\w.]*)$", t.strip())
        if m:
            return m.group(1)
    return getattr(part, "_strip", None)


def assign_strips(c):
    """The name of a terminal strip (X1, X2 ...) is a free text next to it in
    sPlan. Terminals of a row (the same height, side by side without a gap)
    form a strip; its name is the X text nearest to it."""
    terminals = sorted([p for p in c.parts if A.is_terminal(p)],
                       key=lambda p: (round(p.center[1] / 50), p.bbox[0]))
    rows, current = [], []
    for p in terminals:
        if current and (abs(p.center[1] - current[-1].center[1]) > 40
                        or p.bbox[0] - current[-1].bbox[2] > 25):
            rows.append(current)
            current = []
        current.append(p)
    if current:
        rows.append(current)
    cands = [(t["text"].strip(), A.center(t["pts"]), t) for t in c.texts
             if A.STRIP_NAME.match(t["text"].strip())]
    cands += [(s.strip(), p.center, None) for p in terminals for s in p.description
              if A.STRIP_NAME.match(s.strip())]
    for k, row in enumerate(rows):
        for p in row:
            p._row = f"row{k + 1}"
        x0, y0 = row[0].bbox[0], min(p.bbox[1] for p in row)
        best = None
        for name, m, t in cands:
            d = abs(m[0] - x0) + abs(m[1] - y0)
            if m[0] <= row[-1].bbox[2] + 20 and d < 400 and (best is None or d < best[0]):
                best = (d, name, t)
        if best:
            name = best[1].lstrip("-")
            for p in row:
                p._strip = name
                p.description = [s for s in p.description if s.strip().lstrip("-") != name]
            if best[2] is not None and best[2] in c.texts:
                c.texts.remove(best[2])


def _image_png(o, folder, name):
    from PIL import Image
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, name + ".png")
    Image.open(io.BytesIO(o["bmp"])).save(path)
    return path


def document_sheet(plan, title, number, images, texts, image_folder):
    """A sheet without a circuit (data sheet excerpts, photos): images and
    texts to scale (0.1 mm -> px: factor 0.5; the A4 content on the A3
    folio). Images need Pillow."""
    s = plan.sheet(title, "", "", number)
    f = 0.5
    try:
        from PIL import Image
    except ImportError:
        if images:
            plan.questions.append(f"Sheet {number}: {len(images)} image(s) left out: the Python "
                                  f"package Pillow is not installed.")
        images, Image = [], None
    for k, o in enumerate(images):
        x0, y0, x1, y1 = A.bbox(o["pts"])
        try:
            png = _image_png(o, image_folder, f"sheet{number}_image{k + 1}")
            w = Image.open(png).size[0]
        except Exception as e:                     # noqa: BLE001  any unreadable bitmap
            plan.questions.append(f"Sheet {number}: image {k + 1} cannot be read ({e}).")
            continue
        ident = f"img{number.replace('.', '_')}_{k}"
        s.ops.append({"op": "add_image", "folio": s.folio, "file": png,
                      "x": round(60 + x0 * f, 1), "y": round(40 + y0 * f, 1), "id": ident})
        factor = (x1 - x0) * f / max(1, w)
        if abs(factor - 1) > 0.01:
            s.ops.append({"op": "scale_image", "folio": s.folio, "index": f"${ident}",
                          "factor": round(factor, 4)})
    for t in texts:
        if t.get("rotation", 0) not in (0, 90, -90):
            continue
        x, y = t["pts"][0]
        size = max(5, min(14, round(t.get("height", 30) * f * 0.55)))
        rot = {0: 0, 90: 90, -90: 270}[t.get("rotation", 0)]
        for i, z in enumerate(t["text"].split("\n")):
            if z.strip():
                s.text(z, 60 + x * f, 40 + y * f + i * size * 1.4, size, rotation=rot)
    return s


# the fixed texts of sPlan's (German) title block, which are no values
TITLE_BLOCK_LABELS = {"Datum", "Name", "Änderungen", "gez.:", "gepr.:", "Norm:", "Bezeichnung",
                      "Zeichnungs-Nr.:", "Blatt", "von", "Bezeichnung:"}


def metadata(sheet):
    """The texts of the sPlan title block (below the drawing area): title,
    name, date."""
    texts = [o for o in sheet.objects if o["kind"] == "text" and o["pts"]
             and o["pts"][0][1] > A.FRAME[3]]
    values = [t["text"].strip() for t in texts]
    date = next((w for w in values if re.match(r"^\d{1,2}\.\d{1,2}\.\d{2,4}$", w)), "")
    name = next((w for w in values if re.match(r"^[A-ZÄÖÜ]\.\s?[A-Za-zäöüß]+$", w)), "")
    rest = [w for w in values if w not in TITLE_BLOCK_LABELS and w not in (date, name)
            and not w.isdigit() and len(w) > 3]
    return {"title": max(rest, key=len) if rest else "", "date": date, "author": name}


def link_coils(all_devices, questions):
    """Coil (master) <-> contacts (slaves): by the sPlan parent id, else by
    the same designator (contactor K3 / contact K3)."""
    masters, by_label = {}, {}
    for s, d in all_devices:
        if d.kind == "coil":
            masters[d.part.id] = d
            by_label.setdefault(d.label, d)
    for s, d in all_devices:
        if d.kind not in ("no", "nc") or S.symbol(d.path).link_type != "slave":
            continue
        m = (masters.get(d.part.parent) if d.part.parent else None) or by_label.get(d.label)
        if m is None:
            questions.append(f"Contact {d.label} (sheet {s.number}): no coil {d.label} found in "
                             f"the schematic.")
            continue
        s.link(m.placed, d.placed)


def plan_of(source, target, mapping=None):
    """Reads and lays out the schematic: (plan, metadata, sheet list for
    the comparison); nothing is run yet."""
    global MAPPING
    pr = splan8.read(source)
    folder = os.path.dirname(os.path.abspath(target))
    MAPPING = M.load_for(target, mapping)
    meta = metadata(pr.sheets[0])
    plan = Plan(meta["title"] or os.path.splitext(os.path.basename(source))[0], folder,
                author=meta["author"] or "")
    plan.questions = []
    image_folder = os.path.join(folder, os.path.splitext(os.path.basename(target))[0] + "_images")
    all_devices, sheet_list = [], []
    n = len(pr.sheets)
    for i, sh in enumerate(pr.sheets, 1):
        _progress(5 + 70 * i / n, "sheet")
        c = A.analyse(sh, i)
        assign_strips(c)
        qs = [f"Sheet {i} ({sh.name}): {q}" for q in c.questions]
        sheet_list.append((i, sh.name, len(plan.sheets)))
        if not any(p.terminals for p in c.parts):
            document_sheet(plan, sh.name, str(i), c.images, c.texts, image_folder)
            qs.append(f"Sheet {i} ({sh.name}): no circuit (only images and texts), taken over "
                      f"as a documentation sheet, images and texts to scale.")
        else:
            _, _, images = draw_sheet(plan, c, str(i), qs, all_devices)
            if images:
                document_sheet(plan, f"{sh.name} - images", f"{i}.1", images, [], image_folder)
        plan.questions += qs
    link_coils(all_devices, plan.questions)
    plan.articles = [r for r in MAPPING.rules if r.used]
    for r in MAPPING.unused():
        plan.questions.append(f"The mapping {r.when} matches no part; please check the entry.")
    return plan, meta, sheet_list


def convert(source, target, pdf=False, compare=None, mapping=None, template_only=False):
    _progress(2, "read")
    folder = os.path.dirname(os.path.abspath(target))
    os.makedirs(folder, exist_ok=True)
    if template_only:
        pr = splan8.read(source)
        contents = [(i, A.analyse(sh, i)) for i, sh in enumerate(pr.sheets, 1)]
        p = M.write_template(mapping or M.default_path(target).replace("_mapping.json",
                                                                      "_mapping_template.json"),
                             contents)
        _progress(100, "done")
        return p
    plan, meta, sheet_list = plan_of(source, target, mapping)
    _progress(80, "write")
    result, _ = plan.generate(os.path.basename(target), pdf=False)
    if not os.path.isfile(result):
        raise RuntimeError("QElectroTech wrote no project; the reason is in the output above")
    _progress(88, "check")
    plan.layout = tidy_layout(result)
    write_questions(questions_path(target), source, plan, meta)
    if pdf:
        _progress(95, "pdf")
        out = result[:-4] + ".pdf"
        print("pdf:", qet_runner.export_pdf(result, out).get("ok"))
        if compare:
            compare_pdf(compare, out, sheet_list,
                        os.path.join(folder, os.path.splitext(os.path.basename(target))[0]
                                     + "_comparison.pdf"))
    _progress(100, "done")
    return result


def questions_path(target):
    return re.sub(r"\.qet$", "", target) + "_open_questions.txt"


def tidy_layout(qet, rounds=2):
    """Runs QET's layout check (qet_layout_check) and applies its fixes
    (grid offsets, alignment). Returns the figures of the last check."""
    r = {}
    for _ in range(rounds):
        r = qet_runner.layout_check(qet, limit=2000)
        fixes = r.get("fixes") or []
        if not fixes:
            break
        tmp = qet[:-4] + "_tidy.qet"
        e = qet_runner.edit(qet, fixes, tmp, timeout=1800)
        if not e.get("ok"):
            if os.path.exists(tmp):
                os.remove(tmp)
            break
        os.replace(tmp, qet)
    r = qet_runner.layout_check(qet, limit=50)
    print("layout:", r.get("score"), r.get("summary"))
    return r


def write_questions(path, source, plan, meta):
    z = [f"Open questions of the sPlan import of {os.path.basename(source)}",
         f"Project: {meta['title']}  ·  Author: {meta['author']}  ·  Date in the original: "
         f"{meta['date']}",
         "", "The import could not derive these points unambiguously from the sPlan file. The",
         "assumption named is what the QElectroTech project shows; please check it there.", ""]
    lay = getattr(plan, "layout", None) or {}
    if lay.get("summary"):
        sm = lay["summary"]
        z += [f"Layout check (qet_layout_check): {lay.get('score')}/100: "
              f"{sm.get('straight_wires')} of {sm.get('wires')} wires straight, "
              f"{sm.get('labels_on_wires')} texts on wires, {sm.get('overlaps')} overlaps.", ""]
    z += ["- " + q for q in plan.questions]
    articles = getattr(plan, "articles", [])
    if articles:
        z += ["", "Articles of the mapping file (sPlan knows no articles; the reason per device, "
                  "please confirm):"]
        z += [f"- {r.when}: {r.manufacturer} {r.reference} {r.type}: {r.evidence or 'no reason given'}"
              for r in articles]
    if plan.open_ends:
        z += ["", "Cross-reference arrows without a counterpart of the same name on another sheet",
              "(the target is the text at the arrow, e.g. \"X4:1\" = strip X4, terminal 1):"]
        z += [f"- Sheet {plan.sheets[f].number}: {nm}" for f, nm in plan.open_ends]
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(z) + "\n")


def compare_pdf(original, new, sheet_list, target):
    """The original page on the left, the new sheet on the right. Needs
    pymupdf."""
    try:
        import pymupdf as fitz
    except ImportError:
        print("no comparison: the Python package pymupdf is not installed")
        return
    o, nn = fitz.open(original), fitz.open(new)
    out = fitz.open()
    for i, name, folio in sheet_list:
        if i - 1 >= len(o) or folio >= len(nn):
            continue
        r1, r2 = o[i - 1].rect, nn[folio].rect
        h2 = r2.height * r1.width / r2.width
        p = out.new_page(width=r1.width * 2 + 20, height=max(r1.height, h2) + 30)
        p.insert_text((10, 18), f"sPlan original, sheet {i} ({name})", fontsize=11)
        p.insert_text((r1.width + 30, 18), "QElectroTech", fontsize=11)
        p.show_pdf_page(fitz.Rect(0, 25, r1.width, 25 + r1.height), o, i - 1)
        p.show_pdf_page(fitz.Rect(r1.width + 20, 25, 2 * r1.width + 20, 25 + h2), nn, folio)
    out.save(target)
    print("comparison:", target)


def default_target(source):
    return os.path.join(os.path.dirname(os.path.abspath(source)), "QET",
                        safe_name(os.path.splitext(os.path.basename(source))[0]) + ".qet")


def main(argv):
    options = {}
    for name in ("--compare", "--mapping"):
        if name in argv:
            k = argv.index(name)
            if k + 1 >= len(argv):
                print(f"{name} needs a file", file=sys.stderr)
                return 2
            options[name] = argv[k + 1]
            del argv[k:k + 2]
    args = [a for a in argv if not a.startswith("--")]
    if not args:
        print(__doc__.strip().split("\n\n")[2], file=sys.stderr)
        return 2
    source = args[0]
    target = args[1] if len(args) > 1 else default_target(source)
    try:
        result = convert(source, target, pdf="--pdf" in argv, compare=options.get("--compare"),
                         mapping=options.get("--mapping"),
                         template_only="--mapping-template" in argv)
    except (OSError, splan8.FormatError, RuntimeError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    print("done:", result)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
