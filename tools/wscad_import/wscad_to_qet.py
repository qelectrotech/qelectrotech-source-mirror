"""WSCAD project (a folder with Database/WSCAD6.db and .wsELD pages) ->
editable QElectroTech project.

    python wscad_to_qet.py <WSCAD project folder> [<target.qet>] [--pdf]

The source may also be a page file of the project or its database. Without a
target: <project folder>/QET/<folder name>.qet. Next to the project the
import writes <name>_open_questions.txt (what was not drawn, and why),
<name>_devices.csv and <name>_connections.csv. "PROGRESS <percent> <step>"
lines feed the progress bar of QElectroTech's import dialog; the last line
is "done: <project>".

The translation (FORMAT_WSCAD.md):
- every schematic page (.wsELD) becomes a folio; the frame and title block
  of the page (<Template>) and its free graphics become folio graphics, the
  values of the title block (page title, sheet, plant =, location +) go
  into the folio properties too;
- every symbol with an equipment (<SL hid> = Symbol.SymbolID of the
  database) and terminals becomes a QET element with its WSCAD graphics,
  its texts (designator as label, terminal numbers, function text, cross
  reference) and terminals in the WSCAD direction (connDir 1 up, 2 right,
  4 down, 8 left); article and manufacturer as element information. The
  elements are written into the user collection, below Imported/WSCAD/;
- wires: the connection list of the database (Connection: symbol/terminal
  -> symbol/terminal, per page) becomes QET conductors, with the colour,
  cross section and potential of the connection;
- arrows with a target (SL kind 14) become linked QET folio reports; a coil
  (kind 2) or master (kind 7) and the contacts (kind 3) of the same
  designator become master/slave. WSCAD's cross-reference texts stay,
  QET's own are switched off.
"""
import base64
import collections
import csv
import hashlib
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, ".."))

import wscad as W                                              # noqa: E402
import wscad_graphics as G                                     # noqa: E402
from qet_sheet import qet_runner                               # noqa: E402
from qet_sheet import symbols as S                             # noqa: E402
from qet_sheet import symbol_builder                           # noqa: E402
from qet_sheet.sheet import Plan                               # noqa: E402

FOLDER = "WSCAD"                              # custom://<import folder>/WSCAD/...
TOL = 0.02                                    # mm: points count as the same
# WSCAD font size (size, default 2.5 mm) -> em in the PDF 0.933 x size
# (measured: 3.75 -> 3.5; 2.5 -> 2.33); wscad_graphics works with a text
# height h (em = EM x h)
HEIGHT_PER_SIZE = 0.933 / G.EM
SIZE_DEFAULT = 2.5
LINE_PER_SIZE = 1.0                           # line spacing of multi-line texts (PDF: 2.49 mm at 2.5 mm)
LINE_DATA_BLOCK = 1.2                         # lines of a data block (PDF: 3.0 mm at 2.5 mm)
# QET draws the frame and title block from scene (5, 5) (Diagram::margin);
# the PDF export starts there. The origin of the WSCAD page therefore lies
# at 5 px (measured: without the offset everything was 1.32 mm up and left)
MARGIN_PX = 5.0
OPPOSITE = {1: 4, 4: 1, 2: 8, 8: 2}
STEP = {1: (0, -1), 2: (1, 0), 4: (0, 1), 8: (-1, 0)}         # WSCAD: y downwards
DESCRIPTION = "Taken over from a WSCAD project (tools/wscad_import)"


def _progress(percent, step):
    print(f"PROGRESS {int(percent)} {step}", flush=True)


