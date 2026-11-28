"""Plan and sheet: grid, symbols placed by an anchor terminal, wires,
potential rails, cross references, and the QElectroTech project made of it.

The grid is that of an A3 landscape folio in QET's pixels: 10 columns of
156 px. A sheet collects qet_edit operations (misc/qet-mcp); Plan.generate()
runs them through QElectroTech.

Potential rails are made of their taps: whoever needs the potential asks
tap("L1", x) and gets a T-junction on the rail; when the plan is generated
the junctions are wired from left to right, with a coming arrow on the left
and a going arrow on the right. Rails of the same name on different sheets
become linked folio reports (QET writes the "/folio.column" itself).
"""
import html
import os
import re

from . import qet_runner
from . import symbols as S
from .symbol_builder import support

COLUMNS, COLUMN_WIDTH, X0 = 10, 156, 20       # column i: X0 + i*156 ... +156
RAIL_Y, RAIL_SPACING = 80, 20
# font sizes (pt) of the kinds of free text
SIZES = {"label": 10, "data": 7, "function": 10, "reference": 7, "terminal": 7}


def grid(x):
    """Rounded to QET's 10 px grid (symbols snap there, so wires run
    straight)."""
    return int(round(x / 10.0)) * 10


def column_left(i):
    return grid(X0 + i * COLUMN_WIDTH)


def _direction(start, towards):
    """The terminal of a corner at `start` that points to `towards`."""
    dx, dy = towards[0] - start[0], towards[1] - start[1]
    if abs(dx) >= abs(dy):
        return "E" if dx > 0 else "W"
    return "S" if dy > 0 else "N"


def _rotate(x, y, degrees):
    """Position of a terminal after a clockwise rotation (QET, y down)."""
    return {0: (x, y), 90: (-y, x), 180: (-x, -y), 270: (y, -x)}[degrees % 360]


def _free_label_position(sym, label):
    """When the label at its default place is in the way of a wire (wires
    come vertically from above or below to the terminals), it goes to the
    left of the symbol, clear of every terminal. Returns the new (x, y), or
    None when the default place is free."""
    if not sym.label_position or sym.label_position[3] != "AlignLeft" or not sym.terminals:
        return None
    lx, ly, size, _ = sym.label_position
    width, height = 0.62 * size * len(label) + 4, 1.9 * size
    for t in sym.terminals:
        if not lx - 1 <= t.x <= lx + width:
            continue
        if (t.orientation == "n" and ly < t.y) or (t.orientation == "s" and ly + height > t.y):
            left = min(u.x for u in sym.terminals)
            return (round(left - 8 - width), ly)
    return None


class Placed:
    """A placed symbol; terminal(name) gives the terminal, pos(name) its
    position on the sheet."""

    def __init__(self, sheet, ident, path, x, y, rotation=0):
        self.sheet, self.ident, self.path, self.x, self.y = sheet, ident, path, x, y
        self.symbol, self.rotation = S.symbol(path), rotation % 360

    @property
    def ref(self):
        return f"${self.ident}"

    def terminal(self, which):
        return self.symbol.terminal(which)

    def pos(self, which):
        t = self.terminal(which)
        dx, dy = _rotate(t.x, t.y, self.rotation)
        return (round(self.x + dx, 1), round(self.y + dy, 1))


class Rail:
    def __init__(self, name, y, x1, x2, text="", start=None, end=None):
        self.name, self.y, self.x1, self.x2, self.text = name, y, x1, x2, text
        # arrows at the ends: None = automatic (only when another sheet
        # carries the potential), False = none (the potential starts or ends
        # here), True = always
        self.start, self.end = start, end
        self.taps = []                                       # (x, Placed)
        self.coming = self.going = None


