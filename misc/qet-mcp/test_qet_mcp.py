#!/usr/bin/env python3
"""
Regression suite for qet_mcp.py.

Three layers, cheapest first:

  unit         no QElectroTech needed: validation, script generation, the
               terminal-order rule, the diff, the part schema, the protocol
  protocol     the real stdio transport, one JSON-RPC message per line
  integration  drives a built QElectroTech; skipped unless one is found

    python3 test_qet_mcp.py                       # unit + protocol
    QET_BINARY=/path/to/qelectrotech \\
    QET_ELEMENTS=/path/to/qelectrotech/elements \\
    QET_EXAMPLES=/path/to/qelectrotech/examples \\
        python3 test_qet_mcp.py                   # everything

Several tests exist because the behaviour they pin was once wrong and
looked right. Those say so in their docstring, so nobody "simplifies" one
away: a check that has never been seen to fail is not evidence.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import qet_mcp as m  # noqa: E402

BINARY = os.environ.get("QET_BINARY", "")
ELEMENTS = os.environ.get("QET_ELEMENTS", "")
EXAMPLES = os.environ.get("QET_EXAMPLES", "")

have_binary = bool(BINARY) and os.access(BINARY, os.X_OK)
have_elements = bool(ELEMENTS) and Path(ELEMENTS).is_dir()
have_examples = bool(EXAMPLES) and Path(EXAMPLES).is_dir()

needs_binary = unittest.skipUnless(have_binary, "set QET_BINARY to a built qelectrotech")
needs_elements = unittest.skipUnless(have_binary and have_elements,
                                     "set QET_BINARY and QET_ELEMENTS")
needs_examples = unittest.skipUnless(have_binary and have_examples,
                                     "set QET_BINARY and QET_EXAMPLES")

COIL = "common://10_electric/10_allpole/310_relays_contactors_contacts/01_coils/bobine_ka_a_remanence.elmt"
SLAVE = ("common://10_electric/10_allpole/310_relays_contactors_contacts/"
         "02_contacts_cross_referencing/15_protection_contacts/contact_relais_nf_esclave.elmt")
TERMINAL = "common://10_electric/10_allpole/130_terminals_terminal_strips/borne_2.elmt"

# No shipped element has masterType/slaveType "plc" -- these two minimal
# fixtures (derived from a real coil/slave pair) exist only so PLC IO tests
# have something to place. ElementsLocation only resolves an absolute .elmt
# path when it happens to sit under QETApp::commonElementsDirN(), so they
# are placed via a dedicated elements_dir pointed at this directory, not the
# real QET_ELEMENTS collection.
PLC_FIXTURES = str(HERE / "fixtures")
PLC_MASTER = "common://plc_master_test.elmt"
PLC_SLAVE = "common://plc_slave_test.elmt"


def png(path: Path) -> None:
    """A real 64x32 PNG from the standard library, so no imaging dependency."""
    import struct
    import zlib
    w, h = 64, 32
    raw = b"".join(b"\x00" + bytes([255, 0, 0] * w) for _ in range(h))

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def pdf(path: Path, page_colors=("1 0 0", "0 0 1")) -> None:
    """A minimal, hand-built multi-page PDF -- one filled rectangle per
    page, in a different colour each, so a test can tell which page a
    render actually came from. No external tool (ghostscript, reportlab)
    needed; the xref offsets are computed exactly as each object is
    written, since QPdfDocument (pdfium) is not guaranteed to tolerate a
    wrong one the way some readers repair-scan for."""
    n = len(page_colors)
    page_objs = list(range(3, 3 + n))          # 3 .. 2+n
    content_objs = list(range(3 + n, 3 + 2 * n))  # 3+n .. 2+2n
    objects = {}

    objects[1] = b"<< /Type /Catalog /Pages 2 0 R >>"
    kids = " ".join(f"{o} 0 R" for o in page_objs)
    objects[2] = f"<< /Type /Pages /Kids [{kids}] /Count {n} >>".encode()
    for i, (page_obj, content_obj) in enumerate(zip(page_objs, content_objs)):
        objects[page_obj] = (
            f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] "
            f"/Contents {content_obj} 0 R /Resources << >> >>").encode()
        stream = f"{page_colors[i]} rg 10 10 150 150 re f".encode()
        objects[content_obj] = (f"<< /Length {len(stream)} >>\nstream\n").encode() \
            + stream + b"\nendstream"

    out = bytearray(b"%PDF-1.4\n")
    offsets = {}
    for num in sorted(objects):
        offsets[num] = len(out)
        out += f"{num} 0 obj\n".encode() + objects[num] + b"\nendobj\n"
    xref_start = len(out)
    total = len(objects) + 1
    out += f"xref\n0 {total}\n".encode()
    out += b"0000000000 65535 f \n"
    for num in sorted(objects):
        out += f"{offsets[num]:010d} 00000 n \n".encode()
    out += f"trailer\n<< /Size {total} /Root 1 0 R >>\nstartxref\n{xref_start}\n%%EOF".encode()
    path.write_bytes(bytes(out))


# ==========================================================================
# unit
# ==========================================================================

class ToolRegistry(unittest.TestCase):
    def test_names_are_unique_and_prefixed(self):
        names = [t["name"] for t in m.TOOLS]
        self.assertEqual(len(names), len(set(names)))
        self.assertTrue(all(n.startswith("qet_") for n in names))

    def test_every_schema_is_well_formed(self):
        for t in m.TOOLS:
            with self.subTest(tool=t["name"]):
                s = t["inputSchema"]
                self.assertEqual(s["type"], "object")
                self.assertTrue(set(s.get("required", [])) <= set(s["properties"]),
                                "a required key is missing from properties")
                self.assertTrue(callable(t["handler"]))
                self.assertTrue(t["description"].strip())
                json.dumps(s)          # must be serialisable as-is

    def test_expected_tools_exist(self):
        self.assertEqual({t["name"] for t in m.TOOLS}, {
            "qet_project_info", "qet_elements", "qet_conductors", "qet_diff",
            "qet_scan", "qet_element_info", "qet_export", "qet_edit", "qet_query",
            "qet_project_new", "qet_element_search", "qet_check", "qet_element_build",
            "qet_continuity"})


class EditValidation(unittest.TestCase):
    """Every argument error must be raised before QElectroTech is launched.

    tool_edit is called with a binary that does not exist: if validation
    were lazy the failure would be 'not an executable', not the ValueError
    asserted here.
    """

    def build(self, ops):
        return m._build_script(ops, "/tmp/out.qet")

    def test_every_op_generates_a_script(self):
        # one minimal valid instance of every op
        f = {"op": "add_folio", "id": "f"}
        samples = {
            "add_folio": [f],
            "set_folio_title": [f, {"op": "set_folio_title", "folio": "$f", "title": "t"}],
            "add_element": [f, {"op": "add_element", "id": "e", "folio": "$f", "path": "p", "x": 1, "y": 2}],
        }
        el = [f, {"op": "add_element", "id": "e", "folio": "$f", "path": "p", "x": 1, "y": 2}]
        two = el + [{"op": "add_element", "id": "e2", "folio": "$f", "path": "p", "x": 1, "y": 2}]
        samples.update({
            "set_position": el + [{"op": "set_position", "folio": "$f", "element": "$e", "x": 1, "y": 1}],
            "move_element": el + [{"op": "move_element", "folio": "$f", "element": "$e", "dx": 1, "dy": 1}],
            "rotate_element": el + [{"op": "rotate_element", "folio": "$f", "element": "$e", "angle": 90}],
            "set_label": el + [{"op": "set_label", "folio": "$f", "element": "$e", "label": "K"}],
            "set_info": el + [{"op": "set_info", "folio": "$f", "element": "$e", "key": "k", "value": "v"}],
            "add_conductor": two + [{"op": "add_conductor", "folio": "$f", "from": "$e", "from_terminal": 0,
                                     "to": "$e2", "to_terminal": 0}],
            "delete_element": el + [{"op": "delete_element", "folio": "$f", "element": "$e"}],
            "set_conductor": el + [{"op": "set_conductor", "folio": "$f", "element": "$e", "terminal": 0,
                                    "property": "num", "value": "1"}],
            "move_conductor_segment": el + [{"op": "move_conductor_segment", "folio": "$f",
                                             "element": "$e", "terminal": 0, "segment": 1,
                                             "dx": 10, "dy": 0}],
            "delete_conductor": el + [{"op": "delete_conductor", "folio": "$f", "element": "$e", "terminal": 0}],
            "link_elements": two + [{"op": "link_elements", "folio": "$f", "element": "$e",
                                     "to_folio": "$f", "to": "$e2"}],
            "unlink_element": el + [{"op": "unlink_element", "folio": "$f", "element": "$e"}],
            "remove_folio": [f, {"op": "remove_folio", "folio": "$f"}],
            "set_folio": [f, {"op": "set_folio", "folio": "$f", "property": "author", "value": "a"}],
            "add_text": [f, {"op": "add_text", "id": "t", "folio": "$f", "text": "x", "x": 0, "y": 0}],
            "set_text": [f, {"op": "set_text", "folio": "$f", "index": 0, "text": "x"}],
            "set_text_color": [f, {"op": "set_text_color", "folio": "$f", "index": 0, "color": "#000"}],
            "rotate_text": [f, {"op": "rotate_text", "folio": "$f", "index": 0, "angle": 5}],
            "delete_text": [f, {"op": "delete_text", "folio": "$f", "index": 0}],
            "add_shape": [f, {"op": "add_shape", "folio": "$f", "shape": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 1}],
            "set_shape": [f, {"op": "set_shape", "folio": "$f", "index": 0, "property": "fill", "value": "none"}],
            "delete_shape": [f, {"op": "delete_shape", "folio": "$f", "index": 0}],
            "add_polygon": [f, {"op": "add_polygon", "folio": "$f",
                                "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}],
                                "closed": True}],
            "set_shape_polygon": [f, {"op": "add_polygon", "id": "poly", "folio": "$f",
                                      "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}],
                                      "closed": True},
                                  {"op": "set_shape_polygon", "folio": "$f", "index": "$poly",
                                   "points": [{"x": 2, "y": 2}, {"x": 3, "y": 2}, {"x": 3, "y": 3}]}],
            "add_path": [f, {"op": "add_path", "folio": "$f",
                             "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 0, "kind": "smooth",
                                       "inHandle": {"x": 0.5, "y": 0}, "outHandle": {"x": 1.5, "y": 0}}],
                             "closed": False}],
            "set_shape_path_nodes": [f, {"op": "add_path", "id": "path", "folio": "$f",
                                        "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 0}], "closed": False},
                                     {"op": "set_shape_path_nodes", "folio": "$f", "index": "$path",
                                      "nodes": [{"x": 5, "y": 5}, {"x": 6, "y": 5}]}],
            "set_shape_closed": [f, {"op": "add_polygon", "id": "poly2", "folio": "$f",
                                     "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}],
                                     "closed": True},
                                 {"op": "set_shape_closed", "folio": "$f", "index": "$poly2", "closed": False}],
            "add_image": [f, {"op": "add_image", "folio": "$f", "file": "/x.png", "x": 0, "y": 0}],
            "scale_image": [f, {"op": "scale_image", "folio": "$f", "index": 0, "factor": 2}],
            "rotate_image": [f, {"op": "rotate_image", "folio": "$f", "index": 0, "angle": 5}],
            "delete_image": [f, {"op": "delete_image", "folio": "$f", "index": 0}],
            "add_pdf_page": [f, {"op": "add_pdf_page", "folio": "$f", "file": "/x.pdf",
                                 "page": 1, "dpi": 150, "x": 0, "y": 0}],
            "add_element_text": el + [{"op": "add_element_text", "id": "t", "folio": "$f", "element": "$e",
                                       "source": "info", "value": "comment", "x": 1, "y": 2}],
            "set_element_text": el + [{"op": "set_element_text", "folio": "$f", "element": "$e", "index": 0,
                                       "property": "x", "value": "5"}],
            "delete_element_text": el + [{"op": "delete_element_text", "folio": "$f", "element": "$e", "index": 0}],
            "add_terminal_strip": [{"op": "add_terminal_strip", "installation": "a", "location": "b", "name": "c"}],
            "remove_terminal_strip": [{"op": "remove_terminal_strip", "strip": 0}],
            "add_to_strip": el + [{"op": "add_to_strip", "strip": 0, "folio": "$f", "element": "$e"}],
            "group_terminals": [{"op": "group_terminals", "strip": 0, "indices": [0, 1]}],
            "bridge_terminals": [{"op": "bridge_terminals", "strip": 0, "indices": [0, 1]}],
            "sort_terminal_strip": [{"op": "sort_terminal_strip", "strip": 0}],
            "add_autonum": [{"op": "add_autonum", "kind": "conductor", "name": "W", "parts": ["string:W"]}],
            "remove_autonum": [{"op": "remove_autonum", "kind": "conductor", "name": "W"}],
            "use_conductor_autonum": [f, {"op": "use_conductor_autonum", "folio": "$f", "name": "W"}],
            "use_element_autonum": [{"op": "use_element_autonum", "name": "EL"}],
            "insert_folio": [{"op": "insert_folio", "id": "i", "position": 0}],
            "element_geometry": el + [{"op": "element_geometry", "folio": "$f", "element": "$e"}],
            "undo": [{"op": "undo"}],
            "redo": [{"op": "redo"}],
            "search_and_replace": [{"op": "search_and_replace", "kind": "text", "field": "",
                                    "pattern": "x", "replacement": "y",
                                    "regex": False, "case_sensitive": True}],
            "set_project_title": [{"op": "set_project_title", "title": "T"}],
            "set_folio_border": [f, {"op": "set_folio_border", "folio": "$f", "property": "columns", "value": "10"}],
            "embed_title_block_template": [{"op": "embed_title_block_template", "name": "default"}],
            "duplicate_elements": el + [{"op": "duplicate_elements", "id": "d", "folio": "$f",
                                         "elements": ["$e"], "to_folio": "$f", "x": 50, "y": 50}],
            "number_element": el + [{"op": "number_element", "folio": "$f", "element": "$e"}],
            "add_table": [f, {"op": "add_table", "id": "t", "folio": "$f", "kind": "nomenclature",
                              "name": "BOM", "query": "SELECT label FROM element_nomenclature_view"}],
            "set_table_position": [f, {"op": "add_table", "id": "t", "folio": "$f", "kind": "nomenclature",
                                       "name": "BOM", "query": "SELECT label FROM element_nomenclature_view"},
                                   {"op": "set_table_position", "folio": "$f", "table": "$t", "x": 1, "y": 1}],
            "delete_table": [f, {"op": "add_table", "id": "t", "folio": "$f", "kind": "nomenclature",
                                 "name": "BOM", "query": "SELECT label FROM element_nomenclature_view"},
                             {"op": "delete_table", "folio": "$f", "table": "$t"}],
            "link_plc_io": two + [{"op": "link_plc_io", "folio": "$f", "element": "$e",
                                   "to_folio": "$f", "to": "$e2", "io_index": 0}],
            "add_plc_io": el + [{"op": "add_plc_io", "folio": "$f", "element": "$e",
                                 "type": "entree_digitale", "address": "1.0",
                                 "function": "f", "comment": "c"}],
            "set_plc_io": el + [{"op": "set_plc_io", "folio": "$f", "element": "$e",
                                 "index": 0, "property": "address", "value": "1.1"}],
            "remove_plc_io": el + [{"op": "remove_plc_io", "folio": "$f", "element": "$e", "index": 0}],
        })
        self.assertEqual(set(samples), set(m.OPS),
                         "an op has no sample here: add one so it is exercised")
        for name, ops in samples.items():
            with self.subTest(op=name):
                script = self.build(ops)
                self.assertIn("qet.save(", script)
                self.assertIn(m.OPS[name][0] or "addFolio", script)

    def test_unknown_op(self):
        with self.assertRaisesRegex(ValueError, "unknown op"):
            self.build([{"op": "draw_a_nice_diagram"}])

    def test_missing_argument(self):
        with self.assertRaisesRegex(ValueError, "missing 'path'"):
            self.build([{"op": "add_element", "folio": 0, "x": 1, "y": 2}])

    def test_dangling_reference(self):
        with self.assertRaisesRegex(ValueError, "no earlier"):
            self.build([{"op": "set_label", "folio": "$nope", "element": "$x", "label": "a"}])

    def test_id_rules(self):
        for bad in ("$f", "", 3):
            with self.subTest(id=bad):
                with self.assertRaises(ValueError):
                    self.build([{"op": "add_folio", "id": bad}])
        with self.assertRaisesRegex(ValueError, "already used"):
            self.build([{"op": "add_folio", "id": "f"}, {"op": "add_folio", "id": "f"}])

    def test_enumerated_arguments_are_checked_up_front(self):
        cases = [
            {"op": "set_conductor", "folio": 0, "element": "x", "terminal": 0, "property": "voltage", "value": "1"},
            {"op": "set_folio", "folio": 0, "property": "version", "value": "1"},
            {"op": "set_shape", "folio": 0, "index": 0, "property": "shadow", "value": "1"},
            {"op": "add_shape", "folio": 0, "shape": "hexagon", "x1": 0, "y1": 0, "x2": 1, "y2": 1},
            {"op": "add_autonum", "kind": "wire", "name": "a", "parts": ["string:W"]},
            {"op": "set_element_text", "folio": 0, "element": "x", "index": 0, "property": "shadow", "value": "1"},
            {"op": "add_element_text", "folio": 0, "element": "x", "source": "magic", "value": "v", "x": 0, "y": 0},
            {"op": "set_folio_border", "folio": 0, "property": "thickness", "value": "3"},
        ]
        for c in cases:
            with self.subTest(op=c["op"], bad=c.get("property") or c.get("shape") or c.get("kind")):
                with self.assertRaises(ValueError):
                    self.build([c])

    def test_version_is_not_a_settable_folio_property(self):
        """setFolioProperty('version') reported success and was overwritten by
        the file-format stamp. It must stay refused on every layer."""
        self.assertNotIn("version", m.FOLIO_PROPERTIES)

    def test_list_argument_must_be_list_of_strings(self):
        with self.assertRaisesRegex(ValueError, "list of strings"):
            self.build([{"op": "add_autonum", "kind": "conductor", "name": "a", "parts": "string:W"}])

    def test_indexed_references_and_element_lists(self):
        script = self.build([
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "a", "folio": "$f", "path": "p", "x": 0, "y": 0},
            {"op": "add_element", "id": "b", "folio": "$f", "path": "p", "x": 9, "y": 0},
            {"op": "duplicate_elements", "id": "c", "folio": "$f", "elements": ["$a", "$b"],
             "to_folio": "$f", "x": 5, "y": 5},
            {"op": "set_label", "folio": "$f", "element": "$c[1]", "label": "X"}])
        self.assertIn('R["c"][1]', script)
        self.assertIn('[R["a"], R["b"]]', script)
        for bad in ("$c[", "$c[x]", "$nope[0]"):
            with self.subTest(ref=bad):
                with self.assertRaises(ValueError):
                    self.build([{"op": "add_folio", "id": "f"},
                                {"op": "set_label", "folio": "$f", "element": bad, "label": "X"}])
        for bad in ([], "a", [1, 2]):
            with self.subTest(elements=bad):
                with self.assertRaises(ValueError):
                    self.build([{"op": "add_folio", "id": "f"},
                                {"op": "duplicate_elements", "folio": "$f", "elements": bad,
                                 "to_folio": "$f", "x": 0, "y": 0}])

    def test_an_empty_object_result_is_a_failure(self):
        """elementGeometry returns {} for an element it cannot find. An empty
        object has no .length, so the empty-list test did not cover it."""
        out = m._MARKER + json.dumps(dict(kind="op", index=0, op="element_geometry", id=None, result={}))
        self.assertFalse(m._parse_script_output(out)["operations"][0]["succeeded"])
        out = m._MARKER + json.dumps(dict(kind="op", index=0, op="element_geometry", id=None,
                                          result={"x": 0, "y": 0}))
        self.assertTrue(m._parse_script_output(out)["operations"][0]["succeeded"])
        script = m._build_script([{"op": "add_folio"}], "/o")
        self.assertIn("Object.keys(", script)

    def test_an_empty_list_result_is_a_failure(self):
        """duplicateElements returns [] on failure, which the success check
        did not recognise: neither falsy in JavaScript's === comparisons nor
        equal to False in Python."""
        out = m._MARKER + json.dumps(dict(kind="op", index=0, op="duplicate_elements", id="d", result=[]))
        self.assertFalse(m._parse_script_output(out)["operations"][0]["succeeded"])
        # the JavaScript side must not rely on Array.isArray: QJSEngine returns
        # an empty QStringList as an array-like wrapper for which it is false
        script = m._build_script([{"op": "add_folio"}], "/o")
        self.assertIn(".length === 0", script)
        self.assertNotIn("Array.isArray", script)

    def test_number_arguments_reject_bool_and_text(self):
        for bad in (True, "1", None):
            with self.subTest(x=bad):
                with self.assertRaises(ValueError):
                    self.build([{"op": "add_folio", "id": "f"},
                                {"op": "add_text", "folio": "$f", "text": "t", "x": bad, "y": 0}])

    def test_string_values_are_escaped_into_the_script(self):
        nasty = 'a"b\\c\n</script> '
        script = self.build([{"op": "add_folio", "id": "f"},
                             {"op": "set_folio_title", "folio": "$f", "title": nasty}])
        # the literal must be valid JSON, so JavaScript reads exactly what was sent
        literal = re.search(r'setFolioTitle\(R\["f"\], (".*?")\)', script, re.S).group(1)
        self.assertEqual(json.loads(literal), nasty)

    def test_edit_refuses_in_place_and_empty(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "a.qet"
            p.write_text("<project/>")
            with self.assertRaisesRegex(ValueError, "differ from project"):
                m.tool_edit("/nonexistent", str(p), [{"op": "add_folio"}], str(p))
            with self.assertRaisesRegex(ValueError, "non-empty"):
                m.tool_edit("/nonexistent", str(p), [], str(Path(tmp) / "o.qet"))


class ResultParsing(unittest.TestCase):
    def line(self, **kw):
        return m._MARKER + json.dumps(kw)

    def test_zero_is_a_valid_result(self):
        """0 == False in Python. A valid first index (first text, first folio
        of an empty project) was read as failure and marked a successful edit
        ok=False."""
        out = self.line(kind="op", index=0, op="add_text", id="t", result=0)
        rec = m._parse_script_output(out)["operations"][0]
        self.assertTrue(rec["succeeded"])

    def test_failures_are_recognised(self):
        for bad in (False, "", -1, None):
            with self.subTest(result=bad):
                out = self.line(kind="op", index=0, op="x", id=None, result=bad)
                self.assertFalse(m._parse_script_output(out)["operations"][0]["succeeded"])

    def test_true_and_strings_succeed(self):
        for good in (True, "{uuid}", 1, 7):
            with self.subTest(result=good):
                out = self.line(kind="op", index=0, op="x", id=None, result=good)
                self.assertTrue(m._parse_script_output(out)["operations"][0]["succeeded"])

    def test_noise_and_garbage_are_ignored(self):
        text = "SQLite version\n" + m._MARKER + "{not json\nplain\n" + \
               self.line(kind="save", result=True, stopped_early=False)
        parsed = m._parse_script_output(text)
        self.assertTrue(parsed["saved"])
        self.assertEqual(parsed["operations"], [])


class TerminalOrder(unittest.TestCase):
    """QElectroTech indexes terminals top-to-bottom then left-to-right, not in
    file order. Getting it wrong wires the wrong end of a coil with no error;
    619 of the 837 shipped elements with named terminals list them
    differently from how they are indexed."""

    def nodes(self, *specs):
        return [ET.Element("terminal", {"name": n, "x": str(x), "y": str(y)}) for n, x, y in specs]

    def test_sorted_by_y_then_x(self):
        ordered, amb = m._terminals_in_index_order(
            self.nodes(("A2", 0, 20), ("A1", 0, -20)))
        self.assertEqual([t.get("name") for t in ordered], ["A1", "A2"])
        self.assertFalse(amb)

    def test_left_to_right_within_a_row(self):
        ordered, _ = m._terminals_in_index_order(
            self.nodes(("R", 30, 0), ("L", -30, 0), ("M", 0, 0)))
        self.assertEqual([t.get("name") for t in ordered], ["L", "M", "R"])

    def test_y_dominates_x(self):
        ordered, _ = m._terminals_in_index_order(
            self.nodes(("lowleft", -50, 10), ("highright", 50, -10)))
        self.assertEqual([t.get("name") for t in ordered], ["highright", "lowleft"])

    def test_ties_are_flagged_not_hidden(self):
        _, amb = m._terminals_in_index_order(self.nodes(("a", 5, 5), ("b", 5, 5)))
        self.assertTrue(amb)

    def test_numeric_not_lexical(self):
        ordered, _ = m._terminals_in_index_order(
            self.nodes(("nine", 0, 9), ("ten", 0, 10), ("minus", 0, -10)))
        self.assertEqual([t.get("name") for t in ordered], ["minus", "nine", "ten"])

    @unittest.skipUnless(have_elements, "set QET_ELEMENTS")
    def test_real_element_disagrees_with_file_order(self):
        f = Path(ELEMENTS) / COIL.replace("common://", "")
        root = ET.parse(f).getroot()
        file_order = [t.get("name") for t in root.iter("terminal")]
        ordered, _ = m._terminals_in_index_order(list(root.iter("terminal")))
        self.assertEqual(file_order, ["A2", "A1"])
        self.assertEqual([t.get("name") for t in ordered], ["A1", "A2"])


class ElementBuild(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.out = str(Path(self.tmp.name) / "e.elmt")
        self.ok = dict(names={"en": "x"},
                       parts=[{"type": "line", "x1": 0, "y1": 0, "x2": 10, "y2": 0}],
                       terminals=[{"x": 0, "y": 0, "orientation": "n"}])

    def tearDown(self):
        self.tmp.cleanup()

    def build(self, **over):
        return m.tool_element_build(self.out, **{**self.ok, **over})

    def test_rejections(self):
        cases = {
            "unknown part": dict(parts=[{"type": "squiggle"}]),
            "missing key": dict(parts=[{"type": "rect", "x": 0, "y": 0, "width": 5}]),
            "typo'd key": dict(parts=[{"type": "circle", "x": 0, "y": 0, "diametre": 5}]),
            "bad orientation": dict(terminals=[{"x": 0, "y": 0, "orientation": "up"}]),
            "no names": dict(names={}),
            "bad link_type": dict(link_type="widget"),
            "connectable, no terminals": dict(terminals=[]),
            "one-point polygon": dict(parts=[{"type": "polygon", "points": [[0, 0]]}]),
            "text coordinate": dict(parts=[{"type": "line", "x1": "left", "y1": 0, "x2": 1, "y2": 0}]),
        }
        for label, over in cases.items():
            with self.subTest(label):
                with self.assertRaises(ValueError):
                    self.build(**over)
                self.assertFalse(Path(self.out).exists(), "nothing may be written on refusal")

    def test_an_unexpected_key_on_a_valid_part_is_refused(self):
        """Distinct from a missing key: the part below is complete, and only
        the extra attribute is wrong. Without this the 'allowed keys' rule is
        never exercised -- the typo case above is refused for lacking
        'diameter', which passes even if unexpected keys are accepted."""
        with self.assertRaisesRegex(ValueError, "unexpected 'colour'"):
            self.build(parts=[{"type": "rect", "x": 0, "y": 0, "width": 5, "height": 5,
                               "colour": "red"}])
        self.assertFalse(Path(self.out).exists())

    def test_declared_box_contains_the_drawing(self):
        """The header is a containment constraint, not a formula, and the
        assertion behind it is what stops a clipped element."""
        for parts in ([{"type": "rect", "x": 100, "y": 100, "width": 50, "height": 50}],
                      [{"type": "rect", "x": -200, "y": -150, "width": 40, "height": 30}],
                      [{"type": "circle", "x": -25, "y": -25, "diameter": 50}]):
            with self.subTest(parts=parts[0]["type"]):
                r = self.build(parts=parts)
                x0, y0, x1, y1 = r["bbox"]
                self.assertLessEqual(-r["hotspot_x"], x0)
                self.assertLessEqual(-r["hotspot_y"], y0)
                self.assertGreaterEqual(r["width"] - r["hotspot_x"], x1)
                self.assertGreaterEqual(r["height"] - r["hotspot_y"], y1)

    def test_reports_terminal_index_order_not_input_order(self):
        r = self.build(parts=[{"type": "line", "x1": 0, "y1": -20, "x2": 0, "y2": 20}],
                       terminals=[{"x": 0, "y": 20, "orientation": "s", "name": "low"},
                                  {"x": 0, "y": -20, "orientation": "n", "name": "high"}])
        self.assertEqual(r["terminal_index_order"], ["high", "low"])

    def test_output_reads_back(self):
        r = self.build(names={"en": "Coil", "fr": "Bobine"})
        self.assertEqual(r["verified"]["names"], {"en": "Coil", "fr": "Bobine"})
        root = ET.parse(self.out).getroot()
        self.assertEqual(root.tag, "definition")
        self.assertEqual(root.get("link_type"), "simple")

    def test_special_characters_survive(self):
        r = self.build(names={"en": 'Coil "A" & <B>', "fr": "Résistance"})
        self.assertEqual(r["verified"]["names"]["fr"], "Résistance")
        self.assertEqual(r["verified"]["names"]["en"], 'Coil "A" & <B>')


class ElementSearch(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        m._ELEMENT_INDEX.clear()

    def tearDown(self):
        self.tmp.cleanup()

    def put(self, rel, names, link="simple", terminals=((0, -10, "n"), (0, 10, "s")), kind=""):
        p = self.root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        kinds = f'<kindInformations><kindInformation name="type">{kind}</kindInformation></kindInformations>' if kind else ""
        ns = "".join(f'<name lang="{l}">{n}</name>' for l, n in names.items())
        ts = "".join(f'<terminal x="{x}" y="{y}" orientation="{o}" name="t{i}"/>'
                     for i, (x, y, o) in enumerate(terminals))
        p.write_text(f'<definition type="element" link_type="{link}" width="20" height="40" '
                     f'hotspot_x="10" hotspot_y="20"><names>{ns}</names>{kinds}'
                     f'<description>{ts}</description></definition>', encoding="utf-8")

    def test_accents_and_case_are_ignored_across_languages(self):
        self.put("a/res.elmt", {"en": "Resistor", "fr": "Résistance", "de": "Widerstand"})
        for q in ("resistance", "RÉSISTANCE", "widerstand", "resistor"):
            with self.subTest(q=q):
                self.assertEqual(m.tool_element_search(str(self.root), q)["total_matches"], 1)

    def test_every_word_must_match(self):
        self.put("a/x.elmt", {"en": "Big red coil"})
        self.assertEqual(m.tool_element_search(str(self.root), "red coil")["total_matches"], 1)
        self.assertEqual(m.tool_element_search(str(self.root), "red fuse")["total_matches"], 0)

    def test_filters(self):
        self.put("a/m.elmt", {"en": "Coil"}, link="master", kind="coil")
        self.put("a/s.elmt", {"en": "Contact"}, link="slave", terminals=((0, 0, "n"),) * 1)
        self.put("a/t.elmt", {"en": "Big"}, terminals=[(0, i, "n") for i in range(5)])
        g = lambda **k: {r["name"] for r in m.tool_element_search(str(self.root), **k)["results"]}
        self.assertEqual(g(link_type="master"), {"Coil"})
        self.assertEqual(g(min_terminals=5), {"Big"})
        self.assertEqual(g(max_terminals=1), {"Contact"})
        self.assertEqual(g(kind="coil"), {"Coil"})

    def test_result_is_placeable_path_and_index_ordered_terminals(self):
        self.put("dir/x.elmt", {"en": "X"}, terminals=((0, 20, "s"), (0, -20, "n")))
        r = m.tool_element_search(str(self.root), "x")["results"][0]
        self.assertEqual(r["path"], "common://dir/x.elmt")
        self.assertEqual(r["terminal_names"], ["t1", "t0"])     # t1 is at y=-20: index 0

    def test_new_file_is_found_without_a_manual_reindex(self):
        """A symbol written by qet_element_build must be searchable at once."""
        self.put("a/one.elmt", {"en": "One"})
        self.assertEqual(m.tool_element_search(str(self.root), "two")["total_matches"], 0)
        self.put("a/two.elmt", {"en": "Two"})
        self.assertEqual(m.tool_element_search(str(self.root), "two")["total_matches"], 1)

    def test_exact_name_ranks_first(self):
        self.put("a/long.elmt", {"en": "Remanence coil, latching"})
        self.put("a/short.elmt", {"en": "Coil"})
        self.assertEqual(m.tool_element_search(str(self.root), "coil")["results"][0]["name"], "Coil")

    def test_bad_arguments(self):
        with self.assertRaises(ValueError):
            m.tool_element_search("/no/such/dir")
        with self.assertRaises(ValueError):
            m.tool_element_search(str(self.root), link_type="widget")
        with self.assertRaises(ValueError):
            m.tool_element_search(str(self.root), limit=0)

    def test_unparseable_files_are_skipped_not_fatal(self):
        (self.root / "bad.elmt").write_text("<definition")
        self.put("ok.elmt", {"en": "Fine"})
        self.assertEqual(m.tool_element_search(str(self.root), "fine")["total_matches"], 1)


class Diff(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def project(self, name, *, texts=(), shapes=(), images=(), author="", version="1",
                strips=(), conductors=()):
        inputs = "".join(f'<input x="{x}" y="{y}" rotation="0" font="f" '
                         f'text="&lt;html&gt;&lt;body&gt;&lt;p&gt;{t}&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;"/>'
                         for x, y, t in texts)
        shp = "".join(f'<shape type="Rectangle" x1="{a}" y1="{b}" x2="{c}" y2="{d}">'
                      f'<pen color="{pc}" style="SolidLine" widthF="1"/>'
                      f'<brush color="#fff" style="SolidPattern"/></shape>'
                      for a, b, c, d, pc in shapes)
        img = "".join(f'<image x="{x}" y="{y}" size="{s}" rotation="0"/>' for x, y, s in images)
        st = "".join(f'<terminal_strip><terminal_strip_data uuid="{u}"><informations>'
                     f'<information name="name">{n}</information></informations>'
                     f'</terminal_strip_data><layout/></terminal_strip>' for u, n in strips)
        cond = "".join(f'<conductor terminal1="{a}" terminal2="{b}" num="{n}"/>'
                       for a, b, n in conductors)
        xml = (f'<project title="t"><diagram title="D" author="{author}" version="{version}">'
               f'<elements/><conductors>{cond}</conductors><inputs>{inputs}</inputs>'
               f'<images>{img}</images><shapes>{shp}</shapes></diagram>'
               f'<terminal_strips>{st}</terminal_strips></project>')
        p = self.dir / name
        p.write_text(xml)
        return str(p)

    def test_identical_projects_have_an_empty_diff_everywhere(self):
        a = self.project("a.qet", texts=[(1, 2, "hi")], shapes=[(0, 0, 5, 5, "#000")],
                         images=[(3, 3, 1)], strips=[("{u}", "X1")])
        d = m.tool_diff(a, a)
        for k in ("texts", "shapes", "images", "terminal_strips"):
            with self.subTest(section=k):
                self.assertFalse(d[k]["added"] or d[k]["removed"] or d[k]["changed"])
        self.assertEqual(d["folios"]["changed"], [])

    def test_edited_text_reads_as_removed_plus_added(self):
        a = self.project("a.qet", texts=[(1, 2, "note")])
        b = self.project("b.qet", texts=[(1, 2, "note EDITED")])
        d = m.tool_diff(a, b)["texts"]
        self.assertEqual([t["text"] for t in d["removed"]], ["note"])
        self.assertEqual([t["text"] for t in d["added"]], ["note EDITED"])

    def test_shape_restyle_is_a_change_to_that_item(self):
        a = self.project("a.qet", shapes=[(0, 0, 5, 5, "#000000")])
        b = self.project("b.qet", shapes=[(0, 0, 5, 5, "#ff0000")])
        d = m.tool_diff(a, b)["shapes"]
        self.assertEqual(d["added"], [])
        self.assertEqual(d["changed"][0]["changed"]["line_color"], ["#000000", "#ff0000"])

    def test_image_rescale_is_a_change(self):
        a = self.project("a.qet", images=[(1, 1, 2)])
        b = self.project("b.qet", images=[(1, 1, 3)])
        self.assertEqual(m.tool_diff(a, b)["images"]["changed"][0]["changed"]["scale"], ["2", "3"])

    def test_folio_author_change(self):
        a = self.project("a.qet", author="A")
        b = self.project("b.qet", author="B")
        self.assertEqual(m.tool_diff(a, b)["folios"]["changed"][0]["changed"]["author"], ["A", "B"])

    def test_the_file_version_stamp_is_not_a_change(self):
        """QElectroTech rewrites every folio's version attribute on save;
        comparing it made every folio of a re-saved project look edited."""
        a = self.project("a.qet", version="0.80c")
        b = self.project("b.qet", version="0.200.1-dev")
        self.assertEqual(m.tool_diff(a, b)["folios"]["changed"], [])

    def test_element_text_field_moves_and_restyles_are_reported(self):
        def proj(x, size, frame, extra=""):
            return (f'<project><diagram><elements><element uuid="{{e}}"><dynamic_texts>'
                    f'<dynamic_elmt_text x="{x}" y="0" frame="{frame}" rotation="0" text_width="-1" '
                    f'text_from="ElementInfo" font="Sans,{size},-1,5"><text>KM1</text>'
                    f'<info_name>label</info_name></dynamic_elmt_text>{extra}'
                    f'</dynamic_texts></element></elements><conductors/></diagram></project>')
        a = self.dir / "a.qet"
        b = self.dir / "b.qet"
        a.write_text(proj(30, 9, "false"))
        b.write_text(proj(60, 14, "true", '<dynamic_elmt_text x="1" y="1" frame="false" rotation="0" '
                                          'text_width="-1" text_from="UserText" font="Sans,4"><text>NEW</text>'
                                          '</dynamic_elmt_text>'))
        d = m.tool_diff(str(a), str(b))["element_texts"]
        self.assertEqual(len(d["added"]), 1)
        self.assertEqual(d["changed"][0]["changed"],
                         {"x": ["30", "60"], "size": ["9", "14"], "frame": ["false", "true"]})
        self.assertEqual(d["changed"][0]["item"]["bound_to"], "label")
        self.assertFalse(m.tool_diff(str(a), str(a))["element_texts"]["changed"])

    def test_project_title_and_folio_frame_changes_are_reported(self):
        def proj(title, cols):
            return (f'<project title="{title}"><diagram title="D" cols="{cols}" colsize="60" rows="8" '
                    f'rowsize="80" displaycols="true" displayrows="true"><elements/><conductors/></diagram></project>')
        a, b = self.dir / "a.qet", self.dir / "b.qet"
        a.write_text(proj("Old", 17))
        b.write_text(proj("New", 10))
        d = m.tool_diff(str(a), str(b))
        self.assertEqual(d["project"]["changed"], {"title": ["Old", "New"]})
        self.assertEqual(d["folios"]["changed"][0]["changed"], {"cols": ["17", "10"]})
        self.assertEqual(m.tool_diff(str(a), str(a))["project"]["changed"], {})

    def test_strip_added(self):
        a = self.project("a.qet")
        b = self.project("b.qet", strips=[("{u}", "X9")])
        self.assertEqual(m.tool_diff(a, b)["terminal_strips"]["added"], ["X9"])

    def test_text_html_is_reduced_to_visible_text(self):
        self.assertEqual(m._plain_text("<html><head><style>p{}</style></head><body><p>a &amp; <b>b</b></p></body></html>"), "a & b")


class ConductorKey(unittest.TestCase):
    """The key must survive a save. QElectroTech renumbers the folio-scoped
    terminal ids on every write, so keying on the raw pair made every
    conductor of an untouched folio read as removed and re-added."""

    def diagram(self, terminals, conductors):
        d = ET.Element("diagram")
        for uuid, tid, x, y in terminals:
            el = ET.SubElement(d, "element", {"uuid": uuid})
            ET.SubElement(el, "terminal", {"id": tid, "x": str(x), "y": str(y), "orientation": "n"})
        for a, b in conductors:
            ET.SubElement(d, "conductor", {"terminal1": a, "terminal2": b})
        return d

    def keys(self, d):
        ix = m._terminal_index(d)
        return {m._conductor_key(1, c, ix) for c in d.iter("conductor")}

    def test_renumbering_the_ids_does_not_change_the_key(self):
        before = self.diagram([("{a}", "1", 0, 0), ("{b}", "16", 0, 0)], [("1", "16")])
        after = self.diagram([("{a}", "34", 0, 0), ("{b}", "15", 0, 0)], [("34", "15")])
        self.assertEqual(self.keys(before), self.keys(after))

    def test_end_order_does_not_matter(self):
        one = self.diagram([("{a}", "1", 0, 0), ("{b}", "2", 0, 0)], [("1", "2")])
        two = self.diagram([("{a}", "1", 0, 0), ("{b}", "2", 0, 0)], [("2", "1")])
        self.assertEqual(self.keys(one), self.keys(two))

    def test_different_conductors_get_different_keys(self):
        d = self.diagram([("{a}", "1", 0, 0), ("{b}", "2", 0, 0), ("{c}", "3", 0, 0)],
                         [("1", "2"), ("1", "3")])
        self.assertEqual(len(self.keys(d)), 2)

    def test_elements_without_uuid_stay_apart_and_are_marked(self):
        """Keying them on geometry collapsed nine distinct conductors onto
        one key, which was worse than the instability being fixed."""
        d = self.diagram([("", "1", 0, 0), ("", "2", 0, 0), ("", "3", 0, 0)],
                         [("1", "2"), ("2", "3"), ("1", "3")])
        keys = self.keys(d)
        self.assertEqual(len(keys), 3)
        self.assertTrue(all("#" in k for k in keys))

    def test_current_format_uses_element_and_terminal(self):
        """A terminal uuid belongs to the definition, so two coils of one type
        share it; the (instance, terminal) pair is the identity."""
        d = ET.Element("diagram")
        for a, b in (("{e1}", "{e2}"), ("{e1}", "{e3}")):
            ET.SubElement(d, "conductor", {"element1": a, "terminal1": "{T}",
                                           "element2": b, "terminal2": "{T}"})
        self.assertEqual(len(self.keys(d)), 2)


class ChecksDefinition(unittest.TestCase):
    def test_all_checks_are_read_only_selects(self):
        for name, c in m.CHECKS.items():
            with self.subTest(check=name):
                self.assertRegex(c["sql"].lstrip().upper(), r"^(SELECT|WITH)\b")
                self.assertNotRegex(c["sql"].upper(), r"\b(DELETE|UPDATE|INSERT|DROP|ALTER)\b")
                self.assertIn(c["severity"], ("error", "warning", "info"))
                self.assertTrue(c["note"])

    def test_text_comparisons_treat_null_as_empty(self):
        """A value never set is NULL for an element placed in this session
        and '' for one loaded from a file; `col = ''` matches only the
        latter. Every comparison must go through COALESCE."""
        for name, c in m.CHECKS.items():
            for col in ("label", "manufacturer_reference", "wire_number"):
                for m_ in re.finditer(rf"(?<![A-Za-z_(']){col}\s*(=|<>)\s*''", c["sql"]):
                    self.fail(f"{name}: bare {col} comparison at {m_.start()} misses NULL")

    def test_unknown_check_and_bad_sample(self):
        with tempfile.NamedTemporaryFile(suffix=".qet") as f:
            with self.assertRaises(ValueError):
                m.tool_check("/x", f.name, checks=["nope"])
            with self.assertRaises(ValueError):
                m.tool_check("/x", f.name, sample=-1)


class QueryGuard(unittest.TestCase):
    def test_obvious_writes_are_refused_before_launch(self):
        with tempfile.NamedTemporaryFile(suffix=".qet") as f:
            for sql in ("DELETE FROM element", "DROP TABLE element",
                        "UPDATE element SET x=1", "PRAGMA table_info(element)",
                        "INSERT INTO element VALUES (1)"):
                with self.subTest(sql=sql):
                    with self.assertRaisesRegex(ValueError, "read-only"):
                        m.tool_query("/nonexistent", f.name, sql)

    def test_missing_project(self):
        with self.assertRaisesRegex(ValueError, "no such project"):
            m.tool_query("/x", "/no/such.qet", "SELECT 1")


class ProjectNewValidation(unittest.TestCase):
    def test_refuses_overwrite_and_bad_arguments(self):
        with tempfile.TemporaryDirectory() as tmp:
            existing = Path(tmp) / "a.qet"
            existing.write_text("x")
            with self.assertRaisesRegex(ValueError, "already exists"):
                m.tool_project_new("/x", str(existing), "t")
            new = str(Path(tmp) / "n.qet")
            for kw in ({"folios": True}, {"folios": "2"}, {"folios": 500}, {"title": "  "}):
                with self.subTest(**kw):
                    with self.assertRaises(ValueError):
                        m.tool_project_new("/x", new, **({"title": "t"} | kw))
            self.assertFalse(Path(new).exists())


class ReadTools(unittest.TestCase):
    def test_project_info_and_scan_on_a_fixture(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "a.qet"
            p.write_text('<project title="T" version="1"><diagram title="D1">'
                         '<elements><element uuid="{1}" type="x"/></elements>'
                         '<conductors><conductor terminal1="1" terminal2="2" cable="C1"/></conductors>'
                         '</diagram></project>')
            info = m.tool_project_info(str(p))
            self.assertEqual((info["title"], info["folio_count"]), ("T", 1))
            scan = m.tool_scan(tmp, "conductor", "cable")
            self.assertEqual((scan["files"], scan["total"], scan["non_empty"]), (1, 1, 1))
            # (value, how many nodes carry it)
            self.assertEqual([tuple(v) for v in scan["distinct_values"]], [("C1", 1)])


# ==========================================================================
# protocol
# ==========================================================================

class Protocol(unittest.TestCase):
    def rpc(self, *messages):
        proc = subprocess.run([sys.executable, str(HERE / "qet_mcp.py")],
                              input="\n".join(json.dumps(x) for x in messages) + "\n",
                              capture_output=True, text=True, timeout=30)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        out = [json.loads(line) for line in proc.stdout.splitlines() if line.strip()]
        return {o["id"]: o for o in out if "id" in o}, proc

    def test_handshake_list_and_call_over_real_stdio(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "a.qet"
            p.write_text('<project title="Via stdio"><diagram title="D"/></project>')
            replies, proc = self.rpc(
                {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {}},
                {"jsonrpc": "2.0", "method": "notifications/initialized"},
                {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
                {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
                 "params": {"name": "qet_project_info", "arguments": {"path": str(p)}}},
                {"jsonrpc": "2.0", "id": 4, "method": "ping"})
        self.assertEqual(replies[1]["result"]["serverInfo"]["name"], "qet-mcp")
        self.assertEqual(len(replies[2]["result"]["tools"]), len(m.TOOLS))
        body = json.loads(replies[3]["result"]["content"][0]["text"])
        self.assertEqual(body["title"], "Via stdio")
        self.assertEqual(replies[4]["result"], {})
        self.assertEqual(len(replies), 4, "a notification must not get a reply")

    def test_stdout_carries_only_protocol(self):
        _, proc = self.rpc({"jsonrpc": "2.0", "id": 1, "method": "tools/list"})
        for line in proc.stdout.splitlines():
            if line.strip():
                json.loads(line)

    def test_tool_errors_are_reported_not_thrown(self):
        replies, _ = self.rpc({"jsonrpc": "2.0", "id": 1, "method": "tools/call",
                               "params": {"name": "qet_project_info", "arguments": {"path": "/no/such.qet"}}})
        self.assertTrue(replies[1]["result"]["isError"])

    def test_unknown_tool_and_method(self):
        replies, _ = self.rpc(
            {"jsonrpc": "2.0", "id": 1, "method": "tools/call", "params": {"name": "nope"}},
            {"jsonrpc": "2.0", "id": 2, "method": "no/such"})
        self.assertEqual(replies[1]["error"]["code"], -32602)
        self.assertEqual(replies[2]["error"]["code"], -32601)

    def test_list_flag(self):
        out = subprocess.run([sys.executable, str(HERE / "qet_mcp.py"), "--list"],
                             capture_output=True, text=True, timeout=30).stdout
        for t in m.TOOLS:
            self.assertIn(t["name"], out)


# ==========================================================================
# integration
# ==========================================================================

class Sandbox:
    """A scratch directory plus the binary/collection arguments."""

    def __init__(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def p(self, name):
        return str(self.dir / name)

    def new(self, title="T", folios=1, **kw):
        r = m.tool_project_new(BINARY, self.p(f"{title}.qet"), title=title, folios=folios,
                               overwrite=True, **kw)
        assert r["ok"], r
        return r["output"]

    def edit(self, project, ops, out="out.qet"):
        return m.tool_edit(BINARY, project, ops, self.p(out), elements_dir=ELEMENTS)

    def close(self):
        self.tmp.cleanup()


@needs_elements
class Integration(unittest.TestCase):
    def setUp(self):
        self.sb = Sandbox()

    def tearDown(self):
        self.sb.close()

    def ok(self, r):
        self.assertTrue(r["ok"], json.dumps({k: r.get(k) for k in ("hint", "operations")}, default=str)[:600])
        return r

    # ---- project_new ----

    def test_new_project_reads_back(self):
        r = m.tool_project_new(BINARY, self.sb.p("n.qet"), title='A "b" & <c>',
                               folios=["Power", "Control"], author="Me")
        self.assertTrue(r["ok"])
        self.assertEqual(r["project"]["title"], 'A "b" & <c>')
        self.assertEqual([f["title"] for f in r["project"]["folios"]], ["Power", "Control"])

    def test_new_project_with_zero_folios(self):
        r = m.tool_project_new(BINARY, self.sb.p("z.qet"), title="Z", folios=0)
        self.assertTrue(r["ok"])
        self.assertEqual(r["project"]["folio_count"], 0)

    # ---- edit: geometry, wiring, labels ----

    def test_place_wire_label_number_and_verify_by_diff(self):
        base = self.sb.new()
        ops = [
            {"op": "add_autonum", "kind": "conductor", "name": "W", "parts": ["string:W", "unit:1:1"]},
            {"op": "use_conductor_autonum", "folio": 0, "name": "W"},
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "b", "folio": 0, "path": COIL, "x": 300, "y": 100},
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0, "to": "$b", "to_terminal": 0},
            {"op": "set_label", "folio": 0, "element": "$a", "label": "KM1"},
            {"op": "rotate_element", "folio": 0, "element": "$b", "angle": 90},
        ]
        r = self.ok(self.sb.edit(base, ops))
        self.assertTrue(all(o["succeeded"] for o in r["operations"]))
        d = r["diff"]
        self.assertEqual(len(d["elements"]["added"]), 2)
        self.assertEqual(len(d["conductors"]["added"]), 1)
        self.assertEqual(d["conductors"]["removed"], [])
        nums = [c["num"] for c in m.tool_conductors(r["output"])["conductors"]]
        self.assertEqual(nums, ["W1"])

    def test_noop_edit_has_no_conductor_churn(self):
        """Re-saving renumbers the file's terminal ids; the diff must not
        report the untouched conductors as removed and re-added."""
        if not have_examples:
            self.skipTest("set QET_EXAMPLES")
        src = shutil.copy(Path(EXAMPLES) / "ArduinoLCD.qet", self.sb.p("in.qet"))
        r = self.ok(self.sb.edit(src, [{"op": "add_folio"}]))
        c = r["diff"]["conductors"]
        self.assertEqual((len(c["added"]), len(c["removed"])), (0, 0))
        self.assertEqual(c["before"], c["after"])

    def test_failed_operation_stops_the_run_and_reports_it(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 0, "y": 0},
            {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 99, "to": "$a", "to_terminal": 0},
            {"op": "set_label", "folio": "$f", "element": "$a", "label": "NEVER"},
        ])
        self.assertFalse(r["ok"])
        self.assertTrue(r["stopped_early"])
        self.assertEqual(len(r["operations"]), 3)
        self.assertIn("script", r, "the generated script is returned for debugging")

    def test_first_index_zero_is_not_a_failure(self):
        """add_folio on an empty project and the first text both return 0."""
        base = m.tool_project_new(BINARY, self.sb.p("e.qet"), title="E", folios=0)["output"]
        r = self.ok(self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_text", "id": "t", "folio": "$f", "text": "hi", "x": 5, "y": 5}]))
        self.assertEqual([o["result"] for o in r["operations"]], [0, 0])

    def test_two_potentials_are_refused_not_hung(self):
        """ConductorCreator opens a modal dialog here, and headless nobody
        can answer it: with the guard removed this call never returns."""
        base = self.sb.new()
        r = m.tool_edit(BINARY, base, [
            {"op": "add_folio", "id": "f"},
            *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL, "x": 100 + i * 200, "y": 100}
              for i in range(4)],
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e1", "to_terminal": 0},
            {"op": "add_conductor", "folio": "$f", "from": "$e2", "from_terminal": 0, "to": "$e3", "to_terminal": 0},
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e2", "to_terminal": 0},
        ], self.sb.p("conflict.qet"), elements_dir=ELEMENTS, timeout=60)
        self.assertFalse(r.get("timed_out"), "the call hung on a modal dialog")
        self.assertFalse(r["ok"])
        self.assertEqual(r["operations"][-1]["result"], False)

    # ---- edit: conductor properties, potentials ----

    def test_conductor_property_applies_to_the_whole_potential(self):
        base = self.sb.new()
        hub = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL, "x": 100 + i * 200, "y": 100}
                 for i in range(3)],
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e1", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e2", "to_terminal": 0},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0, "property": "num", "value": "W7"},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0, "property": "style", "value": "dashed"}]
        r = self.ok(self.sb.edit(base, hub))
        rows = m.tool_conductors(r["output"])["conductors"]
        self.assertEqual([c["num"] for c in rows], ["W7", "W7"])

    # ---- conductor manual routing ----

    def test_move_conductor_segment_reroutes_the_path(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 400, "y": 300},
               {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 0,
                "to": "$b", "to_terminal": 0},
               # the vertical segment between the two horizontal legs of the
               # default L-route -- see test_two_terminals_default_route_is_l_shaped
               # for how this index was established
               {"op": "move_conductor_segment", "folio": "$f", "element": "$a",
                "terminal": 0, "segment": 2, "dx": 50, "dy": 0}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertTrue(r["operations"][-1]["succeeded"])
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertIn("<segment", xml, "a manually modified path is written to the file")

    def test_two_terminals_default_route_is_l_shaped(self):
        """Establishes the canonical segment order a freshly auto-routed
        conductor has -- static, movable, movable, static -- that the
        tool description promises and other tests here rely on. Each
        segment is checked from a fresh, unmodified base: a successful
        move can insert a new segment and shift every index after it, so
        checking several in the same run would not be testing the
        original layout for the later ones."""
        base = self.sb.new(title="B")
        common = [{"op": "add_folio", "id": "f"},
                  {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
                  {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 400, "y": 300},
                  {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 0,
                   "to": "$b", "to_terminal": 0}]
        expected = {0: False, 1: True, 2: True, 3: False}
        for segment, should_succeed in expected.items():
            r = self.sb.edit(base, common + [
                {"op": "move_conductor_segment", "folio": "$f", "element": "$a",
                 "terminal": 0, "segment": segment, "dx": 1, "dy": 0}],
                out=f"seg{segment}.qet")
            self.assertEqual(r["operations"][-1]["succeeded"], should_succeed,
                             f"segment {segment}")

    def test_move_conductor_segment_bad_index_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 400, "y": 300},
            {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 0,
             "to": "$b", "to_terminal": 0},
            {"op": "move_conductor_segment", "folio": "$f", "element": "$a",
             "terminal": 0, "segment": 99, "dx": 1, "dy": 1}])
        self.assertFalse(r["ok"])

    # ---- search and replace ----

    def test_search_and_replace_element_info_across_folios(self):
        base = self.sb.new(folios=2)
        ops = [{"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "b", "folio": 0, "path": COIL, "x": 300, "y": 100},
               {"op": "add_element", "id": "c", "folio": 1, "path": COIL, "x": 100, "y": 100},
               {"op": "set_label", "folio": 0, "element": "$a", "label": "OLD-KM1"},
               {"op": "set_label", "folio": 0, "element": "$b", "label": "KM2"},
               {"op": "set_label", "folio": 1, "element": "$c", "label": "OLD-KM3"},
               {"op": "search_and_replace", "id": "n", "kind": "element_info", "field": "label",
                "pattern": "OLD-", "replacement": "NEW-", "regex": False, "case_sensitive": True}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual(r["operations"][-1]["result"], 2)
        rows = m.tool_elements(r["output"])["elements"]
        labels = sorted(e["label"] for e in rows if e.get("label"))
        self.assertEqual(labels, ["KM2", "NEW-KM1", "NEW-KM3"])

    def test_search_and_replace_conductor_updates_whole_potential(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL,
                 "x": 100 + i * 200, "y": 100} for i in range(3)],
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e1", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e2", "to_terminal": 0},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0,
                "property": "cable", "value": "CAB-OLD-1"},
               {"op": "search_and_replace", "kind": "conductor", "field": "cable",
                "pattern": "OLD", "replacement": "NEW", "regex": False, "case_sensitive": True}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual(r["operations"][-1]["result"], 1)
        rows = m.tool_conductors(r["output"])["conductors"]
        self.assertEqual([c["cable"] for c in rows], ["CAB-NEW-1", "CAB-NEW-1"])

    def test_search_and_replace_regex_on_text(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_text", "folio": "$f", "text": "note about OLD wiring", "x": 0, "y": 0},
               {"op": "add_text", "folio": "$f", "text": "OLDER stuff unrelated", "x": 0, "y": 20},
               {"op": "search_and_replace", "kind": "text", "field": "",
                "pattern": r"\bOLD\b", "replacement": "NEW", "regex": True, "case_sensitive": True}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual(r["operations"][-1]["result"], 1, "\\b keeps OLDER from matching")

    def test_search_and_replace_no_match_returns_zero_and_pushes_no_undo_step(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
               {"op": "set_label", "folio": "$f", "element": "$a", "label": "KM1"},
               {"op": "search_and_replace", "kind": "element_info", "field": "label",
                "pattern": "NOPE", "replacement": "X", "regex": False, "case_sensitive": True},
               {"op": "undo"}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual(r["operations"][-2]["result"], 0)
        # undo must revert set_label, not a no-op search_and_replace entry
        rows = m.tool_elements(r["output"])["elements"]
        self.assertFalse(any(e.get("label") == "KM1" for e in rows))

    def test_search_and_replace_bad_arguments_are_refused(self):
        base = self.sb.new()
        with self.assertRaises(ValueError):
            self.sb.edit(base, [{"op": "search_and_replace", "kind": "bogus", "field": "",
                                 "pattern": "x", "replacement": "y",
                                 "regex": False, "case_sensitive": True}])
        with self.assertRaises(ValueError):
            self.sb.edit(base, [{"op": "search_and_replace", "kind": "conductor", "field": "bogus",
                                 "pattern": "x", "replacement": "y",
                                 "regex": False, "case_sensitive": True}])
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "search_and_replace", "kind": "text", "field": "",
             "pattern": "(unclosed", "replacement": "y", "regex": True, "case_sensitive": True}])
        self.assertFalse(r["ok"], "an invalid regex must be refused, not crash")

    def test_ambiguous_terminal_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL, "x": 100 + i * 200, "y": 100}
              for i in range(3)],
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e1", "to_terminal": 0},
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e2", "to_terminal": 0},
            {"op": "set_conductor", "folio": "$f", "element": "$e0", "terminal": 0, "property": "num", "value": "X"}])
        self.assertFalse(r["ok"], "the hub carries two conductors and names none of them")

    def test_delete_conductor_removes_only_that_one(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL, "x": 100 + i * 200, "y": 100}
                 for i in range(3)],
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e1", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e2", "to_terminal": 0},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0, "property": "num", "value": "W7"},
               {"op": "delete_conductor", "folio": "$f", "element": "$e1", "terminal": 0}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual([c["num"] for c in m.tool_conductors(r["output"])["conductors"]], ["W7"])

    # ---- cross references, strips, folios ----

    def test_cross_reference_across_folios(self):
        base = self.sb.new(folios=2)
        r = self.ok(self.sb.edit(base, [
            {"op": "add_element", "id": "m", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "s", "folio": 1, "path": SLAVE, "x": 100, "y": 100},
            {"op": "link_elements", "folio": 0, "element": "$m", "to_folio": 1, "to": "$s"}]))
        self.assertEqual(r["output_exists"], True)
        linked = m.tool_query(BINARY, r["output"], "SELECT COUNT(*) AS n FROM element")["rows"][0]["n"]
        self.assertEqual(linked, 2)
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertEqual(xml.count("<link_uuid"), 2, "written on both folios")

    def test_master_to_master_link_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 0, "y": 0},
            {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 200, "y": 0},
            {"op": "link_elements", "folio": "$f", "element": "$a", "to_folio": "$f", "to": "$b"}])
        self.assertFalse(r["ok"])

    def test_terminal_strip_takes_terminals_only(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "t", "folio": "$f", "path": TERMINAL, "x": 0, "y": 0},
            {"op": "add_element", "id": "c", "folio": "$f", "path": COIL, "x": 200, "y": 0},
            {"op": "add_terminal_strip", "id": "s", "installation": "=A", "location": "+B", "name": "X1"},
            {"op": "add_to_strip", "strip": "$s", "folio": "$f", "element": "$t"},
            {"op": "add_to_strip", "strip": "$s", "folio": "$f", "element": "$c"}])
        self.assertFalse(r["ok"])
        self.assertEqual(r["operations"][-2]["succeeded"], True)
        self.assertEqual(r["operations"][-1]["succeeded"], False)
        self.assertEqual(r["diff"]["terminal_strips"]["added"], ["=A +B X1"])

    def test_group_bridge_and_sort_a_terminal_strip(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_terminal_strip", "id": "s", "installation": "=A", "location": "+B", "name": "X1"}]
        for i in range(4):
            ops.append({"op": "add_element", "id": f"t{i}", "folio": "$f", "path": TERMINAL,
                       "x": i * 80, "y": 0})
        for i in range(4):
            ops.append({"op": "add_to_strip", "strip": "$s", "folio": "$f", "element": f"$t{i}"})
        ops.append({"op": "group_terminals", "strip": "$s", "indices": [0, 1]})
        r = self.ok(self.sb.edit(base, ops, "grouped.qet"))
        self.assertTrue(all(o["succeeded"] for o in r["operations"]))

        r2 = self.ok(self.sb.edit(r["output"], [
            # 2 and 3 are the two real terminals group_terminals([0, 1]) left
            # untouched, so both are still alone on their own position --
            # unlike 0 and 1, which are now grouped together and are not
            # bridgeable against something on a different position.
            {"op": "bridge_terminals", "strip": 0, "indices": [2, 3]},
            {"op": "sort_terminal_strip", "strip": 0}], "sorted.qet"))
        self.assertTrue(all(o["succeeded"] for o in r2["operations"]))

    def test_group_and_bridge_refusals(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_terminal_strip", "id": "s", "installation": "=A", "location": "+B", "name": "X1"},
               {"op": "add_element", "id": "t0", "folio": "$f", "path": TERMINAL, "x": 0, "y": 0},
               {"op": "add_to_strip", "strip": "$s", "folio": "$f", "element": "$t0"}]
        r = self.sb.edit(base, ops + [{"op": "group_terminals", "strip": "$s", "indices": [0]}])
        self.assertFalse(r["ok"], "only one real terminal exists: fewer than two named")
        r = self.sb.edit(base, ops + [{"op": "group_terminals", "strip": "$s", "indices": [0, 99]}])
        self.assertFalse(r["ok"], "index 99 does not exist")
        r = self.sb.edit(base, ops + [{"op": "group_terminals", "strip": 9, "indices": [0, 1]}])
        self.assertFalse(r["ok"], "no strip at index 9")

    def test_group_receiver_is_the_larger_position_not_the_first_named(self):
        """The terminal strip editor's own group button chooses the position
        that already carries the most real terminals as the receiver, not
        the first one selected. Built to distinguish the two: group three
        terminals first (one position ends up with three), then group a
        previously-alone fourth terminal together with one of those three,
        naming the alone one first -- it must be the one that moves.

        Asserted on the sorted multiset of counts, not on which listing
        index reports which: stripRealTerminals()'s order is rebuilt from
        the strip's current physical-position layout and is not preserved
        identity across a group() call -- measured directly, the element
        that had been listed first before grouping was listed last after,
        not merely shifted. Only the count each real terminal's position
        carries is a claim this test can actually stand behind."""
        script = ("var f=qet.addFolio();"
                  f"var T='{TERMINAL}';var t=[];for(var i=0;i<5;i++)t.push(qet.addElement(f,T,i*80,0));"
                  "var s=qet.addTerminalStrip('=A','+B','X1');"
                  "for(var i=0;i<5;i++)qet.addTerminalToStrip(s,f,t[i]);"
                  "qet.groupTerminals(s,[2,3,4]);"
                  "qet.groupTerminals(s,[0,2]);"
                  "var counts=qet.stripRealTerminals(s).map(function(l){"
                  "return parseInt(l.match(/\\((\\d+) terminal/)[1]);});"
                  "qet.log('GT '+JSON.stringify(counts));")
        out = m._run_qet(BINARY, [self.sb.new()], elements_dir=ELEMENTS, script=script)
        line = [l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("GT ")][0]
        counts = json.loads(line[3:])
        self.assertEqual(sorted(counts), [1, 4, 4, 4, 4],
                         "one real terminal alone, four merged onto one position")

    def test_indices_argument_validation(self):
        """A single index ([0]) is syntactically a valid list of integers --
        it is refused at runtime, by the C++ (at least two are required to
        group or bridge anything), covered separately in
        test_group_and_bridge_refusals, not rejected as malformed here."""
        base = self.sb.new()
        for bad in ([], ["0", "1"], [True, False], None):
            with self.subTest(indices=bad):
                with self.assertRaises(ValueError):
                    self.sb.edit(base, [{"op": "group_terminals", "strip": 0, "indices": bad}])

    # ---- tables ----

    def test_add_table_requires_a_non_empty_query(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_table", "folio": "$f", "kind": "nomenclature", "name": "BOM", "query": ""}])
        self.assertFalse(r["ok"], "each query widget defaults to zero selected "
                                  "columns, so an empty query is refused rather than "
                                  "silently producing a rowless table")

    def test_add_table_populates_rows_from_the_query(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL,
                 "x": 100 + i * 150, "y": 100} for i in range(3)],
               *[{"op": "set_label", "folio": "$f", "element": f"$e{i}", "label": f"KM{i}"}
                 for i in range(3)],
               {"op": "add_table", "id": "t", "folio": "$f", "kind": "nomenclature",
                "name": "BOM",
                "query": "SELECT label, quantity FROM element_nomenclature_view ORDER BY label"}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertEqual(r["operations"][-1]["result"], 0, "first table on the folio")
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertEqual(xml.count("<graphics_table"), 1)

    def test_add_table_unknown_kind_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_table", "folio": "$f", "kind": "bogus", "name": "x", "query": "SELECT 1"}])
        self.assertFalse(r["ok"])

    def test_two_tables_need_repositioning_or_they_overlap(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_table", "id": "t1", "folio": "$f", "kind": "nomenclature",
                "name": "BOM", "query": "SELECT label FROM element_nomenclature_view"},
               {"op": "add_table", "id": "t2", "folio": "$f", "kind": "summary",
                "name": "SUMMARY", "query": "SELECT * FROM project_summary_view"},
               {"op": "set_table_position", "folio": "$f", "table": "$t2", "x": 400, "y": 300}]
        r = self.ok(self.sb.edit(base, ops))
        # Both tables land at the same (50, 50) until repositioned, so each
        # add_table's returned index reflects only a snapshot at that call
        # -- the second add can legitimately sort before the first and hand
        # back 0 again, not necessarily 1. What must hold is that each
        # result is a valid index and the immediately-following
        # set_table_position (naming "$t2" from the moment it was returned)
        # still succeeds.
        self.assertEqual([o["result"] in (0, 1) for o in r["operations"][1:3]], [True, True])
        self.assertTrue(r["operations"][-1]["succeeded"])

    def test_set_table_position_bad_index_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "set_table_position", "folio": "$f", "table": 9, "x": 0, "y": 0}])
        self.assertFalse(r["ok"])

    def test_delete_table_removes_it(self):
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_table", "id": "t", "folio": "$f", "kind": "nomenclature",
             "name": "BOM", "query": "SELECT label FROM element_nomenclature_view"},
            {"op": "delete_table", "folio": "$f", "table": "$t"}]))
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertEqual(xml.count("<graphics_table"), 0)

    def test_remove_folio_and_set_folio_property(self):
        base = self.sb.new(folios=3)
        r = self.ok(self.sb.edit(base, [
            {"op": "set_folio", "folio": 0, "property": "author", "value": "Shane"},
            {"op": "remove_folio", "folio": 2}]))
        self.assertEqual(r["diff"]["folios"]["after"], 2)
        self.assertEqual(r["diff"]["folios"]["changed"][0]["changed"]["author"], ["", "Shane"])

    # ---- geometry, folio order, undo ----

    def test_element_geometry_matches_the_file_and_the_hotspot(self):
        """The origin is what the file stores, and the box sits at the
        element's hotspot from it (17, 32 for this coil); a 90 degree turn
        swaps the box's width and height about a fixed origin."""
        base = self.sb.new()
        ops = [{"op": "add_element", "id": "e", "folio": 0, "path": COIL, "x": 200, "y": 300},
               {"op": "element_geometry", "folio": 0, "element": "$e"},
               {"op": "move_element", "folio": 0, "element": "$e", "dx": 50, "dy": -20},
               {"op": "rotate_element", "folio": 0, "element": "$e", "angle": 90},
               {"op": "element_geometry", "folio": 0, "element": "$e"}]
        r = self.ok(self.sb.edit(base, ops))
        g0, g1 = r["operations"][1]["result"], r["operations"][4]["result"]
        self.assertEqual((g0["x"], g0["y"]), (200, 300))
        self.assertEqual((g0["x"] - g0["left"], g0["y"] - g0["top"]), (17, 32), "the coil's hotspot")
        self.assertEqual((g0["right"] - g0["left"], g0["bottom"] - g0["top"]), (40, 60))
        self.assertEqual((g1["x"], g1["y"], g1["rotation"]), (250, 280, 90))
        self.assertEqual((g1["right"] - g1["left"], g1["bottom"] - g1["top"]), (60, 40), "turned")
        saved = m.tool_elements(r["output"])["elements"][0]
        self.assertEqual((float(saved["x"]), float(saved["y"])), (g1["x"], g1["y"]))

    def test_geometry_of_a_missing_element_fails_the_run(self):
        base = self.sb.new()
        r = self.sb.edit(base, [{"op": "element_geometry", "folio": 0, "element": "{nope}"},
                                {"op": "add_folio"}])
        self.assertFalse(r["ok"])
        self.assertEqual(len(r["operations"]), 1, "later operations are skipped")

    def test_insert_folio_orders_and_refuses_out_of_range(self):
        base = self.sb.new(folios=["A", "B"])
        r = self.ok(self.sb.edit(base, [
            {"op": "insert_folio", "id": "f0", "position": 0},
            {"op": "set_folio_title", "folio": "$f0", "title": "FIRST"},
            {"op": "insert_folio", "id": "f2", "position": 2},
            {"op": "set_folio_title", "folio": "$f2", "title": "MID"},
            {"op": "insert_folio", "id": "f5", "position": 4},
            {"op": "set_folio_title", "folio": "$f5", "title": "LAST"}]))
        titles = [f["title"] for f in m.tool_project_info(r["output"])["folios"]]
        self.assertEqual(titles, ["FIRST", "A", "MID", "B", "LAST"])
        for bad in (-1, 3, 999):
            with self.subTest(position=bad):
                # QList::insert() is undefined past the end, so this must be
                # refused rather than reach it
                self.assertFalse(self.sb.edit(base, [{"op": "insert_folio", "position": bad}])["ok"])

    def test_undo_and_redo_step_the_stack_and_fail_when_empty(self):
        base = self.sb.new(folios=1)
        r = self.ok(self.sb.edit(base, [{"op": "add_folio"}, {"op": "add_folio"}, {"op": "undo"}]))
        self.assertEqual(r["diff"]["folios"]["after"], 2, "three folios, one undone")
        r = self.ok(self.sb.edit(base, [{"op": "add_folio"}, {"op": "undo"}, {"op": "redo"}]))
        self.assertEqual(r["diff"]["folios"]["after"], 2)
        nothing = self.sb.edit(base, [{"op": "undo"}])
        self.assertFalse(nothing["ok"], "an undo with nothing to undo is a failure")

    # ---- project title and folio frame ----

    def test_project_title_and_folio_frame(self):
        base = self.sb.new(folios=1)
        r = self.ok(self.sb.edit(base, [
            {"op": "set_project_title", "title": "Renamed"},
            {"op": "set_folio_border", "folio": 0, "property": "columns", "value": "10"},
            {"op": "set_folio_border", "folio": 0, "property": "column-width", "value": "40"},
            {"op": "set_folio_border", "folio": 0, "property": "display-rows", "value": "false"}]))
        self.assertEqual(r["diff"]["project"]["changed"], {"title": [Path(base).stem, "Renamed"]})
        ch = r["diff"]["folios"]["changed"][0]["changed"]
        self.assertEqual(ch["cols"], ["17", "10"])
        self.assertEqual(ch["colsize"], ["60", "40"])
        self.assertEqual(ch["displayrows"], ["true", "false"])
        self.assertEqual(m.tool_project_info(r["output"])["title"], "Renamed")

    def test_folio_frame_bounds_are_refused(self):
        """Counts are 1-99 and sizes 1-1000. 0 is what the application's own
        panel allows at the bottom, and was left refused rather than assumed
        safe."""
        base = self.sb.new()
        for prop, bad in (("columns", "0"), ("columns", "100"), ("rows", "1.5"),
                          ("column-width", "0"), ("row-height", "1001"), ("display-rows", "maybe")):
            with self.subTest(prop=prop, value=bad):
                r = self.sb.edit(base, [{"op": "set_folio_border", "folio": 0, "property": prop, "value": bad}])
                self.assertFalse(r["ok"])
        for prop, good in (("columns", "99"), ("columns", "1"), ("column-width", "1"), ("row-height", "1000")):
            with self.subTest(prop=prop, value=good):
                self.ok(self.sb.edit(base, [{"op": "set_folio_border", "folio": 0, "property": prop, "value": good}]))

    # ---- duplicating ----

    def test_duplicate_returns_copies_in_the_order_asked(self):
        """The application's own pasted list is in scene order: asking for the
        elements at x = 700, 100, 900 returned the copies of 100, 700, 900, so
        a caller pairing by index was wired to the wrong copies without any
        error. Checked with a scrambled request over a zig-zag layout, where
        'every copy is the same translation from the source at its index' can
        only hold if the pairing is right."""
        base = self.sb.new(folios=2)
        ops = [{"op": "add_element", "id": f"e{i}", "folio": 0, "path": COIL,
                "x": 100 + i * 200, "y": 100 + (i % 2) * 90} for i in range(5)]
        ops.append({"op": "duplicate_elements", "id": "c", "folio": 0, "to_folio": 1, "x": 50, "y": 50,
                    "elements": ["$e3", "$e0", "$e4", "$e2", "$e1"]})
        # give each copy a distinct label through its own reference, then read them back
        ops += [{"op": "set_label", "folio": 1, "element": f"$c[{k}]", "label": f"C{k}"} for k in range(5)]
        r = self.ok(self.sb.edit(base, ops))
        root = ET.parse(r["output"]).getroot()
        diagrams = root.findall("diagram")

        def positions(d):
            return {e.get("uuid"): (float(e.get("x")), float(e.get("y"))) for e in d.iter("element")}
        src, dst = positions(diagrams[0]), positions(diagrams[1])
        by_label = {}
        for e in diagrams[1].iter("element"):
            for i in e.iter("elementInformation"):
                if i.get("name") == "label":
                    by_label[i.text] = e.get("uuid")
        ordered = [i for i in (3, 0, 4, 2, 1)]
        offsets = set()
        for k, i in enumerate(ordered):
            src_uuid = r["operations"][i]["result"]
            (sx, sy), (cx, cy) = src[src_uuid], dst[by_label[f"C{k}"]]
            offsets.add((round(cx - sx, 3), round(cy - sy, 3)))
        self.assertEqual(len(offsets), 1, f"copies are not a pure translation of the requested order: {offsets}")

    def test_duplicate_copies_the_conductors_between_and_only_those(self):
        base = self.sb.new(folios=2)
        r = self.ok(self.sb.edit(base, [
            *[{"op": "add_element", "id": x, "folio": 0, "path": COIL, "x": 100 + i * 200, "y": 100}
              for i, x in enumerate("abc")],
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0, "to": "$b", "to_terminal": 0},
            {"op": "add_conductor", "folio": 0, "from": "$b", "from_terminal": 1, "to": "$c", "to_terminal": 0},
            {"op": "duplicate_elements", "id": "d", "folio": 0, "elements": ["$a", "$b"],
             "to_folio": 1, "x": 50, "y": 50}]))
        info = m.tool_project_info(r["output"])
        self.assertEqual([f["conductors"] for f in info["folios"]], [2, 1],
                         "a->b copies; b->c does not, since c was not copied")
        self.assertEqual([f["elements"] for f in info["folios"]], [3, 2])

    def test_duplicate_is_one_undo_step_and_refusals(self):
        script = (f"var f=qet.addFolio(),g=qet.addFolio();var a=qet.addElement(f,'{COIL}',0,0),b=qet.addElement(f,'{COIL}',200,0);"
                  "qet.addConductor(f,a,0,b,0);qet.selectElement(b);var out={};"
                  "out.n=qet.duplicateElements(f,[a,b],g,50,50).length;"
                  "out.kept=qet.selectedElements(f).join()===b;"
                  "qet.undo();out.after_undo=[qet.elementCount(g),qet.conductorCount(g)];"
                  "out.bad=[qet.duplicateElements(f,['{nope}'],g,0,0).length,"
                  "qet.duplicateElements(f,[a],9,0,0).length,qet.duplicateElements(f,[],g,0,0).length];"
                  "qet.log('DU '+JSON.stringify(out));")
        out = m._run_qet(BINARY, [self.sb.new("dupu")], elements_dir=ELEMENTS, script=script)
        line = [l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("DU ")][0]
        r = json.loads(line[3:])
        self.assertEqual(r["n"], 2)
        self.assertTrue(r["kept"], "copying works by selecting, so the selection must be given back")
        self.assertEqual(r["after_undo"], [0, 0], "elements and conductor go in one step")
        self.assertEqual(r["bad"], [0, 0, 0])

    def test_a_failed_duplicate_stops_the_run(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "duplicate_elements", "folio": "$f", "elements": ["{not-an-element}"],
             "to_folio": "$f", "x": 1, "y": 1},
            {"op": "add_element", "folio": "$f", "path": COIL, "x": 0, "y": 0}])
        self.assertFalse(r["ok"])
        self.assertTrue(r["stopped_early"])
        self.assertEqual(len(r["operations"]), 2)

    # ---- element auto-numbering ----

    def test_element_numbering_labels_in_sequence_and_undoes_as_one_step(self):
        """setUpFormula() writes the label straight into the element and only
        the counter's advance is undoable, so for an element already placed
        one undo rolled the counter back and left the label: c3 stayed 'K3'
        while the counter went back to expecting K3."""
        script = (f"var f=qet.addFolio();qet.addAutoNum('element','EL',['elementprefix','unit:1:1']);"
                  "qet.useElementAutoNum('EL');var e=[];"
                  f"for(var i=0;i<3;i++)e.push(qet.addElement(f,'{COIL}',i*200,0));"
                  "e.forEach(function(u){qet.numberElement(f,u)});"
                  "var out={after:e.map(function(u){return qet.elementLabel(f,u)})};"
                  "qet.undo();out.undone=e.map(function(u){return qet.elementLabel(f,u)});"
                  "qet.numberElement(f,e[2]);out.again=qet.elementLabel(f,e[2]);"
                  "qet.log('EN '+JSON.stringify(out));")
        out = m._run_qet(BINARY, [self.sb.new("num")], elements_dir=ELEMENTS, script=script)
        line = [l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("EN ")][0]
        r = json.loads(line[3:])
        self.assertEqual(r["after"], ["K1", "K2", "K3"])
        self.assertEqual(r["undone"], ["K1", "K2", ""], "the label must go back with the counter")
        self.assertEqual(r["again"], "K3", "the counter must agree with the labels")

    def test_element_numbering_refusals(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 0, "y": 0},
            {"op": "number_element", "folio": "$f", "element": "$a"}])
        self.assertFalse(r["ok"], "no context selected: nothing to apply")
        r = self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_autonum", "kind": "element", "name": "EL", "parts": ["elementprefix", "unit:1:1"]},
            {"op": "use_element_autonum", "name": "EL"},
            {"op": "add_element", "id": "s", "folio": "$f", "path": SLAVE, "x": 0, "y": 0},
            {"op": "number_element", "folio": "$f", "element": "$s"}])
        self.assertFalse(r["ok"], "a slave takes its label from its master")

    # ---- element text fields ----

    def test_element_text_fields_follow_the_info_and_can_be_restyled(self):
        base = self.sb.new()
        first = self.ok(self.sb.edit(base, [
            {"op": "add_element", "id": "k", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "set_label", "folio": 0, "element": "$k", "label": "KM1"}]))
        uuid = first["operations"][0]["result"]
        r = self.ok(self.sb.edit(first["output"], [
            {"op": "set_element_text", "folio": 0, "element": uuid, "index": 2, "property": "x", "value": "60"},
            {"op": "set_element_text", "folio": 0, "element": uuid, "index": 2, "property": "size", "value": "14"},
            {"op": "set_element_text", "folio": 0, "element": uuid, "index": 2, "property": "frame", "value": "true"},
            {"op": "add_element_text", "id": "c", "folio": 0, "element": uuid, "source": "info",
             "value": "comment", "x": 40, "y": 30},
            {"op": "set_info", "folio": 0, "element": uuid, "key": "comment", "value": "24VDC coil"}], "restyled.qet"))
        d = r["diff"]["element_texts"]
        self.assertEqual(len(d["added"]), 1)
        self.assertEqual(d["changed"][0]["changed"],
                         {"x": ["30", "60"], "size": ["9", "14"], "frame": ["false", "true"]})
        # the added field is bound to the comment key, so it shows the value set afterwards
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertRegex(xml, r"<text>24VDC coil</text>\s*<info_name>comment</info_name>")

    def test_shows_follows_the_information_and_text_does_not(self):
        """Two things are called text on an information-bound field. The
        stored "text" property is an unused placeholder ('' or 'Texte'); what
        is drawn -- "shows" -- follows the element's information immediately.
        An earlier version of this test claimed the display was stale and
        that a helper was needed; measuring showed toPlainText() matched the
        information at every checkpoint, and it was the stored text that
        looked wrong. Reading the stored text as the display is the mistake
        this pins."""
        script = (f"var f=qet.addFolio();var e=qet.addElement(f,'{COIL}',0,0);"
                  "var out=[];function snap(t){var lab=-1,com=-1;"
                  "qet.elementTexts(f,e).forEach(function(l,i){if(l.indexOf(\"info='label'\")>=0)lab=i;"
                  "if(l.indexOf(\"info='comment'\")>=0)com=i;});"
                  "out.push({t:t,shows:qet.elementTextProperty(f,e,lab,'shows'),info:qet.elementInfo(f,e,'label'),"
                  "stored:qet.elementTextProperty(f,e,lab,'text'),"
                  "cshows:com>=0?qet.elementTextProperty(f,e,com,'shows'):null,"
                  "cinfo:com>=0?qet.elementInfo(f,e,'comment'):null});}"
                  "snap('fresh');qet.setElementLabel(f,e,'KM1');snap('label');"
                  "qet.addElementText(f,e,'info','comment',1,1);qet.setElementInfo(f,e,'comment','hello');snap('comment');"
                  "qet.setElementLabel(f,e,'KM9');snap('relabel');"
                  "qet.log('TX '+JSON.stringify(out));")
        out = m._run_qet(BINARY, [self.sb.new("live2")], elements_dir=ELEMENTS, script=script)
        line = [l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("TX ")][0]
        rows = json.loads(line[3:])
        for r in rows:
            with self.subTest(step=r["t"]):
                self.assertEqual(r["shows"], r["info"])
                if r["cinfo"] is not None:
                    self.assertEqual(r["cshows"], r["cinfo"])
        self.assertEqual([r["shows"] for r in rows], ["", "KM1", "KM1", "KM9"])
        # the stored property is not the display: it never became KM1/KM9
        self.assertEqual({r["stored"] for r in rows}, {""})

    def test_bad_element_text_arguments_are_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_element", "id": "k", "folio": 0, "path": COIL, "x": 0, "y": 0},
            {"op": "add_element_text", "folio": 0, "element": "$k", "source": "info", "value": "nonsense",
             "x": 0, "y": 0}])
        self.assertFalse(r["ok"], "'nonsense' is not an element information key")

    # ---- text, shapes, images ----

    def test_text_shape_and_image_round_trip_through_diff(self):
        img = self.sb.dir / "pic.png"
        png(img)
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_text", "id": "t", "folio": 0, "text": "note", "x": 50, "y": 50},
            {"op": "set_text_color", "folio": 0, "index": "$t", "color": "#ff0000"},
            {"op": "add_shape", "id": "s", "folio": 0, "shape": "rectangle", "x1": 10, "y1": 10, "x2": 200, "y2": 120},
            {"op": "set_shape", "folio": 0, "index": "$s", "property": "fill", "value": "#00ff00"},
            {"op": "add_image", "id": "i", "folio": 0, "file": str(img), "x": 60, "y": 200},
            {"op": "scale_image", "folio": 0, "index": "$i", "factor": 2}]))
        d = r["diff"]
        self.assertEqual([t["text"] for t in d["texts"]["added"]], ["note"])
        self.assertEqual(len(d["shapes"]["added"]), 1)
        self.assertEqual(len(d["images"]["added"]), 1)

    # ---- polygon and path shapes ----

    def test_add_polygon_with_more_than_two_points(self):
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_polygon", "folio": 0,
             "points": [{"x": 0, "y": 0}, {"x": 100, "y": 0}, {"x": 100, "y": 100},
                       {"x": 0, "y": 100}, {"x": 50, "y": 150}],
             "closed": True}]))
        self.assertEqual(len(r["diff"]["shapes"]["added"]), 1)
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertEqual(xml.count("<point "), 5, "every vertex is written")

    def test_add_polygon_too_few_points_is_refused(self):
        base = self.sb.new()
        with self.assertRaises(ValueError):
            self.sb.edit(base, [
                {"op": "add_polygon", "folio": 0, "points": [{"x": 0, "y": 0}], "closed": True}])

    def test_set_shape_polygon_replaces_points(self):
        base = self.sb.new()
        ops = [{"op": "add_polygon", "id": "p", "folio": 0,
               "points": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 10, "y": 10}],
               "closed": True},
               {"op": "set_shape_polygon", "folio": 0, "index": "$p",
                "points": [{"x": 20, "y": 20}, {"x": 30, "y": 20}, {"x": 30, "y": 30}, {"x": 20, "y": 30}]}]
        r = self.ok(self.sb.edit(base, ops))
        self.assertTrue(r["operations"][-1]["succeeded"])

    def test_set_shape_polygon_on_a_non_polygon_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_shape", "id": "s", "folio": 0, "shape": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 1},
            {"op": "set_shape_polygon", "folio": 0, "index": "$s",
             "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}]}])
        self.assertFalse(r["ok"])

    def test_add_path_with_curve_nodes(self):
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_path", "folio": 0,
             "nodes": [{"x": 0, "y": 0},
                      {"x": 100, "y": 0, "kind": "smooth",
                       "inHandle": {"x": 80, "y": 0}, "outHandle": {"x": 120, "y": 0}},
                      {"x": 100, "y": 100}],
             "closed": False}]))
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertIn('kind="smooth"', xml)

    def test_set_shape_path_nodes_on_a_non_path_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_polygon", "id": "p", "folio": 0,
             "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}], "closed": True},
            {"op": "set_shape_path_nodes", "folio": 0, "index": "$p",
             "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 1}]}])
        self.assertFalse(r["ok"])

    def test_set_shape_closed_toggles_the_flag(self):
        base = self.sb.new()
        ops = [{"op": "add_polygon", "id": "p", "folio": 0,
               "points": [{"x": 0, "y": 0}, {"x": 1, "y": 0}, {"x": 1, "y": 1}], "closed": True},
               {"op": "set_shape_closed", "folio": 0, "index": "$p", "closed": False}]
        r = self.ok(self.sb.edit(base, ops))
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertIn('closed="0"', xml)

    def test_points_and_nodes_argument_validation(self):
        base = self.sb.new()
        for bad in ([], [{"x": 0, "y": 0}], [{"x": 0}], "not-a-list", [{"x": "a", "y": 0}]):
            with self.subTest(points=bad):
                with self.assertRaises(ValueError):
                    self.sb.edit(base, [{"op": "add_polygon", "folio": 0, "points": bad, "closed": True}])
        with self.assertRaises(ValueError):
            self.sb.edit(base, [{"op": "add_path", "folio": 0,
                                 "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 0, "kind": "bogus"}],
                                 "closed": False}])

    # ---- PDF page import ----

    def test_add_pdf_page_renders_the_named_page(self):
        doc = self.sb.dir / "two_pages.pdf"
        pdf(doc)
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(doc), "page": 1, "dpi": 72,
             "x": 0, "y": 0},
            {"op": "add_pdf_page", "folio": 0, "file": str(doc), "page": 2, "dpi": 72,
             "x": 300, "y": 0}]))
        self.assertEqual(r["operations"][0]["result"], 0)
        self.assertEqual(r["operations"][1]["result"], 1)
        xml = Path(r["output"]).read_text(encoding="utf-8")
        images = re.findall(r"<image [^>]*>([^<]+)</image>", xml)
        self.assertEqual(len(images), 2)
        self.assertNotEqual(images[0], images[1],
                            "page 1 and page 2 must render different content")

    def test_add_pdf_page_dpi_controls_pixel_size(self):
        doc = self.sb.dir / "one_page.pdf"
        pdf(doc, page_colors=("1 0 0",))
        base = self.sb.new()
        r72 = self.ok(self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(doc), "page": 1, "dpi": 72,
             "x": 0, "y": 0}], out="dpi72.qet"))
        r144 = self.ok(self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(doc), "page": 1, "dpi": 144,
             "x": 0, "y": 0}], out="dpi144.qet"))
        def png_width(qet_path):
            import base64
            import struct
            xml = Path(qet_path).read_text(encoding="utf-8")
            b64 = re.search(r"<image [^>]*>([^<]+)</image>", xml).group(1)
            png_bytes = base64.b64decode(b64)
            return struct.unpack(">I", png_bytes[16:20])[0]  # IHDR width

        w72 = png_width(r72["output"])
        w144 = png_width(r144["output"])
        self.assertAlmostEqual(w144 / w72, 2.0, places=1)

    def test_add_pdf_page_bad_page_number_is_refused(self):
        doc = self.sb.dir / "two_pages.pdf"
        pdf(doc)
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(doc), "page": 99, "dpi": 150,
             "x": 0, "y": 0}])
        self.assertFalse(r["ok"])

    def test_add_pdf_page_bad_file_is_refused(self):
        base = self.sb.new()
        r = self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(self.sb.dir / "missing.pdf"),
             "page": 1, "dpi": 150, "x": 0, "y": 0}])
        self.assertFalse(r["ok"])

        not_a_pdf = self.sb.dir / "notpdf.pdf"
        not_a_pdf.write_bytes(b"not a pdf")
        r = self.sb.edit(base, [
            {"op": "add_pdf_page", "folio": 0, "file": str(not_a_pdf), "page": 1,
             "dpi": 150, "x": 0, "y": 0}])
        self.assertFalse(r["ok"])

    def test_image_is_embedded_not_referenced(self):
        img = self.sb.dir / "pic.png"
        png(img)
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_image", "folio": 0, "file": str(img), "x": 0, "y": 0}]))
        img.unlink()                                  # the source is gone
        again = m.tool_query(BINARY, r["output"], "SELECT COUNT(*) AS n FROM diagram")
        self.assertTrue(again["ok"], "the project must open without the source file")
        self.assertIn("<image", Path(r["output"]).read_text(encoding="utf-8"))

    def test_bad_and_oversize_images_are_refused(self):
        bad = self.sb.dir / "notimg.png"
        bad.write_bytes(b"not a png")
        big = self.sb.dir / "big.png"
        big.write_bytes(b"\x89PNG" + b"0" * (11 * 1024 * 1024))
        base = self.sb.new()
        for f in (bad, big, self.sb.dir / "missing.png"):
            with self.subTest(file=f.name):
                r = self.sb.edit(base, [{"op": "add_image", "folio": 0, "file": str(f), "x": 0, "y": 0}])
                self.assertFalse(r["ok"])

    # ---- query, check ----

    def test_query_refuses_writes_and_tells_empty_from_error(self):
        base = self.sb.new()
        chained = m.tool_query(BINARY, base, "SELECT 1; DROP TABLE element")
        self.assertFalse(chained["ok"])
        empty = m.tool_query(BINARY, base, "SELECT * FROM element WHERE uuid = 'none'")
        self.assertTrue(empty["ok"])
        self.assertEqual(empty["row_count"], 0)
        typo = m.tool_query(BINARY, base, "SELECT nosuchcolumn FROM element")
        self.assertFalse(typo["ok"])
        self.assertIn("no such column", typo["error"])

    def test_query_lists_tables_and_views(self):
        r = m.tool_query(BINARY, self.sb.new(), "")
        names = " ".join(str(v) for row in r["rows"] for v in ([row] if isinstance(row, str) else row.values()))
        for want in ("element_nomenclature_view", "wiring_list_view", "project_summary_view", "conductor"):
            self.assertIn(want, names)

    # ---- continuity / ERC ----

    def test_continuity_reports_unconnected_terminals(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 300, "y": 100},
               {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 0,
                "to": "$b", "to_terminal": 0}]
        r = self.ok(self.sb.edit(base, ops))
        result = m.tool_continuity(BINARY, r["output"], elements_dir=ELEMENTS)
        self.assertTrue(result["ok"], result)
        unconnected = [f for f in result["findings"] if f["kind"] == "unconnected_terminal"]
        # terminal 0 of both a and b is wired; terminal 1 of both is not
        self.assertEqual(len(unconnected), 2)
        self.assertTrue(all(f["severity"] == "info" for f in unconnected))
        self.assertEqual(result["info"], 2)
        self.assertEqual(result["errors"], 0)

    def test_continuity_no_findings_on_a_fully_wired_pair(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "a", "folio": "$f", "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "b", "folio": "$f", "path": COIL, "x": 300, "y": 100},
               {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 0,
                "to": "$b", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$a", "from_terminal": 1,
                "to": "$b", "to_terminal": 1}]
        r = self.ok(self.sb.edit(base, ops))
        result = m.tool_continuity(BINARY, r["output"], elements_dir=ELEMENTS)
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["finding_count"], 0)

    def test_continuity_finds_no_false_positive_after_a_clean_potential_write(self):
        """setConductorProperty() propagates to the whole potential -- a
        potential built entirely through the scripting API must never
        trip potential_mismatch."""
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL,
                 "x": 100 + i * 200, "y": 100} for i in range(3)],
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e1", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e2", "to_terminal": 0},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0,
                "property": "cable", "value": "CAB-1"}]
        r = self.ok(self.sb.edit(base, ops))
        result = m.tool_continuity(BINARY, r["output"], elements_dir=ELEMENTS)
        mismatches = [f for f in result["findings"] if f["kind"] == "potential_mismatch"]
        self.assertEqual(mismatches, [])

    def test_continuity_detects_a_tampered_potential(self):
        """A potential's members disagreeing on a property is not
        something the scripting API or the GUI can produce -- simulate a
        hand-edited file by patching the saved XML directly."""
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL,
                 "x": 100 + i * 200, "y": 100} for i in range(3)],
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e1", "to_terminal": 0},
               {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0,
                "to": "$e2", "to_terminal": 0},
               {"op": "set_conductor", "folio": "$f", "element": "$e1", "terminal": 0,
                "property": "cable", "value": "CAB-1"}]
        r = self.ok(self.sb.edit(base, ops))
        xml = Path(r["output"]).read_text(encoding="utf-8")
        first = xml.find('cable="CAB-1"')
        second = xml.find('cable="CAB-1"', first + 1)
        self.assertNotEqual(second, -1, "both potential members should carry the value")
        tampered = xml[:second] + 'cable="CAB-DIFFERENT"' + xml[second + len('cable="CAB-1"'):]
        tampered_path = self.sb.p("tampered.qet")
        Path(tampered_path).write_text(tampered, encoding="utf-8")

        result = m.tool_continuity(BINARY, tampered_path, elements_dir=ELEMENTS)
        mismatches = [f for f in result["findings"] if f["kind"] == "potential_mismatch"]
        self.assertEqual(len(mismatches), 1)
        self.assertEqual(mismatches[0]["property"], "cable")
        self.assertEqual(sorted(mismatches[0]["values"]), ["CAB-1", "CAB-DIFFERENT"])
        self.assertEqual(mismatches[0]["severity"], "error")
        self.assertEqual(result["errors"], 1)

    def test_continuity_can_be_scoped_to_one_folio(self):
        base = self.sb.new(folios=2)
        r = self.ok(self.sb.edit(base, [
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100}]))
        both = m.tool_continuity(BINARY, r["output"], elements_dir=ELEMENTS)
        only_1 = m.tool_continuity(BINARY, r["output"], folio=1, elements_dir=ELEMENTS)
        self.assertGreater(both["finding_count"], 0)
        self.assertEqual(only_1["finding_count"], 0, "folio 1 has no elements at all")

    def test_auto_numbered_conductor_text_reaches_the_database(self):
        """ConductorCreator inserted the database row before refreshText()
        resolved the formula, so the wiring list showed 'W%sequ_1' instead
        of 'W1' while the drawing and the saved file were right."""
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_autonum", "kind": "conductor", "name": "W", "parts": ["string:W", "unit:1:1"]},
            {"op": "use_conductor_autonum", "folio": 0, "name": "W"},
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "b", "folio": 0, "path": COIL, "x": 300, "y": 100},
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0, "to": "$b", "to_terminal": 0}]))
        # the query must run in the same process as the edit to see live state
        script = ("var f=qet.addFolio();qet.addAutoNum('conductor','W',['string:W','unit:1:1']);"
                  "qet.useConductorAutoNum(f,'W');"
                  f"var a=qet.addElement(f,'{COIL}',100,100),b=qet.addElement(f,'{COIL}',300,100);"
                  "qet.addConductor(f,a,0,b,0);"
                  "qet.log('DB '+JSON.stringify(qet.query(\"SELECT wire_number FROM wiring_list_view\")));")
        out = m._run_qet(BINARY, [self.sb.new("live")], elements_dir=ELEMENTS, script=script)
        line = [l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("DB ")][0]
        self.assertEqual(json.loads(line[3:]), [{"wire_number": "W1"}])

    def test_check_fires_on_a_project_with_known_defects(self):
        """Newly placed elements store NULL where a loaded file stores '', and
        `col = ''` matches only the second: the first version of these checks
        passed on exactly the projects qet_edit produces."""
        base = self.sb.new(folios=2)
        r = self.ok(self.sb.edit(base, [
            *[{"op": "add_element", "id": x, "folio": 0, "path": COIL, "x": 100 + i * 200, "y": 100}
              for i, x in enumerate("abc")],
            {"op": "set_label", "folio": 0, "element": "$a", "label": "KM1"},
            {"op": "set_label", "folio": 0, "element": "$b", "label": "KM1"},
            {"op": "set_info", "folio": 0, "element": "$a", "key": "manufacturer_reference", "value": "REF"},
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0, "to": "$b", "to_terminal": 0}]))
        c = m.tool_check(BINARY, r["output"])
        found = {f["check"]: f for f in c["findings"]}
        self.assertIn("duplicate_master_labels", found)
        self.assertEqual(found["duplicate_master_labels"]["severity"], "error")
        self.assertIn("unlabelled_masters", found)
        self.assertIn("empty_folios", found)                       # folio 2
        self.assertIn("unnumbered_conductors", found)
        self.assertEqual(found["masters_without_manufacturer_reference"]["count"], 2)
        self.assertFalse(c["ok"])

    def test_check_passes_a_clean_project(self):
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_autonum", "kind": "conductor", "name": "W", "parts": ["string:W", "unit:1:1"]},
            {"op": "use_conductor_autonum", "folio": 0, "name": "W"},
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "b", "folio": 0, "path": COIL, "x": 300, "y": 100},
            {"op": "set_label", "folio": 0, "element": "$a", "label": "KM1"},
            {"op": "set_label", "folio": 0, "element": "$b", "label": "KM2"},
            {"op": "set_info", "folio": 0, "element": "$a", "key": "manufacturer_reference", "value": "A"},
            {"op": "set_info", "folio": 0, "element": "$b", "key": "manufacturer_reference", "value": "B"},
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0, "to": "$b", "to_terminal": 0}]))
        c = m.tool_check(BINARY, r["output"])
        self.assertTrue(c["ok"], c["findings"])
        self.assertEqual(c["findings"], [])

    # ---- element authoring ----

    def test_authored_element_is_searchable_placeable_and_index_ordered(self):
        coll = self.sb.dir / "coll"
        m.tool_element_build(str(coll / "99_custom" / "res.elmt"),
                             names={"en": "Test resistor", "fr": "Résistance de test"},
                             parts=[{"type": "rect", "x": -10, "y": -20, "width": 20, "height": 40}],
                             terminals=[{"x": 0, "y": 30, "orientation": "s", "name": "low"},
                                        {"x": 0, "y": -30, "orientation": "n", "name": "high"}],
                             informations={"type": "resistor"})
        hit = m.tool_element_search(str(coll), "resistance")["results"][0]
        self.assertEqual(hit["path"], "common://99_custom/res.elmt")
        self.assertEqual(hit["terminal_names"], ["high", "low"])
        base = self.sb.new()
        r = m.tool_edit(BINARY, base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "r1", "folio": "$f", "path": hit["path"], "x": 100, "y": 100},
            {"op": "add_element", "id": "r2", "folio": "$f", "path": hit["path"], "x": 100, "y": 300},
            {"op": "add_conductor", "folio": "$f", "from": "$r1", "from_terminal": 1, "to": "$r2", "to_terminal": 0}],
            self.sb.p("r.qet"), elements_dir=str(coll))
        self.assertTrue(r["ok"], r.get("hint"))

    def test_terminal_index_matches_what_the_scripting_api_reports(self):
        """The index order is not the file order. Place elements whose terminals
        are named and listed differently in the file, and compare the
        predicted order with the one the scripting API reports. Restricted to
        elements with distinct, non-blank names, since blank names compare
        equal in any order and would make the comparison vacuous."""
        idx = m._index_collection(Path(ELEMENTS))
        pool = [i for i in idx if i["terminals"] >= 2 and i["link_type"] in ("simple", "master", "slave", "terminal")
                and all(i["terminal_names"]) and len(set(i["terminal_names"])) == i["terminals"]
                and not i["terminal_order_ambiguous"]]

        def file_order(i):
            return [t.get("name") or "" for t in ET.parse(i["file"]).getroot().iter("terminal")]
        differ = [i for i in pool if file_order(i) != i["terminal_names"]]
        self.assertGreater(len(differ), 50, "the property under test must actually vary")
        pick = differ[:40] + [i for i in pool if file_order(i) == i["terminal_names"]][:10]
        js = "var f=qet.addFolio();var out=[];\n" + "".join(
            f"var u=qet.addElement(f,{json.dumps(i['path'])},{100 + (n % 10) * 80},{100 + (n // 10) * 80});"
            f"out.push({{p:{json.dumps(i['path'])},t:u?qet.elementTerminals(f,u):[]}});\n"
            for n, i in enumerate(pick)) + "qet.log('ROWS '+JSON.stringify(out));\n"
        out = m._run_qet(BINARY, [self.sb.new("order")], elements_dir=ELEMENTS, script=js, tail=2_000_000)
        rows = json.loads([l for l in (out["stdout"] + out["stderr"]).splitlines() if l.startswith("ROWS ")][0][5:])
        by = {r["p"]: r["t"] for r in rows}
        wrong = []
        for i in pick:
            actual = [re.match(r"\d+: (.*) \(\d+ conductor", t).group(1) for t in by[i["path"]]]
            if actual != i["terminal_names"]:
                wrong.append((i["path"], i["terminal_names"], actual))
        self.assertEqual(wrong, [], f"{len(wrong)} of {len(pick)} disagree")

    # ---- capability probe ----

    def test_every_method_the_server_needs_exists_in_the_binary(self):
        r = m.tool_edit(BINARY, self.sb.new(), [{"op": "add_folio"}], self.sb.p("p.qet"),
                        elements_dir=ELEMENTS)
        self.assertEqual(r["missing_methods"], [])

    def test_a_binary_without_scripting_is_reported_not_run(self):
        """Point the tool at a script-less 'binary' (any executable that is
        not QElectroTech) and it must refuse rather than hang."""
        fake = self.sb.dir / "fake"
        fake.write_text("#!/bin/sh\nexit 0\n")
        fake.chmod(0o755)
        r = m.tool_edit(str(fake), self.sb.new(), [{"op": "add_folio"}], self.sb.p("x.qet"), timeout=15)
        self.assertFalse(r["ok"])


