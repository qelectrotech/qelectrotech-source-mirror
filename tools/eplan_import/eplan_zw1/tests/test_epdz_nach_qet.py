"""Tests of the .epdz import (epdz_nach_qet.py) on an export archive made here, from nothing.

    python3 tools/eplan_import/eplan_zw1/tests/test_epdz_nach_qet.py

The archive is built by make_export(): a manifest.db written with sqlite3 and two page SVGs drawn by hand --
a coil -K1 wired to a switch -S1 and to an interruption point on page 1, its partner, a contact of -K1 and a
lamp -H1 on page 2. No real EPLAN project is involved.

The unit tests need nothing else. The import itself needs a QElectroTech built with scripting: set
QET_BINARY to it (ctest does), or they are skipped. 7-Zip, when there is one, is used to pack the folder into
an .epdz once, so that the unpacking path is run too.
"""
import contextlib
import io
import os
import sqlite3
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from unittest import mock

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import eplan_grafik as G      # noqa: E402
import epdz_nach_qet as E     # noqa: E402
import qet_backend            # noqa: E402

SVG_HEAD = ('<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
            'viewBox="0 0 420 297"><style>.P1{stroke-width:0.25;fill:none}.F1{font-size:2.2356px}</style>')
FRAME = ('<g><desc>Frame</desc><path class="P1" d="M5 5 L415 5 L415 292 L5 292 Z"/>'
         '<text class="F1" transform="translate(300 20) scale(1,-1)">Test frame</text></g>')

# page 1: -K1 coil (A1 top at (105,210), A2 bottom at (105,190)), -S1 switch (13 at (150,230), 14 at (150,210)
# left open), an interruption point at (105,170) whose jump target is the one on page 2
PAGE_1 = SVG_HEAD + FRAME + (
    '<g id="Id17_100"><title>-K1</title>'
    '<path class="P1" d="M105 210 L105 205"/><path class="P1" d="M100 205 L110 205 L110 195 L100 195 Z"/>'
    '<path class="P1" d="M105 195 L105 190"/>'
    '<text class="F1" transform="translate(112 202) scale(1,-1)">-K1</text>'
    '<text class="F1" transform="translate(106 209.5) scale(1,-1)">A1</text>'
    '<text class="F1" transform="translate(106 192.5) scale(1,-1)">A2</text></g>'
    '<g id="Id17_200"><title>-S1</title>'
    '<path class="P1" d="M150 230 L150 225 L146 217"/><path class="P1" d="M150 215 L150 210"/>'
    '<text class="F1" transform="translate(153 222) scale(1,-1)">-S1</text>'
    '<text class="F1" transform="translate(151 229.5) scale(1,-1)">13</text></g>'
    '<g id="Id70_300"><title>L1</title><a xlink:href="javascript:jump(\'Id70_300\',\'Id70_301\')">'
    '<path class="P1" d="M104 171 L105 169 L106 171"/></a></g>'
    # the conductors: up and across to -S1, down to the interruption point
    '<path class="P1" d="M105 210 L105 230 L150 230"/>'
    '<path class="P1" d="M105 190 L105 170"/>'
    '</svg>')

# page 2: the partner arrow at (50,250), the NO contact of -K1 (13 at (80,250), 14 at (80,235)), the lamp -H1
# (X1 at (100,222)); the conductor from the contact to the lamp bends with a short slant, as EPLAN draws a
# branch
PAGE_2 = SVG_HEAD + FRAME + (
    '<g id="Id70_301"><title>L1</title><path class="P1" d="M49 251 L51 250 L49 249"/></g>'
    '<g id="Id17_101"><title>-K1</title>'
    '<path class="P1" d="M80 250 L80 245 L76 238"/><path class="P1" d="M80 240 L80 235"/>'
    '<text class="F1" transform="translate(83 243) scale(1,-1)">-K1</text>'
    '<text class="F1" transform="translate(81 249.5) scale(1,-1)">13</text>'
    '<text class="F1" transform="translate(81 236.5) scale(1,-1)">14</text></g>'
    '<g id="Id17_103"><title>-H1</title>'
    '<path class="P1" d="M100 222 L100 218"/><circle class="P1" cx="100" cy="215" r="3"/></g>'
    '<path class="P1" d="M51 250 L80 250"/>'
    '<path class="P1" d="M80 235 L80 224 L82 222 L100 222"/>'
    '</svg>')


