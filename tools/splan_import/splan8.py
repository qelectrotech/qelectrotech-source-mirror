"""Reader for sPlan 8 schematics (Abacom, extension .spl8).

The format is not published. Every rule here was worked out from the file
contents, see FORMAT_SPL8.md (the section numbers are given at the
functions). The file is an uncompressed Delphi stream (little endian):
numbers int32/float32, strings as int32 character count + UTF-16LE, font
names as int32 + Pascal short string. Coordinates in 0.1 mm, origin top
left, y downwards.

    from splan8 import read
    project = read("schematic.spl8")
    for sheet in project.sheets:
        for o in sheet.objects: ...

Every object is a dict with at least "kind" and "pts" (list of (x, y) in
0.1 mm).
"""
import math
import struct
import zlib

MAGIC = b"\x07SPLAN80"

# object kinds (type byte, section 3)
KINDS = {1: "ellipse", 2: "rectangle", 3: "text", 4: "line", 5: "polygon", 6: "group",
         7: "part", 8: "dot", 9: "image", 10: "multiline_text", 11: "contact", 12: "bezier"}


class FormatError(ValueError):
    pass


class _Reader:
    def __init__(self, b, p=0):
        self.b, self.p = b, p

    def u8(self):
        if self.p >= len(self.b):
            raise FormatError(f"end of file at 0x{self.p:x}")
        v = self.b[self.p]
        self.p += 1
        return v

    def i32(self):
        try:
            v = struct.unpack_from("<i", self.b, self.p)[0]
        except struct.error:
            raise FormatError(f"end of file at 0x{self.p:x}") from None
        self.p += 4
        return v

    def f32(self):
        try:
            v = struct.unpack_from("<f", self.b, self.p)[0]
        except struct.error:
            raise FormatError(f"end of file at 0x{self.p:x}") from None
        self.p += 4
        return v

    def ws(self):
        n = self.i32()
        if n < 0 or n > 200000:
            raise FormatError(f"string length {n} at 0x{self.p - 4:x}")
        v = self.raw(2 * n).decode("utf-16le", "replace")
        return v

    def raw(self, n):
        v = self.b[self.p:self.p + n]
        if len(v) < n:
            raise FormatError(f"end of file at 0x{self.p:x}")
        self.p += n
        return v

    def points(self, n):
        return [(self.f32(), self.f32()) for _ in range(n)]


def _header(r):
    """Section 4: object header after the type byte (23 bytes): stroke (3),
    11 x 0, fill colour, line colour, flag."""
    h = r.raw(3)
    r.raw(11)
    fill = r.raw(4)
    color = r.raw(4)
    flag = r.u8()
    return {"stroke": h[1], "width": h[2], "fill": fill.hex(), "color": color.hex(), "flag": flag}


def _text_body(r):
    """Section 5, text: header, frame of 4 corners, text, font, attributes,
    GUID."""
    o = _header(r)
    o["pts"] = r.points(4)
    o["text"] = r.ws()
    r.i32()
    n = r.u8()
    o["font"] = r.raw(n).decode("latin1")
    attr = r.raw(14)
    o["font_style"] = attr[5]
    o["link"] = r.ws()                                # active link (URL/file), else empty
    o["guid"] = r.raw(16).hex()
    r.raw(17)
    r.raw(5)
    p = o["pts"]
    o["rotation"] = round(math.degrees(math.atan2(p[1][1] - p[0][1], p[1][0] - p[0][0])))
    # font height = frame height across the reading direction (0.1 mm)
    o["height"] = math.hypot(p[3][0] - p[0][0], p[3][1] - p[0][1])
    return o


def _text_with_value(r):
    """Section 5, free text: when it holds a variable (<PAGENO>,
    <PARENT_ID> ...), the evaluated text follows."""
    o = _text_body(r)
    o["template"] = o["text"]
    if "<" in o["text"]:
        o["text"] = r.ws()
    return o