def _num(v, default=0.0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def _points(s):
    z = [float(v) for v in (s or "").replace(",", " ").split()]
    return list(zip(z[0::2], z[1::2]))


def _style(s):
    return dict(kv.split(":", 1) for kv in (s or "").split(";") if ":" in kv)


def _kind(sl):
    """SL type "14.3" -> (14, 3)."""
    a, _, b = (sl.get("type") or "0").partition(".")
    return int(_num(a)), int(_num(b))


def _key(p):
    """A point to 1/100 mm (WSCAD stores two decimals; arrows lie 0.01 mm
    apart, and a grid of TOL flipped at x/0.02 = n.5 by rounding noise)."""
    return (round(p[0] * 100), round(p[1] * 100))


# --- texts ------------------------------------------------------------------

def _alignment(style):
    """WSCAD align (calibrated on the PDF: bit 2 right, bit 4 centre; bit 8
    bottom, bit 16 alone middle, else top; without align: base line left)
    -> (column 0/1/2, row 'top'|'middle'|'bottom'|'base')."""
    if "align" not in style:
        return 0, "base"
    a = int(_num(style.get("align")))
    column = 1 if a & 4 else (2 if a & 2 else 0)
    row = "bottom" if a & 8 else ("middle" if a & 16 else "top")
    return column, row


def _align(column, row):
    """-> alignment 1-12 of wscad_graphics. WSCAD "bottom" is on the base
    line in the PDF (anchor at 0.86-0.92 of the line height)."""
    return {"top": 1, "middle": 4, "bottom": 10, "base": 10}[row] + column


def _lines(t):
    """WSCAD line breaks: \\a or \\n as characters, CR/LF; empty lines in
    between (CR LF twice) are dropped."""
    t = (t or "").replace("\\a", "\n").replace("\\n", "\n").replace("\r\n", "\n").replace("\r", "\n")
    return "\n".join(z for z in t.split("\n") if z.strip())


OWN_STRUCTURE = [""]                          # "=A1+ST" of the page being converted


def _without_own_structure(ref):
    """A cross reference "=A1+ST/CE:1.3" within the same structure reads
    "/CE:1.3" in WSCAD (PDF)."""
    e = OWN_STRUCTURE[0]
    return ref[len(e):] if e and ref.startswith(e + "/") else ref


def plc_address(value, language="DE"):
    """WSPlcChannelAddress "0#|#True#|#True#|#EN|T|I|L|DE|T|E#|##|#0#|#7#|#1#|#.#|#"
    -> "E0.7": fields #|#: 1 bit addressing, 3 prefix per language
    (language|T|prefix, separated by |L|), 5 byte, 6 bit, 8 separator (PDF:
    "E0.7", "AEW0" without a bit)."""
    f = (value or "").split("#|#")
    if len(f) < 9:
        return value or ""
    prefix = {}
    for entry in f[3].split("|L|"):
        p = entry.split("|")
        if len(p) >= 3:
            prefix[p[0]] = p[2]
    pre = prefix.get(language) or prefix.get("DE") or next(iter(prefix.values()), "")
    bit = f[6] if f[1] == "True" and f[6].lstrip("-").isdigit() and int(f[6]) >= 0 else ""
    return f"{pre}{f[5]}" + (f"{f[8]}{bit}" if bit != "" else "")


def _value(p):
    """The shown value of a property <P>: a designator its prd, a
    multi-language text the text of the language, a PLC address, else val."""
    if p is None:
        return ""
    if p.get("prd") is not None:
        return p.get("prd") or ""
    if p.get("type") == "WSPlcChannelAddress":
        return plc_address(p.get("val"))
    w = _lines(W.text_of(p) if p.find("T") is not None else (p.get("val") or ""))
    return _without_own_structure(w) if p.get("id") == "QV" else w


def _text_value(parent, xpath):
    """PHL xpath="../P[@id='X']" -> the shown value."""
    m = re.match(r"\.\./P\[@id='([^']+)'\]", xpath or "")
    if not m or parent is None:
        return ""
    # several properties of one name (a cross reference with lineid 0, 1 ...)
    # one below the other
    return "\n".join(w for w in (_value(p) for p in parent.findall(f"P[@id='{m.group(1)}']")) if w)


# --- graphics ---------------------------------------------------------------

class Mapping:
    """WSCAD coordinates (mm, y down, relative to ox/oy) -> wscad_graphics
    coordinates (mm, y up): relative to the insertion point for an element
    (mirrored at y), absolute for the folio (H - y)."""

    def __init__(self, ox, oy, height=None):
        self.ox, self.oy, self.height = ox, oy, height

    def __call__(self, x, y):
        x, y = x + self.ox, y + self.oy
        return (x, self.height - y) if self.height is not None else (x, -y)


def _stroke(e):
    lt = e.get("lt")
    dashed = lt not in (None, "", "0", "1")
    width = _num(e.get("dk"), 0.0) or None
    return dashed, ({"width": width} if width else {})


def _arc(start, span):
    """A WSCAD arc (GDI: start angle and span in degrees, clockwise with y
    down) -> shape "A" (radians, counter-clockwise with y up): from
    -(start+span) to -start."""
    return math.radians(-(start + span)), math.radians(-start)


def shapes(node, m, texts=True):
    """The basic shapes of a WSCAD node (children line, polyline, polygon,
    rect, kreis, text) -> wscad_graphics shapes."""
    out = []
    for e in node:
        tag = e.tag
        if tag == "line":
            d, st = _stroke(e)
            out.append(("L", m(_num(e.get("x1")), _num(e.get("y1"))),
                        m(_num(e.get("x2")), _num(e.get("y2"))), d, None, st))
        elif tag in ("polyline", "polygon"):
            pts = [m(x, y) for x, y in _points(e.get("points"))]
            if len(pts) < 2:
                continue
            d, st = _stroke(e)
            closed = tag == "polygon"
            if closed:
                pts = pts + [pts[0]]
            out.append(("P", pts, closed, closed and e.get("fill") == "1", d, None, st))
        elif tag == "rect":
            x, y, w, h = (_num(e.get(k)) for k in ("x", "y", "wd", "hg"))
            pts = [m(x, y), m(x + w, y), m(x + w, y + h), m(x, y + h), m(x, y)]
            d, st = _stroke(e)
            out.append(("P", pts, True, e.get("fill") == "1", d, None, st))
        elif tag == "kreis":                     # WSCAD's (German) name of a circle
            c = m(_num(e.get("cx")), _num(e.get("cy")))
            r = _num(e.get("r"))
            d, st = _stroke(e)
            if e.get("as") is not None:
                a0, a1 = _arc(_num(e.get("as")), _num(e.get("ag")))
            elif e.get("type") == "KreisH":
                # half circle: qd = quarter bits (1: 0-90 degrees, 2: 90-180,
                # 4: 180-270, 8: 270-360, GDI angles)
                start = {3: 0, 6: 90, 12: 180, 9: 270}.get(int(_num(e.get("qd"), 3)), 0)
                a0, a1 = _arc(start, 180)
            else:
                a0 = a1 = 0.0
            out.append(("A", c, r, a0, a1, d, None, st))
        elif tag == "text" and texts:
            out += text_shapes(W.text_of(e.find("P")), _num(e.get("x")), _num(e.get("y")),
                               _style(e.get("style")), m)
    return out


def text_shapes(t, x, y, style, m, box=None):
    """A text at a WSCAD anchor (local) -> shapes "T" (one per line)."""
    t = _lines(t)
    if not t.strip():
        return []
    size = _num(style.get("size"), SIZE_DEFAULT)
    column, row = _alignment(style)
    if box and "align" in style:                 # a placeholder with a frame: anchor in the frame
        w, h = box
        x += (0, w / 2, w)[column]
        y += {"top": 0, "middle": h / 2, "bottom": h, "base": h}[row]
    angle = _num(style.get("winkel"))           # "winkel": WSCAD's (German) name of the angle
    lines = t.split("\n")
    n = len(lines)
    dz = LINE_PER_SIZE * size
    # multi-line: the first line at the anchor, also for "bottom" (PDF:
    # first base line at the anchor); only "middle" centres the block
    before = {"top": 0, "base": 0, "middle": (n - 1) / 2, "bottom": 0}[row]
    c, s_ = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    out = []
    for k, z in enumerate(lines):
        d = (k - before) * dz                    # offset across the text (downwards in the text)
        # a rotated text (WSCAD angle counter-clockwise, y down): "down" in
        # the text is (sin a, cos a)
        lx, ly = x + d * s_, y + d * c
        out.append(("T", m(lx, ly), z, size * HEIGHT_PER_SIZE, math.radians(angle),
                    _align(column, row), None, {}))
    return out


def framed(text, ph, st):
    """A placeholder with a frame (wd/hg): tc:5 cuts at the end of the frame,
    tc:6 wraps at word boundaries into lines of the frame width -> (text,
    frame or None)."""
    if not ph.get("wd"):
        return text, None
    width, size = _num(ph.get("wd")), _num(st.get("size"), SIZE_DEFAULT)
    box = (width, _num(ph.get("hg")))
    if st.get("tc") == "6":
        return _wrapped(text, width, size), box
    if st.get("tc") == "5" or st.get("pl") == "1":
        return _cut(text, width, size), box
    return text, box


def _wrapped(text, width_mm, size):
    px = G.font_px(size * HEIGHT_PER_SIZE)
    out = []
    for paragraph in _lines(text).split("\n"):
        line = ""
        for word in paragraph.split():
            probe = (line + " " + word).strip()
            if line and G.width_px(probe, px) / G.K > width_mm:
                out.append(line)
                line = word
            else:
                line = probe
        if line:
            out.append(line)
    return "\n".join(out)


def _cut(text, width_mm, size):
    """A one-line placeholder with a frame (pl:1): WSCAD cuts it at the end of
    the frame."""
    px = G.font_px(size * HEIGHT_PER_SIZE)
    line = _lines(text).split("\n")[0] if text else ""
    while line and G.width_px(line, px) / G.K > width_mm:
        line = line[:-1]
    return line


def placeholders(parent, m, with_label=True):
    """The visible placeholders (PHL) of a node -> shapes; the designator
    (BMKZ) as the label (shape "D" label)."""
    out = []
    for ph in parent.findall("PHL"):
        if ph.get("hide") is not None:
            continue
        st = _style(ph.get("style"))
        value = _text_value(parent, ph.get("xpath"))
        if not value:
            continue
        value, box = framed(value, ph, st)
        f = text_shapes(value, _num(ph.get("x")), _num(ph.get("y")), st, m, box=box)
        if with_label and st.get("wstype") == "BMKZ" and len(f) == 1:
            t = f[0]
            # for right-aligned or centred information texts QET keeps the
            # right edge or centre when the value is set, counted from the
            # empty text of the new element (measured: "-X00" moved left by
            # its width). In the element it is therefore placed by its (half)
            # width further in the writing direction.
            column = ((t[5] or 7) - 1) % 3
            w = G.width_px(t[2], G.font_px(t[3])) / G.K * (0, 0.5, 1)[column]
            (ax, ay), ang = t[1], t[4]
            f = [("D", (ax + w * math.cos(ang), ay + w * math.sin(ang)), t[2], t[3], t[4], t[5],
                  "label")]
        out += f
    return out


def data_block(sl, db, m):
    """DataBlock params="QV,False;Funktionstext,True;...": the lines of the
    properties named (True = hidden), empty ones dropped, one below the other
    from the anchor."""
    lines = []
    for part in (db.get("params") or "").split(";"):
        name, _, hidden = part.partition(",")
        if hidden.strip() == "True" or not name:
            continue
        if name == "QV":
            q = sl.find("QV/P[@id='QV']")
            value = _without_own_structure(q.get("val", "")) if q is not None else ""
        else:
            value = _text_value(sl, f"../P[@id='{name}']")
        lines += [z for z in _lines(value).split("\n") if z.strip()]
    if not lines:
        return []
    st = _style(db.get("style"))
    size = _num(st.get("size"), SIZE_DEFAULT)
    x, y = _num(db.get("x")), _num(db.get("y"))
    st_top = dict(st, align=str(int(_num(st.get("align"))) & ~24))
    out = []
    for k, z in enumerate(lines):
        out += text_shapes(z, x, y + k * LINE_DATA_BLOCK * size, st_top, m)
    return out


def _below(node, tag):
    """The descendants with this tag, not descending into embedded symbols
    (an <SL> in an <SL>, e.g. the contact image of a coil): those have
    their own, absolute position and do not belong to the element."""
    for k in node:
        if k.tag == tag:
            yield k
        if k.tag != "SL":
            yield from _below(k, tag)


def pins(sl):
    """Every terminal of a symbol (also nested in PinGP): (node, x, y,
    direction)."""
    out = []
    for p in _below(sl, "Pin"):
        x, y = _points(p.get("pos"))[0] if p.get("pos") else (0.0, 0.0)
        r = int(_num(p.get("connDir") or p.get("dir"), 0))
        out.append((p, x, y, r if r in (1, 2, 4, 8) else 0))
    return out


def pin_texts(sl, m):
    """Terminal number (nr at posNR) and name (nm at posNM) unless hidden."""
    out = []
    virtual = []
    for v in _below(sl, "VPin"):                 # virtual terminals (PLC channel list): their texts only
        x, y = _points(v.get("pos"))[0] if v.get("pos") else (0.0, 0.0)
        virtual.append((v, x, y, 0))
    for p, px, py, _ in pins(sl) + virtual:
        for attr, pos, style, hide in (("nr", "posNR", "styleNR", "hideNR"),
                                       ("nm", "posNM", "styleNM", "hideNM")):
            t = p.get(attr)
            if not t or p.get(hide) is not None or not p.get(pos):
                continue
            dx, dy = _points(p.get(pos))[0]
            out += text_shapes(t, px + dx, py + dy, _style(p.get(style)), m)
    return out


def image(e, ox, oy, height, folio):
    """<image> (Base64, PNG/JPEG) -> a QET folio image."""
    try:
        raw = base64.b64decode(e.text or "")
    except ValueError:
        return None
    if len(raw) < 8:
        return None
    x, y, w, h = (_num(e.get(k)) for k in ("x", "y", "wd", "hg"))
    factor = _num(e.get("faktor"), 1.0) or 1.0   # shown size = wd/hg x factor
    w, h = w * factor, h * factor
    return G.image_xml(folio, raw, ox + x, height - (oy + y), ox + x + w, height - (oy + y + h))


# --- page -------------------------------------------------------------------

def _file_name(text):
    return re.sub(r"[^\w.-]+", "_", text or "symbol").strip("_")[:50] or "symbol"


class Page:
    """A schematic page: symbols, terminal points and the drawn wiring (for
    the routes of the conductors)."""

    def __init__(self, info, file_):
        self.info, self.f = info, file_
        hdr = file_.find("Header")
        ps = hdr.find("PaperSize") if hdr is not None else None
        b, h = (_points(ps.get("val"))[0] if ps is not None and ps.get("val")
                else (info.get("width"), info.get("height")))
        self.width, self.height = b or 297.0, h or 210.0
        self.symbols = file_.findall("SL")
        self.terminals = {}                      # point key -> [(sl, index, direction)]
        self.edges = collections.defaultdict(set)  # point key -> neighbours (wire pieces)
        self.through = set()                     # points a route may pass (corners, T, polylines)
        self.point = {}
        self.by_hid = {}                         # SymbolID -> (origin, [(x, y) of the terminals])
        for sl in self.symbols:
            ox, oy = _num(sl.get("x")), _num(sl.get("y"))
            a, _ = _kind(sl)
            ps_ = pins(sl)
            hid = int(_num(sl.get("hid")))
            if hid:
                self.by_hid[hid] = ((ox, oy), [(px, py) for _, px, py, _ in ps_])
            keys = []
            for i, (_, px, py, r) in enumerate(ps_):
                p = (ox + px, oy + py)
                k = _key(p)
                keys.append(k)
                self.point[k] = p
                self.terminals.setdefault(k, []).append((sl, i, r))
                if a == 10:
                    self.through.add(k)
            if a == 10:                          # corner/T/bridge: every terminal joined
                for k1 in keys:
                    for k2 in keys:
                        if k1 != k2:
                            self._edge(k1, k2)
        self._facing()
        for cn in file_.findall("CN"):           # drawn wires
            for pl in cn.findall("polyline"):
                pts = _points(pl.get("points"))
                for p, q in zip(pts, pts[1:]):
                    kp, kq = _key(p), _key(q)
                    self.point[kp], self.point[kq] = p, q
                    self._edge(kp, kq)
                for p in pts[1:-1]:
                    self.through.add(_key(p))

    def terminal_position(self, hid, pin_db):
        """The position (mm, page) of a terminal of the database (PinID,
        (name, number, x, y relative to the symbol base)): the PinID counts
        the <Pin> of the symbol in file order; else the terminal at the same
        relative position. The symbol base of the database (xBase) differs
        for rotated arrows, so the origin comes from the page."""
        if hid not in self.by_hid:
            return None
        (ox, oy), rel = self.by_hid[hid]
        pid, (_, _, rx, ry) = pin_db
        if not rel:
            return None
        if 1 <= pid <= len(rel) and math.dist(rel[pid - 1], (rx, ry)) <= 0.05:
            px, py = rel[pid - 1]
        else:
            px, py = min(rel, key=lambda q: math.dist(q, (rx, ry)))
        return (ox + px, oy + py)

    def _edge(self, a, b):
        self.edges[a].add(b)
        self.edges[b].add(a)

    def _facing(self):
        """WSCAD connects a terminal with the next terminal in its direction
        that points back at it."""
        every = [(k, r) for k, lst in self.terminals.items() for _, _, r in lst if r]
        for k, r in every:
            p = self.point[k]
            dx, dy = STEP[r]
            best = None
            for k2, r2 in every:
                if r2 != OPPOSITE[r] or k2 == k:
                    continue
                q = self.point[k2]
                if dx and abs(q[1] - p[1]) <= TOL and (q[0] - p[0]) * dx > TOL:
                    d = abs(q[0] - p[0])
                elif dy and abs(q[0] - p[0]) <= TOL and (q[1] - p[1]) * dy > TOL:
                    d = abs(q[1] - p[1])
                else:
                    continue
                if best is None or d < best[0]:
                    best = (d, k2)
            if best:
                self._edge(k, best[1])

    def route(self, a, b):
        """The shortest drawn route from terminal point a to b (points),
        only through corners, T symbols and drawn wires."""
        ka, kb = _key(a), _key(b)
        if ka == kb:
            return [a]
        before = {ka: None}
        queue = collections.deque([ka])
        while queue:
            k = queue.popleft()
            if k == kb:
                break
            if k != ka and k not in self.through:
                continue
            for n in self.edges.get(k, ()):
                if n not in before:
                    before[n] = k
                    queue.append(n)
        if kb not in before:
            return None
        path, k = [], kb
        while k is not None:
            path.append(self.point[k])
            k = before[k]
        return path[::-1]


# --- roles (master/slave) ------------------------------------------------------

def _tag(sl):
    p = sl.find("P[@id='BMKZ']")
    return (p.get("prd") or "") if p is not None else ""


def _name(sl):
    rn = sl.find("P[@id='ReferenzName']")
    return ((rn.get("val") if rn is not None and rn.get("val")
             else W.text_of(sl.find("P[@id='BtName']"))) or "")


def roles(pages, sym_db):
    """A coil (kind 2) or master (kind 7) and the contacts (kind 3) of the
    same full designator -> master/slave. The symbol names come from
    WSCAD's (German) libraries: "Schutz"/"Automat" = protective device,
    "Öffner" = normally closed, "Schließer" = normally open."""
    by_tag = collections.defaultdict(list)
    for pg in pages:
        for sl in pg.symbols:
            hid = int(_num(sl.get("hid")))
            a, _ = _kind(sl)
            if hid and a in (2, 3, 7) and hid in sym_db and pins(sl):
                by_tag[sym_db[hid]["tag"]].append((a, hid, sl))
    role = {}
    for tag, lst in by_tag.items():
        main = [x for x in lst if x[0] in (2, 7)]
        contacts = [x for x in lst if x[0] == 3]
        if not main or not contacts or tag.endswith("-"):
            continue
        a, hid, sl = main[0]
        n = _name(sl).lower()
        role[hid] = ("master", "protection" if a == 7 or "schutz" in n or "automat" in n else "coil")
        for _, h2, s2 in contacts:
            n2 = _name(s2).lower()
            state = ("NC" if re.search(r"(-o$|öffner|_o$)", n2)
                     else ("NO" if re.search(r"(-s$|schlie)", n2) else "SW"))
            role[h2] = ("slave", state, "simple")
    return role, by_tag


# --- conversion -----------------------------------------------------------------

def _px(p):
    """A WSCAD page point (mm, y down) -> QET scene (px)."""
    return round(MARGIN_PX + p[0] * G.K, 1), round(MARGIN_PX + p[1] * G.K, 1)


def _write_element(xml, name, stats):
    """custom://<import folder>/WSCAD/<name>_<hash>.elmt: the same graphics
    give the same file."""
    if not S.CUSTOM:
        raise FileNotFoundError("the user collection is not set (QET_CUSTOM_ELEMENTS_DIR)")
    sig = hashlib.sha1(re.sub(r'uuid="\{[^}]*\}"', "", xml).encode("utf-8")).hexdigest()[:10]
    rel = f"{symbol_builder.IMPORT_FOLDER}/{FOLDER}/{_file_name(name)}_{sig}.elmt"
    target = os.path.join(S.CUSTOM, *rel.split("/"))
    if not os.path.exists(target):
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "w", encoding="utf-8", newline="\n") as f:
            f.write(xml)
        stats["new_elements"] += 1
    return "custom://" + rel