def make_export(folder):
    """Write an unpacked export archive into @p folder; -> folder."""
    pages = os.path.join(folder, "packages", "pages", "items", "pagesvg")
    spaces = os.path.join(folder, "packages", "installationspaces", "items", "installationspacee3d")
    os.makedirs(pages)
    os.makedirs(spaces)
    with open(os.path.join(pages, "1.svg"), "w", encoding="utf-8") as f:
        f.write(PAGE_1)
    with open(os.path.join(pages, "2.svg"), "w", encoding="utf-8") as f:
        f.write(PAGE_2)
    with open(os.path.join(spaces, "space.e3d"), "wb") as f:
        f.write(b"E3D test")
    db = sqlite3.connect(os.path.join(folder, "manifest.db"))
    db.executescript("""
        CREATE TABLE package (id INTEGER PRIMARY KEY, name TEXT, type TEXT);
        CREATE TABLE property (packageid INTEGER, propid INTEGER, propindex INTEGER, value TEXT);
        CREATE TABLE item (packageid INTEGER, locator TEXT);
        CREATE TABLE page_package (packageid INTEGER, name TEXT, ep1440 TEXT, ep1240 TEXT, ep1640 TEXT);
        CREATE TABLE function_package (packageid INTEGER);
        CREATE TABLE mergedconnection_package (packageid INTEGER);
        CREATE TABLE installationspace_package (packageid INTEGER, name TEXT);
    """)
    packages = [(1, "Test project", "project"), (2, "1", "page"), (3, "2", "page"),
                (10, "-K1", "function"), (11, "-K1", "function"), (12, "-S1", "function"),
                (13, "-H1", "function"), (14, "L1", "interruptionpoint"), (15, "L1", "interruptionpoint"),
                (20, "W1", "mergedconnection"), (21, "W2", "mergedconnection"), (30, "Cabinet", "installationspace")]
    db.executemany("INSERT INTO package VALUES (?, ?, ?)", packages)
    props = {
        2: {1140: "=A1", 1240: "+S1", 11000: "1", 11011: "Control"},
        3: {1140: "=A1", 1240: "+S1", 11000: "2", 11011: "Lamp"},
        10: {2000: "253/17/100/0", 20001: "-K1", 20006: "-K1", 20188: "200/1", 20122: "1", 20038: "A1;A2",
             20215: "-K1", 20026: "Coil", 20100: "SIE.3RT2015-1BB41", 20193: "Contactor"},
        11: {2000: "253/17/101/0", 20001: "-K1", 20006: "-K1", 20188: "201/1", 20038: "13;14", 20215: "-K1",
             20026: "NO contact"},
        12: {2000: "253/17/200/0", 20001: "-S1", 20006: "-S1", 20188: "201/1", 20038: "13;14", 20215: "-S1",
             20026: "Push button"},
        13: {2000: "253/17/103/0", 20001: "-H1", 20006: "-H1", 20188: "1300/1", 20038: "X1;X2", 20215: "-H1",
             20026: "Lamp"},
        14: {2000: "253/70/300/0"},
        15: {2000: "253/70/301/0"},
        # the two conductors differ in colour and potential: a master/slave link must not mind that
        20: {31019: "-K1:A1", 31020: "-S1:13", 31004: "BK", 31007: "1.5", 33003: "L1", 31003: "0.4 m"},
        21: {31019: "-K1:14", 31020: "-H1:X1", 31004: "BU", 31007: "0.75", 33003: "N"},
    }
    for pid, values in props.items():
        db.executemany("INSERT INTO property VALUES (?, ?, 0, ?)", [(pid, k, v) for k, v in values.items()])
    db.executemany("INSERT INTO page_package VALUES (?, ?, ?, ?, ?)",
                   [(2, "1", "==A1", "+S1", "1"), (3, "2", "==A1", "+S1", "2")])
    db.executemany("INSERT INTO item VALUES (?, ?)",
                   [(2, "packages/pages/items/pagesvg/1.svg"), (3, "packages/pages/items/pagesvg/2.svg"),
                    (30, "packages/installationspaces/items/installationspacee3d/space.e3d")])
    db.executemany("INSERT INTO function_package VALUES (?)", [(10,), (11,), (12,), (13,)])
    db.executemany("INSERT INTO mergedconnection_package VALUES (?)", [(20,), (21,)])
    db.execute("INSERT INTO installationspace_package VALUES (30, 'Cabinet')")
    db.commit()
    db.close()
    return folder


