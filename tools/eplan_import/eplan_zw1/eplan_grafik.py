"""EPLAN drawing primitives -> QElectroTech element files and folio graphics.

Primitives are tuples in mm, origin bottom left, y up, as the EPLAN export draws them:
- ("L", a, b, dashed, colour, style)                           line
- ("A", centre, r, start, end, dashed, colour, style)          arc or circle (angles in radians, start == end: circle)
- ("P", points, closed, filled, dashed, colour, style)          polyline / polygon
- ("T", anchor, text, height, angle, alignment, colour, style)  fixed text
- ("D", anchor, text, height, angle, alignment, info)           text from an element information (dynamic text)
style is a dict with "width" (line width in mm), "bold", "italic"; colour None is black. Text alignment
follows EPLAN's numbering: 1-3 top, 4-6 middle, 7-9 bottom, 10-12 baseline, each left/centre/right;
missing = 7.

Scene units: K px per mm (an A3 folio, 420 mm wide, is 1560 px on 10 columns of 156 px), X0 px left margin,
H_MM the page height in mm (y is flipped).
"""
import html
import math
import os
import re
import struct
import uuid

K = 1560 / 420                    # px per mm
X0, H_MM = 20, 297
DIRECTION = {1: "n", 2: "e", 4: "s", 8: "w"}
# Font of the texts: EPLAN's default font is Tahoma (font 0 of the font table); EPLAN_FONT chooses another.
FONT = os.environ.get("EPLAN_FONT", "Tahoma")
# Font metrics. EPLAN's cell height is 1.5 x the text height = ascent + descent of the font (Tahoma
# usWinAscent/Descent 2049/423 at 2048 units per em) -> em = 1.5 * 2048 / 2472 = 1.243 x the text height.
# QElectroTech: pixel size = round(pt * 4 / 3); a QGraphicsTextItem (folio text, element label) has a 4 px
# margin: first letter at x + 4, baseline at y + 4 + ascent; element texts <text> have no margin, x/y is the
# left baseline point, or with anchor="alignment" its centre/right.
TAHOMA_EM = 1.5 * 2048 / 2472
TAHOMA_TOP = 2049 / 2472
CELL = 1.5                        # cell height of the font / text height
QET_MARGIN = 4
_ADVANCE = {}


