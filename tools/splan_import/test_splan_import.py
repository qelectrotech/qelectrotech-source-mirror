"""Tests of the sPlan import without sPlan and without QElectroTech: the
schematics are written here byte by byte, as FORMAT_SPL8.md describes them.

    python -m unittest discover -s tools/splan_import -p "test_*.py"

The layout test needs the common collection of the source tree (the
"elements" submodule); it is skipped without it.
"""
import contextlib
import io
import json
import os
import struct
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
COMMON = os.path.normpath(os.path.join(HERE, "..", "..", "elements"))
_CUSTOM = tempfile.TemporaryDirectory(prefix="splan-test-custom-")
# the collections, before qet_sheet reads them
os.environ["QET_COMMON_ELEMENTS_DIR"] = COMMON
os.environ["QET_CUSTOM_ELEMENTS_DIR"] = _CUSTOM.name
os.environ.pop("QET_COMPANY_ELEMENTS_DIR", None)
sys.path.insert(0, HERE)

import part_mapping as M  # noqa: E402
import splan8  # noqa: E402
import splan_analysis as A  # noqa: E402
import splan_to_qet as Q  # noqa: E402


# --- an sPlan 8 writer, the reader's sections in reverse ------------------------
def i32(v):
    return struct.pack("<i", v)


def f32(v):
    return struct.pack("<f", v)


def ws(s):
    return i32(len(s)) + s.encode("utf-16le")


def header():
    return bytes(3) + bytes(11) + bytes(4) + bytes(4) + b"\x00"


def points(pts):
    return b"".join(f32(x) + f32(y) for x, y in pts)


def text_body(text, pts, font="Arial"):
    return (header() + points(pts) + ws(text) + i32(0) + bytes([len(font)]) + font.encode("latin1")
            + bytes(14) + ws("") + bytes(16) + bytes(17) + bytes(5))


