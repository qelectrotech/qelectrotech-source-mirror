"""Tests of the WSCAD import without WSCAD and without QElectroTech: the
projects are made here (an SQLite database and packed page files, as
FORMAT_WSCAD.md describes them).

    python -m unittest discover -s tools/wscad_import -p "test_*.py"

Writing packed pages needs the Python package "cryptography"; those tests
are skipped without it.
"""
import contextlib
import gzip
import io
import os
import sqlite3
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
_CUSTOM = tempfile.TemporaryDirectory(prefix="wscad-test-custom-")
# the collections, before qet_sheet reads them
os.environ["QET_CUSTOM_ELEMENTS_DIR"] = _CUSTOM.name
os.environ.pop("QET_COMPANY_ELEMENTS_DIR", None)
sys.path.insert(0, HERE)

import wscad as W  # noqa: E402
import wscad_graphics as G  # noqa: E402
import wscad_to_qet as Q  # noqa: E402

try:
    from cryptography.hazmat.primitives import padding
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
    HAVE_AES = True
except ImportError:
    HAVE_AES = False


def packed(content, version=(6, 20, 0, 0)):
    """A packed section: version, length, key and IV length, key, IV, three
    check bytes, then AES-CBC (PKCS7) over GZip."""
    key, iv = bytes(range(32)), bytes(range(100, 116))
    p = padding.PKCS7(128).padder()
    plain = p.update(gzip.compress(content)) + p.finalize()
    e = Cipher(algorithms.AES(key), modes.CBC(iv)).encryptor()
    secret = e.update(plain) + e.finalize()
    head = bytes(version) + struct.pack("<I", len(content)) + bytes([32, 16]) + key + iv
    return head + bytes([key[0], key[1], iv[0]]) + secret


def page_file(path, xml):
    """A page file: mark, version 6.4, counter, five section entries of 19
    bytes, section 2 = the page."""
    page = packed(b"\x06\x14\x00\x00" + xml.encode("utf-8"))
    begin = 12 + 5 * 19
    entries = struct.pack("<BBBIId", 1, 1, 2, begin, len(page), 0.0)
    entries += b"\x00" * 19 * 4
    with open(path, "wb") as f:
        f.write(b"\x01\x02\x00\x00" + bytes((6, 4, 0, 0)) + struct.pack("<I", 103) + entries + page)


# a circuit breaker -Q1 (terminal 2 on the right), a corner, a coil -K1 (A1
# on top); the corner leads from -Q1:2 to -K1:A1; a free line
PAGE = """<?xml version="1.0" encoding="utf-8"?><WS><Solution><Project><File>
<Header><PaperSize val="297 210" /></Header>
<line x1="5" y1="200" x2="100" y2="200" />
<SL type="1.0" x="8.75" y="5" hid="11" pid="12"><P id="BMKZ" prd="-Q1" />
  <line x1="1.25" y1="0" x2="1.25" y2="5" />
  <PHL x="-1" y="3" style="wstype:BMKZ;align:26;size:3.75" xpath="../P[@id='BMKZ']" />
  <Pin connDir="2" pos="1.25 5" nr="2" /></SL>
<SL type="10.2" x="18.75" y="10"><P id="BtName"><T id="DE">Winkel</T></P>
  <polyline points="0 0 1.25 0 1.25 1.25" />
  <Pin connDir="8" pos="0 0" /><Pin connDir="4" pos="1.25 1.25" /></SL>
<SL type="1.0" x="18.75" y="30" hid="21" pid="22"><P id="BMKZ" prd="-K1" />
  <rect x="-2" y="0" wd="5" hg="5" />
  <Pin connDir="1" pos="1.25 0" nr="A1" /></SL>
</File></Project></Solution></WS>"""


