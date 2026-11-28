"""EPLAN export archive (.epdz) -> editable QElectroTech project (parts, conductors, report arrows).

    python epdz_nach_qet.py <export.epdz> [target.qet] [--unpacked DIR] [--pdf] [--no-collection]
                            [--stop-on-eof]
    python epdz_nach_qet.py --check

File > Import EPLAN project in QElectroTech runs this program: it unpacks the archive itself and passes the
folder with --unpacked. Run on its own, the program unpacks the archive with 7-Zip (qet_backend.seven_zip).
--check only says whether everything the import needs is there (exit code 0).
Output: one line "PROGRESS <percent> <step>" per step and "done: <project>" at the end.

What an .epdz holds (an EPLAN export for EPLAN eVIEW/eBUILD, a 7-Zip archive):
- manifest.db (SQLite): packages (pages, functions, merged connections, interruption points, ...) with their
  EPLAN properties (table "property": propid, value); property 2000 is the object identifier
  "253/<class>/<number>/0" (17 function, 59 connection definition point, 70 interruption point);
- one SVG per page (packages/pages/items/pagesvg). Every function is a group id="Id<class>_<number>" with
  <title> = its full device tag; its graphics lie on the page turned and placed already (mm, y up).
  Connections are free lines outside any group; frame and title block are in the group described "Frame".

Translation (EPLAN property numbers as in the EPLAN help):
- terminal = end point of the symbol graphics on which a connection line ends (EPLAN draws connections from
  connection point to connection point). Name: 20038 connection point designations ("A1;A2", "1/L1;2/T1")
  or 20030 terminal number; matched by the connection point text of the group next to the point, else by the
  connection list (31019 source, 31020 target: "TAG:terminal"), else in reading order.
- connection points of a device ("TAG:1", a group inside or next to the device's group) belong to the
  device's part.
- conductors: the connections of the list whose two ends lie in the same drawn net; parts of a net the list
  does not join are joined at their nearest terminals (the net stays as EPLAN draws it). Colour 31004,
  section 31007, potential 33003 on the conductor. Each conductor follows the corners of the EPLAN line.
- interruption points (Id70) become linked QElectroTech report arrows; the partner comes from the jump
  target of the group.
- coil / protective device (20188 category 200, 5xx) with the contacts (201 NO, 202 NC, 203/204) of the same
  device tag (20006) as master/slave; EPLAN's cross reference texts stay, QElectroTech's own are off.
  The same for the report arrows: EPLAN's page text beside an arrow already names its target, so every
  arrow's own label is left empty (<report label="">). EPLAN's frame also brings its own column and row
  numbering, so QElectroTech's column and row headers are switched off on every folio.
- article 20100 ("abbreviation.order number"), order number 20919, designation 20193, function text 20031,
  technical characteristics 20027 as element informations.
Symbols without terminal and without article (structure and location boxes), connection definition points
(Id59) and the other page graphics become folio graphics. Pictures are taken only from inside the
archive. Beside the project: <target>_devices.csv, <target>_connections.csv, <target>_3D (the .e3d
installation spaces) and <target>_import_report.txt (what was translated); like the project itself they
are written anew on every import, replacing what was there. The working folder _epdz_import_<name> is
removed at the end.
--stop-on-eof (the import dialog passes it): when standard input closes, stop the QElectroTech runs this
program started and exit with code 3 -- the dialog's Cancel.

Note: the identifiers inside this module are German (it was written for a fork whose working language is
German); comments, messages and output are English.
"""
import collections
import csv
import hashlib
import heapq
import math
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import threading
import xml.etree.ElementTree as ET

HIER = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HIER)
import eplan_grafik as G                                       # noqa: E402
import qet_backend                                             # noqa: E402
from qet_plan import Plan                                      # noqa: E402

SV = "{http://www.w3.org/2000/svg}"
XL = "{http://www.w3.org/1999/xlink}"
ORDNER = "EPLAN_import/EPDZ"                  # folder of the generated parts in the collection
TOL = 0.02                                    # mm: points closer than this are the same point
# SVG font size (em) -> EPLAN text height: in the export 2.5 mm -> 2.2356, 1.8 mm -> 1.6146
SCHRIFT_ZU_HOEHE = 2.5 / 2.2356
# function definition categories without master/slave role (terminals, cables, PLC, connection points, ...)
OHNE_ROLLE = {100, 101, 300, 301, 400, 401, 8, 1302, 15}
# EPLAN manufacturer abbreviation (part of the article number 20100) -> manufacturer
HERSTELLER = {"SIE": "Siemens", "WEI": "Weidmüller", "RIT": "Rittal", "PXC": "Phoenix Contact",
              "MURR": "Murrelektronik", "HAR": "HARTING", "HELU": "Helukabel", "IFM": "ifm", "FIN": "Finder",
              "K&N": "Kraus & Naimer", "MCT": "METZ CONNECT", "LAPP": "Lapp", "SICK": "SICK",
              "BOS": "Bosch Rexroth", "SEW": "SEW-EURODRIVE", "WAG": "WAGO", "SCHM": "Schmersal"}
HERKUNFT = "Made from an EPLAN export archive (.epdz) by epdz_nach_qet.py"


def _fortschritt(prozent, schritt):
    print(f"PROGRESS {int(prozent)} {schritt}", flush=True)


def _px(p):
    return round(G.X0 + p[0] * G.K, 1), round((G.H_MM - p[1]) * G.K, 1)


def _schluessel(p):
    return (round(p[0] / TOL), round(p[1] / TOL))


# --- archive and database ------------------------------------------------------------------------------------

def auspacken(epdz, arbeit):
    """Unpack @p epdz into <arbeit>/x with 7-Zip."""
    sieben = qet_backend.seven_zip()
    if not sieben:
        raise RuntimeError("no 7-Zip found to unpack the archive: set SEVENZIP, or unpack it and pass --unpacked")
    ziel = os.path.join(arbeit, "x")
    if os.path.isdir(ziel):
        shutil.rmtree(ziel)
    os.makedirs(ziel)
    subprocess.run([sieben, "x", "-y", f"-o{ziel}", epdz], check=True, stdout=subprocess.DEVNULL)
    return ziel


class Daten:
    """manifest.db: the properties of each package, functions by object identifier, pages, connections."""

    def __init__(self, db):
        c = sqlite3.connect(f"file:{db}?mode=ro", uri=True)
        try:
            self.props = collections.defaultdict(dict)
            for pid, propid, _, wert in c.execute("SELECT packageid, propid, propindex, value FROM property "
                                                  "WHERE propid IS NOT NULL ORDER BY packageid, propid, propindex"):
                if wert not in (None, "") and propid not in self.props[pid]:
                    self.props[pid][propid] = wert
            # object identifier "253/17/28300/0" -> package
            self.objekt = {}
            for pid, p in self.props.items():
                o = p.get(2000)
                if o and o.count("/") >= 2:
                    k = o.split("/")
                    self.objekt[f"{k[1]}_{k[2]}"] = pid
            # 1140 plant, 1240 location, 1540 document type, 11000 page counter, 11011 page description
            self.seiten = []
            for pid, name in c.execute("SELECT packageid, name FROM page_package"):
                p = self.props[pid]
                self.seiten.append({"id": pid, "name": name, "anlage": p.get(1140, ""), "ort": p.get(1240, ""),
                                    "doku": p.get(1540, ""), "nr": p.get(11000, ""), "titel": p.get(11011, "")})
            self.seiten.sort(key=lambda s: (s["anlage"], s["ort"], s["doku"],
                                            int(s["nr"]) if s["nr"].isdigit() else 10 ** 6, s["nr"]))
            self.verbindungen = []
            for (pid,) in c.execute("SELECT packageid FROM mergedconnection_package"):
                p = self.props[pid]
                if p.get(31019) and p.get(31020):
                    self.verbindungen.append({"von": p[31019].strip(), "nach": p[31020].strip(),
                                              "farbe": p.get(31004, ""), "querschnitt": p.get(31007, ""),
                                              "potential": p.get(33003, "") or p.get(20045, ""),
                                              "laenge": p.get(31003, "")})
            self.funktionen = [pid for (pid,) in c.execute("SELECT packageid FROM function_package")]
        finally:
            c.close()

    def funktion(self, oid):
        pid = self.objekt.get(oid)
        return self.props.get(pid, {}) if pid else {}