class Geometry(unittest.TestCase):
    def test_path_commands(self):
        self.assertEqual(E.pfad("M0 0 L10 0 V5 H0 Z"), [([(0, 0), (10, 0), (10, 5), (0, 5), (0, 0)], True)])
        arc = E.pfad("M10 0 A10 10 0 0 1 -10 0")[0][0]
        self.assertEqual(arc[-1], (-10, 0))
        for x, y in arc:
            self.assertAlmostEqual(x * x + y * y, 100, delta=1e-6)

    def test_slanted_branch_becomes_a_corner(self):
        self.assertEqual(E.ecken([(80, 235), (80, 224), (82, 222), (100, 222)]), [(80, 222)])
        self.assertEqual(E.ecken([(96, 60), (96, 70), (98, 72), (98, 90)]), [(96, 72), (98, 72)])
        self.assertEqual(E.ecken([(0, 0), (0, 10), (10, 10)]), [(0, 10)])

    def test_nets_join_at_t_junctions_only(self):
        lines = [([(0, 0), (10, 0)], False, {}), ([(5, 0), (5, 5)], False, {}), ([(20, -5), (20, 5)], False, {}),
                 ([(15, 0), (25, 0)], False, {})]
        v = E.netze(lines)
        k = E._schluessel
        self.assertEqual(v.finde(k((0, 0))), v.finde(k((5, 5))))          # T junction
        self.assertNotEqual(v.finde(k((20, 5))), v.finde(k((25, 0))))     # crossing without a shared point

    def test_text_position_and_angle(self):
        (x, y), a = E._textlage("rotate(90 10 10) translate(20 10) scale(1,-1)")
        self.assertAlmostEqual(x, 10)
        self.assertAlmostEqual(y, 20)
        self.assertAlmostEqual(a, 1.5707963, places=6)

    def test_element_terminals_and_role(self):
        xml = G.with_role(G.element_xml("T", [("L", (0, 0), (0, 5), False, None, {})], [(0, 0, 1), (0, 5, 4)],
                                        names=["A1", "A2"]), ("master", "coil"))
        root = ET.fromstring(xml)
        self.assertEqual(root.get("link_type"), "master")
        self.assertEqual([t.get("name") for t in root.iter("terminal")], ["A1", "A2"])
        self.assertEqual([t.get("orientation") for t in root.iter("terminal")], ["n", "s"])

    def test_png_size_without_pymupdf(self):
        png = b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x08\x00\x00\x00\x04"
        self.assertEqual(G._png_size(png), (8, 4))


