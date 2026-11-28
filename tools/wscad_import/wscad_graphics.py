"""Shapes in mm -> QElectroTech element files and folio graphics.

A shape is a tuple, all coordinates in mm with y upwards:
  ("L", p1, p2, dashed, color, style)                 line
  ("P", points, closed, filled, dashed, color, style) polyline/polygon
  ("A", centre, r, a0, a1, dashed, color, style)      arc (radians, counter-
                                                      clockwise; a0 == a1: circle)
  ("T", anchor, text, h, angle, align, color, style)  fixed text
  ("D", anchor, text, h, angle, align, info_name)     element information
                                                      shown as text (elements)
`style` is a dict (width in mm); `align` 1-12: 1-3 top, 4-6 middle, 7-9
bottom, 10-12 base line, each left/centre/right; `h` is the text height.

The scale is that of a printed sheet at 96 dpi: K px per mm. Folio graphics
start at QET's scene origin plus the margin of the folio (Diagram::margin).
"""
import html
import math
import re
import struct
import uuid

K = 96 / 25.4                    # px per mm
FONT = "Roboto Mono"             # the font of WSCAD's PDF export
# Roboto Mono is monospaced: every glyph advances 0.6 em, so the widths of
# texts are known without the font file
ADVANCE = 0.6
# font metrics the text positions were calibrated with (em per text height,
# cell height per text height, ascent per cell)
EM = 1.5 * 2048 / 2472
CELL = 1.5
ASCENT = 2049 / 2472
QET_MARGIN = 4                   # padding of QGraphicsTextItem (free texts, element information)
ORIENTATION = {1: "n", 2: "e", 4: "s", 8: "w"}
STYLE = "line-style:{s};line-weight:{w};filling:{f};color:black"


def font_px(h_mm):
    """QET pixel size of the font whose em matches the text height h."""
    return max(1, round(h_mm * EM * K))


def font_pt(h_mm):
    """Point size for QET (a multiple of 0.75 pt, so that Qt picks exactly
    font_px pixels)."""
    return font_px(h_mm) * 0.75


def width_px(line, px):
    return len(line) * ADVANCE * px


def baseline_px(h_mm, align):
    """The base line of the first line below the anchor (scene px, y down):
    top = top of the cell, middle = its middle, bottom = its bottom, 10-12 =
    the base line."""
    cell = CELL * h_mm * K
    top = cell * ASCENT
    a = align or 7
    if a in (1, 2, 3):
        return top
    if a in (4, 5, 6):
        return top - cell / 2
    if a in (7, 8, 9):
        return top - cell
    return 0.0