def box(x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def text(t, x, y, w=60, h=25):
    """A horizontal text with its top left corner at x/y."""
    return b"\x03" + text_body(t, box(x, y, x + w, y + h))


def contact(t, x, y):
    return b"\x0b" + text_body(t, box(x - 10, y - 10, x + 10, y + 10)) + ws(t)


def line(pts, arrow_start=0, arrow_end=0):
    tail = bytearray(21)
    tail[5], tail[6] = arrow_start, arrow_end
    return b"\x04" + header() + i32(len(pts) - 1) + points(pts) + bytes(tail)


def rectangle(x0, y0, x1, y1):
    return b"\x02" + header() + points(box(x0, y0, x1, y1) + [(x0, y0)]) + bytes(31)


def dot(x, y):
    return b"\x08" + header() + points([(x, y)])


def part(children, tag, value, symbol, ident=0, parent=0, pos=(0, 0)):
    x, y = pos
    return (b"\x07" + i32(len(children) - 1) + b"".join(children)
            + text_body(tag, box(x, y - 40, x + 40, y - 15)) + ws(tag)
            + text_body(value, box(x, y - 70, x + 40, y - 45))
            + bytes(4) + ws(symbol) + ws("K") + i32(-1) + f32(0) + f32(0) + i32(0) + b"\x00"
            + ws("") * 4 + b"\x00" + i32(ident) + i32(parent) + b"\x00")


def schematic(sheets):
    """sheets: [(name, [object bytes])]"""
    b = splan8.MAGIC + b"\x46\x40" + bytes(6) + b"\x00" + ws("") + ws("") + i32(0) + i32(0)
    for name, objects in sheets:
        b += (ws(name) + i32(297) + i32(210) + i32(10) + i32(0) + i32(0) + f32(1.0)
              + i32(len(objects) - 1) + b"".join(objects))
        b += b"\xff" * 104                             # the sheet footer (section 11)
    return b


def control_circuit():
    """A rail +24V on top, 0V at the bottom; contact K1 (13/14) switches the
    coil of K1 (A1/A2). A wire with a junction dot on the 24 V rail, one
    that only crosses it."""
    contact_s = part([rectangle(500, 600, 560, 700), contact("13", 530, 610),
                      contact("14", 530, 690)], "K1", "", "Schliesser", ident=11, parent=10,
                     pos=(500, 600))
    coil = part([rectangle(1000, 900, 1100, 1000), contact("A1", 1050, 910),
                 contact("A2", 1050, 990)], "K1", "Schütz", "Spule", ident=10, pos=(1000, 900))
    return [
        line([(200, 400), (2700, 400)]), text("+24V", 120, 370),
        line([(200, 1100), (2700, 1100)]), text("0V", 140, 1070, w=50),
        contact_s, coil,
        line([(530, 400), (530, 600)]),
        line([(530, 700), (530, 850), (1050, 850), (1050, 900)]),
        line([(1050, 1000), (1050, 1100)]),
        line([(1500, 300), (1500, 700)]), dot(1500, 400),          # joined by the dot
        line([(1800, 300), (1800, 700)]),                          # only crosses
    ]


class Reader(unittest.TestCase):
    def test_reads_what_was_written(self):
        pr = splan8.read(schematic([("Control", control_circuit()), ("Second", [text("x", 300, 300)])]))
        self.assertEqual([s.name for s in pr.sheets], ["Control", "Second"])
        kinds = [o["kind"] for o in pr.sheets[0].objects]
        self.assertEqual(kinds.count("line"), 7)
        self.assertEqual(kinds.count("part"), 2)
        self.assertEqual(kinds.count("dot"), 1)
        coil = [o for o in pr.sheets[0].objects if o["kind"] == "part"][1]
        self.assertEqual((coil["tag"], coil["value"], coil["symbol"], coil["id"]),
                         ("K1", "Schütz", "Spule", 10))
        self.assertEqual([k["name"] for k in splan8.walk(coil) if k["kind"] == "contact"],
                         ["A1", "A2"])

    def test_arrow_ends(self):
        pr = splan8.read(schematic([("A", [line([(300, 300), (300, 500)], arrow_end=2)])]))
        o = pr.sheets[0].objects[0]
        self.assertEqual((o["arrow_start"], o["arrow_end"]), (0, 2))

    def test_refuses_other_files(self):
        with self.assertRaises(splan8.FormatError):
            splan8.read(b"PK\x03\x04 not a schematic")
        whole = schematic([("A", control_circuit())])
        with self.assertRaises(splan8.FormatError):
            splan8.read(whole[:300])
        broken = bytearray(whole)
        first = whole.index(b"\x04", 16 + 30)             # the type byte of the first object
        broken[first] = 0x63
        with self.assertRaises(splan8.FormatError):
            splan8.read(bytes(broken))


class Analysis(unittest.TestCase):
    def setUp(self):
        self.c = A.analyse(splan8.read(schematic([("Control", control_circuit())])).sheets[0], 1)
        self.parts = {p.symbol: p for p in self.c.parts}

    def net_of(self, symbol, name):
        return next(t.net for t in self.parts[symbol].terminals if t.name == name)

    def test_terminals_are_named_by_the_contacts(self):
        self.assertEqual(sorted(t.name for t in self.parts["Spule"].terminals), ["A1", "A2"])
        self.assertEqual(sorted(t.name for t in self.parts["Schliesser"].terminals), ["13", "14"])
        sides = {t.name: t.side for t in self.parts["Spule"].terminals}
        self.assertEqual(sides, {"A1": "top", "A2": "bottom"})

    def test_nets_from_the_geometry(self):
        self.assertEqual(self.net_of("Schliesser", "13").name, "+24V")
        self.assertEqual(self.net_of("Spule", "A2").name, "0V")
        self.assertIs(self.net_of("Schliesser", "14"), self.net_of("Spule", "A1"))
        rail = self.net_of("Schliesser", "13")
        xs = {round(p[0]) for w in rail.wires for p in w["pts"]}
        self.assertIn(1500, xs)                            # the wire with the junction dot
        self.assertNotIn(1800, xs)                         # a crossing is no connection

    def test_classify(self):
        self.assertEqual(Q.classify(self.parts["Spule"]), "coil")
        self.assertEqual(Q.classify(self.parts["Schliesser"]), "no")

    def test_unnamed_arrow_is_a_question(self):
        c = A.analyse(splan8.read(schematic([("A", [line([(300, 300), (300, 500)], arrow_end=2)])]))
                      .sheets[0], 1)
        self.assertTrue(any("no name" in q for q in c.questions))


class Mapping(unittest.TestCase):
    def test_rule_and_template(self):
        c = A.analyse(splan8.read(schematic([("Control", control_circuit())])).sheets[0], 1)
        coil = next(p for p in c.parts if p.symbol == "Spule")
        with tempfile.TemporaryDirectory() as d:
            path = M.write_template(os.path.join(d, "t.json"), [(1, c)])
            with open(path, encoding="utf-8") as f:
                data = json.load(f)
            self.assertEqual(len(data["devices"]), 2)
            self.assertIn("A1", next(x for x in data["devices"]
                                     if x["_sPlan"]["symbol"] == "Spule")["_sPlan"]["terminals"])
            data["devices"] = [{"when": {"sheet": 1, "tag": "K1", "symbol": "Spule"},
                                "manufacturer": "Maker", "reference": "R-1", "standard": True}]
            with open(path, "w", encoding="utf-8") as f:
                json.dump(data, f)
            m = M.Mapping(path)
        self.assertIsNone(m.find(coil, 2))
        r = m.find(coil, 1)
        self.assertEqual(r.info(), {"manufacturer": "Maker", "manufacturer_reference": "R-1"})
        self.assertEqual(m.unused(), [])


@unittest.skipUnless(os.path.isdir(os.path.join(COMMON, "10_electric")),
                     "the common collection (elements submodule) is not checked out")
class Layout(unittest.TestCase):
    def test_operations_of_the_plan(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "control.spl8")
            with open(src, "wb") as f:
                f.write(schematic([("Control", control_circuit())]))
            with contextlib.redirect_stdout(io.StringIO()):
                plan, meta, sheets = Q.plan_of(src, os.path.join(d, "QET", "control.qet"))
        ops = plan.operations()
        paths = [o["path"] for o in ops if o["op"] == "add_element"]
        self.assertIn(Q.S.COIL, paths)
        self.assertIn(Q.S.NO_CONTACT, paths)
        self.assertEqual(sorted(plan.sheets[0].rails), ["+24V", "0V"])
        links = [o for o in ops if o["op"] == "link_elements"]
        self.assertEqual(len(links), 1)                     # the contact to its coil
        self.assertTrue(any(o["op"] == "add_conductor" for o in ops))
        # the helpers came into the user collection, nothing else was written
        written = {f for _, _, fs in os.walk(_CUSTOM.name) for f in fs}
        self.assertTrue({"tap_down.elmt", "tap_up.elmt"} & written)
        self.assertEqual(sheets, [(1, "Control", 0)])

    def test_mapping_template_from_the_command_line(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "control.spl8")
            with open(src, "wb") as f:
                f.write(schematic([("Control", control_circuit())]))
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                code = Q.main([src, os.path.join(d, "c.qet"), "--mapping-template"])
            self.assertEqual(code, 0)
            lines = out.getvalue().splitlines()
            self.assertEqual(lines[0], "PROGRESS 2 read")
            self.assertTrue(lines[-1].startswith("done: "))
            self.assertTrue(os.path.isfile(lines[-1][len("done: "):]))


if __name__ == "__main__":
    unittest.main()