def _terminal_index(p, local):
    """The QET index of the terminal at the local WSCAD position (mm, y down)
    of an element."""
    rx, ry = local[0] * G.K, local[1] * G.K
    return min(p.symbol.terminals, key=lambda t: math.hypot(t.x - rx, t.y - ry)).index


def _info(sl):
    """Element information from the WSCAD properties (German ids: Artikel =
    article, Hersteller = manufacturer, BtName = designation, Funktionstext
    = function text; "Bestand" = existing stock, no article)."""
    info = {}
    article = _value(sl.find("P[@id='Artikel']")).strip()
    maker = _value(sl.find("P[@id='Hersteller']")).strip()
    if article and article.lower() != "bestand":
        info["manufacturer_reference"] = article
    if maker:
        info["manufacturer"] = maker
    bt = _value(sl.find("P[@id='BtName']"))
    if bt:
        info["designation"] = bt.replace("\n", " ")
    ft = _value(sl.find("P[@id='Funktionstext']"))
    if ft:
        info["function"] = ft.replace("\n", " ")
    return info


def symbol_texts(sl, m, with_label=True):
    """The texts of a symbol relative to its origin: placeholders, cross
    reference, data block, channel groups, terminal numbers."""
    fo = placeholders(sl, m, with_label=with_label)
    # when the data block shows the cross reference ("QV,False"), WSCAD does
    # not draw the QV placeholder
    ref_in_block = any(re.search(r"(^|;)QV,False", db.get("params") or "")
                       for db in sl.findall("DataBlock"))
    for q in ([] if ref_in_block else sl.findall("QV")):
        fo += placeholders(q, m, with_label=False)
    for db in sl.findall("DataBlock"):
        fo += data_block(sl, db, m)
    for gp in _below(sl, "PinGP"):               # contact and PLC channel groups
        fo += placeholders(gp, m, with_label=False)
        for q in gp.findall("QV"):
            fo += placeholders(q, m, with_label=False)
        for ph in gp.findall("PH"):
            st = _style(ph.get("style"))
            value, box = framed(_value(ph.find("P")), ph, st)
            fo += text_shapes(value, _num(ph.get("x")), _num(ph.get("y")), st, m, box=box)
    fo += pin_texts(sl, m)
    return fo


