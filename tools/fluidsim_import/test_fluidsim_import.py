"""Tests of the FluidSIM import without a FluidSIM installation: every circuit here is built by the test itself.

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

    python3 -m unittest tools/fluidsim_import/test_fluidsim_import.py
    QET_BINARY=/path/to/qelectrotech python3 -m unittest tools/fluidsim_import/test_fluidsim_import.py

With QET_BINARY the import also writes a real project through that QElectroTech.
"""
import gzip
import os
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import fluidsim_import as fi  # noqa: E402
import fluidsim_elements as fe  # noqa: E402
import fluidsim_grafcet as fg  # noqa: E402
import own_graphics as og  # noqa: E402

QET_BINARY = os.environ.get("QET_BINARY", "")


def packed(*values):
    return struct.pack("<%dd" % len(values), *values).hex().upper()


# A small electro-pneumatic circuit as FluidSIM 6 writes it: a 4/2 valve with a solenoid on the left and a spring
# on the right, a silencer on its port 3, a double-acting cylinder on 4 and 2, the air supply on 1, the valve
# solenoid -MB1 (not wired), a relay -KF1 with a contact -KF1, and a GRAFCET with two steps and a transition.
# The valve symbol also carries a line and a text of its own: neither may reach the imported project.
CIRCUIT = """<?xml version="1.0" encoding="UTF-8"?>
<fluidtools2>
 <section type="symbol-graphics">
  <symbol ident="S1">
   <symbol-layer ident="StaticLayer"><line x1="-777" y1="0" x2="777" y2="0"/><text x1="0" y1="0">VENDOR ART</text></symbol-layer>
   <symbol-layer ident="CON1">
    <connector type="pneumatic" ident="4" designation="4" cx="6" cy="-18"/>
    <connector type="pneumatic" ident="2" designation="2" cx="30" cy="-18"/>
    <connector type="pneumatic" ident="1" designation="1" cx="6" cy="18"/>
    <connector type="pneumatic" ident="3" designation="3" cx="30" cy="18"/>
   </symbol-layer>
   <symbol-layer ident="B4_1" ref-x="0" ref-y="0">
    <rect x1="0" y1="-12" width="36" height="24"/>
    <line x1="6" y1="12" x2="6" y2="-12" end-cap="arrow1"/>
    <line x1="30" y1="-12" x2="30" y2="12" end-cap="arrow1"/>
   </symbol-layer>
   <symbol-layer ident="B4_8" ref-x="36" ref-y="0">
    <rect x1="0" y1="-12" width="36" height="24"/>
    <line x1="6" y1="12" x2="30" y2="-12" end-cap="arrow1"/>
    <line x1="6" y1="-12" x2="30" y2="12" end-cap="arrow1"/>
   </symbol-layer>
   <symbol-layer ident="AEML1" ref-x="-10" ref-y="0"><connector type="electric-label" ident="9" designation="-MB1" cx="0" cy="0"/></symbol-layer>
  </symbol>
  <symbol ident="S2"><symbol-layer ident="L">
   <connector type="electric" ident="1" cx="0" cy="-12"/><connector type="electric" ident="2" cx="0" cy="12"/>
  </symbol-layer></symbol>
  <symbol ident="S3"><symbol-layer ident="L">
   <rect x1="-30" y1="-12" width="60" height="24"/>
   <connector type="pneumatic" ident="A" cx="-24" cy="12"/><connector type="pneumatic" ident="B" cx="12" cy="12"/>
  </symbol-layer></symbol>
  <symbol ident="S4"><symbol-layer ident="L"><connector type="pneumatic" ident="1" cx="0" cy="0"/></symbol-layer></symbol>
  <symbol ident="S5"><symbol-layer ident="L">
   <circle cx="0" cy="0" r="10"/><connector type="pneumatic" ident="1" designation="1" cx="0" cy="-12"/>
  </symbol-layer></symbol>
  <symbol ident="S6"><symbol-layer ident="L">
   <connector type="electric" ident="1" designation="13" cx="0" cy="-12"/>
   <connector type="electric" ident="2" designation="14" cx="0" cy="12"/>
  </symbol-layer></symbol>
 </section>
 <section type="circuit-objects">
  <component ident="O1" object-ref="Pneu.DirectionalValve.Valve42" symbol-ref="S1" x1="100" y1="100" width="72" height="36">
   <parameter ident="posNoMax" value="2"/><parameter ident="posNoInit" value="1"/><parameter ident="springR" value="True"/>
   <connector ident="1" con-ref="CON1.1"/><connector ident="2" con-ref="CON1.2"/>
   <connector ident="3" con-ref="CON1.3"/><connector ident="4" con-ref="CON1.4"/>
   <dki-data><description value="4/n way valve"/></dki-data>
  </component>
  <component ident="O2" object-ref="ET.Relay.ValveSolenoid" symbol-ref="S2" x1="300" y1="100" width="24" height="24">
   <classification-data><type value="solenoid-valve-coil"/></classification-data>
   <dki-data><designation value="-MB1"/><description value="Valve solenoid"/></dki-data>
  </component>
  <component ident="O3" object-ref="Pneu.Actuator.DoubleActingCYL" symbol-ref="S3" x1="100" y1="0" width="60" height="24">
   <connector ident="A" con-ref="L.A"/><connector ident="B" con-ref="L.B"/>
   <dki-data><description value="Double-acting cylinder"/></dki-data>
  </component>
  <component ident="O4" object-ref="Pneu.Supply.Silencer" symbol-ref="S4" x1="160" y1="130" width="12" height="12"/>
  <component ident="O5" object-ref="Pneu.Supply.AirSupply" symbol-ref="S5" x1="94" y1="160" width="24" height="24">
   <connector ident="1" con-ref="L.1"/>
   <dki-data><description value="Compressed air supply"/></dki-data>
  </component>
  <component ident="O6" object-ref="ET.Relay.Relay" symbol-ref="S2" x1="400" y1="100" width="24" height="24">
   <classification-data><type value="solenoid-coil"/></classification-data>
   <dki-data><designation value="-KF1"/><description value="Relay"/></dki-data>
  </component>
  <component ident="O7" object-ref="ET.Switch.MakeSwitch" symbol-ref="S6" x1="450" y1="100" width="24" height="24">
   <classification-data><type value="make-contact"/></classification-data>
   <dki-data><designation value="-KF1"/><description value="Make switch"/></dki-data>
  </component>
  <component ident="O20" object-ref="Grafcet.Step" symbol-ref="SG" x1="600" y1="0" width="24" height="24"><const ident="name" value="1"/><const ident="flags" value="initial"/></component>
  <component ident="O21" object-ref="Grafcet.Transition" symbol-ref="SG" x1="600" y1="40" width="24" height="12"><const ident="condition" value="RE -SF1.signal"/></component>
  <component ident="O22" object-ref="Grafcet.Step" symbol-ref="SG" x1="600" y1="80" width="24" height="24"><const ident="name" value="2"/></component>
  <connect con1="O1.4" con2="O3.A"/>
  <connect con1="O1.2" con2="O3.B"/>
  <connect con1="O5.1" con2="O1.1"/>
  <connect con1="O20.GC_S_out" con2="O21.GC_T_in"/>
  <connect con1="O21.GC_T_out" con2="O22.GC_S_in"/>
 </section>
</fluidtools2>
"""