def kategorie(p):
    """Category of the function definition (20188 "category/..."), 0 when there is none."""
    try:
        return int((p.get(20188) or "0").split("/")[0])
    except ValueError:
        return 0


# --- reading the SVG ---------------------------------------------------------------------------------------------

def _css(root):
    """The classes of the page (each SVG numbers its own): P... lines, F... fonts."""
    st = {}
    for s in root.iter(SV + "style"):
        for m in re.finditer(r"(?:text)?\.([A-Za-z]+\d+)\{([^}]*)\}", s.text or ""):
            st[m.group(1)] = {k.strip(): v.strip() for k, _, v in
                              (kv.partition(":") for kv in m.group(2).split(";")) if k.strip()}
    return st


def _linienstil(st):
    try:
        breite = float(st.get("stroke-width"))
    except (TypeError, ValueError):
        breite = None
    return {"dashed": st.get("stroke-dasharray", "none") not in ("none", ""),
            "filled": st.get("fill", "none") not in ("none", ""), "width": breite}


def _bogen(p0, rx, ry, phi, gross, dreh, p1):
    """SVG arc (endpoint form, SVG 1.1 appendix F.6.5) -> points without the start point."""
    if rx == 0 or ry == 0 or p0 == p1:
        return [p1]
    phi = math.radians(phi)
    c, s = math.cos(phi), math.sin(phi)
    dx, dy = (p0[0] - p1[0]) / 2, (p0[1] - p1[1]) / 2
    x1, y1 = c * dx + s * dy, -s * dx + c * dy
    rx, ry = abs(rx), abs(ry)
    lam = x1 * x1 / (rx * rx) + y1 * y1 / (ry * ry)
    if lam > 1:
        rx, ry = rx * math.sqrt(lam), ry * math.sqrt(lam)
    zaehler = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1
    nenner = rx * rx * y1 * y1 + ry * ry * x1 * x1
    f = math.sqrt(max(0.0, zaehler / nenner)) if nenner else 0.0
    if gross == dreh:
        f = -f
    cx1, cy1 = f * rx * y1 / ry, -f * ry * x1 / rx
    cx, cy = c * cx1 - s * cy1 + (p0[0] + p1[0]) / 2, s * cx1 + c * cy1 + (p0[1] + p1[1]) / 2

    def winkel(ux, uy, vx, vy):
        return math.atan2(ux * vy - uy * vx, ux * vx + uy * vy)
    t1 = winkel(1, 0, (x1 - cx1) / rx, (y1 - cy1) / ry)
    dt = winkel((x1 - cx1) / rx, (y1 - cy1) / ry, (-x1 - cx1) / rx, (-y1 - cy1) / ry)
    if not dreh and dt > 0:
        dt -= 2 * math.pi
    elif dreh and dt < 0:
        dt += 2 * math.pi
    n = max(4, int(abs(dt) / (math.pi / 24)) + 1)
    aus = []
    for k in range(1, n + 1):
        t = t1 + dt * k / n
        aus.append((cx + rx * math.cos(t) * c - ry * math.sin(t) * s, cy + rx * math.cos(t) * s + ry * math.sin(t) * c))
    aus[-1] = p1
    return aus


def pfad(d):
    """SVG path -> [(points, closed)] (M, L, H, V, A, Z; absolute commands, as the EPLAN export writes them)."""
    tok = re.findall(r"[MLHVAZmlhvaz]|-?\d*\.?\d+(?:[eE][-+]?\d+)?", d)
    zuege, cur, cmd, i = [], None, None, 0

    def zahl():
        nonlocal i
        v = float(tok[i])
        i += 1
        return v
    while i < len(tok):
        if tok[i].isalpha():
            cmd = tok[i].upper()
            i += 1
            if cmd == "Z":
                if zuege:
                    pts, _ = zuege[-1]
                    if pts and pts[-1] != pts[0]:
                        pts.append(pts[0])
                    zuege[-1] = (pts, True)
                    cur = pts[0] if pts else cur
                continue
        if cmd == "M":
            cur = (zahl(), zahl())
            zuege.append(([cur], False))
            cmd = "L"
        elif cmd == "L":
            cur = (zahl(), zahl())
            zuege[-1][0].append(cur)
        elif cmd == "H":
            cur = (zahl(), cur[1])
            zuege[-1][0].append(cur)
        elif cmd == "V":
            cur = (cur[0], zahl())
            zuege[-1][0].append(cur)
        elif cmd == "A":
            rx, ry, phi, gross, dreh, x, y = (zahl() for _ in range(7))
            zuege[-1][0].extend(_bogen(cur, rx, ry, phi, int(gross), int(dreh), (x, y)))
            cur = (x, y)
        else:
            i += 1
    return [(pts, zu) for pts, zu in zuege if len(pts) >= 2]


_TRANSL = re.compile(r"translate\(\s*([-\d.eE]+)[ ,]+([-\d.eE]+)\s*\)")
_ROT = re.compile(r"rotate\(\s*([-\d.eE]+)(?:[ ,]+([-\d.eE]+)[ ,]+([-\d.eE]+))?\s*\)")


def _textlage(tf):
    """transform of a text (rotate(a cx cy) translate(x y) scale(1,-1)) -> anchor (mm) and angle (radians,
    counter-clockwise). The anchor is the top left (dominant-baseline text-before-edge)."""
    m = _TRANSL.search(tf or "")
    x, y = (float(m.group(1)), float(m.group(2))) if m else (0.0, 0.0)
    r = _ROT.search(tf or "")
    ang = 0.0
    if r:
        a = math.radians(float(r.group(1)))
        cx, cy = (float(r.group(2)), float(r.group(3))) if r.group(2) else (0.0, 0.0)
        c, s = math.cos(a), math.sin(a)
        x, y = cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c
        ang = a
    return (x, y), ang


class Gruppe:
    """An EPLAN group of the page (function, connection definition point, interruption point)."""

    def __init__(self, ident, titel, eltern):
        self.id, self.titel, self.eltern = ident, titel or "", eltern
        self.klasse, self.oid = ident[2:].split("_")[0], ident[2:]
        self.linien, self.kreise, self.texte = [], [], []
        self.partner = None

    def punkte(self):
        for pts, _, _ in self.linien:
            yield from pts
        for (cx, cy), r, _ in self.kreise:
            yield (cx - r, cy - r)
            yield (cx + r, cy + r)
        for t in self.texte:
            yield t["lage"]


def seite_lesen(datei):
    """-> (groups, free graphics {linien, kreise, texte, bilder}, frame {...})."""
    root = ET.parse(datei).getroot()
    st = _css(root)
    gruppen = []
    frei = {"linien": [], "kreise": [], "texte": [], "bilder": []}
    rahmen = {"linien": [], "kreise": [], "texte": [], "bilder": []}

    def stil(e):
        return _linienstil(st.get(e.get("class") or "", {}))

    def ablage(owner, im_rahmen):
        if owner is not None:
            return {"linien": owner.linien, "kreise": owner.kreise, "texte": owner.texte, "bilder": frei["bilder"]}
        return rahmen if im_rahmen else frei

    def lauf(e, owner, im_rahmen):
        for ch in e:
            tag = ch.tag.replace(SV, "")
            if tag == "g":
                ident = ch.get("id") or ""
                d = ch.find(SV + "desc")
                rahmen_hier = im_rahmen or (d is not None and d.text == "Frame")
                if re.match(r"Id\d+_\d+$", ident):
                    t = ch.find(SV + "title")
                    g = Gruppe(ident, t.text if t is not None else "", owner)
                    gruppen.append(g)
                    lauf(ch, g, rahmen_hier)
                else:
                    lauf(ch, owner, rahmen_hier)
            elif tag == "a":
                if owner is not None:
                    m = re.findall(r"'(Id\d+_\d+)'", ch.get(XL + "href") or "")
                    if len(m) >= 2 and m[0] == owner.id:
                        owner.partner = m[1]
                lauf(ch, owner, im_rahmen)
            elif tag == "path":
                for pts, zu in pfad(ch.get("d") or ""):
                    ablage(owner, im_rahmen)["linien"].append((pts, zu, stil(ch)))
            elif tag == "circle":
                k = ((float(ch.get("cx")), float(ch.get("cy"))), float(ch.get("r")), stil(ch))
                ablage(owner, im_rahmen)["kreise"].append(k)
            elif tag == "text":
                text = "".join(ch.itertext())
                if not text.strip():
                    continue
                lage, ang = _textlage(ch.get("transform"))
                try:
                    h = float(st.get(ch.get("class") or "", {}).get("font-size", "2.2356").rstrip("px"))
                except ValueError:
                    h = 2.2356
                ablage(owner, im_rahmen)["texte"].append(
                    {"text": text, "lage": lage, "winkel": ang, "hoehe": round(h * SCHRIFT_ZU_HOEHE * 20) / 20})
            elif tag == "image":
                x, y = float(ch.get("x", 0)), float(ch.get("y", 0))
                b = (ch.get(XL + "href") or "", x, y, x + float(ch.get("width", 0)), y + float(ch.get("height", 0)))
                (rahmen if im_rahmen else frei)["bilder"].append(b)
            elif tag not in ("desc", "title", "defs", "style", "script"):
                lauf(ch, owner, im_rahmen)
    lauf(root, None, False)
    return gruppen, frei, rahmen