def symbol_graphics(sl, height, folio):
    """A symbol without an element (graphics, contact image ...) as folio
    graphics: (shapes, images)."""
    ox, oy = _num(sl.get("x")), _num(sl.get("y"))
    m = Mapping(ox, oy, height)
    fo = shapes(sl, m) + symbol_texts(sl, m, with_label=False)
    images = [x for x in (image(im, ox, oy, height, folio) for im in _below(sl, "image")) if x]
    return fo, images


_NO_LABEL = re.compile(r'\n\s*<dynamic_text [^>]*text_from="ElementInfo"[^>]*><text></text>'
                       r'<info_name>label</info_name></dynamic_text>')


def element(sl, s, role, stats):
    """A symbol as a QET element: its own graphics, texts, terminals; placed
    at the symbol origin (SL x/y)."""
    ox, oy = _num(sl.get("x")), _num(sl.get("y"))
    m = Mapping(0.0, 0.0)
    fo = shapes(sl, m) + symbol_texts(sl, m)
    ps = pins(sl)
    terminals = [(px, -py, r or 4) for _, px, py, r in ps]
    names = [p.get("nr") or p.get("nm") or str(i + 1) for i, (p, _, _, _) in enumerate(ps)]
    name = _name(sl) or "Symbol"
    xml = G.element_xml(f"WSCAD {name}", fo, terminals, DESCRIPTION, names=names)
    hid = int(_num(sl.get("hid")))
    xml = G.with_role(xml, role.get(hid))
    if not any(f[0] == "D" for f in fo):      # no visible designator: the label as information only
        xml = _NO_LABEL.sub("", xml)
    path = _write_element(xml, name, stats)
    p = s.place(path, *_px((ox, oy)), label=_tag(sl), info=_info(sl), label_xy=())
    p.wx, p.wy = ox, oy
    stats["elements"] += 1
    stats["terminals"] += len(ps)
    return p