def _f(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def _style(f):
    """The extra data of a shape (its last field, a dict): width in mm."""
    return f[-1] if f and isinstance(f[-1], dict) else {}


def _color(f, i):
    return f[i] if len(f) > i and isinstance(f[i], str) else None


def line_weight(width):
    """A line width (mm) -> QET's line weight of an element (thin 0.5 px,
    normal 1 px, hight 2 px, eleve 5 px)."""
    if not width:
        return "thin"
    px = width * K
    return "thin" if px < 0.7 else "normal" if px < 1.5 else "hight" if px < 3.5 else "eleve"


def font(pt):
    """A QFont string."""
    return f"{FONT},{_f(pt)},-1,5,50,0,0,0,0,0,Regular"


def _text_box(t, h_mm, align):
    """(pt, px, dx, dy): offset of the left base line point of the first
    line from the anchor."""
    px = font_px(h_mm)
    w = max(width_px(z, px) for z in t.split("\n"))
    column = ((align or 7) - 1) % 3
    return font_pt(h_mm), px, -w * (0, 0.5, 1)[column], baseline_px(h_mm, align)


def with_role(xml, role):
    """role: None | ('master', kind) | ('slave', state, kind) -> link_type
    and kindInformations (master: coil/protection; slave: state NO/NC/SW,
    type simple/power)."""
    if not role:
        return xml
    if role[0] == "master":
        kind = f'<kindInformation name="type">{role[1]}</kindInformation>'
    else:
        kind = (f'<kindInformation name="type">{role[2]}</kindInformation>'
                f'<kindInformation name="state">{role[1]}</kindInformation>'
                '<kindInformation name="number">1</kindInformation>')
    xml = xml.replace('link_type="simple"', f'link_type="{role[0]}"', 1)
    return xml.replace("    <description>",
                       f"    <kindInformations>{kind}</kindInformations>\n    <description>", 1)


def element_xml(name, shapes, terminals, description="", names=None, info=None):
    """An element file. shapes in mm relative to the insertion point;
    terminals = [(x_mm, y_mm, direction 1/2/4/8)]; names: the terminal names
    (else 1, 2, ...); info: element information {name: value}. A "D" shape
    with the info "label" replaces the label at the top left."""
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
            style = STYLE.format(s="dashed" if f[3] else "normal", f="none",
                                 w=line_weight(_style(f).get("width")))
            parts.append(f'<line x1="{_f(x1)}" y1="{_f(y1)}" x2="{_f(x2)}" y2="{_f(y2)}" '
                         f'length1="1.5" length2="1.5" end1="none" end2="none" style="{style}" '
                         'antialias="true"/>')
        elif f[0] == "A":
            (cx, cy), r = f[1], f[2]
            P(cx - r, cy - r)
            P(cx + r, cy + r)
            X, Y = cx * K - r * K, -cy * K - r * K
            a0, a1 = math.degrees(f[3]), math.degrees(f[4])
            style = STYLE.format(s="dashed" if f[5] else "normal", f="none",
                                 w=line_weight(_style(f).get("width")))
            if abs(a1 - a0) < 1e-6 or abs(abs(a1 - a0) - 360) < 1e-6:
                parts.append(f'<ellipse x="{_f(X)}" y="{_f(Y)}" width="{_f(2 * r * K)}" '
                             f'height="{_f(2 * r * K)}" style="{style}" antialias="true"/>')
            else:
                span = (a1 - a0) % 360 or 360
                parts.append(f'<arc x="{_f(X)}" y="{_f(Y)}" width="{_f(2 * r * K)}" '
                             f'height="{_f(2 * r * K)}" start="{_f(a0 % 360)}" angle="{_f(span)}" '
                             f'style="{style}" antialias="true"/>')
        elif f[0] == "P":
            pts = [P(*q) for q in f[1]]
            if f[2]:
                pts = pts[:-1]
            attr = " ".join(f'x{i + 1}="{_f(x)}" y{i + 1}="{_f(y)}"' for i, (x, y) in enumerate(pts))
            style = STYLE.format(s="dashed" if f[4] else "normal", f="black" if f[3] else "none",
                                 w=line_weight(_style(f).get("width")))
            parts.append(f'<polygon {attr} closed="{"true" if f[2] else "false"}" antialias="true" '
                         f'style="{style}"/>')
        elif f[0] == "T":
            (x, y), t, h, ang, align = f[1], f[2], f[3], f[4], f[5]
            pt, px, _, dy = _text_box(t, h, align)
            column = ((align or 7) - 1) % 3
            align_attr = ('', ' Halignment="AlignHCenter" anchor="alignment"',
                          ' Halignment="AlignRight" anchor="alignment"')[column]
            X, Y = P(x, y)
            r = math.radians(-math.degrees(ang) % 360)
            c, s_ = math.cos(r), math.sin(r)
            for k, line in enumerate(t.split("\n")):
                ly = dy + k * CELL * h * K           # line spacing = the cell height
                w = width_px(line, px)
                tx, ty = X - ly * s_, Y + ly * c     # base line point in the rotated text
                parts.append(f'<text text="{html.escape(line, quote=True)}" x="{_f(tx)}" '
                             f'y="{_f(ty)}" rotation="{_f(-math.degrees(ang) % 360)}"{align_attr} '
                             f'font="{font(pt)}" color="#000000"/>')
                lx0 = -w * (0, 0.5, 1)[column]
                for ex, ey in ((lx0, -px), (lx0 + w, 0.3 * px)):
                    xs.append(tx + ex * c - ey * s_)
                    ys.append(ty + ex * s_ + ey * c)
        elif f[0] == "D":
            # element information as a text: a QGraphicsTextItem, positioned
            # by its top left corner including the 4 px padding and rotated
            # about it, so that it stands where the fixed text would
            (x, y), t, h, ang, align, info_name = f[1], f[2], f[3], f[4], f[5], f[6]
            px, pt = font_px(h), font_pt(h)
            column = ((align or 7) - 1) % 3
            X, Y = P(x, y)
            w = width_px(t, px)
            lx = -w * (0, 0.5, 1)[column] - QET_MARGIN
            ly = baseline_px(h, align) - QET_MARGIN - px
            r = -ang
            c, s_ = math.cos(r), math.sin(r)
            tx, ty = X + lx * c - ly * s_, Y + lx * s_ + ly * c
            for ex, ey in ((QET_MARGIN, QET_MARGIN), (QET_MARGIN + w, QET_MARGIN + 1.3 * px)):
                xs.append(tx + ex * c - ey * s_)
                ys.append(ty + ex * s_ + ey * c)
            halign = ("AlignLeft", "AlignHCenter", "AlignRight")[column]
            u = uuid.uuid5(uuid.NAMESPACE_URL, f"{name}/info/{info_name}/{len(dyn)}")
            if info_name == "label":
                has_label = True
            dyn.append(f'<dynamic_text x="{_f(tx)}" y="{_f(ty)}" z="3" text_width="-1" '
                       f'Halignment="{halign}" Valignment="AlignTop" frame="false" '
                       f'rotation="{_f(-math.degrees(ang) % 360)}" keep_visual_rotation="false" '
                       f'text_from="ElementInfo" uuid="{{{u}}}" font="{font(pt)}">'
                       f'<text>{html.escape(t)}</text><info_name>{html.escape(info_name)}'
                       '</info_name></dynamic_text>')
    for i, (x, y, r) in enumerate(terminals):
        X, Y = P(x, y)
        u = uuid.uuid5(uuid.NAMESPACE_URL, f"{name}/{i}")
        nm = html.escape(names[i] if names and i < len(names) else str(i + 1), quote=True)
        parts.append(f'<terminal uuid="{{{u}}}" name="{nm}" x="{_f(X)}" y="{_f(Y)}" '
                     f'orientation="{ORIENTATION.get(r, "s")}" type="Generic"/>')
    x0, x1 = math.floor(min(xs) / 10) * 10 - 10, math.ceil(max(xs) / 10) * 10 + 10
    y0, y1 = math.floor(min(ys) / 10) * 10 - 10, math.ceil(max(ys) / 10) * 10 + 10
    u = uuid.uuid5(uuid.NAMESPACE_URL, name)
    lab = uuid.uuid5(uuid.NAMESPACE_URL, name + "/label")
    infos = ""
    if info:
        infos = ("    <elementInformations>" + "".join(
            f'<elementInformation name="{html.escape(k, quote=True)}" show="1">'
            f'{html.escape(str(v))}</elementInformation>' for k, v in info.items())
            + "</elementInformations>\n")
    label = "" if has_label else (
        f'\n        <dynamic_text x="{x0 + 2}" y="{y0 - 2}" z="3" text_width="-1" '
        'Halignment="AlignLeft" Valignment="AlignTop" frame="false" rotation="0" '
        'keep_visual_rotation="false" text_from="ElementInfo" '
        f'uuid="{{{lab}}}" font="{FONT},7,-1,5,50,0,0,0,0,0,Regular"><text></text>'
        '<info_name>label</info_name></dynamic_text>')
    return (f'<definition version="0.100.0" type="element" link_type="simple" '
            f'width="{x1 - x0}" height="{y1 - y0}" hotspot_x="{-x0}" hotspot_y="{-y0}">\n'
            f'    <uuid uuid="{{{u}}}"/>\n    <names>\n'
            f'        <name lang="en">{html.escape(name)}</name>\n    </names>\n' + infos
            + f'    <informations>{html.escape(description)}</informations>\n    <description>\n        '
            + "\n        ".join(parts + dyn) + label + '\n    </description>\n</definition>\n')


# --- folio graphics ---------------------------------------------------------

class Folio:
    """Converts mm (y upwards from the bottom of a sheet of height `height`)
    to scene px, the folio starting at (x0, y0) px."""

    def __init__(self, height, x0=0.0, y0=0.0):
        self.height, self.x0, self.y0 = height, x0, y0

    def px(self, x, y):
        return self.x0 + x * K, self.y0 + (self.height - y) * K


def _shape(kind, pts, dashed=False, closed=False, filled=False, color=None, width=None):
    """width: line width (mm) -> pen width in scene px; without one 1 px."""
    style = "DashLine" if dashed else "SolidLine"
    pen = (f'<pen style="{style}" color="{color or "#000000"}" '
           f'widthF="{_f(max(0.3, width * K)) if width else 1}"/>')
    brush = f'<brush style="{"SolidPattern" if filled else "NoBrush"}" color="{color or "#000000"}"/>'
    if kind == "Polygon":
        points = "".join(f'<point x="{_f(x)}" y="{_f(y)}"/>' for x, y in pts)
        return (f'<shape uuid="{{{uuid.uuid4()}}}" type="Polygon" is_movable="0" '
                f'closed="{int(closed)}" z="1">{pen}{brush}<points>{points}</points></shape>')
    (x1, y1), (x2, y2) = pts
    return (f'<shape uuid="{{{uuid.uuid4()}}}" type="{kind}" is_movable="0" closed="0" '
            f'x1="{_f(x1)}" y1="{_f(y1)}" x2="{_f(x2)}" y2="{_f(y2)}" z="1">{pen}{brush}</shape>')


def _arc_points(c, r, a0, a1, n=None):
    span = (a1 - a0) % (2 * math.pi) or 2 * math.pi
    n = n or max(6, int(span / (math.pi / 16)))
    return [(c[0] + r * math.cos(a0 + span * k / n), c[1] + r * math.sin(a0 + span * k / n))
            for k in range(n + 1)]


def _input(t, x, y, pt, ang=0.0, color=None):
    content = html.escape(t).replace("\n", "<br/>")
    if color:                              # <input> has no colour; QET reads the text as HTML
        content = f'<font color="{color}">{content}</font>'
    return (f'<input uuid="{{{uuid.uuid4()}}}" x="{_f(x)}" y="{_f(y)}" '
            f'rotation="{_f(-math.degrees(ang) % 360)}" font="{font(pt)}" '
            f'text="{html.escape(content, quote=True)}"/>')


def text_on_folio(folio, t, anchor, h_mm, ang, align, color=None):
    """A free text at an anchor (mm) with an alignment -> an <input> per
    line. The alignment is that of the first line; further lines follow
    1.5 x the text height further down. QET's text item has a 4 px padding
    and its base line at y + 4 + ascent."""
    px, pt = font_px(h_mm), font_pt(h_mm)
    lines = t.split("\n")
    column = ((align or 7) - 1) % 3
    ax, ay = folio.px(*anchor)
    ly0 = baseline_px(h_mm, align) - QET_MARGIN - px
    a = align or 7
    if a in (4, 5, 6, 7, 8, 9) and len(lines) > 1:
        ly0 -= (len(lines) - 1) * h_mm * CELL * K * (0.5 if a in (4, 5, 6) else 1.0)
    # a rotated text (angle counter-clockwise): the offset turned with it;
    # QET turns a text item about its top left corner
    r = -ang
    c, s_ = math.cos(r), math.sin(r)
    out = []
    for k, z in enumerate(lines):
        if not z.strip():
            continue
        lx = -width_px(z, px) * (0, 0.5, 1)[column] - QET_MARGIN
        ly = ly0 + k * h_mm * CELL * K
        out.append(_input(z, ax + lx * c - ly * s_, ay + lx * s_ + ly * c, pt, ang, color))
    return "".join(out)


def folio_xml(folio, shapes):
    """Shapes (mm, absolute on the sheet) -> (shapes, inputs) as XML."""
    out_shapes, inputs = [], []
    for f in shapes:
        if f[0] == "L":
            a, b = folio.px(*f[1]), folio.px(*f[2])
            if math.dist(a, b) > 0.2:
                out_shapes.append(_shape("Line", [a, b], f[3], color=_color(f, 4),
                                         width=_style(f).get("width")))
        elif f[0] == "A":
            c, r = f[1], f[2]
            if abs(f[4] - f[3]) < 1e-6:
                p1, p2 = folio.px(c[0] - r, c[1] + r), folio.px(c[0] + r, c[1] - r)
                out_shapes.append(_shape("Ellipse", [p1, p2], f[5], color=_color(f, 6),
                                         width=_style(f).get("width")))
            else:
                out_shapes.append(_shape("Polygon", [folio.px(*q) for q in _arc_points(c, r, f[3], f[4])],
                                         f[5], color=_color(f, 6), width=_style(f).get("width")))
        elif f[0] == "P":
            pts = [folio.px(*q) for q in f[1]]
            if f[2]:
                pts = pts[:-1]
            out_shapes.append(_shape("Polygon", pts, f[4], f[2], f[3], color=_color(f, 5),
                                     width=_style(f).get("width")))
        elif f[0] == "T":
            inputs.append(text_on_folio(folio, f[2], f[1], f[3], f[4], f[5], color=_color(f, 6)))
    return out_shapes, inputs


def image_size(data):
    """(width, height) of PNG, JPEG, GIF or BMP data, or None."""
    if data[:8] == b"\x89PNG\r\n\x1a\n" and len(data) >= 24:
        return struct.unpack(">II", data[16:24])
    if data[:6] in (b"GIF87a", b"GIF89a") and len(data) >= 10:
        return struct.unpack("<HH", data[6:10])
    if data[:2] == b"BM" and len(data) >= 26:
        w, h = struct.unpack("<ii", data[18:26])
        return abs(w), abs(h)
    if data[:2] == b"\xff\xd8":
        i = 2
        while i + 9 < len(data):
            if data[i] != 0xFF:
                i += 1
                continue
            marker = data[i + 1]
            if marker in (0xD8, 0x01) or 0xD0 <= marker <= 0xD7:
                i += 2
                continue
            length = struct.unpack(">H", data[i + 2:i + 4])[0]
            if 0xC0 <= marker <= 0xCF and marker not in (0xC4, 0xC8, 0xCC):
                h, w = struct.unpack(">HH", data[i + 5:i + 9])
                return w, h
            i += 2 + length
    return None


def image_xml(folio, data, x0, y0, x1, y1):
    """An image (PNG/JPEG ... bytes) fitted into the rectangle (mm) ->
    <image>, or None when its size cannot be read."""
    import base64
    size = image_size(data)
    if not size or not size[0] or not size[1]:
        return None
    w, h = size
    left, top = folio.px(min(x0, x1), max(y0, y1))
    right, bottom = folio.px(max(x0, x1), min(y0, y1))
    scale = min((right - left) / w, (bottom - top) / h)
    # QET: the position is the top left corner of the unscaled image, scaled
    # about its centre
    x = (left + right) / 2 - w / 2
    y = (top + bottom) / 2 - h / 2
    return (f'<image uuid="{{{uuid.uuid4()}}}" x="{_f(x)}" y="{_f(y)}" z="0" rotation="0" '
            f'size="{scale:.6f}" is_movable="false">{base64.b64encode(data).decode()}</image>')


EMPTY_TITLE_BLOCK = "WSCAD import (empty)"


def _empty_title_block():
    """A title block template with no content: the frame and title block of
    WSCAD are drawn as folio graphics."""
    return (f'<titleblocktemplate name="{EMPTY_TITLE_BLOCK}"><information>The frame and title '
            'block come from the WSCAD page</information><logos/><grid rows="1;" cols="r100%;">'
            '<field row="0" col="0" name="" displaylabel="false" align="left" valign="center" '
            'fontsize="6" hadjust="false"><value/><label/></field></grid></titleblocktemplate>')


def insert_into_project(path, per_folio, empty_title_block=True):
    """per_folio: {folio index: (shapes, inputs, images)} into the saved
    .qet; with empty_title_block every folio gets the empty template,
    embedded in the project."""
    with open(path, encoding="utf-8") as f:
        s = f.read()
    parts = re.split(r"(<diagram [^>]*>)", s)
    folio = -1
    for k, part in enumerate(parts):
        if part.startswith("<diagram "):
            folio += 1
            if empty_title_block:
                head = re.sub(r'\s(titleblocktemplate|titleblocktemplateCollection)="[^"]*"', "", part)
                parts[k] = head[:-1].rstrip("/") + (
                    f' titleblocktemplate="{EMPTY_TITLE_BLOCK}" titleblocktemplateCollection="embedded"'
                    + ("/>" if head.endswith("/>") else ">"))
            continue
        if folio < 0 or folio not in per_folio:
            continue
        shapes, inputs, images = per_folio[folio]
        end = part.find("</diagram>")
        head, rest = part[:end], part[end:]
        for tag, new in (("shapes", shapes), ("inputs", inputs), ("images", images)):
            if not new:
                continue
            # only the container of the folio (indent 8), not that of an element (16)
            m = re.search(r"\n {8}<" + tag + r"(/?)>", head)
            if m and m.group(1):
                head = head[:m.start()] + f"\n        <{tag}>" + "".join(new) + f"</{tag}>" + head[m.end():]
            elif m:
                head = head[:m.end()] + "".join(new) + head[m.end():]
            else:
                head += f"<{tag}>" + "".join(new) + f"</{tag}>"
        parts[k] = head + rest
    s = "".join(parts)
    if empty_title_block and f'name="{EMPTY_TITLE_BLOCK}"' not in s:
        tb = _empty_title_block()
        if re.search(r"<titleblocktemplates\s*/>", s):
            s = re.sub(r"<titleblocktemplates\s*/>", f"<titleblocktemplates>{tb}</titleblocktemplates>", s,
                       count=1)
        elif "<titleblocktemplates>" in s:
            s = s.replace("<titleblocktemplates>", "<titleblocktemplates>" + tb, 1)
        else:
            m = re.search(r"<project\b[^>]*>", s)
            if m:
                s = s[:m.end()] + f"\n    <titleblocktemplates>{tb}</titleblocktemplates>" + s[m.end():]
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(s)