@needs_binary
class PlcIntegration(unittest.TestCase):
    """PLC IO table and PLC-slave linking, against the fixtures in
    fixtures/ rather than the real QET_ELEMENTS collection (see
    PLC_FIXTURES)."""

    def setUp(self):
        self.sb = Sandbox()

    def tearDown(self):
        self.sb.close()

    def ok(self, r):
        self.assertTrue(r["ok"], json.dumps({k: r.get(k) for k in ("hint", "operations")}, default=str)[:600])
        return r

    def edit(self, project, ops, out="out.qet"):
        return m.tool_edit(BINARY, project, ops, self.sb.p(out), elements_dir=PLC_FIXTURES)

    def test_add_set_and_remove_plc_io(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "m", "folio": "$f", "path": PLC_MASTER, "x": 100, "y": 100},
               {"op": "add_plc_io", "id": "io0", "folio": "$f", "element": "$m",
                "type": "entree_digitale", "address": "1.0", "function": "Start", "comment": "c"},
               {"op": "add_plc_io", "id": "io1", "folio": "$f", "element": "$m",
                "type": "sortie_digitale", "address": "1.1", "function": "Motor K1", "comment": ""},
               {"op": "set_plc_io", "folio": "$f", "element": "$m", "index": "$io0",
                "property": "address", "value": "1.2"},
               {"op": "remove_plc_io", "folio": "$f", "element": "$m", "index": "$io1"}]
        r = self.ok(self.edit(base, ops))
        self.assertEqual([o["result"] for o in r["operations"][2:4]], [0, 1])
        self.assertTrue(all(o["succeeded"] for o in r["operations"][4:]))

    def test_add_plc_io_on_non_master_is_refused(self):
        base = self.sb.new()
        r = self.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "add_element", "id": "s", "folio": "$f", "path": PLC_SLAVE, "x": 100, "y": 100},
            {"op": "add_plc_io", "folio": "$f", "element": "$s",
             "type": "entree_digitale", "address": "1.0", "function": "f", "comment": "c"}])
        self.assertFalse(r["ok"])

    def test_link_plc_io_targets_a_specific_row(self):
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "m", "folio": "$f", "path": PLC_MASTER, "x": 100, "y": 100},
               {"op": "add_element", "id": "s", "folio": "$f", "path": PLC_SLAVE, "x": 300, "y": 100},
               {"op": "add_plc_io", "folio": "$f", "element": "$m",
                "type": "entree_digitale", "address": "1.0", "function": "Start", "comment": ""},
               {"op": "add_plc_io", "folio": "$f", "element": "$m",
                "type": "sortie_digitale", "address": "1.1", "function": "Motor K1", "comment": ""},
               {"op": "link_plc_io", "folio": "$f", "element": "$m",
                "to_folio": "$f", "to": "$s", "io_index": 1}]
        r = self.ok(self.edit(base, ops))
        self.assertTrue(r["operations"][-1]["succeeded"])
        xml = Path(r["output"]).read_text(encoding="utf-8")
        self.assertIn("<link_uuid", xml, "the link was actually written")

    def test_link_plc_io_slave_first_order_also_works(self):
        """linkElements(slave, master, groupIndex) must set the same group
        index as linkElements(master, slave, groupIndex) -- the C++ side
        must build the undo command from whichever of the two is the
        Slave, not always the first argument."""
        base = self.sb.new()
        ops = [{"op": "add_folio", "id": "f"},
               {"op": "add_element", "id": "m", "folio": "$f", "path": PLC_MASTER, "x": 100, "y": 100},
               {"op": "add_element", "id": "s", "folio": "$f", "path": PLC_SLAVE, "x": 300, "y": 100},
               {"op": "add_plc_io", "folio": "$f", "element": "$m",
                "type": "entree_digitale", "address": "1.0", "function": "Start", "comment": ""},
               {"op": "link_plc_io", "folio": "$f", "element": "$s",
                "to_folio": "$f", "to": "$m", "io_index": 0}]
        r = self.ok(self.edit(base, ops))
        self.assertTrue(r["operations"][-1]["succeeded"])