def report_arrow(sl, s, going, stats):
    """An arrow with a target (kind 14) as a QET folio report; its text and
    cross reference come as folio text."""
    ox, oy = _num(sl.get("x")), _num(sl.get("y"))
    fo = shapes(sl, Mapping(0.0, 0.0), texts=False)
    ps = pins(sl)
    terminals = [(px, -py, r or 4) for _, px, py, r in ps[:1]]
    kind = "going" if going else "coming"
    xml = G.element_xml(f"WSCAD arrow {kind}", fo, terminals, DESCRIPTION, names=["1"])
    xml = xml.replace('link_type="simple"',
                      'link_type="%s"' % ("next_report" if going else "previous_report"), 1)
    xml = _NO_LABEL.sub("", xml)
    path = _write_element(xml, f"arrow_{kind}", stats)
    p = s.place(path, *_px((ox, oy)), label="", label_xy=())
    p.wx, p.wy = ox, oy
    stats["arrows"] += 1
    return p


def _is_going(sl):
    """"Pfeil: Richtung rechts" (arrow: to the right) leaves, "Pfeil:
    Richtung von links" (from the left) comes in."""
    return " von " not in (" " + _name(sl).lower() + " ")


REF_TARGET = re.compile(r"^\s*(=[^+/]*)?(\+[^/]*)?/(\d+)\.")