def project(folder, pages=(("=A1+ST_Plan.0001.wsELD", PAGE),)):
    """A project folder with the database and the page files."""
    os.makedirs(os.path.join(folder, "Database"))
    c = sqlite3.connect(os.path.join(folder, "Database", "WSCAD6.db"))
    c.execute("CREATE TABLE Page (PageID INTEGER, FileName TEXT, FileType TEXT, Ref TEXT, "
              "Width REAL, Height REAL, AspF TEXT, AspL TEXT)")
    c.execute("CREATE TABLE Symbol (SymbolID INTEGER, ParentID INTEGER, ItemType INTEGER, BMK TEXT, "
              "PageID INTEGER, x REAL, y REAL)")
    c.execute("CREATE TABLE Pin (SymbolID INTEGER, PinID INTEGER, Name TEXT, Number TEXT, x REAL, y REAL)")
    c.execute("CREATE TABLE Connection (SymbolID1 INTEGER, Pin1 INTEGER, SymbolID2 INTEGER, Pin2 INTEGER, "
              "PageID INTEGER, Colorcode TEXT, Dimension TEXT, Potential TEXT, WireName TEXT, "
              "WireType TEXT, CableID INTEGER)")
    c.execute("CREATE TABLE Data (ID INTEGER, Type INTEGER, Data TEXT)")
    for k, (name, xml) in enumerate(pages, 1):
        c.execute("INSERT INTO Page VALUES (?, ?, 'wsELD', '', 297, 210, 'A1', 'ST')", (k, name.upper()))
        if xml is not None:
            page_file(os.path.join(folder, name), xml)
    c.execute("INSERT INTO Symbol VALUES (11, 0, 1, '=A1+ST-Q1', 1, 8.75, 5)")
    c.execute("INSERT INTO Symbol VALUES (21, 0, 1, '=A1+ST-K1', 1, 18.75, 30)")
    c.execute("INSERT INTO Pin VALUES (11, 1, '', '2', 1.25, 5)")
    c.execute("INSERT INTO Pin VALUES (21, 1, '', 'A1', 1.25, 0)")
    c.execute("INSERT INTO Connection VALUES (11, 1, 21, 1, 1, 'BK', '1.5', 'L1', '', '', 0)")
    c.execute("INSERT INTO Data VALUES (11, 57, 'ART-1')")
    c.commit()
    c.close()


@unittest.skipUnless(HAVE_AES, "the Python package cryptography is not installed")
class PageFile(unittest.TestCase):
    def test_unpacking_gives_the_xml(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "=A1+ST_PLAN.0001.wsELD")
            page_file(path, PAGE)
            f = W.page_xml(path)
            self.assertEqual(f.tag, "File")
            self.assertEqual([sl.get("type") for sl in f.findall("SL")], ["1.0", "10.2", "1.0"])

    def test_bad_padding_is_noticed(self):
        raw = bytearray(packed(b"abc"))
        raw[-1] ^= 0xFF                              # the last block spoilt
        with self.assertRaises(W.FormatError):
            W.unpack(bytes(raw))

    def test_truncated_sections(self):
        with self.assertRaises(W.FormatError):
            W.unpack(b"\x06\x14\x00\x00\x03\x00\x00\x00")
        with self.assertRaises(W.FormatError):
            W.unpack(packed(b"abc")[:-5])


class Texts(unittest.TestCase):
    def test_alignment_as_calibrated_on_the_pdf(self):
        self.assertEqual(Q._alignment({"align": "26"}), (2, "bottom"))     # designator bottom right
        self.assertEqual(Q._alignment({"align": "22"}), (1, "middle"))     # column number
        self.assertEqual(Q._alignment({"align": "16"}), (0, "middle"))     # arrow cross reference
        self.assertEqual(Q._alignment({"align": "6"}), (1, "top"))
        self.assertEqual(Q._alignment({}), (0, "base"))

    def test_plc_address(self):
        self.assertEqual(Q.plc_address("0#|#True#|#True#|#EN|T|I|L|DE|T|E|L|HU|T|I#|##|#0#|#7#|#1#|#.#|#"),
                         "E0.7")
        self.assertEqual(Q.plc_address("0#|#True#|#True#|#HU|T|Q|L|DE|T|A#|##|#2#|#3#|#1#|#.#|#"), "A2.3")
        self.assertEqual(Q.plc_address("0#|#False#|#True#|#EN|T|AIW|L|DE|T|AEW#|##|#1#|#-2147483648#|#1#|#.#|#"),
                         "AEW1")
        self.assertEqual(Q.plc_address("x"), "x")

    def test_own_structure_in_the_cross_reference(self):
        Q.OWN_STRUCTURE[0] = "=A1+ST"
        try:
            self.assertEqual(Q._without_own_structure("=A1+ST/CE:1.3"), "/CE:1.3")
            self.assertEqual(Q._without_own_structure("=A1+P1/2.4"), "=A1+P1/2.4")
        finally:
            Q.OWN_STRUCTURE[0] = ""

    def test_lines(self):
        self.assertEqual(Q._lines("Supply\r\n\r\nSwitch room"), "Supply\nSwitch room")
        self.assertEqual(Q._lines("Transformer\\aTerminals"), "Transformer\nTerminals")

    def test_multi_line_bottom_starts_at_the_anchor(self):
        f = Q.text_shapes("a\nb", 10, 20, {"align": "30"}, Q.Mapping(0, 0, 210))
        self.assertAlmostEqual(f[0][1][1], 210 - 20)                       # the first line at the anchor
        self.assertAlmostEqual(f[1][1][1], 210 - 22.5)                     # the second 2.5 mm below

    def test_frame_wraps_and_cuts(self):
        long_ = "Feedback emergency stop of the cabinet and the transport table"
        wrapped, box = Q.framed(long_, ET.fromstring('<PH wd="20" hg="7" />'), {"tc": "6"})
        self.assertGreater(wrapped.count("\n"), 0)
        self.assertEqual(box, (20.0, 7.0))
        cut, _ = Q.framed(long_, ET.fromstring('<PHL wd="20" hg="2.5" />'), {"tc": "5"})
        self.assertTrue(long_.startswith(cut) and len(cut) < len(long_))