class Sheet:
    def __init__(self, plan, folio, title, plant, location, number):
        self.plan, self.folio, self.title = plan, folio, title
        self.plant, self.location, self.number = plant, location, str(number)
        self.ops, self.rails, self.n = [], {}, 0
        f = folio
        self.ops.append({"op": "set_folio_title", "folio": f, "title": title})
        if plan.title_block:
            self.ops.append({"op": "set_folio", "folio": f, "property": "template",
                             "value": plan.title_block})
        self.ops += [
            {"op": "set_folio", "folio": f, "property": "plant", "value": plant},
            {"op": "set_folio", "folio": f, "property": "locmach", "value": location},
            {"op": "set_folio", "folio": f, "property": "folio", "value": str(number)},
            {"op": "set_folio_border", "folio": f, "property": "preset", "value": "a3-landscape"},
            {"op": "set_folio_border", "folio": f, "property": "columns", "value": str(COLUMNS)},
            {"op": "set_folio_border", "folio": f, "property": "column-width",
             "value": str(COLUMN_WIDTH)},
            {"op": "set_folio_border", "folio": f, "property": "display-rows", "value": "false"},
            {"op": "set_conductor_default", "folio": f, "property": "displaytext",
             "value": "false"}]

    # --- basic items ------------------------------------------------------
    def _id(self, ident=None):
        self.n += 1
        return f"f{self.folio}_{ident or 'e'}_{self.n}"

    def place(self, path, x, y, anchor=None, label=None, info=None, fields=None, ident=None,
              label_xy=None, dy=0, rotation=0):
        """Places the symbol so that terminal `anchor` lies at x/y (without
        an anchor: its insertion point).
        info: element information {manufacturer, manufacturer_reference,
        designation, ...} -> set_info.
        fields: the texts of the user fields, in order."""
        sym = S.symbol(path)
        if label is not None and not isinstance(label, str):
            label = str(label)
        ex, ey = x, y + dy
        if anchor is not None:
            t = sym.terminal(anchor)
            ax, ay = _rotate(t.x, t.y, rotation)
            ex, ey = x - ax, y + dy - ay
        p = Placed(self, self._id(ident), path, round(ex, 1), round(ey, 1), rotation)
        self.ops.append({"op": "add_element", "folio": self.folio, "path": path, "x": p.x,
                         "y": p.y, "id": p.ident})
        if rotation % 360:
            self.ops.append({"op": "rotate_element", "folio": self.folio, "element": p.ref,
                             "angle": rotation % 360})
        self.ops.append({"op": "set_label", "folio": self.folio, "element": p.ref,
                         "label": label or ""})
        for k, v in (info or {}).items():
            if v:
                self.ops.append({"op": "set_info", "folio": self.folio, "element": p.ref,
                                 "key": k, "value": str(v)})
        if sym.fields:
            # user fields by their index in the element file; without values
            # only placeholders such as "..." are cleared
            for k, (i, default) in enumerate(zip(sym.field_index, sym.field_default)):
                if fields is not None:
                    value = fields[k] if k < len(fields) else ""
                elif default.strip() in ("…", "...", "..", "?"):
                    value = ""
                else:
                    continue
                self.ops.append({"op": "set_element_text", "folio": self.folio,
                                 "element": p.ref, "index": i, "property": "text",
                                 "value": value})
        if label and label_xy is None and not rotation % 360:
            label_xy = _free_label_position(sym, label)
        if label_xy:
            # after the links: link_elements puts the label back otherwise
            for k, v in zip(("x", "y"), label_xy):
                self.plan.label_ops.append({"op": "set_element_text", "folio": self.folio,
                                            "element": p.ref, "index": sym.label_index,
                                            "property": k, "value": str(v)})
        return p

    def wire(self, a, terminal_a, b, terminal_b, dashed=False):
        self.ops.append({"op": "add_conductor", "folio": self.folio, "from": a.ref,
                         "from_terminal": a.terminal(terminal_a).uuid, "to": b.ref,
                         "to_terminal": b.terminal(terminal_b).uuid})
        if dashed:
            self.ops.append({"op": "set_conductor", "folio": self.folio, "element": a.ref,
                             "terminal": a.terminal(terminal_a).uuid, "property": "style",
                             "value": "dashed"})

    def route(self, a, terminal_a, b, terminal_b, via=()):
        """A wire through corner points (a corner without a dot): via =
        [(x, y), ...]."""
        last, last_terminal = a, terminal_a
        previous = a.pos(terminal_a)
        points = list(via) + [b.pos(terminal_b)]
        for i, (x, y) in enumerate(via):
            c = self.place(support("corner"), x, y, anchor="N")
            following = points[i + 1]
            self.wire(last, last_terminal, c, _direction((x, y), previous))
            last, last_terminal, previous = c, _direction((x, y), following), (x, y)
        self.wire(last, last_terminal, b, terminal_b)

    def text(self, s, x, y, size="data", rotation=0):
        """Free text, origin top left; rotation 270 = upright, read from
        below."""
        # per sheet and text: the same text may differ in size between sheets
        self.plan.font_sizes[(self.folio, s)] = SIZES[size] if isinstance(size, str) else size
        op = {"op": "add_text", "folio": self.folio, "text": s, "x": round(x, 1),
              "y": round(y, 1)}
        self.ops.append(op)
        if rotation:
            op["id"] = self._id("t")
            self.ops.append({"op": "rotate_text", "folio": self.folio, "index": f"${op['id']}",
                             "angle": rotation})

    def text_right(self, s, x_end, y, size="data"):
        """A text that ends about at x_end (a label left of a symbol)."""
        g = SIZES[size] if isinstance(size, str) else size
        # a generous width (keeps clear of the symbol), never past the left edge
        self.text(s, max(24, x_end - 0.78 * g * len(s)), y, size)

    def link(self, master, *slaves):
        """Master/slave link (coil -> contacts), also across sheets."""
        for s in slaves:
            self.plan.links.append((s.sheet.folio, s, master.sheet.folio, master))

    # --- cross-reference arrows -------------------------------------------
    def arrow(self, name, x, y, direction, text=True):
        """A vertical potential arrow with its terminal at x/y.
        direction "down": the wire comes from above and leaves through the
        arrow (going); "from_above": the potential comes in from above
        (coming); "from_below": it comes in from below. Plan.generate links
        the arrow with its counterpart of the same name on another sheet;
        QET then writes "/folio/column" next to it."""
        path, rot = {"down": (support("going_arrow_vertical"), 0),
                     "from_below": (S.COMING, 270)}.get(
                         direction, (support("coming_arrow_vertical"), 0))
        t = S.symbol(path).terminal(0)
        dy = _rotate(t.x, t.y, rot)[1]
        y = grid(y - dy) + dy           # the arrow's origin on the grid
        if direction == "down":
            p = self.place(path, x, y, anchor=0)
            self.plan.sources.append((self.folio, name, p))
            ty = y + 60 + 7 * len(name)
        elif direction == "from_below":
            p = self.place(path, x, y, anchor=0, rotation=270)
            self.plan.targets.append((self.folio, name, p))
            ty = y + 36 + 7 * len(name)
        else:
            p = self.place(path, x, y, anchor=0)
            self.plan.targets.append((self.folio, name, p))
            ty = y - 74
        if text:
            self.text(name, x - 6, ty, "function", rotation=270)
        return p

    # --- potential rails --------------------------------------------------
    def rail(self, name, y=None, from_column=0, to_column=COLUMNS - 1, text="", start=None,
             end=None):
        """A potential rail. start=False: the potential starts on this sheet
        (no coming arrow); end=False: it ends here. Left out, Plan.generate
        decides from the other sheets."""
        if y is None:
            y = RAIL_Y + RAIL_SPACING * len(self.rails)
        # ends where the arrows (terminal 13 px from the origin) sit on the grid
        r = Rail(name, y, column_left(from_column) + 43, column_left(to_column + 1) - 53,
                 text=text, start=start, end=end)
        self.rails[name] = r
        return r

    def tap(self, name, x, side=None):
        """A T-junction on rail `name` at x. side "below" (the device is
        below the rail, the default) or "above". Returns (Placed, terminal
        name) for the wire to the device."""
        r = self.rails[name]
        above = side == "above"
        path = support("tap_up" if above else "tap_down")
        sym = S.symbol(path)
        w, branch = sym.terminal("W"), sym.terminal("N" if above else "S")
        p = self.place(path, x - branch.x, r.y - w.y)
        r.taps.append((x, p))
        return p, ("N" if above else "S")

    def _finish_rails(self, partner=None):
        """partner(name, folio) -> (comes_from_elsewhere, goes_on): the
        automatic arrows at the ends."""
        for r in self.rails.values():
            comes, goes = partner(r.name, self.folio) if partner else (True, True)
            with_start = r.start if r.start is not None else comes
            with_end = r.end if r.end is not None else goes
            taps = [p for _, p in sorted(r.taps, key=lambda t: t[0])]
            if not taps and not (with_start and with_end):
                with_start = with_end = True             # a rail without taps: join it through
            points = [(p, "W", "E") for p in taps]
            if with_start:
                r.coming = self.place(S.COMING, r.x1, r.y, anchor=0)
                self.plan.targets.append((self.folio, r.name, r.coming))
                points.insert(0, (r.coming, 0))
            if with_end:
                r.going = self.place(S.GOING, r.x2, r.y, anchor=0)
                self.plan.sources.append((self.folio, r.name, r.going))
                points.append((r.going, 0))
            for a, b in zip(points, points[1:]):
                self.wire(a[0], a[2] if len(a) == 3 else a[1], b[0], b[1])
            # without a coming arrow the rail starts at its first tap: the
            # name stands right of it, not on its wire
            x_left = r.x1 if with_start else (min(x for x, _ in r.taps) + 6 if r.taps else r.x1)
            self.text(r.name, x_left + 8, r.y - 22, "function")
            if with_end:
                # on the right: the name above the rail, before the arrow
                # (QET's folio report "/31/0" stands at the arrow)
                self.text(r.name, r.x2 - 9 * len(r.name) - 14, r.y - 22, "function")
            if r.text:
                self.text(r.text, r.x1 - 52, r.y - 20, "reference")