def arrow_pairs(pages):
    """Pairs the arrows with a target over every page before anything is
    drawn. The partner has the same wire name on another page; first the
    pairs whose cross references point at each other ("/2.0" = sheet 2 of
    the same structure, "=A1+P1/3.2" another one), then the cross reference
    of this arrow, then the next later and the last earlier sheet. The arrow
    on the earlier sheet goes, the one on the later sheet comes: the
    direction in the symbol name is not reliable. -> (pairs [(sl, sl)],
    {id(sl): going}, number left alone)."""
    arrows = []
    for si, pg in enumerate(pages):
        values = _title_block_values(pg.f.find("Template"))
        number = values.get("Blattnummer") or str(si + 1)
        plant, location = pg.info.get("plant") or "", pg.info.get("location") or ""
        for sl in pg.symbols:
            if _kind(sl)[0] != 14 or not pins(sl):
                continue
            name = _value(sl.find("P[@id='Leitungsname']"))
            q = sl.find("QV/P[@id='QV']")
            m = REF_TARGET.match(q.get("val", "") if q is not None else "")
            target = None
            if m:
                target = ((m.group(1) or "=" + plant).lstrip("="),
                          (m.group(2) or "+" + location).lstrip("+"), m.group(3))
            arrows.append({"sl": sl, "page": si, "name": name, "at": (plant, location, number),
                           "target": target})
    pairs, free = [], set(range(len(arrows)))
    for i, a in enumerate(arrows):
        if i not in free or not a["name"]:
            continue
        cands = [j for j in free if j != i and arrows[j]["name"] == a["name"]
                 and arrows[j]["page"] != a["page"]]
        if not cands:
            continue

        def rank(j):
            b = arrows[j]
            there, back = a["target"] == b["at"], b["target"] == a["at"]
            later = b["page"] > a["page"]
            return (not (there and back), not there, not later, abs(b["page"] - a["page"]))
        j = min(cands, key=rank)
        free -= {i, j}
        pairs.append((a, arrows[j]) if a["page"] < arrows[j]["page"] else (arrows[j], a))
    going = {}
    for a, b in pairs:
        going[id(a["sl"])], going[id(b["sl"])] = True, False
    return [(a["sl"], b["sl"]) for a, b in pairs], going, len(free)


class _Union:
    def __init__(self):
        self.p = {}

    def find(self, a):
        self.p.setdefault(a, a)
        while self.p[a] != a:
            self.p[a] = self.p[self.p[a]]
            a = self.p[a]
        return a

    def union(self, a, b):
        a, b = self.find(a), self.find(b)
        if a != b:
            self.p[b] = a


def _title_block_values(tp):
    """The fields of the WSCAD title block (German ids: Blattnummer = sheet
    number, Dateiinhalt = page title, Funktion = plant, Ort = location)."""
    values = {}
    if tp is not None:
        for ph in tp.findall("PH"):
            p = ph.find("P")
            if p is not None:
                values[p.get("id")] = _value(p)
    return values


def _label_as_text(p):
    """The label of an element as a user text instead of the information
    "label", so that QET's own cross reference stays off (WSCAD's is
    drawn)."""
    sym = p.symbol
    if sym.label_position is None:
        return
    label = next((op.get("label", "") for op in p.sheet.ops if op.get("op") == "set_label"
                  and op.get("element") == p.ref), "")
    for prop, value in (("source", "text"), ("text", label)):
        p.sheet.plan.label_ops.append({"op": "set_element_text", "folio": p.sheet.folio,
                                       "element": p.ref, "index": sym.label_index,
                                       "property": prop, "value": value})


def _folio_graphics(pg, folio, values_tp):
    """The frame and title block of the page (its WSCAD template) and its
    free graphics: (shapes, images)."""
    tp = values_tp
    H = pg.height
    m = Mapping(0.0, 0.0, H)
    fo, images = [], []
    if tp is not None:
        fo += shapes(tp, m)
        for ph in tp.findall("PH"):
            st = _style(ph.get("style"))
            value, box = framed(_value(ph.find("P")), ph, st)
            fo += text_shapes(value, _num(ph.get("x")), _num(ph.get("y")), st, m, box=box)
        for lg in tp.findall("logo"):
            for im in lg.iter("image"):
                if not _num(im.get("wd")):      # older projects: an image without a size takes the logo's frame
                    for k in ("x", "y", "wd", "hg"):
                        im.set(k, lg.get(k, "0"))
                x = image(im, 0.0, 0.0, H, folio)
                if x:
                    images.append(x)
    fo += shapes(pg.f, m)
    for cl in pg.f.findall("Cableline"):
        fo.append(("L", m(_num(cl.get("x1")), _num(cl.get("y1"))),
                   m(_num(cl.get("x2")), _num(cl.get("y2"))), cl.get("lt") not in (None, "0", "1"),
                   None, {}))
    for im in pg.f.findall("image"):
        x = image(im, 0.0, 0.0, H, folio)
        if x:
            images.append(x)
    return fo, images


def plan_of(folder, target):
    """Reads the project and lays it out: (plan, everything generate needs
    afterwards); nothing is run yet."""
    P = W.Project(folder)
    try:
        return _plan_of(P, folder, target)
    finally:
        P.close()