# --- primitives (see eplan_grafik) -----------------------------------------------------------------------------

def _form_linie(pts, zu, st, d=(0.0, 0.0)):
    pts = [(x - d[0], y - d[1]) for x, y in pts]
    s = {"width": st["width"]}
    if len(pts) == 2 and not zu:
        return ("L", pts[0], pts[1], st["dashed"], None, s)
    return ("P", pts, zu, st["filled"] and zu, st["dashed"], None, s)


def _form_kreis(c, r, st, d=(0.0, 0.0)):
    return ("A", (c[0] - d[0], c[1] - d[1]), r, 0.0, 0.0, st["dashed"], None, {"width": st["width"]})


def _form_text(t, d=(0.0, 0.0), info=None):
    x, y = t["lage"]
    if info:
        return ("D", (x - d[0], y - d[1]), t["text"], t["hoehe"], t["winkel"], 1, info)
    return ("T", (x - d[0], y - d[1]), t["text"], t["hoehe"], t["winkel"], 1, None, {})


def _blattgrafik(linien, kreise):
    return [_form_linie(*l_) for l_ in linien] + [_form_kreis(*k) for k in kreise]


def _blatttexte(texte):
    return [G.text_on_folio(t["text"], t["lage"], t["hoehe"], t["winkel"], 1) for t in texte]


# --- nets --------------------------------------------------------------------------------------------------------

class Vereinigung:
    """Union-find."""

    def __init__(self):
        self.p = {}

    def finde(self, a):
        self.p.setdefault(a, a)
        while self.p[a] != a:
            self.p[a] = self.p[self.p[a]]
            a = self.p[a]
        return a

    def vereine(self, a, b):
        a, b = self.finde(a), self.finde(b)
        if a != b:
            self.p[b] = a


def _auf_strecke(p, a, b):
    """Whether p lies inside the segment a-b."""
    ab = (b[0] - a[0], b[1] - a[1])
    lang = math.hypot(*ab)
    if lang < TOL:
        return False
    t = ((p[0] - a[0]) * ab[0] + (p[1] - a[1]) * ab[1]) / (lang * lang)
    if t <= 1e-6 or t >= 1 - 1e-6:
        return False
    return math.dist(p, (a[0] + t * ab[0], a[1] + t * ab[1])) <= TOL


def netze(linien):
    """Free polylines -> union over point keys: shared points, and end points on a segment (T junction),
    join. Crossings without a shared point do not (as in EPLAN)."""
    v = Vereinigung()
    strecken = []
    for n, (pts, _, _) in enumerate(linien):
        v.finde(_schluessel(pts[0]))
        for a, b in zip(pts, pts[1:]):
            v.vereine(_schluessel(a), _schluessel(b))
            strecken.append((a, b, n))
    for n, (pts, _, _) in enumerate(linien):
        for p in (pts[0], pts[-1]):
            for a, b, m in strecken:
                if m != n and _auf_strecke(p, a, b):
                    v.vereine(_schluessel(p), _schluessel(a))
    return v


# --- helpers -----------------------------------------------------------------------------------------------------

def _ohne_vorsatz(wert, zeichen):
    return (wert or "").lstrip(zeichen)


def _richtung(p, linien_an):
    """Direction of the conductor away from the terminal: 1 up, 2 right, 4 down, 8 left (eplan_grafik.DIRECTION)."""
    for a, b in linien_an:
        q = b if math.dist(a, p) <= TOL else a
        dx, dy = q[0] - p[0], q[1] - p[1]
        if abs(dx) < TOL and abs(dy) < TOL:
            continue
        if abs(dy) >= abs(dx):
            return 1 if dy > 0 else 4
        return 2 if dx > 0 else 8
    return 4


def _waagerecht_senkrecht(a, b):
    return abs(a[0] - b[0]) <= TOL or abs(a[1] - b[1]) <= TOL


def ecken(pts):
    """Drawn conductor path -> the corners between its ends, for a QElectroTech conductor. EPLAN draws a branch
    as a short slant ((96,70)-(98,72) between a vertical and a horizontal line); QElectroTech conductors run
    only horizontally and vertically, so the slant becomes a corner where its neighbours meet."""
    P = [pts[0]]
    for p in pts[1:]:
        if math.dist(p, P[-1]) > TOL:
            P.append(p)
    i = 0
    while i < len(P) - 1:
        a, b = P[i], P[i + 1]
        if _waagerecht_senkrecht(a, b):
            i += 1
            continue
        vor = P[i - 1] if i > 0 else None
        nach = P[i + 2] if i + 2 < len(P) else None
        if vor is not None and abs(vor[0] - a[0]) <= TOL:          # vertical before
            c = (a[0], b[1])
        elif vor is not None and abs(vor[1] - a[1]) <= TOL:        # horizontal before
            c = (b[0], a[1])
        elif nach is not None and abs(nach[0] - b[0]) <= TOL:      # vertical after
            c = (b[0], a[1])
        else:
            c = (a[0], b[1])
        # the slant between a vertical and a horizontal line becomes the corner where they meet; anywhere else
        # (at a terminal, or a jog between two parallel lines) the corner is added and both ends stay
        quer = vor is not None and nach is not None and (
            (abs(vor[0] - a[0]) <= TOL and abs(nach[1] - b[1]) <= TOL) or
            (abs(vor[1] - a[1]) <= TOL and abs(nach[0] - b[0]) <= TOL))
        if quer:
            P[i:i + 2] = [c]
        else:
            P.insert(i + 1, c)
        i += 1
    Q = [P[0]]
    for k in range(1, len(P) - 1):
        a, b, c = Q[-1], P[k], P[k + 1]
        if (abs(a[0] - b[0]) <= TOL and abs(b[0] - c[0]) <= TOL) or (abs(a[1] - b[1]) <= TOL and abs(b[1] - c[1]) <= TOL):
            continue
        Q.append(b)
    Q.append(P[-1])
    return Q[1:-1]


def _suche(idx, ende):
    """Terminals for the connection end "TAG:terminal" or "TAG:plug:terminal". The export writes the plug
    designation only once per plug as a text; where it is missing at the terminal, "TAG:terminal" counts when
    it is unique."""
    if ende in idx:
        return idx[ende]
    teile = ende.split(":")
    if len(teile) >= 3:
        kurz = teile[0] + ":" + teile[-1]
        if len(idx.get(kurz, [])) == 1:
            return idx[kurz]
    return []


def _lesereihenfolge(pkte):
    return sorted(range(len(pkte)), key=lambda i: (-round(pkte[i][1], 1), round(pkte[i][0], 1)))


def _textmitte(t):
    x, y = t["lage"]
    w, h = 0.6 * t["hoehe"] * len(t["text"].strip()), t["hoehe"]
    c, s = math.cos(t["winkel"]), math.sin(t["winkel"])
    dx, dy = w / 2, -h / 2
    return (x + dx * c - dy * s, y + dx * s + dy * c)


def _dateiname(text):
    return re.sub(r"[^A-Za-z0-9_+-]", "_", text)[:40].strip("_") or "Symbol"


def _anschluss_index(t, pos, ein):
    """Index (QElectroTech's order) of the terminal at point pos (mm) of a part with insertion point ein (mm)."""
    rx, ry = (pos[0] - ein[0]) * G.K, -(pos[1] - ein[1]) * G.K
    return min(t.symbol.terminals, key=lambda a: math.hypot(a.x - rx, a.y - ry)).index