class Reading(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.x = make_export(os.path.join(self.tmp.name, "x"))

    def tearDown(self):
        self.tmp.cleanup()

    def test_manifest(self):
        D = E.Daten(os.path.join(self.x, "manifest.db"))
        self.assertEqual([s["name"] for s in D.seiten], ["1", "2"])
        self.assertEqual(D.seiten[0]["titel"], "Control")
        self.assertEqual(D.funktion("17_100")[20001], "-K1")
        self.assertEqual(len(D.verbindungen), 2)
        self.assertEqual(D.verbindungen[0]["farbe"], "BK")
        rolle, _ = E._rollen(D, {"17_100", "17_101", "17_200", "17_103"})
        self.assertEqual(rolle["17_100"], ("master", "coil"))
        self.assertEqual(rolle["17_101"], ("slave", "NO", "simple"))
        self.assertNotIn("17_200", rolle)            # a switch with no main function of its tag

    def test_terminals_and_names(self):
        D = E.Daten(os.path.join(self.x, "manifest.db"))
        stat = __import__("collections").Counter()
        se = E.Seite(D.seiten[0], *E.seite_lesen(os.path.join(self.x, "packages", "pages", "items", "pagesvg",
                                                                "1.svg")), D, stat)
        se.namen(D.verbindungen)
        k1 = "Id17_100"
        names = {se.anschl[k1][i][0]: se.pins[(k1, i)] for i in range(len(se.anschl[k1]))}
        self.assertEqual(names, {(105.0, 210.0): "A1", (105.0, 190.0): "A2"})
        self.assertEqual([p for p, _ in se.anschl["Id70_300"]], [(105.0, 170.0)])
        edges = se.kanten(D.verbindungen)
        self.assertEqual(len(edges), 2)
        self.assertEqual(stat["verbindungen_aus_liste"], 1)
        self.assertEqual(se.weg((105.0, 210.0), (150.0, 230.0)), [(105.0, 210.0), (105.0, 230.0), (150.0, 230.0)])


def _binary():
    path = os.environ.get("QET_BINARY", "").strip()
    return path if path and os.path.isfile(path) else None


@unittest.skipUnless(_binary(), "QET_BINARY is not set: the import needs a QElectroTech built with scripting")
class Import(unittest.TestCase):
    """The whole import on the archive, through the headless QElectroTech."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        base = cls.tmp.name
        cls.collection = os.path.join(base, "collection")
        os.makedirs(cls.collection)
        cls.env = mock.patch.dict(os.environ, {
            "QET_ENABLE_SCRIPTING": "1",
            "QET_CUSTOM_ELEMENTS_DIR": cls.collection,
            # nothing of the user's own QElectroTech: no qet-assistant.json, no log, no working copies
            "QET_MCP_INFO_FILE": os.path.join(base, "no-assistant.json"),
            "QET_LOG_DISABLE": "1",
            "QET_WORKING_COPY_DIR": os.path.join(base, "working-copies")})
        cls.env.start()
        cls.x = make_export(os.path.join(base, "export"))
        cls.target = os.path.join(base, "out", "Test project.qet")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = E.main([os.path.join(base, "Test project.epdz"), cls.target, "--unpacked", cls.x])
        cls.output = out.getvalue()
        cls.code = code
        cls.root = ET.parse(cls.target).getroot() if os.path.isfile(cls.target) else None

    @classmethod
    def tearDownClass(cls):
        cls.env.stop()
        cls.tmp.cleanup()

    def folios(self):
        return self.root.findall("diagram")

    def test_done_and_progress(self):
        self.assertEqual(self.code, 0, self.output)
        self.assertIn("done: " + self.target, self.output)
        self.assertIn("PROGRESS 100 done", self.output)
        self.assertIsNotNone(self.root, self.output)

    def test_folios_and_title_block(self):
        folios = self.folios()
        self.assertEqual([f.get("title") for f in folios], ["Control", "Lamp"])
        self.assertEqual({f.get("plant") for f in folios}, {"A1"})
        self.assertEqual({f.get("locmach") for f in folios}, {"S1"})
        self.assertEqual({f.get("titleblocktemplate") for f in folios}, {"eplan_import_blank"})
        names = [t.get("name") for t in self.root.iter("titleblocktemplate")]
        self.assertIn("eplan_import_blank", names)

    def test_parts_and_conductors(self):
        page1, page2 = self.folios()
        self.assertEqual(len(page1.find("elements")), 3)          # -K1, -S1, interruption point
        self.assertEqual(len(page2.find("elements")), 3)          # interruption point, -K1 contact, -H1
        self.assertEqual(len(page1.find("conductors")), 2, self.output)
        self.assertEqual(len(page2.find("conductors")), 2, self.output)
        labels = {e.findtext("elementInformations/elementInformation[@name='label']")
                  for f in self.folios() for e in f.find("elements")}
        self.assertTrue({"-K1", "-S1", "-H1"} <= labels, labels)
        # the parts are in the collection given for the run, and in the project
        made = [n for _, _, fs in os.walk(self.collection) for n in fs if n.endswith(".elmt")]
        self.assertGreaterEqual(len(made), 5)

    def test_conductor_follows_the_drawn_path(self):
        page1 = self.folios()[0]
        lengths = [float(s.get("length")) for c in page1.find("conductors") for s in c.iter("segment")
                   if s.get("orientation") == "horizontal"]
        self.assertTrue(any(abs(abs(v) - 45 * G.K) < 1.5 for v in lengths), lengths)
        props = {(c.get("conductor_color"), c.get("function")) for c in page1.find("conductors")}
        self.assertIn(("BK", "L1"), props)

    def test_master_slave_and_reports_linked(self):
        def links(folio):
            return {e.findtext("elementInformations/elementInformation[@name='label']") or e.get("type"):
                    len(list(e.iter("link_uuid"))) for e in folio.find("elements")}
        linked = sum(len(list(e.iter("link_uuid"))) for f in self.folios() for e in f.find("elements"))
        # coil <-> contact (2) and the two report arrows (2)
        self.assertEqual(linked, 4, (links(self.folios()[0]), links(self.folios()[1]), self.output))

    def test_lists_beside_the_project(self):
        base = os.path.splitext(self.target)[0]
        with open(base + "_devices.csv", encoding="utf-8-sig") as f:
            devices = f.read()
        self.assertIn("-K1;Coil;SIE.3RT2015-1BB41;Contactor", devices)
        with open(base + "_connections.csv", encoding="utf-8-sig") as f:
            self.assertIn("-K1:A1;-S1:13;L1;BK;1.5;0.4 m", f.read())
        self.assertTrue(os.path.isfile(os.path.join(base + "_3D", "space.e3d")))


@unittest.skipUnless(_binary() and qet_backend.seven_zip(), "needs QET_BINARY and a 7-Zip")
class PackedArchive(unittest.TestCase):
    """The archive as an .epdz file, unpacked by the program itself, the parts left out of the collection."""

    def test_packed(self):
        with tempfile.TemporaryDirectory() as base:
            collection = os.path.join(base, "collection")
            os.makedirs(collection)
            env = {"QET_ENABLE_SCRIPTING": "1", "QET_CUSTOM_ELEMENTS_DIR": collection,
                   "QET_MCP_INFO_FILE": os.path.join(base, "no-assistant.json"), "QET_LOG_DISABLE": "1",
                   "QET_WORKING_COPY_DIR": os.path.join(base, "working-copies")}
            x = make_export(os.path.join(base, "export"))
            epdz = os.path.join(base, "packed.epdz")
            subprocess.run([qet_backend.seven_zip(), "a", "-t7z", epdz, "."], cwd=x, check=True,
                           stdout=subprocess.DEVNULL)
            out = io.StringIO()
            with mock.patch.dict(os.environ, env), contextlib.redirect_stdout(out):
                code = E.main([epdz, "--no-collection"])
            self.assertEqual(code, 0, out.getvalue())
            self.assertTrue(os.path.isfile(os.path.join(base, "packed.qet")), out.getvalue())
            self.assertEqual([n for _, _, fs in os.walk(collection) for n in fs], [])
            self.assertFalse(os.path.isdir(os.path.join(base, "_epdz_import_packed", "x")))


class CommandLine(unittest.TestCase):
    def test_default_target(self):
        self.assertEqual(E.standard_ziel(os.path.join("a", "Plan 1.epdz")),
                         os.path.abspath(os.path.join("a", "Plan 1.qet")))

    def test_check_names_what_is_missing(self):
        with mock.patch.dict(os.environ, {"QET_MCP": os.path.join(HERE, "missing.py")}), \
                mock.patch.object(qet_backend, "_MCP", None), \
                mock.patch.object(qet_backend, "mcp_path", lambda: None):
            ok, message = E.pruefen()
        self.assertFalse(ok)
        self.assertIn("qet_mcp.py", message)


if __name__ == "__main__":
    unittest.main()
