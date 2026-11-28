"""Reads a WSCAD project (ELECTRIX/SUITE 6.x and later).

A WSCAD project is a folder:
- Database/WSCAD6.db (SQLite): pages (Page), symbols with designator and
  position (Symbol), terminals (Pin), connections symbol/terminal ->
  symbol/terminal (Connection), potentials, cable cores, texts (String,
  Data): the logical view WSCAD keeps for its reports;
- a file per page, <structure>_<name>.<no>.ws<kind> (wsELD = schematic,
  wsFLD fluid, wsTEC terminal diagram ...).

Page file (FORMAT_WSCAD.md): 4 bytes file mark, 4 bytes version, 4 bytes
counter, 5 section entries of 19 bytes (used, index, kind, offset, length,
time). Kind 2 (page) and 5 (info) are packed: 4 bytes version, 4 bytes
length, key length, IV length, key, IV, 3 check bytes, then AES-CBC
(PKCS7) over GZip. Key and IV are stored in the section itself. Unpacked:
4 bytes version + UTF-8 XML <WS><Solution><Project><File>.

Unpacking needs one of the Python packages "cryptography" or
"pycryptodome" for AES; everything else is the standard library.
"""
import os
import pathlib
import sqlite3
import struct
import xml.etree.ElementTree as ET
import zlib

# the largest unpacked page section: a page of a real project is a few MB
MAX_UNPACKED = 256 * 1024 * 1024

SECTION_PAGE, SECTION_INFO = 2, 5


class FormatError(ValueError):
    pass


def _aes_cbc_decrypt(key, iv, data):
    try:
        from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
    except ImportError:
        try:
            from Crypto.Cipher import AES                # pycryptodome
        except ImportError:
            raise RuntimeError("reading WSCAD pages needs the Python package \"cryptography\" "
                               "(or \"pycryptodome\"): python -m pip install cryptography") from None
        raw = AES.new(key, AES.MODE_CBC, iv).decrypt(data)
    else:
        d = Cipher(algorithms.AES(key), modes.CBC(iv)).decryptor()
        raw = d.update(data) + d.finalize()
    n = raw[-1] if raw else 0
    if not 1 <= n <= 16 or raw[-n:] != bytes([n]) * n:
        raise FormatError("invalid padding (PKCS7)")
    return raw[:-n]


def unpack(section):
    """A packed section -> bytes (4 bytes version + content)."""
    if len(section) < 13:
        raise FormatError("packed section too short")
    kl, il = section[8], section[9]
    if kl not in (16, 24, 32) or il != 16:
        raise FormatError(f"unexpected key/IV length {kl}/{il}")
    key = section[10:10 + kl]
    iv = section[10 + kl:10 + kl + il]
    begin = 10 + kl + il + 3
    data = section[begin:]
    if not data or len(data) % 16:
        raise FormatError("encrypted data is not a whole number of blocks")
    try:
        unpacker = zlib.decompressobj(16 + zlib.MAX_WBITS)      # GZip
        out = unpacker.decompress(_aes_cbc_decrypt(key, iv, data), MAX_UNPACKED + 1)
    except zlib.error as e:
        raise FormatError(f"packed content is no GZip stream: {e}") from None
    if len(out) > MAX_UNPACKED or unpacker.unconsumed_tail:
        raise FormatError(f"packed content unpacks to more than {MAX_UNPACKED // (1024 * 1024)} MB")
    if not unpacker.eof:
        raise FormatError("packed content is no complete GZip stream")
    return out


def sections(path):
    """{kind: bytes} of the used sections of a page file, the packed ones
    unpacked already."""
    with open(path, "rb") as f:
        d = f.read()
    out = {}
    for i in range(5):
        e = d[12 + 19 * i:12 + 19 * (i + 1)]
        if len(e) < 19 or not e[0]:
            continue
        kind = e[2]
        offset, length = struct.unpack("<II", e[3:11])
        if offset + length > len(d):
            raise FormatError(f"section {kind} runs past the end of {os.path.basename(path)}")
        raw = d[offset:offset + length]
        if kind in (SECTION_PAGE, SECTION_INFO):
            raw = unpack(raw)
        out[kind] = raw
    return out