def _plan_of(P, folder, target):
    sym_db = P.symbols()
    pins_db = P.terminals()
    stats, skipped = collections.Counter(), collections.Counter()
    questions = []

    _progress(6, "pages")
    pages = []
    for s in P.schematic_pages():
        try:
            pages.append(Page(s, W.page_xml(s["file"])))
        except (W.FormatError, OSError, SyntaxError) as e:   # an unreadable page: say so, go on
            questions.append(f"Page {s['name']} cannot be read: {e}")
            skipped["unreadable_page"] += 1
    if not pages:
        raise RuntimeError("no readable schematic page (.wsELD) in the project")
    role, by_tag = roles(pages, sym_db)

    plan = Plan(os.path.basename(os.path.normpath(folder)), os.path.dirname(os.path.abspath(target)))
    part_of_hid, per_folio, part_of_arrow = {}, {}, {}
    arrow_pairs_sl, arrow_going, arrows_alone = arrow_pairs(pages)

    for si, pg in enumerate(pages):
        _progress(10 + 70 * si / max(1, len(pages)), "pages")
        tp = pg.f.find("Template")
        values = _title_block_values(tp)
        plant, location = pg.info.get("plant") or "", pg.info.get("location") or ""
        OWN_STRUCTURE[0] = f"={plant}+{location}" if plant or location else ""
        s = plan.sheet(values.get("Dateiinhalt", "").replace("\n", " ") or pg.info.get("name") or "",
                       (values.get("Funktion") or plant).lstrip("="),
                       (values.get("Ort") or location).lstrip("+"),
                       values.get("Blattnummer") or str(si + 1))
        s.ops.append({"op": "set_folio_border", "folio": s.folio, "property": "preset",
                      "value": "a3-landscape" if pg.width > 400 else "a4-landscape"})
        s.ops.append({"op": "set_folio_border", "folio": s.folio, "property": "display-columns",
                      "value": "false"})
        H = pg.height
        folio = G.Folio(H, MARGIN_PX, MARGIN_PX)
        fo, images = _folio_graphics(pg, folio, tp)

        for sl in pg.symbols:
            a, _ = _kind(sl)
            ox, oy = _num(sl.get("x")), _num(sl.get("y"))
            m = Mapping(ox, oy, H)
            hid = int(_num(sl.get("hid")))
            ps = pins(sl)
            if a == 10:                          # corner/T/bridge: the conductor draws the wire
                continue
            if a == 23:                          # wire attribute (wire number, potential): data at the wire
                stats["wire_attributes"] += 1
                continue
            if a == 14 and ps:
                going = arrow_going.get(id(sl), _is_going(sl))
                p = report_arrow(sl, s, going, stats)
                part_of_arrow[id(sl)] = p
                if hid:
                    part_of_hid[hid] = p
                fo += placeholders(sl, m, with_label=False)
                for q in sl.findall("QV"):
                    fo += placeholders(q, m, with_label=False)
                continue
            # embedded symbols (the contact image of a coil, SL kind 9): their
            # own, absolute position -> folio graphics
            for inner in sl.findall("SL"):
                f_, i_ = symbol_graphics(inner, H, folio)
                fo += f_
                images += i_
                stats["contact_images"] += 1
            if ps and hid:
                part_of_hid[hid] = element(sl, s, role, stats)
                for im in _below(sl, "image"):   # images in a symbol (a logo): QET elements have
                    x = image(im, ox, oy, H, folio)   # none, so a folio image at the same place
                    if x:
                        images.append(x)
                continue
            # no terminal or no equipment: folio graphics
            f_, i_ = symbol_graphics(sl, H, folio)
            fo += f_
            images += i_
            stats["graphic_symbols"] += 1
        pg.sheet = s
        per_folio[s.folio] = (folio, fo, images)

    _progress(82, "references")
    pairs = [(part_of_arrow[id(x)], part_of_arrow[id(y)]) for x, y in arrow_pairs_sl
             if id(x) in part_of_arrow and id(y) in part_of_arrow]
    skipped["arrow_without_partner"] += arrows_alone

    # wires from the connection list of the database. WSCAD lists bridges
    # (terminals, distributors) as a complete net (each with each); drawn is a
    # spanning tree per folio: the connections with a drawn route first,
    # then the shortest, each only when its ends are not joined yet.
    _progress(86, "connections")
    page_of_id = {pg.info["id"]: pg for pg in pages}
    cands = collections.defaultdict(list)
    for v in P.connections():
        pg = page_of_id.get(v["PageID"])
        if pg is None:
            skipped["connection_not_on_a_schematic_page"] += 1
            continue
        ends = []
        for sid, pin in ((v["SymbolID1"], v["Pin1"]), (v["SymbolID2"], v["Pin2"])):
            p = part_of_hid.get(sid)
            pd = pins_db.get((sid, pin))
            pos = pg.terminal_position(sid, (pin, pd)) if pd is not None else None
            if p is None or pos is None or p.sheet is not pg.sheet:
                ends.append(None)
                continue
            ends.append((p, pos))
        if None in ends:
            skipped["end_without_element"] += 1
            continue
        (pa, ka), (pb, kb) = ends
        if math.dist(ka, kb) <= TOL:
            skipped["same_point"] += 1
            continue
        ia = _terminal_index(pa, (ka[0] - pa.wx, ka[1] - pa.wy))
        ib = _terminal_index(pb, (kb[0] - pb.wx, kb[1] - pb.wy))
        if pa is pb and ia == ib:
            continue
        w = pg.route(ka, kb)
        cands[pg.sheet.folio].append((w is None, math.dist(ka, kb), pa, ia, pb, ib, v))
    drawn = 0
    degree, end_of, values_of_end = collections.Counter(), {}, {}
    net = _Union()
    for f in sorted(cands):
        for no_route, _, pa, ia, pb, ib, v in sorted(cands[f], key=lambda k: (k[0], k[1])):
            s = pa.sheet
            ea, eb = (s.folio, pa.ref, ia), (s.folio, pb.ref, ib)
            if net.find(ea) == net.find(eb):
                stats["joined_already"] += 1
                continue
            s.wire(pa, ia, pb, ib)
            drawn += 1
            stats["drawn_route" if not no_route else "no_drawn_route"] += 1
            degree[ea] += 1
            degree[eb] += 1
            net.union(ea, eb)
            end_of[ea], end_of[eb] = (s, pa, ia), (s, pb, ib)
            c = {"color": (v["Colorcode"] or "").strip(), "section": (v["Dimension"] or "").strip(),
                 "potential": (v["Potential"] or "").strip()}
            if any(c.values()):
                values_of_end.setdefault(ea, c)

    # colour, cross section and potential belong in QET to the whole
    # potential and must be the same on both sides of a pair of folio
    # reports, else QET does not link them: one value per potential across
    # the pairs, set per folio net at a terminal with exactly one wire
    across = _Union()
    for e in end_of:
        across.union(e, net.find(e))
    for pa, pb in pairs:
        ea, eb = (pa.sheet.folio, pa.ref, 0), (pb.sheet.folio, pb.ref, 0)
        if ea in end_of and eb in end_of:
            across.union(ea, eb)
    per_net, per_pot = collections.defaultdict(list), collections.defaultdict(set)
    for e in end_of:
        per_net[net.find(e)].append(e)
        per_pot[across.find(e)].add(net.find(e))
    unsure = set()
    for nets in per_pot.values():
        c = next((values_of_end[e] for n in nets for e in per_net[n] if e in values_of_end), None)
        if not c:
            continue
        for n in nets:
            end = next((e for e in per_net[n] if degree[e] == 1), None)
            if end is None:
                skipped["net_without_end"] += 1
                unsure.add(n)
                continue
            s, p, i = end_of[end]
            for prop, value in (("conductor_color", c["color"]),
                                ("conductor_section", c["section"]), ("function", c["potential"])):
                if value:
                    s.ops.append({"op": "set_conductor", "folio": s.folio, "element": p.ref,
                                  "terminal": p.terminal(i).uuid, "property": prop,
                                  "value": value})
    # the folio reports linked last (after the label positions): a refused
    # pair would hold up everything after it otherwise
    reference_ops = []
    for pa, pb in pairs:
        ea, eb = (pa.sheet.folio, pa.ref, 0), (pb.sheet.folio, pb.ref, 0)
        if net.find(ea) in unsure or net.find(eb) in unsure:
            skipped["report_without_equal_values"] += 1
            continue
        reference_ops.append({"op": "link_elements", "folio": pa.sheet.folio, "element": pa.ref,
                              "to_folio": pb.sheet.folio, "to": pb.ref})
        stats["reports_linked"] += 1

    # master/slave (WSCAD's cross-reference texts stay, QET's own are off)
    for hid, r_ in role.items():
        if r_[0] != "master" or hid not in part_of_hid:
            continue
        mp = part_of_hid[hid]
        slaves = [part_of_hid[h2] for _, h2, _ in by_tag.get(sym_db[hid]["tag"], [])
                  if role.get(h2, ("",))[0] == "slave" and h2 in part_of_hid]
        if slaves:
            mp.sheet.link(mp, *slaves)
            stats["linked"] += len(slaves)
            for p in [mp, *slaves]:
                _label_as_text(p)
    plan.label_ops += reference_ops
    return plan, {"pages": pages, "per_folio": per_folio, "stats": stats, "skipped": skipped,
                  "drawn": drawn, "questions": questions, "sym_db": sym_db}