class Plan:
    def __init__(self, title, folder, title_block=None, author=""):
        self.title, self.folder, self.title_block, self.author = title, folder, title_block, author
        self.sheets, self.font_sizes, self.links, self.label_ops = [], {}, [], []
        self.sources, self.targets = [], []     # (folio, potential, Placed): going / coming arrows
        self.open_ends = []

    def sheet(self, title, plant, location, number):
        s = Sheet(self, len(self.sheets), title, plant, location, number)
        self.sheets.append(s)
        return s

    def _references(self):
        """Links every going arrow with the next coming arrow of the same
        name on a later sheet (else on an earlier one), each target once;
        then the master/slave links. Arrows left over: self.open_ends."""
        ops, free, self.open_ends = [], list(self.targets), []
        for f, name, p in sorted(self.sources, key=lambda s: s[0]):
            candidates = [t for t in free if t[1] == name and t[0] != f]
            later = [t for t in candidates if t[0] > f]
            choice = (min(later, key=lambda t: t[0]) if later
                      else (min(candidates, key=lambda t: -t[0]) if candidates else None))
            if choice is None:
                self.open_ends.append((f, name))
                continue
            free.remove(choice)
            ops.append({"op": "link_elements", "folio": f, "element": p.ref,
                        "to_folio": choice[0], "to": choice[2].ref})
        for f, slave, mf, master in self.links:
            ops.append({"op": "link_elements", "folio": f, "element": slave.ref, "to_folio": mf,
                        "to": master.ref})
        return ops

    def operations(self):
        """All qet_edit operations of the plan, in order."""
        ops = []
        if self.title_block:
            ops.append({"op": "embed_title_block_template", "name": self.title_block})
        # who carries which potential: the rails and arrows of every sheet
        rails = {}
        for s in self.sheets:
            for r in s.rails.values():
                rails.setdefault(r.name, []).append((s.folio, r))
        sources = {(f, n) for f, n, _ in self.sources}
        targets = {(f, n) for f, n, _ in self.targets}

        def partner(name, folio):
            others = [(f, r) for f, r in rails.get(name, []) if f != folio]
            comes = (any(r.end is not False for _, r in others)
                     or any(n == name and f != folio for f, n in sources))
            goes = (any(f > folio and r.start is not False for f, r in others)
                    or any(n == name and f != folio for f, n in targets))
            return comes, goes
        for s in self.sheets:
            s._finish_rails(partner)
            ops += s.ops
        ops += self._references()
        ops += self.label_ops
        return ops

    def generate(self, name, pdf=True):
        """Makes <folder>/<name> through QElectroTech; returns (path, result
        of qet_edit)."""
        os.environ.setdefault("QET_SCRIPT_TIMEOUT_MS", "900000")
        ops = self.operations()
        empty = os.path.join(self.folder, "_empty_import.qet")
        out = os.path.join(self.folder, name)
        # a new project has at most 200 folios; more are added by add_folio
        qet_runner.project_new(empty, self.title, min(200, len(self.sheets)), self.author)
        if len(self.sheets) > 200:
            ops = [{"op": "add_folio"}] * (len(self.sheets) - 200) + ops
        try:
            r = qet_runner.edit(empty, ops, out)
        finally:
            if os.path.exists(empty):
                os.remove(empty)
        failed = [(o["index"], o["op"]) for o in r.get("operations", []) if not o.get("succeeded")]
        if os.path.exists(out):
            self._font_sizes(out)
        print("ok:", r.get("ok"), "ops:", len(ops), "failed:", failed[:8], r.get("hint") or "")
        if failed:
            print("first failed operation:", ops[failed[0][0]])
        if pdf and os.path.exists(out):
            print("pdf:", qet_runner.export_pdf(out, out[:-4] + ".pdf").get("ok"))
        return out, r

    def _font_sizes(self, path):
        """Font sizes of the free texts (the scripting API sets none), and
        the cross-reference format "/folio/column" (/%F/%c) for every
        master/slave label and report arrow of the written project -- the
        form sPlan schematics and EPLAN plans show."""
        with open(path, encoding="utf-8") as f:
            s = f.read()
        parts = re.split(r'(<diagram [^>]*>)', s)    # the folio = number of headers before
        folio = -1
        for k, part in enumerate(parts):
            if part.startswith("<diagram "):
                folio += 1
                continue
            if folio < 0:
                continue

            def sized(m, f=folio):
                tag = m.group(0)
                t = re.search(r'text-indent:0px;&quot;(?:>|&gt;)(.*?)&lt;/p', tag)
                text = html.unescape(html.unescape(t.group(1))) if t else ""
                size = self.font_sizes.get((f, text), SIZES["data"])
                tag = re.sub(r'font="([^",]+),\d+,', lambda m_: f'font="{m_.group(1)},{size},', tag)
                return re.sub(r'font-size:\d+pt', f'font-size:{size}pt', tag)
            parts[k] = re.sub(r'<input [^<]*', sized, part)
        s = "".join(parts)
        s = re.sub(r'(master_label|slave_label)="[^"]*"', lambda m: m.group(1) + '="/%F/%c"', s)
        s = re.sub(r'<report label="[^"]*"', '<report label="/%F/%c"', s)
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(s)