@needs_examples
class CorpusIntegration(unittest.TestCase):
    def test_conductor_keys_never_collide_across_the_shipped_examples(self):
        """Keying on terminal geometry alone merged nine distinct conductors
        of schema_indus.qet; the shipped corpus is the check."""
        total = collisions = 0
        for f in sorted(Path(EXAMPLES).glob("*.qet")):
            try:
                root = m._root(str(f))
            except Exception:
                continue
            keys = [m._conductor_key(i, c, ix) for i, c, ix in m._conductors(root)]
            total += len(keys)
            collisions += len(keys) - len(set(keys))
        self.assertGreater(total, 3000)
        self.assertEqual(collisions, 0)

    def test_untouched_resave_diffs_clean_on_every_small_example(self):
        sb = Sandbox()
        try:
            for name in ("ArduinoLCD.qet", "Projet_vierge.qet", "grafcet.qet"):
                with self.subTest(project=name):
                    src = shutil.copy(Path(EXAMPLES) / name, sb.p(name))
                    r = m.tool_edit(BINARY, src, [{"op": "add_folio"}], sb.p("o_" + name),
                                    elements_dir=ELEMENTS or None)
                    self.assertTrue(r["ok"], r.get("hint"))
                    c = r["diff"]["conductors"]
                    if "unstable_keys" not in c:
                        self.assertEqual((len(c["added"]), len(c["removed"])), (0, 0))
                    self.assertEqual(r["diff"]["elements"]["moved_count"], 0)
        finally:
            sb.close()


if __name__ == "__main__":
    print(f"binary   : {BINARY or '(none: integration tests skipped)'}")
    print(f"elements : {ELEMENTS or '(none)'}")
    print(f"examples : {EXAMPLES or '(none)'}")
    unittest.main(verbosity=2)