def write_circuit(folder: Path, name="test.circ", text=CIRCUIT):
    path = folder / name
    path.write_bytes(gzip.compress(text.encode("utf-8")))
    return path


PROBE = r"""
import subprocess, sys, tempfile, time
sys.path.insert(0, sys.argv[1])
import fluidsim_import as fi
fi.stop_on_eof()
try:
    with tempfile.TemporaryDirectory(prefix="fluidsim-import-", dir=sys.argv[2]) as tmp:
        child = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        print("ready", child.pid, flush=True)
        child.wait()
        time.sleep(60)
except KeyboardInterrupt:
    print("stopped", flush=True)
    sys.exit(3)
"""


class StopOnEof(unittest.TestCase):
    """--stop-on-eof: closing standard input ends the run, its child processes and its temporary folders."""

    def test_cancel(self):
        import subprocess
        with tempfile.TemporaryDirectory() as base:
            p = subprocess.Popen([sys.executable, "-c", PROBE, str(Path(fi.__file__).parent), base],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
            first = p.stdout.readline().split()
            self.assertEqual(first[0], "ready")
            p.stdin.close()
            self.assertEqual(p.wait(15), 3)
            self.assertIn("stopped", p.stdout.read())
            p.stdout.close()
            self.assertEqual(list(Path(base).iterdir()), [])


class Unpack(unittest.TestCase):
    def test_line_and_polyline(self):
        w = ET.fromstring(f"<s><line end-cap='arrow1'>{packed(30, 30, -12, 12)}</line>"
                          f"<polyline count='3'>{packed(24, 30, 24, 36, 42, 48)}</polyline></s>")
        fe.unpack_coordinates(w)
        li, pl = w.find("line"), w.find("polyline")
        self.assertEqual([float(li.get(k)) for k in ("x1", "y1", "x2", "y2")], [30, -12, 30, 12])
        self.assertEqual(pl.get("points"), "24.0,36.0 30.0,42.0 24.0,48.0")
        self.assertIsNone(li.text)

    def test_ordinary_text_stays(self):
        w = ET.fromstring("<s><line x1='1' y1='2'/><text>Hello</text></s>")
        fe.unpack_coordinates(w)
        self.assertEqual(w.find("text").text, "Hello")


class Receptivity(unittest.TestCase):
    def test_notation(self):
        self.assertEqual(fg.receptivity("On * -KF3"), "On · -KF3")
        self.assertEqual(fg.receptivity("I0 * NOT I1 + NOT I7"), "I0 · NOT I1 ∨ NOT I7")
        self.assertEqual(fg.receptivity("RE -SF1"), "↑-SF1")
        self.assertEqual(fg.receptivity("NOT -SF2.signal"), "NOT -SF2")
        self.assertEqual(fg.receptivity("[C > 2]"), "C > 2")
        self.assertEqual(fg.receptivity("0.5s / X1"), "0.5s / X1")


class Solenoids(unittest.TestCase):
    def test_side_and_mirroring(self):
        sym = ET.fromstring("<symbol><symbol-layer ident='AEML'><connector type='electric-label' designation='-MB1'/>"
                            "</symbol-layer><symbol-layer ident='AEMR'><connector type='electric-label' "
                            "designation='-MB2'/></symbol-layer></symbol>")
        self.assertEqual(fi.solenoids(sym, ET.fromstring("<component/>")), [("left", "-MB1"), ("right", "-MB2")])
        self.assertEqual(fi.solenoids(sym, ET.fromstring("<component scale-x='-1'/>")),
                         [("right", "-MB1"), ("left", "-MB2")])


class FolioPlacement(unittest.TestCase):
    def test_small_circuit_top_left_on_a4_landscape(self):
        # FluidSIM draws small in the middle of an A4 sheet: shift to the top left (multiple of 10), A4 landscape
        objects = [ET.fromstring("<component x1='240' y1='300' width='60' height='36'/>"),
                   ET.fromstring("<component x1='240' y1='420' width='24' height='24'/>")]
        ops = [{"x": 225.0, "y": 265.0}, {"x": 210.0, "y": 360.0}]
        dx, dy, paper = fi.folio_placement(objects, ops)
        self.assertEqual((dx % 10, dy % 10), (0, 0))
        self.assertEqual(min(240 * fe.SCALE, 210.0) + dx, 50.0)   # the left edge of the box at MARGIN
        self.assertEqual(paper, "a4-landscape")

    def test_large_circuit_on_larger_paper(self):
        objects = [ET.fromstring("<component x1='0' y1='0' width='1500' height='900'/>")]
        self.assertEqual(fi.folio_placement(objects, [{"x": 0.0, "y": 0.0}])[2], "a3-landscape")


class Read(unittest.TestCase):
    def test_sheet_in_a_circuits_section(self):
        # some files put their sheet in section type="circuits" -> <file>
        raw = ("<fluidtools2><section type='circuits' count='1'><file name='x.circ'>"
               "<section type='circuit-objects'><component ident='O1'/></section></file></section></fluidtools2>")
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "x.circ"
            p.write_text(raw, encoding="utf-8")
            w = fi.read(p)
        self.assertIsNotNone(fi.section(w, "circuit-objects"))


class OwnGraphics(unittest.TestCase):
    def _own(self, obj_ref, connectors, cls=""):
        sym = ET.fromstring("<symbol><symbol-layer ident='default'>" + "".join(
            f"<connector type='electric' ident='{i}' designation='{d}' cx='{x}' cy='{y}'/>"
            for i, (d, x, y) in enumerate(connectors, 1)) + "</symbol-layer></symbol>")
        new = og.replace(sym, ET.fromstring(f"<component object-ref='{obj_ref}'/>"), cls)
        return next(e for e in new if e.get("ident") == "OWN")

    def test_resistor_without_name(self):
        e = self._own("ET.Basic.Resistor", [("p", -24, 0), ("n", 24, 0)])
        self.assertEqual(e.findall("text"), [])          # no box with a name
        self.assertEqual(len(e.findall("polyline")), 1)  # the body of the resistor

    def test_npn_with_arrow_at_the_emitter(self):
        e = self._own("ET.Semiconductor.NPN_BC140", [("B", -21, 0), ("C", 21, -18), ("E", 21, 18)])
        self.assertEqual(sum(1 for li in e.findall("line") if li.get("end-cap") == "arrow1"), 1)
        self.assertEqual(len(e.findall("circle")), 1)

    def test_motor_protection_switch_three_poles_not_a_coil(self):
        # FluidSIM classifies it as a solenoid-coil; drawn are three poles with a mechanical link
        e = self._own("ET.Switch.MotorCircuitBreaker",
                      [("1", -12, -36), ("2", -12, 36), ("3", 12, -36), ("4", 12, 36), ("5", 36, -36), ("6", 36, 36)],
                      "solenoid-coil")
        self.assertTrue(any(li.get("style") == "dash" for li in e.findall("line")))
        self.assertEqual([t.text for t in e.findall("text")], ["I>"])

    def test_three_phase_motor_circle_with_sign(self):
        e = self._own("ET.Actuator.ThreePhaseSynchronousMotor2", [("", -6, -18), ("", 18, -18), ("", 6, 48)])
        self.assertEqual(len(e.findall("circle")), 1)
        self.assertEqual([t.text for t in e.findall("text")], ["MS", "3~"])

    def test_unknown_part_without_name(self):
        e = self._own("ET.Controller.ControllerPID", [("", -24, 0), ("", 24, 0)])
        self.assertEqual([t.text for t in e.findall("text")], ["PID"])
        e = self._own("ET.Foo.SomethingUnknown", [("", -24, 0), ("", 24, 0)])
        self.assertEqual(e.findall("text"), [])

    def test_only_connectors_of_the_original_remain(self):
        sym = ET.fromstring(CIRCUIT).find("section/symbol")
        new = og.replace(sym, ET.fromstring("<component object-ref='Pneu.DirectionalValve.Valve42'/>"))
        for layer in new.findall("symbol-layer"):
            if layer.get("ident") != "OWN":
                self.assertEqual({e.tag for e in layer}, {"connector"} if len(layer) else set(), layer.get("ident"))

    def test_valve_flow_paths(self):
        sym = ET.fromstring(CIRCUIT).find("section/symbol")
        positions = og.valve_positions(sym)
        self.assertEqual(len(positions), 2)
        self.assertEqual(sorted(positions[0][0]), [("1", "4"), ("2", "3")])
        self.assertEqual(sorted(positions[1][0]), [("1", "2"), ("4", "3")])


class VisibleValues(unittest.TestCase):
    def test_simulated_quantities_and_zero_values_not_in_the_plan(self):
        tb = {"T1": ET.fromstring("<text-box value='P_out = 0 kW'/>"), "T2": ET.fromstring("<text-box value='0 V'/>"),
              "T3": ET.fromstring("<text-box value='p0 = 6 bar'/>")}
        o = ET.fromstring("<component><user-data>"
                          "<attribute name='^#P_out' comment='~FTParam~FTPVar~FTPUnit' display-ref='T1'/>"
                          "<attribute name='^#voltage_DC' comment='~FTParam~FTPUnit' display-ref='T2'/>"
                          "<attribute name='^#p0' comment='~FTParam~FTPUnit' display-ref='T3'/>"
                          "</user-data></component>")
        self.assertEqual([w for w, _ in fi.visible_values(o, tb)], ["p0 = 6 bar"])


class Logo(unittest.TestCase):
    """The LOGO program (logo-box-ref) goes to a folio of its own, the circuit stays."""

    def test_split(self):
        w = ET.fromstring(
            "<fluidtools2><section type='symbol-graphics'/><section type='circuit-objects'>"
            "<component ident='O1' object-ref='Pneu.X' symbol-ref='s1'/>"
            "<component ident='O9' object-ref='Digital.Logo.Logobox' symbol-ref='s2'>"
            "<dki-data><designation value='-A1'/></dki-data></component>"
            "<component ident='O2' logo-box-ref='9' object-ref='Digital.Logo.And' symbol-ref='s3'/>"
            "<component ident='O3' logo-box-ref='9' object-ref='Digital.Logo.OutputQ' symbol-ref='s4'/>"
            "<connect con1='O2.a' con2='O3.b'/><connect con1='O1.a' con2='O9.b'/>"
            "</section></fluidtools2>")
        parts = fi.split_logo(w)
        self.assertEqual([n for n, _ in parts], ["LOGO -A1"])
        self.assertEqual([o.get("ident") for o in fi.circuit_objects(w)], ["O1", "O9"])
        self.assertEqual([o.get("ident") for o in fi.circuit_objects(parts[0][1])], ["O2", "O3"])
        self.assertEqual(len(list(parts[0][1].iter("connect"))), 1)
        self.assertEqual(len(list(w.iter("connect"))), 1)
        self.assertIsNotNone(fi.section(parts[0][1], "symbol-graphics"))

    def test_without_logo(self):
        w = ET.fromstring("<fluidtools2><section type='circuit-objects'>"
                          "<component ident='O1' symbol-ref='s1'/></section></fluidtools2>")
        self.assertEqual(fi.split_logo(w), [])


class Shaft(unittest.TestCase):
    def test_shaft_connector_through_con_ref(self):
        symbols = {"s": ET.fromstring("<symbol ident='s'><symbol-layer ident='StaticLayer'>"
                                      "<connector ident='1' type='rotational' cx='0' cy='0'/>"
                                      "<connector ident='2' type='electric' cx='5' cy='0'/>"
                                      "</symbol-layer></symbol>")}
        o = ET.fromstring("<component ident='O1' symbol-ref='s'>"
                          "<connector ident='MR' con-ref='StaticLayer.1'/>"
                          "<connector ident='U1' con-ref='StaticLayer.2'/></component>")
        self.assertEqual(fi.shaft_connectors(symbols, o), {"MR"})


class GrafcetStructure(unittest.TestCase):
    """Macro steps, enclosing steps and forcing orders with the symbols that come with the import."""

    def obj(self, ident, ref, x, y, **values):
        o = ET.Element("component", {"ident": ident, "object-ref": ref, "x1": str(x), "y1": str(y),
                                     "width": "20", "height": "20"})
        for k, v in values.items():
            ET.SubElement(o, "const", {"ident": k, "value": v})
        return o

    def test_structure(self):
        with tempfile.TemporaryDirectory() as d:
            collection = Path(d)
            (collection / fg.G).mkdir(parents=True)
            for f in fi.GRAFCET_SYMBOLS.glob("*.elmt"):
                (collection / fg.G / f.name).write_bytes(f.read_bytes())
            objects = [
                self.obj("1", "Grafcet.Step", 0, 0, name="1", flags="initial"),
                self.obj("2", "Grafcet.Step", 0, 100, name="2", flags="enclosing", enclosedSubs=" 7 "),
                self.obj("3", "Grafcet.Step", 0, 200, name="5", flags="macro"),
                self.obj("4", "Grafcet.Step", 0, 300, name="5", flags="mentry"),
                self.obj("7", "Grafcet.Action", 40, 100, actionType="6", **{"sub-grafcet": "7", "steps": "INIT"}),
            ]
            ops, count, not_imported = fg.convert(0, objects, [], collection, 1.0,
                                                  lambda o: (float(o.get("x1")), float(o.get("y1"))))
        labels = [o["label"] for o in ops if o["op"] == "set_label"]
        paths = {o["id"]: o["path"] for o in ops if o["op"] == "add_element"}
        self.assertIn("M5", labels)
        self.assertIn("E5", labels)
        self.assertIn("G7{INIT}", labels)
        self.assertTrue(paths["g0_1"].endswith("20202000_step.elmt"))
        self.assertTrue(paths["g0_2"].endswith("20202002_step.elmt"))
        self.assertTrue(paths["g0_3"].endswith("20202003_step.elmt"))
        self.assertEqual(count["steps"], 4)
        self.assertEqual(not_imported, [])

    def test_shipped_symbols_have_named_terminals(self):
        for kind, f in fg.FILES.items():
            with self.subTest(kind=kind):
                names = set(fg.terminals(fi.GRAFCET_SYMBOLS / Path(f).name))
                want = {"initial": {"in", "out", "action"}, "step": {"in", "out", "action"},
                        "transition": {"in", "out"}, "parallel": {"in", "1", "2"}, "sync": {"1", "2", "out"},
                        "jump": {"in"}}.get(kind, {"step"} if kind.isdigit() else set())
                self.assertTrue(want <= names, f"{f}: {sorted(names)}")


class Plan(unittest.TestCase):
    """The whole circuit up to the operations for QElectroTech, without QElectroTech."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        folder = Path(cls.tmp.name)
        cls.collection = folder / "elements"
        (cls.collection / fg.G).mkdir(parents=True)
        for f in fi.GRAFCET_SYMBOLS.glob("*.elmt"):
            (cls.collection / fg.G / f.name).write_bytes(f.read_bytes())
        circuit = write_circuit(folder)
        cls.ops, cls.count = fi.plan(fi.sheets([circuit]), cls.collection, fe.Translation())
        cls.elements = {p.name: p.read_text(encoding="utf-8") for p in cls.collection.glob("*.elmt")}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def ops_of(self, name):
        return [o for o in self.ops if o["op"] == name]

    def test_counts(self):
        # valve, solenoid, cylinder, supply, relay, contact; the silencer only closes a port
        self.assertEqual(self.count["parts"], 6)
        self.assertEqual(self.count["wires"], 3)
        self.assertEqual(self.count["links"], 2)
        self.assertEqual((self.count["steps"], self.count["transitions"]), (2, 1))

    def test_every_element_is_valid_and_holds_no_fluidsim_graphics(self):
        self.assertEqual(len(self.elements), 6)
        for name, text in self.elements.items():
            with self.subTest(element=name):
                root = ET.fromstring(text)
                self.assertEqual(root.tag, "definition")
                self.assertNotIn("VENDOR ART", text)
                self.assertNotIn("<fluid", text)
                self.assertIn("own graphics after ISO 1219-1", root.find("informations").text)
                for e in root.find("description"):
                    for k in ("x", "x1", "x2"):
                        if e.get(k) is not None:
                            self.assertLess(abs(float(e.get(k))), 600.0)   # not the line of +-777 units

    def test_valve_terminals_and_silencer(self):
        valve = next(t for n, t in self.elements.items() if n.startswith("4_n_way_valve"))
        root = ET.fromstring(valve)
        terminals = {t.get("name"): t for t in root.iter("terminal")}
        self.assertEqual(sorted(terminals), ["1", "2", "3", "4"])
        self.assertEqual(root.get("link_type"), "master")
        # the silencer: a box just beyond port 3
        t3 = terminals["3"]
        x3, y3 = float(t3.get("x")), float(t3.get("y"))
        boxes = [r for r in root.iter("rect") if abs(float(r.get("x")) + float(r.get("width")) / 2 - x3) < 0.5
                 and float(r.get("y")) > y3]
        self.assertEqual(len(boxes), 1)

    def test_labels(self):
        labels = {o["element"]: o["label"] for o in self.ops_of("set_label")}
        self.assertEqual(labels["$b0_O1"], "-QM1")          # the valve had none
        self.assertEqual(labels["$b0_O3"], "-MM1")          # the cylinder had none
        self.assertEqual(labels["$b0_O2"], "-MB1")
        self.assertEqual(self.count["labels_added"], 2)

    def test_links(self):
        links = {(o["element"], o["to"]) for o in self.ops_of("link_elements")}
        self.assertEqual(links, {("$b0_O2", "$b0_O1"), ("$b0_O7", "$b0_O6")})

    def test_wires_name_existing_terminals(self):
        uuids = {}
        for o in self.ops_of("add_element"):
            if o["id"].startswith("b"):
                text = self.elements[o["path"].split("://", 1)[1]]
                uuids[o["id"]] = {"{" + t.get("uuid").strip("{}") + "}" for t in ET.fromstring(text).iter("terminal")}
        for o in self.ops_of("add_conductor"):
            if o["from"].startswith("$b"):
                self.assertIn(o["from_terminal"], uuids[o["from"][1:]])
                self.assertIn(o["to_terminal"], uuids[o["to"][1:]])

    def test_folio_on_the_grid(self):
        for o in self.ops_of("add_element"):
            self.assertGreaterEqual(o["x"], 0)
            self.assertGreaterEqual(o["y"], 0)
        border = self.ops_of("set_folio_border")[0]
        self.assertTrue(border["value"].startswith(("a4", "a3")))

    def test_grafcet_receptivity(self):
        labels = [o["label"] for o in self.ops_of("set_label")]
        self.assertIn("↑-SF1", labels)


@unittest.skipUnless(QET_BINARY and Path(QET_BINARY).is_file(), "set QET_BINARY to a built qelectrotech")
class WithQElectroTech(unittest.TestCase):
    """The import writes a real project through QElectroTech."""

    def test_project(self):
        with tempfile.TemporaryDirectory() as d:
            folder = Path(d)
            circuit = write_circuit(folder)
            output = folder / "out.qet"
            report = folder / "report.json"
            fi.main([str(circuit), str(output), "--qet", QET_BINARY, "--report", str(report)])
            self.assertTrue(output.is_file())
            root = ET.parse(output).getroot()
            # the placed elements of the folios, not the definitions in the embedded collection
            elements = [e for d in root.iter("diagram") for e in d.iter("element")]
            conductors = [c for d in root.iter("diagram") for c in d.iter("conductor")]
            self.assertEqual(len(elements), 6 + 3)        # six parts, two steps and a transition
            self.assertGreaterEqual(len(conductors), 3 + 2)
            import json
            data = json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(data["failed_operations"], 0, data.get("failures"))


if __name__ == "__main__":
    unittest.main()