def _advances():
    """Advance width per character (fraction of the em): Tahoma's own when fontTools and tahoma.ttf are
    there, otherwise an estimate that keeps texts in about the right place."""
    if not _ADVANCE:
        try:
            from fontTools.ttLib import TTFont
            f = TTFont(os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts", "tahoma.ttf"))
            upm, hm = f["head"].unitsPerEm, f["hmtx"]
            _ADVANCE.update({cp: hm[g][0] / upm for cp, g in f.getBestCmap().items()})
            _ADVANCE[None] = hm[".notdef"][0] / upm
        except Exception:
            _ADVANCE.update({ord(c): 0.27 for c in "il.,:;|!'ijtfI "})
            _ADVANCE.update({ord(c): 0.85 for c in "mwMW"})
            _ADVANCE[None] = 0.55
    return _ADVANCE


def font_px(h_mm):
    """Pixel size of the QElectroTech font whose em matches EPLAN's text height h."""
    return max(1, round(h_mm * TAHOMA_EM * K))


def font_pt(h_mm):
    """Point size for QElectroTech (a multiple of 0.75 pt, so that Qt picks exactly font_px pixels)."""
    return font_px(h_mm) * 0.75


def width_px(line, px):
    w = _advances()
    return sum(w.get(ord(c), w[None]) for c in line) * px


def baseline_px(h_mm, alignment):
    """Baseline of the first line below the EPLAN anchor (scene px, y down) for each alignment:
    top = top of the cell, middle = middle of the cell, bottom = bottom of the cell, 10-12 = baseline."""
    cell = CELL * h_mm * K
    top = cell * TAHOMA_TOP
    a = alignment or 7
    if a in (1, 2, 3):
        return top
    if a in (4, 5, 6):
        return top - cell / 2
    if a in (7, 8, 9):
        return top - cell
    return 0.0


def _style(f):
    """Extra values of a primitive (its last field, a dict): width (mm), bold, italic."""
    return f[-1] if f and isinstance(f[-1], dict) else {}


# --- as a QElectroTech element (relative to the insertion point) -------------------------------------------

STYLE = "line-style:{s};line-weight:{w};filling:{f};color:black"


def line_weight(width):
    """EPLAN line width (mm) -> QElectroTech line weight of an element (thin 0.5 px, normal 1 px, hight 2 px,
    eleve 5 px; 1 px = 1/K mm = 0.27 mm). Without a width: thin."""
    if not width:
        return "thin"
    px = width * K
    return "thin" if px < 0.7 else "normal" if px < 1.5 else "hight" if px < 3.5 else "eleve"


def font(pt, style=None):
    """QFont string (family,pointSize,pixelSize,styleHint,weight,italic,underline,strikeOut,fixedPitch,rawMode,
    styleName) with bold/italic."""
    style = style or {}
    bold, italic = bool(style.get("bold")), bool(style.get("italic"))
    if not bold and not italic:
        return f"{FONT},{_f(pt)},-1,5,50,0,0,0,0,0,Regular"
    # without a style name: with "Bold", Qt picked a slanted Tahoma for the PDF export; weight and italic suffice
    return f"{FONT},{_f(pt)},-1,5,{75 if bold else 50},{1 if italic else 0},0,0,0,0"


def _f(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def _text_box(t, h_mm, alignment):
    """-> (pt, px, dx, dy): offset of the left baseline point of the first (widest) line from the anchor."""
    px = font_px(h_mm)
    width = max(width_px(z, px) for z in t.split("\n"))
    column = ((alignment or 7) - 1) % 3
    return font_pt(h_mm), px, -width * (0, 0.5, 1)[column], baseline_px(h_mm, alignment)


def with_role(xml, role):
    """role: None | ('master', kind) | ('slave', state, kind) -> link_type and kindInformations
    (QElectroTech: master coil/protection/commutator; slave state NO/NC/SW, type simple/power)."""
    if not role:
        return xml
    if role[0] == "master":
        kind = f'<kindInformation name="type">{role[1]}</kindInformation>'
    else:
        kind = (f'<kindInformation name="type">{role[2]}</kindInformation>'
                f'<kindInformation name="state">{role[1]}</kindInformation>'
                '<kindInformation name="number">1</kindInformation>')
    xml = xml.replace('link_type="simple"', f'link_type="{role[0]}"', 1)
    return xml.replace("    <description>", f"    <kindInformations>{kind}</kindInformations>\n    <description>", 1)


def element_xml(name, shapes, terminals, information="", names=None, infos=None):
    """shapes in mm relative to the insertion point; terminals = [(x_mm, y_mm, direction)];
    names: terminal names (otherwise 1, 2, ...); infos: element informations {name: value}.
    A "D" shape is a text from an element information (dynamic_text text_from="ElementInfo") where the text
    would stand as a fixed shape; a "D" with the information "label" replaces the label at the top left."""
    parts, xs, ys, dyn = [], [0.0], [0.0], []
    has_label = False

    def P(x, y):
        X, Y = x * K, -y * K
        xs.append(X)
        ys.append(Y)
        return X, Y
    for f in shapes:
        if f[0] == "L":
            (x1, y1), (x2, y2) = P(*f[1]), P(*f[2])
            st = STYLE.format(s="dashed" if f[3] else "normal", f="none", w=line_weight(_style(f).get("width")))
            parts.append(f'<line x1="{_f(x1)}" y1="{_f(y1)}" x2="{_f(x2)}" y2="{_f(y2)}" length1="1.5" '
                         f'length2="1.5" end1="none" end2="none" style="{st}" antialias="true"/>')
        elif f[0] == "A":
            (cx, cy), r = f[1], f[2]
            P(cx - r, cy - r)
            P(cx + r, cy + r)
            X, Y = cx * K - r * K, -cy * K - r * K
            a0, a1 = math.degrees(f[3]), math.degrees(f[4])
            st = STYLE.format(s="dashed" if f[5] else "normal", f="none", w=line_weight(_style(f).get("width")))
            if abs(a1 - a0) < 1e-6 or abs(abs(a1 - a0) - 360) < 1e-6:
                parts.append(f'<ellipse x="{_f(X)}" y="{_f(Y)}" width="{_f(2 * r * K)}" height="{_f(2 * r * K)}" '
                             f'style="{st}" antialias="true"/>')
            else:
                span = (a1 - a0) % 360 or 360
                parts.append(f'<arc x="{_f(X)}" y="{_f(Y)}" width="{_f(2 * r * K)}" height="{_f(2 * r * K)}" '
                             f'start="{_f(a0 % 360)}" angle="{_f(span)}" style="{st}" antialias="true"/>')
        elif f[0] == "P":
            pts = [P(*q) for q in f[1]]
            if f[2]:
                pts = pts[:-1]
            attr = " ".join(f'x{i + 1}="{_f(x)}" y{i + 1}="{_f(y)}"' for i, (x, y) in enumerate(pts))
            st = STYLE.format(s="dashed" if f[4] else "normal", f="black" if f[3] else "none",
                              w=line_weight(_style(f).get("width")))
            parts.append(f'<polygon {attr} closed="{"true" if f[2] else "false"}" antialias="true" style="{st}"/>')
        elif f[0] == "T":
            (x, y), t, h, ang, alignment = f[1], f[2], f[3], f[4], f[5]
            pt, px, _, dy = _text_box(t, h, alignment)
            column = ((alignment or 7) - 1) % 3
            align_attr = ('', ' Halignment="AlignHCenter" anchor="alignment"',
                          ' Halignment="AlignRight" anchor="alignment"')[column]
            X, Y = P(x, y)
            r = math.radians(-math.degrees(ang) % 360)
            c, s_ = math.cos(r), math.sin(r)
            for k, line in enumerate(t.split("\n")):
                ly = dy + k * CELL * h * K          # line spacing = cell height 1.5 x text height
                w = width_px(line, px)
                tx, ty = X - ly * s_, Y + ly * c    # baseline point in the rotated text frame
                parts.append(f'<text text="{html.escape(line, quote=True)}" x="{_f(tx)}" y="{_f(ty)}" '
                             f'rotation="{_f(-math.degrees(ang) % 360)}"{align_attr} '
                             f'font="{font(pt, _style(f))}" color="#000000"/>')
                lx0 = -w * (0, 0.5, 1)[column]
                for ex, ey in ((lx0, -px), (lx0 + w, 0.3 * px)):
                    xs.append(tx + ex * c - ey * s_)
                    ys.append(ty + ex * s_ + ey * c)
        elif f[0] == "D":
            # Element information as a text: a QGraphicsTextItem placed by its top left corner including the
            # 4 px margin and rotated about it -- the same sums as text_on_folio, so that the text stands where
            # the fixed shape stood. The alignment is kept (QElectroTech holds that edge when the value changes).
            (x, y), t, h, ang, alignment, info_ = f[1], f[2], f[3], f[4], f[5], f[6]
            px, pt = font_px(h), font_pt(h)
            column = ((alignment or 7) - 1) % 3
            X, Y = P(x, y)
            w = width_px(t, px)
            lx = -w * (0, 0.5, 1)[column] - QET_MARGIN
            ly = baseline_px(h, alignment) - QET_MARGIN - px
            r = -ang
            c, s_ = math.cos(r), math.sin(r)
            tx, ty = X + lx * c - ly * s_, Y + lx * s_ + ly * c
            for ex, ey in ((QET_MARGIN, QET_MARGIN), (QET_MARGIN + w, QET_MARGIN + 1.3 * px)):
                xs.append(tx + ex * c - ey * s_)
                ys.append(ty + ex * s_ + ey * c)
            halign = ("AlignLeft", "AlignHCenter", "AlignRight")[column]
            u_d = uuid.uuid5(uuid.NAMESPACE_URL, f"{name}/info/{info_}/{len(dyn)}")
            if info_ == "label":
                has_label = True
            dyn.append(f'<dynamic_text x="{_f(tx)}" y="{_f(ty)}" z="3" text_width="-1" Halignment="{halign}" '
                       f'Valignment="AlignTop" frame="false" rotation="{_f(-math.degrees(ang) % 360)}" '
                       f'keep_visual_rotation="false" text_from="ElementInfo" uuid="{{{u_d}}}" '
                       f'font="{font(pt, _style(f))}"><text>{html.escape(t)}</text>'
                       f'<info_name>{html.escape(info_)}</info_name></dynamic_text>')
    for i, (x, y, r) in enumerate(terminals):
        X, Y = P(x, y)
        u = uuid.uuid5(uuid.NAMESPACE_URL, f"{name}/{i}")
        nm = html.escape(names[i] if names and i < len(names) else str(i + 1), quote=True)
        parts.append(f'<terminal uuid="{{{u}}}" name="{nm}" x="{_f(X)}" y="{_f(Y)}" '
                     f'orientation="{DIRECTION.get(r, "s")}" type="Generic"/>')
    x0, x1 = math.floor(min(xs) / 10) * 10 - 10, math.ceil(max(xs) / 10) * 10 + 10
    y0, y1 = math.floor(min(ys) / 10) * 10 - 10, math.ceil(max(ys) / 10) * 10 + 10
    u = uuid.uuid5(uuid.NAMESPACE_URL, name)
    lab = uuid.uuid5(uuid.NAMESPACE_URL, name + "/label")
    ei = ""
    if infos:
        ei = ("    <elementInformations>" + "".join(
            f'<elementInformation name="{html.escape(k_, quote=True)}" show="1">{html.escape(str(v_))}'
            '</elementInformation>' for k_, v_ in infos.items()) + "</elementInformations>\n")
    label_dyn = "" if has_label else (
        f'\n        <dynamic_text x="{x0 + 2}" y="{y0 - 2}" z="3" text_width="-1" Halignment="AlignLeft" '
        f'Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="false" text_from="ElementInfo" '
        f'uuid="{{{lab}}}" font="{FONT},7,-1,5,50,0,0,0,0,0,Regular"><text></text>'
        f'<info_name>label</info_name></dynamic_text>')
    return (f'<definition version="0.100.0" type="element" link_type="simple" width="{x1 - x0}" height="{y1 - y0}" '
            f'hotspot_x="{-x0}" hotspot_y="{-y0}">\n'
            f'    <uuid uuid="{{{u}}}"/>\n    <names>\n        <name lang="en">{html.escape(name)}</name>\n'
            f'    </names>\n' + ei +
            f'    <informations>{html.escape(information)}</informations>\n    <description>\n        '
            + "\n        ".join(parts + dyn) + label_dyn +
            f'\n    </description>\n</definition>\n')


# --- as QElectroTech shapes and texts on the folio -----------------------------------------------------------

def _px(x, y):
    return X0 + x * K, (H_MM - y) * K


def _shape(kind, pts, dashed=False, closed=False, filled=False, colour=None, width=None):
    """width: EPLAN line width (mm) -> pen width in scene px (K px per mm); without a width 1 px."""
    line = "DashLine" if dashed else "SolidLine"
    pen = f'<pen style="{line}" color="{colour or "#000000"}" widthF="{_f(max(0.3, width * K)) if width else 1}"/>'
    brush = f'<brush style="{"SolidPattern" if filled else "NoBrush"}" color="{colour or "#000000"}"/>'
    if kind == "Polygon":
        points = "".join(f'<point x="{_f(x)}" y="{_f(y)}"/>' for x, y in pts)
        return (f'<shape uuid="{{{uuid.uuid4()}}}" type="Polygon" is_movable="0" closed="{int(closed)}" z="1">'
                f'{pen}{brush}<points>{points}</points></shape>')
    (x1, y1), (x2, y2) = pts
    return (f'<shape uuid="{{{uuid.uuid4()}}}" type="{kind}" is_movable="0" closed="0" x1="{_f(x1)}" y1="{_f(y1)}" '
            f'x2="{_f(x2)}" y2="{_f(y2)}" z="1">{pen}{brush}</shape>')


def _arc_points(c, r, a0, a1, n=None):
    span = (a1 - a0) % (2 * math.pi) or 2 * math.pi
    n = n or max(6, int(span / (math.pi / 16)))
    return [(c[0] + r * math.cos(a0 + span * k / n), c[1] + r * math.sin(a0 + span * k / n)) for k in range(n + 1)]


def _input(t, x, y, pt, ang=0.0, colour=None, style=None):
    content = html.escape(t).replace("\n", "<br/>")
    if colour:                       # <input> has no colour attribute; QElectroTech reads the text as HTML
        content = f'<font color="{colour}">{content}</font>'
    return (f'<input uuid="{{{uuid.uuid4()}}}" x="{_f(x)}" y="{_f(y)}" rotation="{_f(-math.degrees(ang) % 360)}" '
            f'font="{font(pt, style)}" text="{html.escape(content, quote=True)}"/>')


def text_on_folio(t, anchor, h_mm, ang, alignment, dx=0.0, dy=0.0, colour=None, style=None):
    """Free text at an EPLAN anchor (mm) with its alignment -> one <input> per line. EPLAN aligns the block
    of lines (n cells of 1.5 x the text height): top/baseline puts the first line at the anchor, middle the
    middle of the block, bottom the last line; each line is aligned left/centre/right on its own.
    QElectroTech's text field: 4 px margin, baseline at y + 4 + ascent."""
    px, pt = font_px(h_mm), font_pt(h_mm)
    lines = t.split("\n")
    column = ((alignment or 7) - 1) % 3
    ax, ay = _px(anchor[0] + dx, anchor[1] + dy)
    ly0 = baseline_px(h_mm, alignment) - QET_MARGIN - px
    a_ = alignment or 7
    if a_ in (4, 5, 6, 7, 8, 9) and len(lines) > 1:
        ly0 -= (len(lines) - 1) * h_mm * CELL * K * (0.5 if a_ in (4, 5, 6) else 1.0)
    # rotated text (EPLAN angle counter-clockwise): the offset in the text frame, turned with it;
    # QElectroTech turns a text field about its top left corner (clockwise positive, y down)
    r = -ang
    c, s_ = math.cos(r), math.sin(r)
    out = []
    for k, z in enumerate(lines):
        if not z.strip():
            continue
        lx = -width_px(z, px) * (0, 0.5, 1)[column] - QET_MARGIN
        ly = ly0 + k * h_mm * CELL * K
        out.append(_input(z, ax + lx * c - ly * s_, ay + lx * s_ + ly * c, pt, ang, colour, style))
    return "".join(out)


def _colour(f, i):
    """Colour of a primitive (None, or missing for primitives of other origin -> black)."""
    return f[i] if len(f) > i and isinstance(f[i], str) else None


def folio_xml(shapes, offset=(0.0, 0.0)):
    """Primitives (mm, absolute on the page) -> (shapes, inputs) as XML strings."""
    out_shapes, inputs = [], []
    ox, oy = offset
    for f in shapes:
        if f[0] == "L":
            a, b = _px(f[1][0] + ox, f[1][1] + oy), _px(f[2][0] + ox, f[2][1] + oy)
            if math.dist(a, b) > 0.2:
                out_shapes.append(_shape("Line", [a, b], f[3], colour=_colour(f, 4), width=_style(f).get("width")))
        elif f[0] == "A":
            c, r = (f[1][0] + ox, f[1][1] + oy), f[2]
            if abs(f[4] - f[3]) < 1e-6:
                p1, p2 = _px(c[0] - r, c[1] + r), _px(c[0] + r, c[1] - r)
                out_shapes.append(_shape("Ellipse", [p1, p2], f[5], colour=_colour(f, 6),
                                         width=_style(f).get("width")))
            else:
                out_shapes.append(_shape("Polygon", [_px(*q) for q in _arc_points(c, r, f[3], f[4])], f[5],
                                         colour=_colour(f, 6), width=_style(f).get("width")))
        elif f[0] == "P":
            pts = [_px(x + ox, y + oy) for x, y in f[1]]
            if f[2]:
                pts = pts[:-1]
            out_shapes.append(_shape("Polygon", pts, f[4], f[2], f[3], colour=_colour(f, 5),
                                     width=_style(f).get("width")))
        elif f[0] == "T":
            inputs.append(text_on_folio(f[2], (f[1][0] + ox, f[1][1] + oy), f[3], f[4], f[5],
                                        colour=_colour(f, 6), style=_style(f)))
    return out_shapes, inputs


def _png_size(data):
    """(width, height) of PNG data, or None."""
    if data[:8] == b"\x89PNG\r\n\x1a\n" and data[12:16] == b"IHDR":
        return struct.unpack(">II", data[16:24])
    return None


def image_xml(path, x0, y0, x1, y1, max_px=1200):
    """A picture of the archive fitted into the EPLAN rectangle (mm) -> <image> (PNG, base64). PNG files are
    taken as they are; other formats need PyMuPDF (pymupdf) and are left out without it."""
    import base64
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError:
        return None
    size = _png_size(data)
    if size is None:
        try:
            import pymupdf
            pix = pymupdf.Pixmap(path)
            while pix.width > max_px or pix.height > max_px:
                pix.shrink(1)
            data, size = pix.tobytes("png"), (pix.width, pix.height)
        except Exception:
            return None
    w, h = size
    if not w or not h:
        return None
    left, top = _px(min(x0, x1), max(y0, y1))
    right, bottom = _px(max(x0, x1), min(y0, y1))
    scale = min((right - left) / w, (bottom - top) / h)
    # QElectroTech: the position is the top left corner of the UNscaled picture, scaled about its centre
    x = (left + right) / 2 - w / 2
    y = (top + bottom) / 2 - h / 2
    return (f'<image uuid="{{{uuid.uuid4()}}}" x="{_f(x)}" y="{_f(y)}" z="0" rotation="0" size="{scale:.6f}" '
            f'is_movable="false">{base64.b64encode(data).decode()}</image>')


def insert_into_project(qet, per_folio):
    """per_folio: {folio index: (shapes, inputs[, images])} -> into the saved .qet (after it was made)."""
    with open(qet, encoding="utf-8") as f:
        s = f.read()
    parts = re.split(r"(<diagram [^>]*>)", s)
    folio = -1
    for k, part in enumerate(parts):
        if part.startswith("<diagram "):
            folio += 1
            continue
        if folio < 0 or folio not in per_folio:
            continue
        shapes, inputs = per_folio[folio][:2]
        images = per_folio[folio][2] if len(per_folio[folio]) > 2 else []
        end = part.find("</diagram>")
        head, rest = part[:end], part[end:]
        for tag, new in (("shapes", shapes), ("inputs", inputs), ("images", images)):
            if not new:
                continue
            # only the folio's own container (indented 8), not one of an element (indented 16)
            m = re.search(r"\n {8}<" + tag + r"(/?)>", head)
            if m and m.group(1):
                head = head[:m.start()] + f"\n        <{tag}>" + "".join(new) + f"</{tag}>" + head[m.end():]
            elif m:
                head = head[:m.end()] + "".join(new) + head[m.end():]
            else:
                head += f"<{tag}>" + "".join(new) + f"</{tag}>"
        parts[k] = head + rest
    with open(qet, "w", encoding="utf-8") as f:
        f.write("".join(parts))