class Wires(unittest.TestCase):
    def page(self):
        return Q.Page({"width": 297, "height": 210}, ET.fromstring(PAGE).find(".//File"))

    def test_route_through_the_corner(self):
        w = self.page().route((10.0, 10.0), (20.0, 30.0))
        self.assertIsNotNone(w)
        self.assertEqual((w[0], w[-1]), ((10.0, 10.0), (20.0, 30.0)))

    def test_point_key_does_not_flip(self):
        self.assertEqual(Q._key((40.01, 170.0)), Q._key((40.010000000000005, 170.0)))

    def test_terminal_position_by_pin_id(self):
        pg = self.page()
        # database: PinID 1 at (1.25; 5) relative to the symbol base
        self.assertEqual(pg.terminal_position(11, (1, ("", "2", 1.25, 5.0))), (10.0, 10.0))
        self.assertIsNone(pg.terminal_position(99, (1, ("", "", 0, 0))))


class Arrows(unittest.TestCase):
    def test_pairs_by_cross_reference_and_the_earlier_sheet_goes(self):
        def page(nr, arrows):
            sls = "".join(f'<SL type="14.3" x="0" y="0"><P id="Leitungsname" val="{n}" />'
                          f'<QV><P id="QV" val="{qv}" /></QV><Pin connDir="8" pos="0 0" /></SL>'
                          for n, qv in arrows)
            f = ET.fromstring(f'<File><Template><PH><P id="Blattnummer" val="{nr}" /></PH></Template>'
                              f'{sls}</File>')
            return Q.Page({"width": 297, "height": 210, "plant": "A1", "location": "ST"}, f)
        # L1 on sheet 1 points to sheet 3 (not to the nearer sheet 2)
        s1, s2, s3 = page("1", [("L1", "/3.0")]), page("2", [("L1", "/9.0")]), page("3", [("L1", "/1.0")])
        pairs, going, alone = Q.arrow_pairs([s1, s2, s3])
        self.assertEqual(len(pairs), 1)
        a, b = pairs[0]
        self.assertIs(a, s1.symbols[0])
        self.assertIs(b, s3.symbols[0])
        self.assertTrue(going[id(a)])
        self.assertFalse(going[id(b)])
        self.assertEqual(alone, 1)