def page_xml(path):
    """The <File> element of the page (Header, Template, SL symbols, CN
    connections, free graphics)."""
    s = sections(path)
    if SECTION_PAGE not in s:
        raise FormatError(f"{os.path.basename(path)} has no page section")
    return ET.fromstring(s[SECTION_PAGE][4:]).find(".//File")


def info_xml(path):
    s = sections(path)
    return ET.fromstring(s[SECTION_INFO][4:]).find(".//File") if SECTION_INFO in s else None


def text_of(p, language="DE"):
    """The text of a multi-language property <P><T id="DE">...</T></P>: the
    language asked for, else German (the language WSCAD projects are most
    often made in), else the first one; a plain property gives its val."""
    if p is None:
        return ""
    ts = {t.get("id"): (t.text or "") for t in p.findall("T")}
    if not ts:
        return p.get("val", "") or ""
    return ts.get(language) or ts.get("DE") or next(iter(ts.values()))


class Project:
    """A project folder: the database and the page files."""

    def __init__(self, folder):
        self.folder = folder
        db = os.path.join(folder, "Database", "WSCAD6.db")
        if not os.path.exists(db):
            raise FileNotFoundError(f"no WSCAD project (Database/WSCAD6.db is missing): {folder}")
        # as_uri() encodes the path: a #, ? or % in a folder name stays part of the path
        self.db = sqlite3.connect(pathlib.Path(os.path.abspath(db)).as_uri() + "?mode=ro", uri=True)
        files = {n.upper(): n for n in os.listdir(folder)}
        self.pages = []
        for pid, name, kind, ref, w, h, asp_f, asp_l in self.db.execute(
                "SELECT PageID, FileName, FileType, Ref, Width, Height, AspF, AspL FROM Page"):
            found = files.get((name or "").upper())
            self.pages.append({"id": pid, "file": os.path.join(folder, found) if found else None,
                               "name": name, "kind": kind, "ref": ref, "width": w, "height": h,
                               "plant": asp_f or "", "location": asp_l or ""})
        # order: page groups (structure + page kind) in the order of their
        # first PageID (as WSCAD made them, the order of its PDF export),
        # within a group by page number
        first = {}
        for s in sorted(self.pages, key=lambda s: s["id"]):
            first.setdefault(_page_order(s["name"])[0], s["id"])
        self.pages.sort(key=lambda s: (first[_page_order(s["name"])[0]], _page_order(s["name"])[1]))

    def close(self):
        self.db.close()

    def schematic_pages(self):
        return [s for s in self.pages if (s["kind"] or "").lower() == "wseld" and s["file"]]

    def symbols(self):
        """SymbolID -> row (designator, PageID, x, y, ItemType)."""
        return {r[0]: {"id": r[0], "parent": r[1], "type": r[2], "tag": r[3], "page": r[4],
                       "x": r[5], "y": r[6]}
                for r in self.db.execute(
                    "SELECT SymbolID, ParentID, ItemType, BMK, PageID, x, y FROM Symbol")}

    def terminals(self):
        """(SymbolID, PinID) -> (name, number, x, y) relative to the symbol."""
        return {(r[0], r[1]): (r[2] or "", r[3] or "", r[4], r[5])
                for r in self.db.execute("SELECT SymbolID, PinID, Name, Number, x, y FROM Pin")}

    def connections(self):
        columns = ("SymbolID1", "Pin1", "SymbolID2", "Pin2", "PageID", "Colorcode", "Dimension",
                   "Potential", "WireName", "WireType", "CableID")
        return [dict(zip(columns, r))
                for r in self.db.execute(f"SELECT {', '.join(columns)} FROM Connection")]

    def articles(self):
        """SymbolID -> article number and manufacturer from the data table
        (type 57 = article, 63 = manufacturer)."""
        out = {}
        for sid, kind, value in self.db.execute(
                "SELECT ID, Type, Data FROM Data WHERE Type IN (57, 63)"):
            out.setdefault(sid, {})[{57: "article", 63: "manufacturer"}[kind]] = value
        return out


def _page_order(name):
    """=A1+ST_PLAN.0003.WSELD -> (=A1+ST_PLAN, 3): groups as in the file
    name, by number within them."""
    stem, _, rest = (name or "").rpartition(".")
    base, _, nr = stem.rpartition(".")
    return (base.upper(), int(nr) if nr.isdigit() else 0)