def convert(folder, target, pdf=False):
    _progress(2, "read")
    os.makedirs(os.path.dirname(os.path.abspath(target)), exist_ok=True)
    P = W.Project(folder)
    try:
        plan, d = _plan_of(P, folder, target)
        _progress(90, "lists")
        write_lists(P, d["sym_db"], target)
    finally:
        P.close()
    _progress(92, "write")
    result, r = plan.generate(os.path.basename(target), pdf=False)
    if not os.path.isfile(result):
        raise RuntimeError("QElectroTech wrote no project; the reason is in the output above")
    with open(result, encoding="utf-8") as f:
        s_qet = f.read()
    with open(result, "w", encoding="utf-8", newline="\n") as f:
        f.write(re.sub(r'<report label="[^"]*"', '<report label=""', s_qet))
    insert = {}
    for f_index, (folio, fo, images) in d["per_folio"].items():
        out_shapes, inputs = G.folio_xml(folio, fo)
        insert[f_index] = (out_shapes, inputs, images)
    G.insert_into_project(result, insert)
    failed = [o for o in r.get("operations", []) if not o.get("succeeded")]
    stats, skipped = d["stats"], d["skipped"]
    report = (f"pages {len(d['pages'])}, elements {stats['elements']} (new element files "
              f"{stats['new_elements']}), terminals {stats['terminals']}, wires {d['drawn']} "
              f"(duplicates of the list left out {stats['joined_already']}, with a drawn route "
              f"{stats['drawn_route']}, without {stats['no_drawn_route']}), not drawn {dict(skipped)}, "
              f"arrows {stats['arrows']} (linked pairs {stats['reports_linked']}), master/slave "
              f"{stats['linked']}, graphic symbols {stats['graphic_symbols']}, wire attributes "
              f"{stats['wire_attributes']}, failed operations {len(failed)}")
    print(report)
    write_questions(questions_path(target), folder, report, d["questions"], skipped, plan)
    if pdf:
        _progress(96, "pdf")
        print("pdf:", qet_runner.export_pdf(result, result[:-4] + ".pdf").get("ok"))
    return result


def questions_path(target):
    return re.sub(r"\.qet$", "", target) + "_open_questions.txt"


SKIPPED_TEXT = {
    "unreadable_page": "pages that cannot be read",
    "arrow_without_partner": "arrows with a target without a partner of the same wire name",
    "connection_not_on_a_schematic_page": "connections of the database on no schematic page",
    "end_without_element": "connections with an end on no drawn element",
    "same_point": "connections whose ends lie on the same point",
    "net_without_end": "nets without a terminal of one wire (colour, cross section not set)",
    "report_without_equal_values": "folio reports not linked (different wire data on both sides)"}


def write_questions(path, folder, report, questions, skipped, plan):
    z = [f"Open questions of the WSCAD import of {os.path.basename(os.path.normpath(folder))}", "",
         report, ""]
    z += ["- " + q for q in questions]
    z += [f"- {SKIPPED_TEXT.get(k, k)}: {n}" for k, n in sorted(skipped.items()) if n]
    z += ["", "Wires follow QElectroTech's own routing, not the route drawn in WSCAD.",
          "PE wires are drawn solid; PLC cards and channels are elements, not linked as PLC "
          "master/channel; wire numbers (wire attributes) are not taken over."]
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(z) + "\n")


def write_lists(P, sym_db, target):
    """Device and connection list (CSV, for spreadsheets) next to the
    project."""
    base = os.path.splitext(target)[0]
    articles = P.articles()
    page = {s["id"]: s["name"] for s in P.pages}
    with open(base + "_devices.csv", "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["Designator", "Article", "Manufacturer", "Page"])
        seen = set()
        for s in sorted(sym_db.values(), key=lambda s: s["tag"] or ""):
            t = s["tag"] or ""
            if not t or t.endswith("-") or t in seen:
                continue
            seen.add(t)
            a = articles.get(s["parent"], {}) or articles.get(s["id"], {})
            w.writerow([t, a.get("article", ""), a.get("manufacturer", ""), page.get(s["page"], "")])
    pins_db = P.terminals()

    def end(sid, pin):
        s = sym_db.get(sid, {})
        pd = pins_db.get((sid, pin), ("", "", 0, 0))
        return f"{s.get('tag', '')}:{pd[1] or pd[0]}"
    with open(base + "_connections.csv", "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["From", "To", "Potential", "Colour", "Cross section", "Page"])
        for v in P.connections():
            w.writerow([end(v["SymbolID1"], v["Pin1"]), end(v["SymbolID2"], v["Pin2"]),
                        v["Potential"] or "", v["Colorcode"] or "", v["Dimension"] or "",
                        page.get(v["PageID"], "")])


def safe_name(s):
    return re.sub(r"[^\w.+-]+", "_", s).strip("_") or "project"


def default_target(folder):
    name = os.path.basename(os.path.normpath(folder))
    return os.path.join(os.path.abspath(folder), "QET", safe_name(name) + ".qet")


def project_folder(path):
    """A folder, a page file or Database/WSCAD6.db -> the project folder."""
    if os.path.isfile(path):
        path = os.path.dirname(os.path.abspath(path))
        if os.path.basename(path).lower() == "database":
            path = os.path.dirname(path)
    elif os.path.basename(os.path.normpath(path)).lower() == "database":
        path = os.path.dirname(os.path.abspath(path))
    return path


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    if not args:
        print(__doc__.strip().split("\n\n")[1], file=sys.stderr)
        return 2
    folder = project_folder(args[0])
    target = args[1] if len(args) > 1 else default_target(folder)
    try:
        result = convert(folder, target, pdf="--pdf" in argv)
    except (OSError, RuntimeError, W.FormatError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    _progress(100, "done")
    print("done:", result)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