class Graphics(unittest.TestCase):
    def test_element_with_terminals_and_role(self):
        xml = G.element_xml("WSCAD test", [("L", (0, 0), (0, -5), False, None, {})],
                            [(0, 0, 1), (0, -5, 4)], "d", names=["A1", "A2"])
        xml = G.with_role(xml, ("master", "coil"))
        root = ET.fromstring(xml)
        self.assertEqual(root.get("link_type"), "master")
        self.assertEqual([(t.get("name"), t.get("orientation")) for t in root.iter("terminal")],
                         [("A1", "n"), ("A2", "s")])
        self.assertEqual(xml, G.with_role(G.element_xml("WSCAD test", [("L", (0, 0), (0, -5), False,
                                                                       None, {})],
                                                        [(0, 0, 1), (0, -5, 4)], "d",
                                                        names=["A1", "A2"]), ("master", "coil")))

    def test_image_sizes_without_libraries(self):
        png = b"\x89PNG\r\n\x1a\n" + b"\x00\x00\x00\rIHDR" + struct.pack(">II", 120, 40) + bytes(9)
        self.assertEqual(G.image_size(png), (120, 40))
        jpeg = (b"\xff\xd8\xff\xe0\x00\x04\x00\x00" + b"\xff\xc0\x00\x11\x08" + struct.pack(">HH", 30, 70)
                + bytes(12))
        self.assertEqual(G.image_size(jpeg), (70, 30))
        self.assertIsNone(G.image_size(b"nothing"))

    def test_empty_title_block_embedded(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "p.qet")
            with open(path, "w", encoding="utf-8") as f:
                f.write('<project>\n    <titleblocktemplates/>\n    <diagram title="a" titleblocktemplate="x">\n'
                        '        <shapes/>\n    </diagram>\n</project>\n')
            G.insert_into_project(path, {0: (["<shape/>"], [], [])})
            root = ET.parse(path).getroot()
        self.assertEqual(root.find("titleblocktemplates/titleblocktemplate").get("name"), G.EMPTY_TITLE_BLOCK)
        dia = root.find("diagram")
        self.assertEqual((dia.get("titleblocktemplate"), dia.get("titleblocktemplateCollection")),
                         (G.EMPTY_TITLE_BLOCK, "embedded"))
        self.assertEqual(len(dia.find("shapes")), 1)