def _ablegen(xml, name, stat):
    """Write the element @p xml into the collection under ORDNER, named after its contents (the same symbol
    twice is one file) -> its custom:// path."""
    sig = hashlib.sha1(re.sub(r'uuid="\{[^}]*\}"', "", xml).encode("utf-8")).hexdigest()[:10]
    rel = f"{ORDNER}/{name}_{sig}.elmt"
    ziel = os.path.join(os.environ["QET_CUSTOM_ELEMENTS_DIR"], *rel.split("/"))
    if not os.path.exists(ziel):
        os.makedirs(os.path.dirname(ziel), exist_ok=True)
        with open(ziel, "w", encoding="utf-8") as f:
            f.write(xml)
        stat["schaltbilder_neu"] += 1
    return "custom://" + rel


_OHNE_LABEL = re.compile(r'\n\s*<dynamic_text [^>]*text_from="ElementInfo"[^>]*><text></text>'
                         r'<info_name>label</info_name></dynamic_text>')


def _verweis(e, gehend, stat):
    """An interruption point as a QElectroTech report element (next_report outgoing, previous_report
    incoming) with its EPLAN graphics and its terminal at the end of the conductor; a file of its own in the
    collection, so the import needs no arrows from any collection. The text of the point stays as a folio text
    beside it."""
    h, pt = e["gruppe"], e["pt"]
    fo = [_form_linie(pts, zu, st_, pt) for pts, zu, st_ in h.linien] + \
         [_form_kreis(c, r, st_, pt) for c, r, st_ in h.kreise]
    art = "outgoing" if gehend else "incoming"
    xml = G.element_xml(f"EPLAN interruption point {art}", fo, [(0.0, 0.0, e["ri"])], HERKUNFT, names=["1"])
    xml = xml.replace('link_type="simple"', 'link_type="%s"' % ("next_report" if gehend else "previous_report"), 1)
    xml = _OHNE_LABEL.sub("", xml)
    return _ablegen(xml, f"Report_{art}", stat)


def _label_als_text(t):
    """The label of a part as a user text instead of the element information "label": QElectroTech hangs
    its own cross reference on that information only, and EPLAN's cross reference texts are on the folio."""
    idx = t.symbol.label_index
    if idx is None:
        return
    for prop_, wert_ in (("source", "text"), ("text", t.label)):
        t.folio.plan.late.append({"op": "set_element_text", "folio": t.folio.index, "element": t.ref,
                                  "index": idx, "property": prop_, "value": wert_})


def _artikel_in_sammlung(wurzel):
    """Order numbers (manufacturer_reference, normalised) of the parts in the collection @p wurzel, apart from
    the ones this import made."""
    gefunden = set()
    if not wurzel or not os.path.isdir(wurzel):
        return gefunden
    eigene = os.path.normcase(os.path.join(wurzel, *ORDNER.split("/")))
    for d, _, fs in os.walk(wurzel):
        if os.path.normcase(d).startswith(eigene):
            continue
        for f in fs:
            if f.endswith(".elmt"):
                try:
                    with open(os.path.join(d, f), encoding="utf-8", errors="replace") as h:
                        m = re.search(r'name="manufacturer_reference"[^>]*>([^<]*)<', h.read())
                except OSError:
                    continue
                if m:
                    gefunden.add(_norm(m.group(1)))
    return gefunden


def _norm(nr):
    return re.sub(r"[\s.\-/]", "", (nr or "").upper())


# --- translation ---------------------------------------------------------------------------------------------------