def _object(r, depth=0):
    start, t = r.p, r.u8()
    kind = KINDS.get(t)
    if kind is None:
        raise FormatError(f"unknown object type 0x{t:02x} at 0x{start:x}")
    if depth > 64:
        raise FormatError(f"groups nested too deep at 0x{start:x}")
    if t in (4, 12):                                  # section 6: polyline / Bezier
        o = _header(r)
        n = r.i32()
        o["pts"] = r.points(n + 1)
        tail = r.raw(21)
        o["arrow_start"], o["arrow_end"] = tail[5], tail[6]
    elif t == 5:                                      # section 7: polygon (closed)
        o = _header(r)
        n = r.i32()
        o["pts"] = r.points(n + 1)
        r.raw(27)
    elif t == 1:                                      # section 7: ellipse/circle as points
        o = _header(r)
        n = r.i32()
        o["pts"] = r.points(n + 1)
        r.raw(47)
    elif t == 2:                                      # section 7: rectangle, 4 corners + closing point
        o = _header(r)
        o["pts"] = r.points(5)
        r.raw(31)
    elif t == 3:
        o = _text_with_value(r)
    elif t == 11:                                     # section 5: contact = text + contact name
        o = _text_body(r)
        o["name"] = r.ws()
    elif t == 8:                                      # section 8: junction dot
        o = _header(r)
        o["pts"] = r.points(1)
    elif t == 9:                                      # section 9: image, frame + zlib(BMP)
        o = _header(r)
        o["pts"] = r.points(4)
        d = zlib.decompressobj()
        rest = r.b[r.p:]
        try:
            data = d.decompress(rest)
        except zlib.error as e:
            raise FormatError(f"image at 0x{start:x}: {e}") from None
        r.p += len(rest) - len(d.unused_data)
        o["bmp"] = data[4:]
        r.raw(4)
    elif t == 10:                                     # section 5: multi-line text
        o = _header(r)
        o["pts"] = r.points(4)
        r.u8()
        r.i32()
        n = r.u8()
        o["font"] = r.raw(n).decode("latin1")
        r.raw(9)
        o["lines"] = [r.ws() for _ in range(r.i32())]
        o["text"] = "\n".join(o["lines"])
        r.raw(7)
        p = o["pts"]
        o["rotation"] = round(math.degrees(math.atan2(p[1][1] - p[0][1], p[1][0] - p[0][0])))
        o["height"] = math.hypot(p[3][0] - p[0][0], p[3][1] - p[0][1]) / max(1, len(o["lines"]))
    else:                                             # section 10: group (6) / part (7)
        n = r.i32()
        o = {"children": [_object(r, depth + 1) for _ in range(n + 1)]}
        if t == 7:
            o["tag_text"] = _text_body(r)
            o["tag"] = r.ws()
            o["value_text"] = _text_body(r)
            o["value"] = o["value_text"]["text"]
            if "<" in o["value"]:                     # a value with a variable: the evaluated text follows
                o["value"] = r.ws()
            o["switches"] = r.raw(4).hex()
            o["symbol"] = r.ws()                      # name of the library part
            o["tag_template"] = r.ws()
            o["number"] = r.i32()
            r.f32()
            r.f32()
            r.i32()
            r.u8()
            o["extra"] = [r.ws() for _ in range(4)]
            o["is_parent"] = r.u8()
            o["id"] = r.i32() & 0xFFFFFFFF
            o["parent"] = r.i32() & 0xFFFFFFFF
            r.u8()
    o["kind"], o["type"], o["offset"] = kind, t, start
    o.setdefault("pts", [])
    return o


class Sheet:
    def __init__(self, name, width_mm, height_mm, objects):
        self.name, self.width_mm, self.height_mm, self.objects = name, width_mm, height_mm, objects


class Project:
    def __init__(self, header, sheets):
        self.header, self.sheets = header, sheets


def _next_sheet(b, p, limit=600):
    """Section 11: skips the sheet footer (grid, print and view settings, of
    a length that differs between versions): the next sheet header starts
    with its name (int32 + UTF-16) and the sheet size in mm (int32, int32)."""
    for q in range(p, min(len(b) - 12, p + limit)):
        n = struct.unpack_from("<i", b, q)[0]
        if not 0 < n <= 120:
            continue
        e = q + 4 + 2 * n
        try:
            name = b[q + 4:e].decode("utf-16le")
        except UnicodeDecodeError:
            continue
        if not all(c.isprintable() for c in name) or e + 8 > len(b):
            continue
        w, h = struct.unpack_from("<ii", b, e)
        if 50 <= w <= 3000 and 50 <= h <= 3000:
            return q
    return None


def walk(o):
    """The object and all its children (groups, parts), in order."""
    yield o
    for k in o.get("children", []):
        yield from walk(k)


def read(path_or_bytes):
    if isinstance(path_or_bytes, (bytes, bytearray)):
        b = bytes(path_or_bytes)
    else:
        with open(path_or_bytes, "rb") as f:
            b = f.read()
    if b[:8] != MAGIC:
        raise FormatError("not an sPlan 8 file (the mark \\x07SPLAN80 is missing)")
    r = _Reader(b, 16)                                # section 2: file header, mark + 8 bytes version
    header = {"u0": r.u8(), "s1": r.ws(), "s2": r.ws(), "i1": r.i32(), "i2": r.i32()}
    sheets = []
    while True:                                       # section 3: sheets
        name = r.ws()
        w, h = r.i32(), r.i32()
        r.i32(), r.i32(), r.i32(), r.f32()            # grid, scroll x/y, zoom
        n = r.i32()
        if n < -1 or n > 1000000:
            raise FormatError(f"object count {n + 1} of sheet {name!r}")
        objects = [_object(r) for _ in range(n + 1)]
        sheets.append(Sheet(name, w, h, objects))
        q = _next_sheet(b, r.p)
        if q is None:
            break
        r.p = q
    return Project(header, sheets)


if __name__ == "__main__":
    import collections
    import sys
    pr = read(sys.argv[1])
    for i, sh in enumerate(pr.sheets, 1):
        c = collections.Counter(o["kind"] for o in sh.objects)
        print(i, sh.name, f"{sh.width_mm}x{sh.height_mm}", dict(c))