class Database(unittest.TestCase):
    def test_page_order_and_files(self):
        with tempfile.TemporaryDirectory() as d:
            os.makedirs(os.path.join(d, "Database"))
            c = sqlite3.connect(os.path.join(d, "Database", "WSCAD6.db"))
            c.execute("CREATE TABLE Page (PageID INTEGER, FileName TEXT, FileType TEXT, Ref TEXT, "
                      "Width REAL, Height REAL, AspF TEXT, AspL TEXT)")
            for pid, name in ((29, "=A1+P1_PLAN.0001.WSELD"), (6, "=A1+ST_PLAN.0002.WSELD"),
                              (5, "=A1+ST_PLAN.0001.WSELD")):
                c.execute("INSERT INTO Page VALUES (?, ?, 'wsELD', '', 297, 210, 'A1', ?)",
                          (pid, name, name[4:6]))
            c.commit()
            c.close()
            for name in ("=A1+ST_Plan.0001.wsELD", "=A1+ST_Plan.0002.wsELD", "=A1+P1_Plan.0001.wsELD"):
                open(os.path.join(d, name), "wb").close()
            P = W.Project(d)
            self.assertEqual([s["name"] for s in P.schematic_pages()],
                             ["=A1+ST_PLAN.0001.WSELD", "=A1+ST_PLAN.0002.WSELD", "=A1+P1_PLAN.0001.WSELD"])
            self.assertTrue(all(os.path.isfile(s["file"]) for s in P.schematic_pages()))
            P.close()

    def test_project_folder_from_a_file(self):
        with tempfile.TemporaryDirectory() as d:
            os.makedirs(os.path.join(d, "Database"))
            db = os.path.join(d, "Database", "WSCAD6.db")
            open(db, "wb").close()
            self.assertEqual(os.path.normpath(Q.project_folder(db)), os.path.normpath(d))
            self.assertEqual(os.path.normpath(Q.project_folder(os.path.join(d, "Database"))),
                             os.path.normpath(d))
            self.assertEqual(Q.default_target(os.path.join(d, "my project")),
                             os.path.join(os.path.abspath(os.path.join(d, "my project")), "QET",
                                          "my_project.qet"))

    def test_no_project(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(FileNotFoundError):
                W.Project(d)


@unittest.skipUnless(HAVE_AES, "the Python package cryptography is not installed")
class Layout(unittest.TestCase):
    def test_operations_of_the_plan(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "demo")
            project(src, pages=(("=A1+ST_Plan.0001.wsELD", PAGE), ("=A1+ST_Plan.0002.wsELD", None)))
            with contextlib.redirect_stdout(io.StringIO()):
                plan, info = Q.plan_of(src, os.path.join(d, "out", "demo.qet"))
        ops = plan.operations()
        added = [o for o in ops if o["op"] == "add_element"]
        self.assertEqual(len(added), 2)                       # -Q1 and -K1; the corner is a wire
        self.assertTrue(all(o["path"].startswith("custom://Imported/WSCAD/") for o in added))
        self.assertEqual([o["label"] for o in ops if o["op"] == "set_label"], ["-Q1", "-K1"])
        self.assertEqual(sum(o["op"] == "add_conductor" for o in ops), 1)
        props = {o["property"]: o["value"] for o in ops if o["op"] == "set_conductor"}
        self.assertEqual(props, {"conductor_color": "BK", "conductor_section": "1.5", "function": "L1"})
        self.assertIn({"op": "set_folio_border", "folio": 0, "property": "preset", "value": "a4-landscape"},
                      ops)
        self.assertEqual(info["stats"]["drawn_route"], 1)
        # the page file that is missing is no schematic page; the frame and
        # graphics wait for the generated project
        self.assertEqual(len(info["pages"]), 1)
        files = {f for _, _, fs in os.walk(_CUSTOM.name) for f in fs}
        self.assertEqual(len([f for f in files if f.endswith(".elmt")]), 2)

    def test_unreadable_page_is_a_question(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "demo")
            project(src, pages=(("=A1+ST_Plan.0001.wsELD", PAGE), ("=A1+ST_Plan.0002.wsELD", None)))
            with open(os.path.join(src, "=A1+ST_Plan.0002.wsELD"), "wb") as f:
                f.write(b"\x01\x02\x00\x00" + bytes(200))
            with open(os.path.join(src, "=A1+ST_Plan.0002.wsELD"), "r+b") as f:
                f.seek(12)
                f.write(struct.pack("<BBBII", 1, 1, 2, 107, 50))
            with contextlib.redirect_stdout(io.StringIO()):
                plan, info = Q.plan_of(src, os.path.join(d, "out", "demo.qet"))
        self.assertEqual(len(info["pages"]), 1)
        self.assertTrue(any("cannot be read" in q for q in info["questions"]))

    def test_convert_with_a_stand_in_for_qelectrotech(self):
        """convert() end to end, QElectroTech replaced by a stand-in that
        writes a project with one folio per sheet."""
        from qet_sheet import qet_runner
        calls = []

        def project_new(output, title, folios, author="", timeout=600):
            calls.append(("new", folios))
            return {"ok": True}

        def edit(project_, operations, output, timeout=3000):
            calls.append(("edit", len(operations)))
            with open(output, "w", encoding="utf-8") as f:
                f.write('<project>\n    <diagram title="x">\n        <shapes/>\n        <inputs/>\n'
                        '        <report label="/%F/%c"/>\n    </diagram>\n</project>\n')
            return {"ok": True, "operations": []}
        saved = qet_runner.project_new, qet_runner.edit
        qet_runner.project_new, qet_runner.edit = project_new, edit
        try:
            with tempfile.TemporaryDirectory() as d:
                src = os.path.join(d, "demo")
                project(src)
                target = os.path.join(d, "out", "demo.qet")
                out = io.StringIO()
                with contextlib.redirect_stdout(out):
                    code = Q.main([os.path.join(src, "Database", "WSCAD6.db"), target])
                self.assertEqual(code, 0)
                lines = out.getvalue().splitlines()
                self.assertEqual(lines[0], "PROGRESS 2 read")
                self.assertEqual(lines[-1], "done: " + target)
                root = ET.parse(target).getroot()
                self.assertEqual(root.find("diagram").get("titleblocktemplate"), G.EMPTY_TITLE_BLOCK)
                self.assertGreater(len(root.find("diagram/shapes")), 0)      # the folio graphics
                self.assertEqual(root.find("diagram/report").get("label"), "")
                for suffix in ("_open_questions.txt", "_devices.csv", "_connections.csv"):
                    self.assertTrue(os.path.isfile(target[:-4] + suffix), suffix)
                with open(target[:-4] + "_devices.csv", encoding="utf-8-sig") as f:
                    self.assertIn("=A1+ST-Q1;ART-1;", f.read())
        finally:
            qet_runner.project_new, qet_runner.edit = saved
        self.assertEqual([c[0] for c in calls], ["new", "edit"])


if __name__ == "__main__":
    unittest.main()