class Seite:
    """A page as read: its terminals, nets and terminal names."""

    def __init__(self, info, gruppen, frei, rahmen, D, stat):
        self.info, self.gruppen, self.frei, self.rahmen = info, gruppen, frei, rahmen
        self.D, self.stat = D, stat
        self.v = netze(frei["linien"])
        self.linien_an = collections.defaultdict(list)
        netzpunkte = set()
        for pts, _, _ in frei["linien"]:
            for p in pts:
                netzpunkte.add(_schluessel(p))
            for a, b in zip(pts, pts[1:]):
                self.linien_an[_schluessel(a)].append((a, b))
                self.linien_an[_schluessel(b)].append((a, b))
        # subgroups "TAG:..." of a device group belong to its part
        # (nested like -PF1, or a group of their own beside the device like -TB1:13 beside -TB1)
        geraet = {}
        for g in gruppen:
            if g.klasse == "17" and ":" not in g.titel:
                geraet.setdefault(g.titel, g)
        self.haupt_von = {}
        for g in gruppen:
            h = g
            while h.eltern is not None and h.klasse == "17" and h.eltern.klasse == "17" \
                    and h.titel.startswith(h.eltern.titel + ":"):
                h = h.eltern
            if h is g and g.klasse == "17" and ":" in g.titel:
                h = geraet.get(g.titel.split(":")[0], g)
            self.haupt_von[g.id] = h
        self.haupt = [g for g in gruppen if g.klasse in ("17", "70") and self.haupt_von[g.id] is g]
        self.teile = collections.defaultdict(list)
        for g in gruppen:
            self.teile[self.haupt_von[g.id].id].append(g)
        # terminals: end points (all corners of a polyline) of the symbol graphics on a net point
        self.anschl = collections.defaultdict(list)          # main group -> [(point, oid of the subgroup)]
        gesehen = set()
        for g in gruppen:
            if g.klasse not in ("17", "70"):
                continue
            h = self.haupt_von[g.id]
            for pts, _, _ in g.linien:
                for p in ((pts[0], pts[-1]) if len(pts) == 2 else pts):
                    k = _schluessel(p)
                    if k in netzpunkte and (h.id, k) not in gesehen:
                        gesehen.add((h.id, k))
                        self.anschl[h.id].append((p, g.oid))
        # a conductor ending on a symbol line: EPLAN draws the connection to the connection point, which may
        # lie inside a stroke of the symbol (a stroke from y 254 to 257.5, the conductor ending at 256)
        enden = [pts[0] for pts, _, _ in frei["linien"]] + [pts[-1] for pts, _, _ in frei["linien"]]
        for g in gruppen:
            if g.klasse != "17":
                continue
            h = self.haupt_von[g.id]
            pk = list(g.punkte())
            if not pk:
                continue
            x0, x1 = min(q[0] for q in pk) - TOL, max(q[0] for q in pk) + TOL
            y0, y1 = min(q[1] for q in pk) - TOL, max(q[1] for q in pk) + TOL
            for p in enden:
                if not (x0 <= p[0] <= x1 and y0 <= p[1] <= y1) or (h.id, _schluessel(p)) in gesehen:
                    continue
                # also on a circle: terminals have their second connection point on the rim of the circle
                # (circle (56,212) r = 0.75, conductor ending at y = 211.25)
                if any(_auf_strecke(p, a, b) for pts, _, _ in g.linien for a, b in zip(pts, pts[1:])) or \
                        any(abs(math.dist(p, c) - r) <= TOL for c, r, _ in g.kreise):
                    gesehen.add((h.id, _schluessel(p)))
                    self.anschl[h.id].append((p, g.oid))
        # interruption points: the conductor ends in the arrow's tip, not on a corner (a line to x = 376 for an
        # arrow from 374 to 378) -> a conductor end inside the arrow's rectangle is its terminal
        for g in gruppen:
            if g.klasse != "70" or self.anschl[g.id]:
                continue
            pk = list(g.punkte())
            if not pk:
                continue
            x0, x1 = min(q[0] for q in pk) - 0.3, max(q[0] for q in pk) + 0.3
            y0, y1 = min(q[1] for q in pk) - 0.3, max(q[1] for q in pk) + 0.3
            for p in enden:
                if x0 <= p[0] <= x1 and y0 <= p[1] <= y1 and (g.id, _schluessel(p)) not in gesehen:
                    gesehen.add((g.id, _schluessel(p)))
                    self.anschl[g.id].append((p, g.oid))
                    break
        # a corner and a conductor end of the same symbol in the same net close together (the conductor runs
        # over the corner into the stroke): only the conductor end is the terminal
        ende_keys = {_schluessel(p) for p in enden}
        for hid in list(self.anschl):
            a = self.anschl[hid]
            weg = set()
            for i, (p, _) in enumerate(a):
                if _schluessel(p) in ende_keys:
                    continue
                for j, (q, _) in enumerate(a):
                    if j != i and _schluessel(q) in ende_keys and math.dist(p, q) <= 4.0 \
                            and self.netz(p) == self.netz(q):
                        weg.add(i)
            self.anschl[hid] = [x for i, x in enumerate(a) if i not in weg]
        # a net counts only with two terminals (otherwise it is graphics that happens to touch a symbol)
        zahl = collections.Counter(self.netz(p) for h in self.anschl for p, _ in self.anschl[h])
        self.draht_netze = {n for n, c in zahl.items() if c >= 2}
        for hid in list(self.anschl):
            self.anschl[hid] = [(p, o) for p, o in self.anschl[hid] if self.netz(p) in self.draht_netze]
        self.pins = {}

    def netz(self, p):
        return self.v.finde(_schluessel(p))

    def _graph(self):
        """The conductor net as a graph: segments split at every line end that lies on them (T junctions)."""
        if hasattr(self, "graph"):
            return
        self.graph, self.koord = collections.defaultdict(dict), {}
        enden = [p for pts, _, _ in self.frei["linien"] for p in (pts[0], pts[-1])]
        for pts, _, _ in self.frei["linien"]:
            for a, b in zip(pts, pts[1:]):
                auf = [a, b] + [p for p in enden if _auf_strecke(p, a, b)]
                auf.sort(key=lambda p: math.dist(a, p))
                for p, q in zip(auf, auf[1:]):
                    kp, kq = _schluessel(p), _schluessel(q)
                    if kp == kq:
                        continue
                    self.koord[kp], self.koord[kq] = p, q
                    w = math.dist(p, q)
                    self.graph[kp][kq] = min(w, self.graph[kp].get(kq, w))
                    self.graph[kq][kp] = self.graph[kp][kq]

    def weg(self, p, q):
        """Shortest drawn path from p to q (points), or None."""
        self._graph()
        start, ziel = _schluessel(p), _schluessel(q)
        if start not in self.graph or ziel not in self.graph:
            return None
        dist, vor, offen = {start: 0.0}, {}, [(0.0, start)]
        while offen:
            d, k = heapq.heappop(offen)
            if k == ziel:
                break
            if d > dist.get(k, math.inf):
                continue
            for n, w in self.graph[k].items():
                if d + w < dist.get(n, math.inf):
                    dist[n], vor[n] = d + w, k
                    heapq.heappush(offen, (d + w, n))
        if ziel not in dist:
            return None
        kette = [ziel]
        while kette[-1] != start:
            kette.append(vor[kette[-1]])
        return [self.koord[k] for k in reversed(kette)]

    def grafik_linien(self):
        return [l_ for l_ in self.frei["linien"] if self.netz(l_[0][0]) not in self.draht_netze]

    # terminal names -----------------------------------------------------------------------------------------------
    def namen(self, verbindungen):
        D = self.D
        self.namen_frei = {}
        for h in self.haupt:
            if h.klasse != "17":
                continue
            frei_h = {}
            for g in self.teile[h.id]:
                p = D.funktion(g.oid)
                n = [z.strip() for z in (p.get(20038) or "").split(";") if z.strip()]
                if not n and p.get(20030):
                    n = [p[20030].strip()] * 2
                frei_h[g.oid] = n
            self.namen_frei[h.id] = frei_h
            # 1. the connection point text of the group next to the point
            kand = []
            texte_h = [t for g in self.teile[h.id] for t in g.texte]
            for i, (pt, o) in enumerate(self.anschl[h.id]):
                for t in texte_h:
                    if t["text"].strip() in frei_h.get(o, []):
                        d = math.dist(pt, _textmitte(t))
                        if d <= 6.0:
                            kand.append((d, i, o, t["text"].strip()))
            for _, i, o, name in sorted(kand):
                if (h.id, i) not in self.pins and name in frei_h[o]:
                    self.pins[(h.id, i)] = name
                    frei_h[o].remove(name)
                    self.stat["name_text"] += 1
        # 2. the connection list: an end "TAG:terminal" whose other end in the same net is named already
        for _ in range(4):
            idx = self.index()
            neu = 0
            for c in verbindungen:
                for von, nach in ((c["von"], c["nach"]), (c["nach"], c["von"])):
                    if _suche(idx, von) or not _suche(idx, nach):
                        continue
                    for hb, ib in _suche(idx, nach):
                        if self.benenne(von, self.netz(self.anschl[hb][ib][0])):
                            neu += 1
                            break
            if not neu:
                break
        # 3. the rest in reading order (top to bottom, left to right)
        for h in self.haupt:
            if h.klasse != "17":
                continue
            for oid_t, namen_o in self.namen_frei[h.id].items():
                offen = [i for i, (_, o) in enumerate(self.anschl[h.id]) if o == oid_t and (h.id, i) not in self.pins]
                pkte = [self.anschl[h.id][i][0] for i in offen]
                for j in _lesereihenfolge(pkte):
                    name = namen_o.pop(0) if namen_o else ""
                    self.pins[(h.id, offen[j])] = name
                    self.stat["name_reihenfolge" if name else "name_leer"] += 1
        # 4. cross-check with the connection list: when the named end is not in the net of its other end but a
        # free point of the same connection point is, the name belongs there (a device connection point with a
        # stroke inside and outside, its text nearer the inner one)
        for _ in range(500):
            idx, neu = self.index(), 0
            for c in verbindungen:
                for von, nach in ((c["von"], c["nach"]), (c["nach"], c["von"])):
                    A, B = _suche(idx, von), _suche(idx, nach)
                    if not A or not B:
                        continue
                    netze_a = {self.netz(self.anschl[h][i][0]) for h, i in A}
                    if any(self.netz(self.anschl[h][i][0]) in netze_a for h, i in B):
                        continue
                    for hb, ib in B:
                        o = self.anschl[hb][ib][1]
                        frei_ = [j for j, (q, oo) in enumerate(self.anschl[hb])
                                 if oo == o and not self.pins.get((hb, j)) and self.netz(q) in netze_a]
                        if frei_:
                            self.pins[(hb, frei_[0])], self.pins[(hb, ib)] = self.pins[(hb, ib)], ""
                            self.stat["name_umgesetzt"] += 1
                            neu += 1
                            break
                    if neu:
                        break
                if neu:
                    break
            if not neu:
                break
        # an unnamed point right beside a named terminal of the same symbol in the same net is that terminal
        # (the conductor runs over a corner into the stroke): dropped
        self.doppelt = set()
        for h in self.haupt:
            a = self.anschl[h.id]
            for i, (p, _) in enumerate(a):
                if self.pins.get((h.id, i)) or h.klasse != "17":
                    continue
                if any(j != i and self.pins.get((h.id, j)) and self.netz(p) == self.netz(q)
                       and math.dist(p, q) <= 4.0 for j, (q, _) in enumerate(a)):
                    self.doppelt.add((h.id, i))
                    self.stat["name_leer"] -= 1
                    self.stat["doppelt"] += 1

    def benenne(self, ende, netz):
        """Name an unnamed terminal in the net after the connection end "TAG:terminal" (also
        "TAG:plug:terminal")."""
        bmk, name = ende.split(":")[0], ende.split(":")[-1]
        for h in self.haupt:
            if h.klasse != "17":
                continue
            for i, (pt, o) in enumerate(self.anschl[h.id]):
                if (h.id, i) in self.pins or self.netz(pt) != netz:
                    continue
                p = self.D.funktion(o)
                namen_o = self.namen_frei[h.id].get(o, [])
                if ende == p.get(20001) and namen_o:
                    nm = namen_o[0]
                elif bmk in (p.get(20006), p.get(20001)) and name in namen_o:
                    nm = name
                else:
                    continue
                self.pins[(h.id, i)] = nm
                namen_o.remove(nm)
                self.stat["name_liste"] += 1
                return True
        return False

    def index(self):
        """Connection end ("TAG:terminal") -> [(main group id, terminal index)]."""
        idx = collections.defaultdict(list)
        for h in self.haupt:
            for i, (_, o) in enumerate(self.anschl[h.id]):
                name = self.pins.get((h.id, i))
                if not name:
                    continue
                p = self.D.funktion(o)
                ks = {f"{p.get(20006, '')}:{name}", f"{p.get(20001, '')}:{name}"}
                if p.get(20001) and ":" in p[20001]:
                    ks.add(p[20001])
                # with a plug designation ("=...-PF1:I3:11"): only a text in the connection point group
                for g in self.teile.get(h.id, []):
                    if g.oid == o:
                        for t in g.texte:
                            st = t["text"].strip()
                            if st and st != name and ":" not in st:
                                ks.add(f"{p.get(20006, '')}:{st}:{name}")
                for k in ks:
                    idx[k].append((h.id, i))
        return idx

    # conductors ---------------------------------------------------------------------------------------------------
    def kanten(self, verbindungen):
        """The connections of the list within one net, then per net the parts joined at the nearest terminal."""
        idx = self.index()
        netz_von = {(h, i): self.netz(pt) for h in self.anschl for i, (pt, _) in enumerate(self.anschl[h])
                    if (h, i) not in self.doppelt}
        kanten, benutzt = [], set()
        for c in verbindungen:
            for a_ in _suche(idx, c["von"]):
                b_ = next((b2 for b2 in _suche(idx, c["nach"]) if b2 != a_ and netz_von.get(b2) == netz_von.get(a_)),
                          None)
                if b_ is None:
                    continue
                paar = frozenset({a_, b_})
                if paar not in benutzt:
                    benutzt.add(paar)
                    kanten.append((a_, b_, c))
                    self.stat["verbindungen_aus_liste"] += 1
                break
        uf = Vereinigung()
        for a_, b_, _ in kanten:
            uf.vereine(a_, b_)
        je_netz = collections.defaultdict(list)
        for k, n in netz_von.items():
            je_netz[n].append(k)
        for ks in je_netz.values():
            teile = collections.defaultdict(list)
            for k in ks:
                teile[uf.finde(k)].append(k)
            gr = list(teile.values())
            while len(gr) > 1:
                g0, best = gr.pop(0), None
                for j, g1 in enumerate(gr):
                    for a_ in g0:
                        for b_ in g1:
                            d = math.dist(self.anschl[a_[0]][a_[1]][0], self.anschl[b_[0]][b_[1]][0])
                            if best is None or d < best[0]:
                                best = (d, a_, b_, j)
                _, a_, b_, j = best
                kanten.append((a_, b_, None))
                gr[j] = g0 + gr[j]
                self.stat["verbindungen_aus_zeichnung"] += 1
        return kanten


def _rollen(D, oids):
    """Master/slave per device tag (20006) from the function definitions."""
    rolle, je_bmk = {}, collections.defaultdict(list)
    for oid in oids:
        p = D.funktion(oid)
        if oid.startswith("17_") and p and ":" not in (p.get(20001) or "") and kategorie(p) not in OHNE_ROLLE:
            je_bmk[p.get(20006, "")].append(oid)
    for bmk, os_ in je_bmk.items():
        if not bmk:
            continue
        # 20122 = 1: main function
        kont = [o for o in os_ if kategorie(D.funktion(o)) in (201, 202, 203, 204) and D.funktion(o).get(20122) != "1"]
        haupt = [o for o in os_ if o not in kont and D.funktion(o).get(20122) == "1"]
        if not kont or not haupt:
            continue
        km = kategorie(D.funktion(haupt[0]))
        rolle[haupt[0]] = ("master", "coil" if km == 200 else "protection" if km // 100 == 5 else "commutator")
        for o in kont:
            rolle[o] = ("slave", {201: "NO", 202: "NC"}.get(kategorie(D.funktion(o)), "SW"), "simple")
    return rolle, je_bmk


def _bauteil(se, h, b, rolle, artikel, stat):
    """The symbol of the group (with its subgroups) as a QElectroTech element in the collection, placed."""
    D = se.D
    p = D.funktion(h.oid)
    # (index in the symbol, point) without the doubled points
    nummern = [i for i in range(len(se.anschl[h.id])) if (h.id, i) not in se.doppelt]
    pkte = [se.anschl[h.id][i] for i in nummern]
    teile = se.teile[h.id]
    if pkte:
        ein = pkte[0][0]
    else:
        alle = [q for g in teile for q in g.punkte()]
        ein = (min(q[0] for q in alle), min(q[1] for q in alle)) if alle else (0.0, 0.0)
    sichtbar = (p.get(20215) or "").strip()          # 20215: the device tag as displayed
    fo, label_text = [], None
    for g in teile:
        fo += [_form_linie(pts, zu, st_, ein) for pts, zu, st_ in g.linien]
        fo += [_form_kreis(c, r, st_, ein) for c, r, st_ in g.kreise]
        for t in g.texte:
            tx = t["text"].strip()
            if g is h and label_text is None and sichtbar and tx.endswith(sichtbar) and tx[:1] in "-=+":
                label_text = tx
                fo.append(_form_text(t, ein, "label"))
            else:
                fo.append(_form_text(t, ein))
    an = [(pt[0] - ein[0], pt[1] - ein[1], _richtung(pt, se.linien_an[_schluessel(pt)])) for pt, _ in pkte]
    namen = [se.pins.get((h.id, i), "") for i in nummern]
    xml = G.element_xml(f"EPLAN {p.get(20026) or h.titel}", fo, an, HERKUNFT, names=namen)
    xml = G.with_role(xml, rolle.get(h.oid))
    if label_text is None:          # EPLAN shows no device tag here: the label only as an information
        xml = _OHNE_LABEL.sub("", xml)
    pfad_ = _ablegen(xml, _dateiname(p.get(20026) or "Symbol"), stat)
    infos = {}
    art = (p.get(20100) or "").strip()
    if art:
        kuerzel, _, nr = art.partition(".")
        infos["manufacturer"] = HERSTELLER.get(kuerzel, kuerzel)
        infos["manufacturer_reference"] = (p.get(20919) or nr or art).strip()
        if artikel is not None and _norm(infos["manufacturer_reference"]) not in artikel:
            stat["artikel_ohne_bauteil"] += 1
    for k, pid in (("designation", 20193), ("function", 20031), ("description", 20027)):
        if p.get(pid):
            infos[k] = p[pid].strip()
    t = b.place(pfad_, *_px(ein), label=label_text or sichtbar, infos=infos)
    stat["bauteile"] += 1
    stat["anschluesse"] += len(pkte)
    return t, ein


def _im_archiv(wurzel, ordner, href):
    """File of a picture reference @p href of a page in @p ordner, or None when it lies outside the
    unpacked archive @p wurzel (an absolute path, a drive, "..", a link out of it) or is a URL."""
    if not href or re.match(r"^[A-Za-z][A-Za-z0-9+.-]+:", href):
        return None                     # data:, file:, http: ... -- not a file of the archive
    if os.path.isabs(href) or re.match(r"^[A-Za-z]:", href) or href.startswith(("/", "\\")):
        return None
    datei = os.path.realpath(os.path.join(ordner, href))
    basis = os.path.realpath(wurzel)
    try:
        if os.path.commonpath([datei, basis]) != basis:
            return None
    except ValueError:                  # another drive
        return None
    return datei


_KINDER, _AUFRAEUMEN = [], []


def _stop_on_eof():
    """--stop-on-eof: when standard input closes, kill the processes started since (the QElectroTech runs),
    remove the working folder and exit with code 3."""
    popen_init = subprocess.Popen.__init__

    def init(self, *a, **k):
        popen_init(self, *a, **k)
        _KINDER.append(self)
    subprocess.Popen.__init__ = init

    def wache():
        try:
            # os.read, not sys.stdin: a thread blocked in the buffered reader holds its lock, and Python
            # aborts at shutdown when it cannot take it ("_enter_buffered_busy")
            while os.read(0, 4096):
                pass
        except Exception:
            pass
        for p in list(_KINDER):
            try:
                p.kill()
            except Exception:
                pass
        for d in _AUFRAEUMEN:
            shutil.rmtree(d, ignore_errors=True)
        print("stopped", flush=True)
        os._exit(3)
    threading.Thread(target=wache, daemon=True).start()


def umsetzen(epdz, ziel_qet, arbeit=None, sammlung=True, entpackt=None):
    """Translate @p epdz into the project @p ziel_qet. sammlung: keep the parts made from the EPLAN symbols in
    the user collection (otherwise they live only in the project); entpackt: the archive already unpacked.
    -> the written project."""
    _fortschritt(2, "read")
    arbeit = arbeit or os.path.join(os.path.dirname(os.path.abspath(ziel_qet)), "_epdz_import")
    eigene_arbeit = not os.path.isdir(arbeit)
    os.makedirs(arbeit, exist_ok=True)
    if eigene_arbeit:
        _AUFRAEUMEN.append(arbeit)
    # where the parts go: QElectroTech finds them there as custom:// for this run
    wurzel = qet_backend.user_collection() if sammlung else None
    if sammlung and not wurzel:
        print("No user collection found (QET_CUSTOM_ELEMENTS_DIR): the parts stay in the project only.")
    if not wurzel:
        wurzel = os.path.join(arbeit, "elements")
    os.makedirs(wurzel, exist_ok=True)
    os.environ["QET_CUSTOM_ELEMENTS_DIR"] = os.path.abspath(wurzel)
    x = entpackt or auspacken(epdz, arbeit)
    D = Daten(os.path.join(x, "manifest.db"))
    svg_ordner = os.path.join(x, "packages", "pages", "items", "pagesvg")
    stat, nicht = collections.Counter(), collections.Counter()

    _fortschritt(8, "pages")
    seiten = []
    for s in D.seiten:
        datei = os.path.join(svg_ordner, s["name"] + ".svg")
        if os.path.exists(datei):
            seiten.append(Seite(s, *seite_lesen(datei), D, stat))
    if not seiten:
        raise RuntimeError("the archive holds no page drawing (packages/pages/items/pagesvg)")
    rolle, je_bmk = _rollen(D, {g.oid for se in seiten for g in se.gruppen})

    plan = Plan(os.path.splitext(os.path.basename(epdz))[0], os.path.dirname(os.path.abspath(ziel_qet)))
    artikel = _artikel_in_sammlung(wurzel) if sammlung else None
    teil_je_oid, je_blatt, kanten_alle, pfeile = {}, {}, [], {}

    for si, se in enumerate(seiten):
        _fortschritt(15 + 65 * si / max(1, len(seiten)), "pages")
        s = se.info
        b = plan.folio(s["titel"] or s["name"], _ohne_vorsatz(s["anlage"], "="), _ohne_vorsatz(s["ort"], "+"),
                       s["nr"])
        formen, texte, bilder = [], [], []
        # frame and title block (EPLAN's form, evaluated already) as folio graphics
        formen += _blattgrafik(se.rahmen["linien"], se.rahmen["kreise"])
        texte += _blatttexte(se.rahmen["texte"])
        for href, x0, y0, x1, y1 in se.rahmen["bilder"] + se.frei["bilder"]:
            datei = _im_archiv(x, svg_ordner, href)
            bx = G.image_xml(datei, x0, y0, x1, y1) if datei else None
            if bx:
                bilder.append(bx)
            else:
                nicht["bild_nicht_lesbar"] += 1
        formen += _blattgrafik(se.grafik_linien(), se.frei["kreise"])
        texte += _blatttexte(se.frei["texte"])
        se.namen(D.verbindungen)

        teil_von = {}
        for h in se.haupt:
            if h.klasse == "70":
                if se.anschl[h.id]:
                    pt = se.anschl[h.id][0][0]
                    pfeile[h.id] = {"blatt": b, "pt": pt, "ri": _richtung(pt, se.linien_an[_schluessel(pt)]),
                                    "gruppe": h, "seite": si}
                else:
                    formen += _blattgrafik(h.linien, h.kreise)
                    nicht["unterbrechung_ohne_leitung"] += 1
                texte += _blatttexte(h.texte)
                continue
            p = D.funktion(h.oid)
            if not se.anschl[h.id] and not p.get(20100):
                for g in se.teile[h.id]:          # structure / location box and the like
                    formen += _blattgrafik(g.linien, g.kreise)
                    texte += _blatttexte(g.texte)
                stat["grafik_gruppe"] += 1
                continue
            t, ein = _bauteil(se, h, b, rolle, artikel, stat)
            teil_von[h.id] = (t, ein)
            teil_je_oid[h.oid] = t
        # connection definition points (59: colour/section texts), structure boxes (88: location box
        # "=KF1+A2", dash-dotted) and other groups without terminal: folio graphics
        for g in se.gruppen:
            if g.klasse not in ("17", "70"):
                formen += _blattgrafik(g.linien, g.kreise)
                texte += _blatttexte(g.texte)
        for a_, b_, c in se.kanten(D.verbindungen):
            kanten_alle.append((b, se, teil_von, a_, b_, c))
        je_blatt[b.index] = (formen, texte, bilder)

    # Report arrows. The jump target of an interruption point is the next one of the same name (EPLAN's jump
    # order), so the points of one potential make a chain over the pages (1427 -> 1943 -> 1944 -> 2283 ...,
    # 1943/1944 left and right on the same page). Pairs are consecutive links (1-2, 3-4 as in the EPLAN help),
    # starting where no pair lies on one page; the first of a pair is the outgoing one.
    _fortschritt(84, "interruptions")
    naechst = {k: e["gruppe"].partner for k, e in pfeile.items() if e["gruppe"].partner in pfeile
               and e["gruppe"].partner != k}
    vorher = {v: k for k, v in naechst.items()}
    ketten, gesehen = [], set()
    for s in [k for k in pfeile if k not in vorher] + list(pfeile):
        if s in gesehen:
            continue
        kette, k = [], s
        while k is not None and k not in gesehen:
            gesehen.add(k)
            kette.append(k)
            k = naechst.get(k)
        ketten.append((kette, naechst.get(kette[-1]) == kette[0] and len(kette) > 1))
    paare = []
    for kette, ring in ketten:
        best = None
        for versatz in ((0, 1) if ring or len(kette) % 2 else (0,)):
            k2 = kette[versatz:] + (kette[:versatz] if ring else [])
            pp = [(k2[i], k2[i + 1]) for i in range(0, len(k2) - 1, 2)]
            gleich = sum(pfeile[a]["blatt"] is pfeile[b]["blatt"] for a, b in pp)
            if best is None or (gleich, -len(pp)) < (best[0], -len(best[1])):
                best = (gleich, pp)
        paare += best[1]
    gepaart = {k for p in paare for k in p}
    pfeil_paare_ids = []
    for paar in [list(p) for p in paare] + [[k] for k in pfeile if k not in gepaart]:
        if len(paar) < 2:
            stat["pfeil_einzeln"] += 1          # drawn, but EPLAN gives it no partner
        name = "epdz:" + "|".join(paar)
        if len(paar) == 2:
            pfeil_paare_ids.append(tuple(paar))
        for n, k in enumerate(paar):
            ek = pfeile[k]
            gehend = n == 0 and len(paar) == 2
            ek["teil"] = ek["blatt"].place(_verweis(ek, gehend, stat), *_px(ek["pt"]))
            stat["pfeile"] += 1
            if len(paar) == 2 and pfeile[paar[0]]["blatt"] is pfeile[paar[1]]["blatt"]:
                continue                       # a pair on one page: linked directly below
            (plan.sources if gehend else plan.targets).append((ek["blatt"].index, name, ek["teil"]))
        if len(paar) == 2 and pfeile[paar[0]]["blatt"] is pfeile[paar[1]]["blatt"]:
            # Plan._report_links looks for the partner on other folios only; QElectroTech links on one too
            ta, tb = pfeile[paar[0]]["teil"], pfeile[paar[1]]["teil"]
            plan.links.append((ta.folio.index, ta, tb.folio.index, tb))

    _fortschritt(88, "connections")
    pfeil_paare = [(pfeile[a]["teil"], pfeile[b]["teil"]) for a, b in pfeil_paare_ids]
    gezeichnet = 0
    grad, potential, ende_teil, werte = collections.Counter(), Vereinigung(), {}, {}
    for b, se, teil_von, a_, b_, c in kanten_alle:
        enden = []
        for hid, i in (a_, b_):
            if hid in teil_von:
                t, ein = teil_von[hid]
                enden.append((t, _anschluss_index(t, se.anschl[hid][i][0], ein)))
            elif hid in pfeile and "teil" in pfeile[hid]:
                enden.append((pfeile[hid]["teil"], 0))
            else:
                enden.append(None)
        if None in enden:
            nicht["ende_ohne_bauteil"] += 1
            continue
        (ta, ia), (tb, ib) = enden
        if ta is tb and ia == ib:
            continue
        if ta.pos(ia) == tb.pos(ib):
            nicht["deckungsgleich"] += 1
            continue
        b.wire(ta, ia, tb, ib)
        gezeichnet += 1
        # the path as drawn: the corners of the shortest way through EPLAN's conductor net
        w = se.weg(se.anschl[a_[0]][a_[1]][0], se.anschl[b_[0]][b_[1]][0])
        if w:
            ek = ecken(w)
            if ek:
                b.ops.append({"op": "set_conductor_path", "folio": b.index, "from": ta.ref,
                              "from_terminal": ta.terminal(ia).uuid, "to": tb.ref,
                              "to_terminal": tb.terminal(ib).uuid,
                              "corners": ";".join("%g,%g" % _px(e) for e in ek)})
                stat["leiterweg"] += 1
        else:
            stat["leiterweg_ohne"] += 1
        ea, eb = (b.index, ta.ref, ia), (b.index, tb.ref, ib)
        grad[ea] += 1
        grad[eb] += 1
        potential.vereine(ea, eb)
        ende_teil[ea], ende_teil[eb] = (b, ta, ia), (b, tb, ib)
        if c:
            werte.setdefault(ea, c)
    # In QElectroTech colour, section and potential hold for the whole potential (setConductorProperty) and
    # must agree on both sides of a report pair, or QElectroTech does not link the arrows: one value per
    # potential across the report pairs, set per folio net at a terminal with exactly one conductor
    ueber = Vereinigung()
    for e in ende_teil:
        ueber.vereine(e, potential.finde(e))
    for ta, tb in pfeil_paare:
        ea, eb = (ta.folio.index, ta.ref, 0), (tb.folio.index, tb.ref, 0)
        if ea in ende_teil and eb in ende_teil:
            ueber.vereine(ea, eb)
    je_netz, je_pot = collections.defaultdict(list), collections.defaultdict(set)
    for e in ende_teil:
        je_netz[potential.finde(e)].append(e)
        je_pot[ueber.finde(e)].add(potential.finde(e))
    for netze_ in je_pot.values():
        es_alle = [e for n in netze_ for e in je_netz[n]]
        c = next((werte[e] for e in es_alle if e in werte), None)
        if not c:
            continue
        for n in netze_:
            blatt_ende = next((e for e in je_netz[n] if grad[e] == 1), None)
            if blatt_ende is None:
                nicht["netz_ohne_ende"] += 1
                continue
            b, t, i = ende_teil[blatt_ende]
            for prop, wert in (("conductor_color", c["farbe"]), ("conductor_section", c["querschnitt"]),
                               ("function", c["potential"])):
                if wert:
                    b.ops.append({"op": "set_conductor", "folio": b.index, "element": t.ref,
                                  "terminal": t.terminal(i).uuid, "property": prop, "value": wert})

    # master/slave links (EPLAN's cross reference texts stay, QElectroTech's own are off)
    for o_m, mt in teil_je_oid.items():
        if rolle.get(o_m, ("",))[0] != "master":
            continue
        sl = [teil_je_oid[o] for o in je_bmk.get(D.funktion(o_m).get(20006, ""), [])
              if rolle.get(o, ("",))[0] == "slave" and o in teil_je_oid]
        if sl:
            mt.folio.link(mt, *sl)
            stat["verknuepft"] += len(sl)
            for t_ in [mt, *sl]:
                _label_als_text(t_)

    _fortschritt(92, "writing")
    erg, r = plan.build(os.path.basename(ziel_qet))
    with open(erg, encoding="utf-8") as f:
        s_qet = f.read()
    with open(erg, "w", encoding="utf-8") as f:
        s_qet = re.sub(r'<report label="[^"]*"', '<report label=""', s_qet)
        # EPLAN's frame, drawn as folio graphics, has its own column and row numbering: QElectroTech's
        # headers off, or the folio shows both
        s_qet = re.sub(r'(<diagram\b[^>]*?\b)displaycols="[^"]*"', r'\1displaycols="false"', s_qet)
        s_qet = re.sub(r'(<diagram\b[^>]*?\b)displayrows="[^"]*"', r'\1displayrows="false"', s_qet)
        f.write(s_qet)
    einfuegen = {}
    for folio, (formen, texte, bilder) in je_blatt.items():
        shapes, inputs = G.folio_xml(formen)
        einfuegen[folio] = (shapes, inputs + texte, bilder)
    G.insert_into_project(erg, einfuegen)

    _fortschritt(96, "lists")
    listen(D, x, ziel_qet)
    fehl = [o for o in r.get("operations", []) if not o.get("succeeded")]
    bericht = (f"pages {len(seiten)}, parts {stat['bauteile']} (new symbols {stat['schaltbilder_neu']}), "
               f"terminals {stat['anschluesse']} (named from a text {stat['name_text']}, from the connection list "
               f"{stat['name_liste']}, in order {stat['name_reihenfolge']}, unnamed {stat['name_leer']}), "
               f"conductors {gezeichnet} (from the connection list {stat['verbindungen_aus_liste']}, from the "
               f"drawing {stat['verbindungen_aus_zeichnung']}, with the drawn path {stat['leiterweg']}), "
               f"not drawn {dict(nicht)}, report arrows {stat['pfeile']}, arrows without partner {stat['pfeil_einzeln'] + len(plan.open)}, "
               f"master/slave {stat['verknuepft']}, graphics groups {stat['grafik_gruppe']}, articles without a "
               f"part in the collection {stat['artikel_ohne_bauteil']}, failed operations {len(fehl)}")
    with open(os.path.splitext(ziel_qet)[0] + "_import_report.txt", "w", encoding="utf-8") as f:
        f.write(bericht + "\n")
        for o in fehl[:200]:
            f.write(f"failed: {o}\n")
    print(bericht)
    if fehl:
        print("first failed operation:", fehl[0])
    if not entpackt:
        shutil.rmtree(x, ignore_errors=True)
    if eigene_arbeit:
        shutil.rmtree(arbeit, ignore_errors=True)
    return erg


def listen(D, x, ziel_qet):
    """Device list and connection list (CSV for spreadsheet programs) and the 3D installation spaces beside
    the project."""
    basis = os.path.splitext(ziel_qet)[0]
    with open(basis + "_devices.csv", "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["Device tag", "Function", "Article", "Designation", "Function text", "Page"])
        for pid in D.funktionen:
            p = D.props.get(pid, {})
            if p.get(20001):
                w.writerow([p.get(20001, ""), p.get(20026, ""), p.get(20100, ""), p.get(20193, ""),
                            (p.get(20031) or "").strip(), p.get(19007, "")])
    with open(basis + "_connections.csv", "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["Source", "Target", "Potential", "Colour", "Section", "Length"])
        for c in D.verbindungen:
            w.writerow([c["von"], c["nach"], c["potential"], c["farbe"], c["querschnitt"], c["laenge"]])
    quelle = os.path.join(x, "packages", "installationspaces", "items", "installationspacee3d")
    if os.path.isdir(quelle):
        ziel = basis + "_3D"
        os.makedirs(ziel, exist_ok=True)
        for n in os.listdir(quelle):
            shutil.copy2(os.path.join(quelle, n), os.path.join(ziel, n))


def standard_ziel(archiv):
    """<folder of the archive>/<name of the archive>.qet, as QElectroTech's import dialog proposes it."""
    return os.path.splitext(os.path.abspath(archiv))[0] + ".qet"


def pruefen():
    """--check: whether this Python runs the import -> (ok, message)."""
    if sys.version_info < (3, 9):
        return False, f"Python 3.9 or later is needed, this is {sys.version.split()[0]}"
    try:
        qet_backend.mcp()
        qet_backend.binary()
    except Exception as e:
        return False, str(e)
    return True, "ok"


def main(argv):
    if "--check" in argv:
        ok, meldung = pruefen()
        print(meldung)
        return 0 if ok else 1
    if "--stop-on-eof" in argv:
        _stop_on_eof()
    args, entpackt, i = [], None, 0
    while i < len(argv):
        a = argv[i]
        if a == "--unpacked" and i + 1 < len(argv):
            entpackt = argv[i + 1]
            i += 2
            continue
        if not a.startswith("--"):
            args.append(a)
        i += 1
    if not args:
        print(__doc__.split("\n\n")[0])
        return 2
    archiv = args[0]
    ziel = args[1] if len(args) > 1 else standard_ziel(archiv)
    os.makedirs(os.path.dirname(os.path.abspath(ziel)), exist_ok=True)
    name = os.path.splitext(os.path.basename(archiv))[0].replace(" ", "_")
    erg = umsetzen(archiv, ziel, arbeit=os.path.join(os.path.dirname(os.path.abspath(ziel)), "_epdz_import_" + name),
                   sammlung="--no-collection" not in argv, entpackt=entpackt)
    if "--pdf" in argv:
        _fortschritt(97, "pdf")
        q = qet_backend.mcp()
        print("pdf:", q.tool_export(qet_backend.binary(), erg, "pdf", os.path.splitext(erg)[0] + ".pdf",
                                    timeout=3600).get("ok"))
    _fortschritt(100, "done")
    print("done:", erg)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
