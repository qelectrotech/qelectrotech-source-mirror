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
from unittest import mock
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
SHARED_UUID = "{11111111-2222-3333-4444-555555555555}"
# The shipped "going/coming arrow" pair -- a folio-jump link, next_report on
# one folio linked to previous_report on the next, one terminal each. Same
# category of element as issue #974's custom "naechste_folie_rechts.elmt" /
# "vorherige_folie_links.elmt".
NEXT_REPORT = "common://10_electric/10_allpole/100_folio_referencing/02going_arrow.elmt"
PREVIOUS_REPORT = "common://10_electric/10_allpole/100_folio_referencing/01coming_arrow.elmt"
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


def fake_qet(bindir: Path, name: str = "qelectrotech") -> Path:
    """An executable file standing in for QElectroTech. The policy checks
    which file it is, never runs it."""
    bindir.mkdir(parents=True, exist_ok=True)
    exe = bindir / name
    exe.write_text("#!/bin/sh\nexit 0\n")
    exe.chmod(0o755)
    return exe


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
            "qet_continuity", "qet_items", "qet_script_api", "qet_script_test",
        "qet_script_install", "qet_script_list", "qet_script_read",
        "qet_script_remove", "qet_live_status", "qet_live_run_script",
        "qet_live_run_stored", "qet_live_command", "qet_live_show_folio",
        "qet_live_undo_last", "qet_live_screenshot"})


class EditValidation(unittest.TestCase):
    """Every argument error must be raised before QElectroTech is launched.

    tool_edit is called with a binary that does not exist: if validation
    were lazy the failure would be 'not an executable', not the ValueError
    asserted here.
    """

    def build(self, ops):
        return m._build_script(ops, "/tmp/out.qet")

    def test_tables_and_text_fields_by_uuid(self):
        """A table or a symbol text field named by uuid is looked up at run
        time; a field's lookup is scoped to the op's element."""
        U = "{11111111-2222-4333-8444-555555555555}"
        E = "{aaaaaaaa-0000-4000-8000-000000000001}"
        s = self.build([{"op": "set_table_position", "folio": 2, "table": U, "x": 1, "y": 2}])
        self.assertIn(f'qet.setTablePosition(2, qet.tableIndex(2, "{U}"), 1, 2)', s)
        s = self.build([{"op": "delete_table", "folio": 0, "table": 3}])
        self.assertIn("qet.deleteTable(0, 3)", s)
        s = self.build([{"op": "set_element_text", "folio": 1, "element": E, "index": U,
                         "property": "x", "value": "5"}])
        self.assertIn(f'qet.setElementTextProperty(1, "{E}", '
                      f'qet.elementTextIndex(1, "{E}", "{U}"), "x", "5")', s)
        # the element may be one placed earlier in the same run
        s = self.build([{"op": "add_folio", "id": "f"},
                        {"op": "add_element", "id": "k", "folio": "$f", "path": "p", "x": 0, "y": 0},
                        {"op": "delete_element_text", "folio": "$f", "element": "$k", "index": U}])
        f = '(F["f"] ? qet.folioIndex(F["f"]) : R["f"])'   # a folio "$name": see test_folio_ref_follows_the_folio
        self.assertIn(f'qet.deleteElementText({f}, R["k"], qet.elementTextIndex({f}, R["k"], "{U}"))', s)
        # the lookups are required only when a uuid is used
        self.assertIn('"tableIndex"', self.build([{"op": "delete_table", "folio": 0, "table": U}]))
        self.assertNotIn('"tableIndex"', self.build([{"op": "delete_table", "folio": 0, "table": 0}]))
        for op in ({"op": "delete_table", "folio": 0, "table": "second"},
                   {"op": "delete_element_text", "folio": 0, "element": E, "index": "label"}):
            with self.subTest(op=op["op"]):
                with self.assertRaisesRegex(ValueError, "index or its uuid"):
                    self.build([op])

    def test_folio_by_uuid(self):
        """A folio named by uuid is looked up at run time, wherever an op
        takes a folio; the lookup is required only then."""
        U = "{11111111-2222-4333-8444-555555555555}"
        E = "{aaaaaaaa-0000-4000-8000-000000000001}"
        s = self.build([{"op": "set_folio", "folio": U, "property": "author", "value": "a"}])
        self.assertIn(f'qet.setFolioProperty(qet.folioIndex("{U}"), "author", "a")', s)
        self.assertIn('"folioIndex"', s)
        # the item lookups take the resolved folio too
        s = self.build([{"op": "delete_table", "folio": U, "table": E}])
        self.assertIn(f'qet.deleteTable(qet.folioIndex("{U}"), qet.tableIndex(qet.folioIndex("{U}"), "{E}"))', s)
        s = self.build([{"op": "link_elements", "folio": 0, "element": E, "to_folio": U, "to": E}])
        self.assertIn(f'qet.linkElements(0, "{E}", qet.folioIndex("{U}"), "{E}")', s)
        self.assertNotIn('"folioIndex"', self.build(
            [{"op": "set_folio", "folio": 0, "property": "author", "value": "a"}]))
        with self.assertRaisesRegex(ValueError, "saved once"):
            self.build([{"op": "set_folio", "folio": "", "property": "author", "value": "a"}])
        with self.assertRaisesRegex(ValueError, "folio index, its uuid"):
            self.build([{"op": "set_folio", "folio": "first", "property": "author", "value": "a"}])
            
    def test_folio_ref_follows_the_folio(self):
        """A "$name" made by add_folio or insert_folio is looked up by the
        folio's uuid when used as a folio, since a later insert or removal
        shifts its index; the stored index is the fallback on a build that
        cannot report folio uuids. Used as anything else, it is unchanged."""
        s = self.build([{"op": "add_folio", "id": "f"},
                        {"op": "insert_folio", "id": "g", "position": 0},
                        {"op": "set_folio_title", "folio": "$f", "title": "t"},
                        {"op": "set_folio_title", "folio": "$g", "title": "u"}])
        self.assertIn("F[\"f\"] = (typeof qet.folioUuid === 'function' && v0 >= 0) "
                      "? qet.folioUuid(v0) : '';", s)
        self.assertIn('qet.setFolioTitle((F["f"] ? qet.folioIndex(F["f"]) : R["f"]), "t")', s)
        self.assertIn('qet.setFolioTitle((F["g"] ? qet.folioIndex(F["g"]) : R["g"]), "u")', s)
        # not required: an edit still runs on a build without folio uuids
        self.assertNotIn('"folioUuid"', s)
        # a "$name" from any other op is untouched
        s = self.build([{"op": "add_text", "id": "t", "folio": 0, "text": "x", "x": 0, "y": 0},
                        {"op": "delete_text", "folio": 0, "index": "$t"}])
        self.assertIn('R["t"]', s)
        self.assertNotIn("F[", s.split("if (missing.length === 0) {")[1])

    def test_conductor_by_uuid(self):
        """A conductor named by uuid is turned into one of its ends at run
        time; the lookup is required only then, and "conductor" cannot be
        given alongside element + terminal."""
        U = "{11111111-2222-4333-8444-555555555555}"
        s = self.build([{"op": "delete_conductor", "folio": 1, "conductor": U}])
        self.assertIn(f'var e0 = qetMcpConductorEnd(0, 1, "{U}");', s)
        self.assertIn("(e0 ? qet.deleteConductor(1, e0.element, e0.terminal) : false)", s)
        self.assertIn('"conductorEnds"', s)
        s = self.build([{"op": "set_conductor", "folio": 0, "conductor": U,
                         "property": "num", "value": "W1"}])
        self.assertIn('qet.setConductorProperty(0, e0.element, e0.terminal, "num", "W1")', s)
        s = self.build([{"op": "move_conductor_segment", "folio": 2, "conductor": U,
                         "segment": 1, "dx": 10, "dy": 0}])
        self.assertIn(f'var e0 = qetMcpConductorEnd(0, 2, "{U}");', s)
        self.assertIn("(e0 ? qet.moveConductorSegment(2, e0.element, e0.terminal, 1, 10, 0) : false)", s)
        # an end whose terminal or element is missing is never picked
        self.assertIn("if (ends[k] === '?') continue;", s)
        self.assertNotIn('"conductorEnds"', self.build(
            [{"op": "delete_conductor", "folio": 0, "element": U, "terminal": 0}]))
        with self.assertRaisesRegex(ValueError, "not both"):
            self.build([{"op": "delete_conductor", "folio": 0, "conductor": U,
                         "element": U, "terminal": 0}])
        with self.assertRaisesRegex(ValueError, "must be a conductor uuid"):
            self.build([{"op": "delete_conductor", "folio": 0, "conductor": "W1"}])
        with self.assertRaisesRegex(ValueError, "saved before conductors carried a uuid"):
            self.build([{"op": "delete_conductor", "folio": 0, "conductor": ""}])

    def test_terminal_by_uuid(self):
        """A terminal named by uuid is turned into its index at run time, on
        the element it belongs to: the op's element, or each add_conductor
        end's own. The lookup is required only then; a plain index is
        passed through as before."""
        E, F = "{11111111-2222-4333-8444-555555555555}", "{21111111-2222-4333-8444-555555555555}"
        T, V = "{31111111-2222-4333-8444-555555555555}", "{41111111-2222-4333-8444-555555555555}"
        s = self.build([{"op": "add_conductor", "folio": 1, "from": E, "from_terminal": T,
                         "to": F, "to_terminal": V}])
        self.assertIn(f'qet.addConductor(1, "{E}", qetMcpTerminal(0, "from_terminal", 1, "{E}", "{T}"), '
                      f'"{F}", qetMcpTerminal(0, "to_terminal", 1, "{F}", "{V}"))', s)
        self.assertIn('"terminalIndex"', s)
        s = self.build([{"op": "set_conductor", "folio": 0, "element": E, "terminal": T,
                         "property": "num", "value": "W1"}])
        self.assertIn(f'qet.setConductorProperty(0, "{E}", qetMcpTerminal(0, "terminal", 0, "{E}", "{T}"), '
                      '"num", "W1")', s)
        s = self.build([{"op": "delete_conductor", "folio": 0, "element": E, "terminal": T}])
        self.assertIn(f'qet.deleteConductor(0, "{E}", qetMcpTerminal(0, "terminal", 0, "{E}", "{T}"))', s)
        s = self.build([{"op": "move_conductor_segment", "folio": 0, "element": E,
                         "terminal": T, "segment": 1, "dx": 5, "dy": 0}])
        self.assertIn(f'qet.moveConductorSegment(0, "{E}", qetMcpTerminal(0, "terminal", 0, "{E}", "{T}"), 1, 5, 0)', s)
        # a $name element and a folio uuid reach the lookup resolved
        s = self.build([{"op": "add_folio", "id": "f"},
                        {"op": "add_element", "id": "a", "folio": "$f", "path": "x.elmt", "x": 0, "y": 0},
                        {"op": "delete_conductor", "folio": "$f", "element": "$a", "terminal": T}])
        f = '(F["f"] ? qet.folioIndex(F["f"]) : R["f"])'   # a folio "$name": see test_folio_ref_follows_the_folio
        self.assertIn(f'qet.deleteConductor({f}, R["a"], qetMcpTerminal(2, "terminal", {f}, R["a"], "{T}"))', s)
        s = self.build([{"op": "delete_conductor", "folio": V, "element": E, "terminal": T}])
        self.assertIn(f'qetMcpTerminal(0, "terminal", qet.folioIndex("{V}"), "{E}", "{T}")', s)
        # an index is unchanged and needs no lookup
        s = self.build([{"op": "delete_conductor", "folio": 0, "element": E, "terminal": 2}])
        self.assertIn(f'qet.deleteConductor(0, "{E}", 2)', s)
        self.assertNotIn('"terminalIndex"', s)
        with self.assertRaisesRegex(ValueError, "terminal index or its uuid"):
            self.build([{"op": "delete_conductor", "folio": 0, "element": E, "terminal": "A1"}])
        with self.assertRaisesRegex(ValueError, "terminal index or its uuid"):
            self.build([{"op": "delete_conductor", "folio": 0, "element": E, "terminal": True}])

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

    def test_a_uuid_index_is_resolved_at_run_time(self):
        u = "{11111111-1111-4111-8111-111111111111}"
        script = self.build([{"op": "set_shape", "folio": 2, "index": u,
                              "property": "fill", "value": "none"},
                             {"op": "delete_text", "folio": 1, "index": u},
                             {"op": "delete_image", "folio": 0, "index": u}])
        self.assertIn(f'qet.setShapeProperty(2, qet.shapeIndex(2, "{u}"), ', script)
        self.assertIn(f'qet.deleteText(1, qet.textIndex(1, "{u}"))', script)
        self.assertIn(f'qet.deleteImage(0, qet.imageIndex(0, "{u}"))', script)
        need = json.loads(re.search(r"var need = (\[.*?\]);", script).group(1))
        self.assertTrue({"shapeIndex", "textIndex", "imageIndex"} <= set(need))

    def test_an_index_only_edit_does_not_need_the_uuid_resolvers(self):
        script = self.build([{"op": "delete_shape", "folio": 0, "index": 3}])
        need = json.loads(re.search(r"var need = (\[.*?\]);", script).group(1))
        self.assertFalse({"shapeIndex", "textIndex", "imageIndex"} & set(need))

    def test_malformed_arguments_are_refused(self):
        """Each argument kind rejects what it cannot take, before any launch.
        Found by the mutation audit: forcing any of these checks off went
        unnoticed."""
        f = {"op": "add_folio", "id": "f"}
        cases = [
            ("indices", {"op": "group_terminals", "strip": 0, "indices": []}),
            ("indices", {"op": "group_terminals", "strip": 0, "indices": [1, "2"]}),
            ("indices", {"op": "group_terminals", "strip": 0, "indices": [True]}),
            ("indices", {"op": "bridge_terminals", "strip": 0, "indices": 3}),
            ("true or false", {"op": "add_polygon", "folio": 0, "closed": 1,
                               "points": [{"x": 0, "y": 0}, {"x": 1, "y": 1}]}),
            ("at least 2", {"op": "add_polygon", "folio": 0, "closed": True,
                            "points": [{"x": 0, "y": 0}]}),
            ("at least 2", {"op": "add_polygon", "folio": 0, "closed": True, "points": "xy"}),
            ("entries must be", {"op": "add_polygon", "folio": 0, "closed": True,
                                 "points": [{"x": 0, "y": 0}, {"x": "1", "y": 1}]}),
            ("entries must be", {"op": "add_polygon", "folio": 0, "closed": True,
                                 "points": [{"x": 0, "y": 0}, {"x": True, "y": 1}]}),
            ("entries must be", {"op": "add_polygon", "folio": 0, "closed": True,
                                 "points": [{"x": 0, "y": 0}, [1, 1]]}),
            ("at least 2 nodes", {"op": "add_path", "folio": 0, "closed": False,
                                  "nodes": [{"x": 0, "y": 0}]}),
            ("corner, smooth or symmetric", {"op": "add_path", "folio": 0, "closed": False,
                                             "nodes": [{"x": 0, "y": 0},
                                                       {"x": 1, "y": 1, "kind": "sharp"}]}),
            ("inHandle", {"op": "add_path", "folio": 0, "closed": False,
                          "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 1, "inHandle": [0, 0]}]}),
            ("outHandle", {"op": "add_path", "folio": 0, "closed": False,
                           "nodes": [{"x": 0, "y": 0}, {"x": 1, "y": 1, "outHandle": {"x": 0}}]}),
            ("unknown kind", {"op": "search_and_replace", "kind": "folio", "field": "x",
                              "pattern": "a", "replacement": "b", "regex": False,
                              "case_sensitive": False}),
            ("unknown conductor field", {"op": "search_and_replace", "kind": "conductor",
                                         "field": "label", "pattern": "a", "replacement": "b",
                                         "regex": False, "case_sensitive": False}),
            ("non-empty", {"op": "search_and_replace", "kind": "element_info", "field": "",
                           "pattern": "a", "replacement": "b", "regex": False,
                           "case_sensitive": False}),
            ("not both", {"op": "delete_conductor", "folio": 0,
                          "conductor": "{11111111-2222-4333-8444-555555555555}",
                          "element": "{11111111-2222-4333-8444-555555555555}"}),
            ("not both", {"op": "delete_conductor", "folio": 0,
                          "conductor": "{11111111-2222-4333-8444-555555555555}", "terminal": 0}),
            ("not an object", "add_folio"),
        ]
        for message, op in cases:
            with self.subTest(op=op):
                with self.assertRaisesRegex(ValueError, message):
                    self.build([f, op])

    def test_valid_shapes_of_those_arguments_pass(self):
        """The other side: the same kinds accept what they should, so the
        refusals above are not everything failing."""
        s = self.build([
            {"op": "add_polygon", "folio": 0, "closed": False,
             "points": [{"x": 0, "y": 0}, {"x": 1.5, "y": -2}]},
            {"op": "add_path", "folio": 0, "closed": True,
             "nodes": [{"x": 0, "y": 0, "kind": "smooth", "inHandle": {"x": 1, "y": 1}},
                       {"x": 5, "y": 5, "outHandle": {"x": 2, "y": 2}}]},
            {"op": "add_polygon", "folio": 0, "closed": True,
             "points": [{"x": 0, "y": 0, "kind": "anything"}, {"x": 1, "y": 1}]},
            {"op": "search_and_replace", "kind": "conductor", "field": "num", "pattern": "a",
             "replacement": "b", "regex": True, "case_sensitive": False},
            {"op": "search_and_replace", "kind": "text", "field": "", "pattern": "a",
             "replacement": "b", "regex": False, "case_sensitive": True}])
        self.assertIn("qet.addPolygon(0, [{", s)
        self.assertIn("qet.searchAndReplace(\"conductor\", \"num\", \"a\", \"b\", true, false)", s)
        self.assertIn("qet.searchAndReplace(\"text\", \"\", \"a\", \"b\", false, true)", s)

    def test_no_id_stores_nothing(self):
        s = self.build([{"op": "add_folio"}])
        self.assertNotIn("R[", s.split("if (missing.length === 0) {")[1])

    def test_a_string_index_that_is_not_a_uuid_is_refused(self):
        for bad in ("3", "{nope}", "11111111-1111"):
            with self.subTest(index=bad):
                with self.assertRaises(ValueError):
                    self.build([{"op": "delete_shape", "folio": 0, "index": bad}])

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
        literal = re.search(r'setFolioTitle\(\(F\["f"\] \? qet\.folioIndex\(F\["f"\]\) : R\["f"\]\), (".*?")\)',
                            script, re.S).group(1)
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


    def test_capabilities_notes_and_save(self):
        """Every field the script reports reaches the result, attached where
        it belongs. Found by the mutation audit: only the binary tests read
        these back."""
        text = "\n".join([
            self.line(kind="capabilities", missing=["addFolio"]),
            self.line(kind="op", index=0, op="a", id=None, result=True),
            self.line(kind="op", index=1, op="b", id=None, result=False),
            self.line(kind="op_note", index=1, note="why"),
            self.line(kind="save", result=True, stopped_early=True),
            self.line(kind="something_newer", result=False)])   # not a save
        parsed = m._parse_script_output(text)
        self.assertEqual(set(parsed), {"missing_methods", "operations", "saved", "stopped_early"})
        self.assertEqual(parsed["missing_methods"], ["addFolio"])
        self.assertTrue(parsed["saved"])
        self.assertTrue(parsed["stopped_early"])
        self.assertNotIn("note", parsed["operations"][0])
        self.assertEqual(parsed["operations"][1]["note"], "why")
            # no capabilities line: unknown, not "nothing missing"
        self.assertIsNone(m._parse_script_output(self.line(kind="save", result=False))["missing_methods"])
        self.assertEqual(m._parse_script_output(self.line(kind="capabilities", missing=None))
                         ["missing_methods"], [])

    def test_a_line_without_the_marker_is_ignored(self):
        """Even one that would parse if read from where the marker would be."""
        almost = "x" * (len(m._MARKER) - 1) + json.dumps({"kind": "save", "result": True})
        parsed = m._parse_script_output(almost)
        self.assertIsNone(parsed["saved"])
        self.assertFalse(parsed["stopped_early"])


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

    def test_every_part_gets_its_own_uuid(self):
        parts = [{"type": "line", "x1": 0, "y1": 0, "x2": 10, "y2": 0},
                 {"type": "rect", "x": 0, "y": 0, "width": 5, "height": 5},
                 {"type": "text", "x": 0, "y": 0, "text": "K"}]
        r = self.build(parts=parts)
        desc = ET.parse(self.out).getroot().find("description")
        written = [e.get("uuid") for e in desc if e.tag != "terminal"]
        self.assertEqual(written, r["part_uuids"])
        self.assertEqual(len(set(written)), 3)
        self.assertEqual([p["uuid"] for p in r["verified"]["part_list"]], written)

    def test_a_given_part_uuid_is_kept_and_braced(self):
        r = self.build(parts=[{"type": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 0,
                               "uuid": "11111111-1111-4111-8111-111111111111"}])
        self.assertEqual(r["part_uuids"], ["{11111111-1111-4111-8111-111111111111}"])

    def test_bad_or_repeated_part_uuids_are_refused(self):
        line = {"type": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 0}
        u = "{11111111-1111-4111-8111-111111111111}"
        for parts in ([{**line, "uuid": "nope"}], [{**line, "uuid": u}, {**line, "uuid": u}]):
            with self.subTest(parts=parts):
                with self.assertRaises(ValueError):
                    self.build(parts=parts)

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


class ElementFileExact(unittest.TestCase):
    """Exact answers for the tools that write and read .elmt files and need
    no QElectroTech. Found by a mutation audit: the existing tests checked a
    few fields, so dropping any other one from a result, or getting a
    header number off by one, went unnoticed."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        m._ELEMENT_INDEX.clear()

    def tearDown(self):
        self.tmp.cleanup()

    def test_fmt(self):
        self.assertEqual([m._fmt(v) for v in (True, False, 3, 2.0, 2.5, -0.25, "x")],
                         ["true", "false", "3", "2", "2.5", "-0.25", "x"])

    def test_part_extent_per_kind(self):
        ext = m._part_extent
        self.assertEqual(ext("line", {"x1": 1, "y1": 2, "x2": 3, "y2": 4}), [(1, 2), (3, 4)])
        for kind in ("rect", "ellipse", "arc"):
            self.assertEqual(ext(kind, {"x": -5, "y": 1, "width": 10, "height": 4}),
                             [(-5, 1), (5, 5)], kind)
        self.assertEqual(ext("circle", {"x": 2, "y": 3, "diameter": 6}), [(2, 3), (8, 9)])
        self.assertEqual(ext("polygon", {"points": [[0, 1], [2, 3], [4, 5]]}),
                         [(0, 1), (2, 3), (4, 5)])
        self.assertEqual(ext("text", {"x": 7, "y": 8, "text": "a"}), [(7, 8)])
        self.assertEqual(ext("dynamic_text", {"x": 7}), [])

    def test_element_geometry(self):
        """Box, hotspot and size, worked out by hand: points reach x -10..20
        and y -15..5; a 5 unit margin, rounded out to tens."""
        g = m._element_geometry(
            [{"type": "line", "x1": 0, "y1": 0, "x2": 20, "y2": 0},
             {"type": "rect", "x": -10, "y": -5, "width": 10, "height": 10}],
            [{"x": 0, "y": -15, "orientation": "n"}])
        self.assertEqual(g, {"width": 50, "height": 30, "hotspot_x": 20, "hotspot_y": 20,
                             "bbox": [-10, -15, 20, 5]})
        # a terminal alone is enough; nothing at all is refused
        self.assertEqual(m._element_geometry([], [{"x": 0, "y": 0, "orientation": "n"}]),
                         {"width": 20, "height": 20, "hotspot_x": 10, "hotspot_y": 10, "bbox": [0, 0, 0, 0]})
        with self.assertRaisesRegex(ValueError, "at least one part or terminal"):
            m._element_geometry([], [])

    def test_part_element(self):
        E = lambda part: dict(m._part_element(part, "{u}").attrib)
        self.assertEqual(E({"type": "polygon", "points": [[0, 1], [2.5, 3]]}),
                         {"uuid": "{u}", "x1": "0", "y1": "1", "x2": "2.5", "y2": "3",
                          "closed": "true", "antialias": "true", "style": m.DEFAULT_STYLE})
        self.assertEqual(E({"type": "polygon", "points": [[0, 1], [2, 3]], "closed": False,
                            "antialias": False, "style": "x"})["closed"], "false")
        self.assertEqual(E({"type": "text", "x": 1, "y": 2, "text": "K1"}),
                         {"uuid": "{u}", "x": "1", "y": "2", "text": "K1", "rotation": "0",
                          "font": "Sans Serif,9,-1,5,50,0,0,0,0,0", "color": "#000000"})
        self.assertEqual(E({"type": "text", "x": 1, "y": 2, "text": "K1", "size": 12,
                            "rotation": 90, "color": "red"})["font"],
                         "Sans Serif,12,-1,5,50,0,0,0,0,0")
        rect = E({"type": "rect", "x": 0, "y": 0, "width": 4, "height": 2, "antialias": False})
        self.assertEqual(rect, {"uuid": "{u}", "x": "0", "y": "0", "width": "4", "height": "2",
                                "antialias": "false", "style": m.DEFAULT_STYLE})

    def test_build_writes_and_reports_exactly(self):
        out = self.root / "k" / "coil.elmt"
        U = "{11111111-2222-4333-8444-555555555555}"
        r = m.tool_element_build(
            str(out), {"fr": "Bobine", "en": "Coil"},
            [{"type": "line", "x1": 0, "y1": -10, "x2": 0, "y2": 10}],
            terminals=[{"x": 0, "y": 20, "orientation": "s", "name": "A2"},
                       {"x": 0, "y": -20, "orientation": "n", "name": "A1", "type": "Inner"}],
            link_type="master", informations={"type": "coil"}, uuid=U)
        self.assertEqual(set(r), {"ok", "output", "bytes", "width", "height", "hotspot_x",
                                  "hotspot_y", "bbox", "terminal_index_order", "part_uuids",
                                  "verified"})
        self.assertTrue(r["ok"])
        self.assertEqual(r["output"], str(out))
        self.assertEqual(r["bytes"], out.stat().st_size)
        self.assertEqual(r["terminal_index_order"], ["A1", "A2"])
        root = ET.parse(out).getroot()
        self.assertEqual(dict(root.attrib), {
            "version": "0.100.0", "type": "element", "link_type": "master",
            "width": str(r["width"]), "height": str(r["height"]),
            "hotspot_x": str(r["hotspot_x"]), "hotspot_y": str(r["hotspot_y"])})
        self.assertEqual(root.find("uuid").get("uuid"), U)
        self.assertEqual([(n.get("lang"), n.text) for n in root.iter("name")],
                         [("en", "Coil"), ("fr", "Bobine")])
        self.assertEqual([(k.get("name"), k.text) for k in root.iter("kindInformation")],
                         [("type", "coil")])
        t2, t1 = root.findall("description/terminal")
        self.assertEqual({k: v for k, v in t1.attrib.items() if k != "uuid"},
                         {"x": "0", "y": "-20", "orientation": "n", "type": "Inner", "name": "A1"})
        self.assertEqual(t2.get("type"), "Generic")
        self.assertRegex(t1.get("uuid"), m._UUID_RE)
        self.assertNotEqual(t1.get("uuid"), t2.get("uuid"))

    def test_build_refusals(self):
        out = str(self.root / "x.elmt")
        line = [{"type": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 0}]
        term = [{"x": 0, "y": 0, "orientation": "n"}]
        for message, kwargs in [
                ("names must be a non-empty", dict(names={}, parts=line, terminals=term)),
                ("unknown link_type", dict(names={"en": "a"}, parts=line, terminals=term, link_type="x")),
                ("parts must be a list", dict(names={"en": "a"}, parts={}, terminals=term)),
                ("terminal 0 is not an object", dict(names={"en": "a"}, parts=line, terminals=[1])),
                ("terminal 0 is missing 'orientation'", dict(names={"en": "a"}, parts=line,
                                                             terminals=[{"x": 0, "y": 0}])),
                ("orientation is one of", dict(names={"en": "a"}, parts=line,
                                               terminals=[{"x": 0, "y": 0, "orientation": "up"}])),
                ("cannot be connected", dict(names={"en": "a"}, parts=line, terminals=[])),
                ("same uuid", dict(names={"en": "a"}, terminals=term, parts=[
                    dict(line[0], uuid="{11111111-2222-4333-8444-555555555555}"),
                    dict(line[0], uuid="{11111111-2222-4333-8444-555555555555}")]))]:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    m.tool_element_build(out, **kwargs)

    def test_element_info_exactly(self):
        p = self.root / "e.elmt"
        p.write_text(
            '<definition type="element" link_type="simple" width="20" height="40">'
            '<names><name lang="en">E</name></names>'
            '<elementInformations><elementInformation name="x"><info_name> label </info_name>'
            '</elementInformation><elementInformation name="y"><info_name></info_name>'
            '</elementInformation></elementInformations>'
            '<description><line uuid="{L}"/><terminal x="0" y="10" orientation="s" type="Generic" '
            'uuid="{B}" name="2"/><terminal x="0" y="-10" orientation="n" name="1"/>'
            '<arc/><terminal x="5" y="-10" orientation="e"/></description></definition>',
            encoding="utf-8")
        r = m.tool_element_info(str(p))
        self.assertEqual(set(r), {"file", "type", "link_type", "width", "height", "names",
                                  "terminal_count", "terminals", "terminal_order",
                                  "info_fields", "parts", "part_list"})
        self.assertEqual((r["file"], r["type"], r["link_type"], r["width"], r["height"]),
                         (str(p), "element", "simple", "20", "40"))
        self.assertEqual(r["terminal_count"], 3)
        self.assertEqual(r["terminals"][0], {"index": 0, "x": "0", "y": "-10", "orientation": "n",
                                             "name": "1", "type": "", "uuid": ""})
        self.assertEqual(r["terminals"][2], {"index": 2, "x": "0", "y": "10", "orientation": "s",
                                             "name": "2", "type": "Generic", "uuid": "{B}"})
        self.assertEqual(r["info_fields"], ["label"])
        self.assertEqual(r["parts"], {"line": 1, "terminal": 3, "arc": 1})
        self.assertEqual(r["part_list"], [{"type": "line", "uuid": "{L}"}, {"type": "arc", "uuid": ""}])
        self.assertNotIn("undefined", r["terminal_order"])
        p.write_text(p.read_text().replace('x="5" y="-10"', 'x="0" y="-10"'), encoding="utf-8")
        self.assertIn("undefined", m.tool_element_info(str(p))["terminal_order"])

    def test_index_entry_exactly(self):
        f = self.root / "a" / "k.elmt"
        f.parent.mkdir()
        f.write_text('<definition type="element" link_type="master" width="30" height="50">'
                     '<names><name lang="fr">Bobine</name></names><kindInformations>'
                     '<kindInformation name="type"> coil </kindInformation>'
                     '<kindInformation name="other">x</kindInformation></kindInformations>'
                     '<description><terminal x="0" y="5" orientation="s" name="A2"/>'
                     '<terminal x="0" y="5" orientation="s" name="A1"/></description></definition>',
                     encoding="utf-8")
        (self.root / "a" / "broken.elmt").write_text("<definition", encoding="utf-8")
        (self.root / "a" / "other.elmt").write_text("<project/>", encoding="utf-8")
        items = m._index_collection(self.root)
        self.assertEqual(len(items), 1)
        it = {k: v for k, v in items[0].items() if k != "haystack"}
        self.assertEqual(it, {"path": "common://a/k.elmt", "file": str(f), "name": "Bobine",
                              "names": {"fr": "Bobine"}, "link_type": "master", "kind": "coil",
                              "terminals": 2, "terminal_names": ["A2", "A1"],
                              "terminal_order_ambiguous": True, "width": "30", "height": "50"})
        # the index is cached until the collection changes
        self.assertIs(m._index_collection(self.root), items)
        self.assertEqual(m._collection_signature(self.root)[0], 3)

    def test_search_ranking_and_limit(self):
        for rel, name in (("a/1.elmt", "Coil latching"), ("a/2.elmt", "Coil"),
                          ("a/3.elmt", "Remanence coil")):
            f = self.root / rel
            f.parent.mkdir(exist_ok=True)
            f.write_text(f'<definition type="element" link_type="simple"><names>'
                         f'<name lang="en">{name}</name></names><description>'
                         f'<terminal x="0" y="0" orientation="n"/></description></definition>',
                         encoding="utf-8")
        r = m.tool_element_search(str(self.root), "coil")
        self.assertEqual([e["name"] for e in r["results"]],
                         ["Coil", "Coil latching", "Remanence coil"])
        r = m.tool_element_search(str(self.root), "coil", limit=1)
        self.assertEqual((r["total_matches"], r["returned"]), (3, 1))
        with self.assertRaisesRegex(ValueError, "limit must be >= 1"):
            m.tool_element_search(str(self.root), "coil", limit=0)
        self.assertEqual(m.tool_element_search(str(self.root), "coil", limit=1)["results"][0]["name"],
                         "Coil")
        self.assertEqual(set(r), {"query", "total_matches", "returned", "indexed", "results"})
        self.assertEqual((r["query"], r["indexed"]), ("coil", 3))
        self.assertEqual(r["results"][0]["languages"], ["en"])

    def test_search_puts_the_exact_name_then_names_starting_with_it(self):
        for n, name in enumerate(("Relay coil", "Coil relay", "Coil relay X", "A coil relay")):
            f = self.root / f"{n}.elmt"
            f.write_text(f'<definition type="element" link_type="simple"><names>'
                         f'<name lang="en">{name}</name></names><description>'
                         f'<terminal x="0" y="0" orientation="n"/></description></definition>',
                         encoding="utf-8")
        r = m.tool_element_search(str(self.root), "coil relay")
        self.assertEqual([e["name"] for e in r["results"]],
                         ["Coil relay", "Coil relay X", "Relay coil", "A coil relay"])

    def test_search_returns_25_by_default(self):
        for n in range(26):
            (self.root / f"{n:02}.elmt").write_text(
                f'<definition type="element" link_type="simple"><names><name lang="en">Coil {n}'
                f'</name></names><description><terminal x="0" y="0" orientation="n"/>'
                f'</description></definition>', encoding="utf-8")
        r = m.tool_element_search(str(self.root), "coil")
        self.assertEqual((r["total_matches"], r["returned"]), (26, 25))

    def test_search_exact_name_wins_a_tie(self):
        """Same length, same first word: the exact name comes first, not
        the one whose path sorts first."""
        for rel, name in (("a.elmt", "Coil-a"), ("z.elmt", "Coil a")):
            (self.root / rel).write_text(
                f'<definition type="element" link_type="simple"><names><name lang="en">{name}'
                f'</name></names><description><terminal x="0" y="0" orientation="n"/>'
                f'</description></definition>', encoding="utf-8")
        r = m.tool_element_search(str(self.root), "coil a")
        self.assertEqual([e["name"] for e in r["results"]], ["Coil a", "Coil-a"])

    def test_validate_part_refusals(self):
        ok = lambda part: m._validate_part(0, part)
        self.assertEqual(ok({"type": "polygon", "points": [[0, 0], [1, 1]]}), "polygon")
        for message, part in [
                ("not an object", ["line"]),
                ("at least two points", {"type": "polygon", "points": [[0, 0]]}),
                ("at least two points", {"type": "polygon", "points": "ab"}),
                ("each point is", {"type": "polygon", "points": [[0, 0], [1, 1, 1]]}),
                ("each point is", {"type": "polygon", "points": [[0, 0], 5]}),
                ("uuid must look like", {"type": "line", "x1": 0, "y1": 0, "x2": 1, "y2": 1,
                                         "uuid": "L1"})]:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    ok(part)

    def test_terminal_order_with_unreadable_coordinates(self):
        T = lambda **a: ET.Element("terminal", {k: str(v) for k, v in a.items()})
        ordered, _ = m._terminals_in_index_order([T(x=0, y=0.5, name="b"), T(x=0, y="bad", name="a")])
        self.assertEqual([t.get("name") for t in ordered], ["a", "b"])
        ordered, _ = m._terminals_in_index_order([T(x=0.5, y=0, name="b"), T(x="bad", y=0, name="a")])
        self.assertEqual([t.get("name") for t in ordered], ["a", "b"])
        # a missing coordinate counts as 0
        ordered, _ = m._terminals_in_index_order([T(x=0, y=0.5, name="b"), T(x=0, name="a")])
        self.assertEqual([t.get("name") for t in ordered], ["a", "b"])
        ordered, _ = m._terminals_in_index_order([T(x=0.5, y=0, name="b"), T(y=0, name="a")])
        self.assertEqual([t.get("name") for t in ordered], ["a", "b"])


class CheckAndContinuityAnswers(unittest.TestCase):
    """qet_check and qet_continuity turn QElectroTech's log lines into their
    answer. With _run_qet replaced by a stub that returns chosen lines, the
    whole answer can be checked exactly, without QElectroTech. Found by a
    mutation audit: the binary tests look at a finding or two, so a wrong
    summary count, a dropped field or a wrong "passed" went unnoticed."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.qet = Path(self.tmp.name) / "p.qet"
        self.qet.write_text('<project><diagram title="a"/><diagram title="b"/></project>',
                            encoding="utf-8")

    def tearDown(self):
        self.tmp.cleanup()

    def stub(self, lines, **extra):
        out = "\n".join(["noise", m._MARKER + "{not json"] +
                        [m._MARKER + json.dumps(l) for l in lines])
        return mock.patch.object(m, "_run_qet", lambda *a, **k: {"stdout": out, "stderr": "", **extra})

    def test_check_answer_exactly(self):
        rows = [{"label": f"K{i}"} for i in range(12)]
        lines = [
            {"kind": "check", "name": "duplicate_master_labels", "rows": rows, "error": ""},
            {"kind": "check", "name": "unlabelled_masters", "rows": [{"x": 1}], "error": ""},
            {"kind": "check", "name": "unnumbered_conductors", "rows": [{"n": 1}, {"n": 2}], "error": ""},
            {"kind": "check", "name": "duplicate_simple_labels", "rows": [], "error": ""},
            {"kind": "check", "name": "empty_folios", "rows": None, "error": "bad SQL"},
            {"kind": "other", "name": "masters_without_manufacturer_reference", "rows": [1]},
        ]
        with self.stub(lines):
            r = m.tool_check("qet", str(self.qet))      # sample defaults to 10
        C = m.CHECKS
        self.assertEqual(r, {
            "ok": False,
            "summary": {"errors": 1, "warnings": 1, "info": 1, "passed": 1, "check_failures": 2},
            "findings": [
                {"check": "duplicate_master_labels", "severity": "error", "count": 12,
                 "note": C["duplicate_master_labels"]["note"], "rows": rows[:10]},
                {"check": "unlabelled_masters", "severity": "warning", "count": 1,
                 "note": C["unlabelled_masters"]["note"], "rows": [{"x": 1}]},
                {"check": "unnumbered_conductors", "severity": "info", "count": 2,
                 "note": C["unnumbered_conductors"]["note"], "rows": [{"n": 1}, {"n": 2}]}],
            "passed": ["duplicate_simple_labels"],
            "check_failures": [
                {"check": "empty_folios", "error": "bad SQL"},
                {"check": "masters_without_manufacturer_reference", "error": "no result came back"}]})

    def test_check_sorts_by_severity_then_name(self):
        lines = [{"kind": "check", "name": "unlabelled_masters", "rows": [1], "error": ""},
                 {"kind": "check", "name": "empty_folios", "rows": [1], "error": ""}]
        with self.stub(lines):
            r = m.tool_check("qet", str(self.qet), checks=["empty_folios", "unlabelled_masters"])
        self.assertEqual([f["check"] for f in r["findings"]], ["unlabelled_masters", "empty_folios"])

    def test_check_failure_alone_is_not_ok(self):
        lines = [{"kind": "check", "name": "empty_folios", "rows": None, "error": "bad SQL"}]
        with self.stub(lines):
            r = m.tool_check("qet", str(self.qet), checks=["empty_folios"])
        self.assertFalse(r["ok"])

    def test_check_ignores_a_line_without_the_marker(self):
        almost = "x" * (len(m._MARKER) - 1) + json.dumps(
            {"kind": "check", "name": "empty_folios", "rows": [1], "error": ""})
        with mock.patch.object(m, "_run_qet", lambda *a, **k: {"stdout": almost, "stderr": ""}):
            r = m.tool_check("qet", str(self.qet), checks=["empty_folios"])
        self.assertEqual(r["check_failures"], [{"check": "empty_folios", "error": "no result came back"}])

    def test_check_ok_without_errors_and_sample_zero(self):
        lines = [{"kind": "check", "name": "unlabelled_masters", "rows": [{"x": 1}], "error": ""}]
        with self.stub(lines):
            r = m.tool_check("qet", str(self.qet), checks=["unlabelled_masters"], sample=0)
        self.assertTrue(r["ok"])            # a warning is not a failure
        self.assertEqual(r["findings"][0]["rows"], [])
        self.assertEqual(r["findings"][0]["count"], 1)
        with self.assertRaisesRegex(ValueError, "sample must be >= 0"):
            m.tool_check("qet", str(self.qet), sample=-1)
        with self.assertRaisesRegex(ValueError, "no such project"):
            m.tool_check("qet", str(self.qet) + ".missing")

    def test_check_carries_the_launch_hint(self):
        with self.stub([], hint="why it did not start"):
            r = m.tool_check("qet", str(self.qet), checks=["empty_folios"])
        self.assertFalse(r["ok"])
        self.assertEqual(r["hint"], "why it did not start")

    def test_continuity_answer_exactly(self):
        found = [{"severity": "error", "folio": 1, "what": "a"},
                 {"severity": "warning", "folio": 0, "what": "b"},
                 {"severity": "info", "folio": "?", "what": "c"},
                 {"severity": "info", "what": "d"},
                 {"severity": "info", "folio": 1, "what": "e"}]
        with self.stub([{"kind": "continuity", "findings": found},
                        {"kind": "other", "findings": []}]):
            r = m.tool_continuity("qet", str(self.qet), folio=1)
        self.assertEqual((r["finding_count"], r["errors"], r["warnings"], r["info"]), (5, 1, 1, 3))
        self.assertEqual([f.get("folio_number") for f in r["findings"]], [2, 1, None, None, 2])
        self.assertNotIn(m._MARKER, r["stdout"])
        self.assertIn("noise", r["stdout"])

    def test_continuity_passes_the_folio_to_the_script(self):
        seen = []
        def fake(binary, args, **kw):
            seen.append(kw["script"])
            return {"stdout": m._MARKER + json.dumps({"kind": "continuity", "findings": []}),
                    "stderr": ""}
        with mock.patch.object(m, "_run_qet", fake):
            m.tool_continuity("qet", str(self.qet))
            m.tool_continuity("qet", str(self.qet), folio=1)
        self.assertIn("qet.checkContinuity(-1)", seen[0])
        self.assertIn("qet.checkContinuity(1)", seen[1])
        with self.assertRaisesRegex(ValueError, "no such project"):
            m.tool_continuity("qet", str(self.qet) + ".missing")
        almost = "x" * (len(m._MARKER) - 1) + json.dumps({"kind": "continuity", "findings": []})
        with mock.patch.object(m, "_run_qet", lambda *a, **k: {"stdout": almost, "stderr": ""}):
            self.assertFalse(m.tool_continuity("qet", str(self.qet))["ok"])

    def test_continuity_without_findings_is_not_ok(self):
        with self.stub([]):
            r = m.tool_continuity("qet", str(self.qet))
        self.assertFalse(r["ok"])
        self.assertIn("predate qet.checkContinuity()", r["hint"])
        with self.stub([], hint="launch failed"):
            self.assertEqual(m.tool_continuity("qet", str(self.qet))["hint"], "launch failed")

    def test_continuity_folio_bounds(self):
        for bad in (-1, 2):
            with self.subTest(folio=bad):
                with self.assertRaisesRegex(ValueError, f"folio {bad} does not exist"):
                    m.tool_continuity("qet", str(self.qet), folio=bad)
        with self.stub([{"kind": "continuity", "findings": []}]):
            self.assertEqual(m.tool_continuity("qet", str(self.qet), folio=0)["finding_count"], 0)


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


class WrongFolioHint(unittest.TestCase):
    """qet_elements numbers folios from 1, qet_edit from 0; a failed op that
    used the wrong one should say which index to use."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.qet = Path(self.tmp.name) / "p.qet"
        self.qet.write_text(
            '<project><diagram><elements/></diagram>'
            '<diagram><elements><element uuid="{b}" type="x" x="0" y="0"/></elements></diagram>'
            '</project>')

    def tearDown(self):
        self.tmp.cleanup()

    def hint(self, op):
        return m._wrong_folio(str(self.qet), op)

    def test_the_number_qet_elements_showed_gets_the_index_to_use(self):
        h = self.hint({"op": "move_element", "folio": 2, "element": "{b}", "dx": 5, "dy": 0})
        self.assertIn("folio 2 as qet_elements numbers it", h)
        self.assertIn('"folio": 1 here', h)

    def test_no_hint_when_the_folio_was_right_or_cannot_be_checked(self):
        for op in ({"op": "move_element", "folio": 1, "element": "{b}"},     # right index
                   {"op": "move_element", "folio": 0, "element": "{nope}"},  # not in the file
                   {"op": "move_element", "folio": 0, "element": "$k1"},     # placed by the script
                   {"op": "move_element", "folio": "$f", "element": "{b}"},  # folio by reference
                   {"op": "add_folio"}):
            with self.subTest(op=op):
                self.assertEqual(self.hint(op), "")

    def test_the_description_says_how_folios_are_counted(self):
        tool = next(t for t in m.TOOLS if t["name"] == "qet_edit")
        self.assertIn("counted from 0", tool["inputSchema"]["properties"]["operations"]["description"])


class ReadToolContracts(unittest.TestCase):
    """Exact answers of the read tools on a small hand-made project.

    A mutation audit (planting small bugs in the server and running this
    suite) found qet_elements, qet_conductors and qet_project_info almost
    untested: a dropped output field, a filter that never applied or a
    count off by one all passed. Every value here is pinned exactly."""

    A = "{aaaaaaaa-0000-4000-8000-000000000001}"
    B = "{bbbbbbbb-0000-4000-8000-000000000002}"
    C = "{cccccccc-0000-4000-8000-000000000003}"
    D = "{dddddddd-0000-4000-8000-000000000004}"

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.qet = str(Path(self.tmp.name) / "p.qet")
        A, B, C, D = self.A, self.B, self.C, self.D
        Path(self.qet).write_text(
            '<project title="T" version="0.100">'
            '<diagram title="One"><elements>'
            f'<element uuid="{A}" type="embed://import/x/Coil.elmt" x="10" y="20">'
            '<terminals><terminal id="1" x="0" y="-10" orientation="n"/>'
            '<terminal id="2" x="0" y="10" orientation="s"/></terminals>'
            '<elementInformations>'
            '<elementInformation show="1" name="label">K1</elementInformation>'
            '<elementInformation show="1" name="comment"> spaced </elementInformation>'
            '<elementInformation show="1">no name, ignored</elementInformation>'
            '</elementInformations></element>'
            f'<element uuid="{B}" type="embed://import/x/relay_coil.elmt" x="30" y="40"/>'
            f'<element uuid="{C}" type="embed://import/x/lamp.elmt" x="50" y="60"/>'
            '</elements><conductors>'
            '<conductor terminal1="1" terminal2="2" num="W1" formula="F" cable="C1" bus="B"'
            ' function="L1" conductor_color="red" conductor_section="1.5" type="multi"/>'
            f'<conductor element1="{B}" terminal1="" terminalname1="A1"'
            f' element2="{C}" terminal2="{{t2}}" num="W2" cable="  "/>'
            '</conductors></diagram>'
            '<diagram title="Two"><elements>'
            f'<element uuid="{D}" type="embed://import/x/Coil.elmt" x="1" y="2"/>'
            '</elements><conductors>'
            f'<conductor element1="{D}" terminal1="{{t1}}" element2="{D}" terminal2="{{t2}}"/>'
            '</conductors></diagram></project>')

    def tearDown(self):
        self.tmp.cleanup()

    def test_project_info_exact(self):
        self.assertEqual(m.tool_project_info(self.qet), {
            "file": self.qet, "title": "T", "version": "0.100",
            "folio_count": 2, "element_count": 4, "conductor_count": 3,
            "folios": [{"index": 1, "uuid": "", "title": "One", "elements": 3, "conductors": 2},
                       {"index": 2, "uuid": "", "title": "Two", "elements": 1, "conductors": 1}]})

    def test_elements_rows_exact(self):
        r = m.tool_elements(self.qet)
        self.assertEqual((r["count"], r["truncated"]), (4, False))
        self.assertEqual(r["elements"][0], {
            "folio": 1, "uuid": self.A, "type": "embed://import/x/Coil.elmt", "name": "Coil",
            "x": "10", "y": "20", "label": "K1",
            # values as QElectroTech saves them (trimmed); a field with no name is skipped
            "info": {"label": "K1", "comment": "spaced"}})
        self.assertEqual([(e["folio"], e["name"], e["label"], e["info"]) for e in r["elements"][1:]],
                         [(1, "relay_coil", "", {}), (1, "lamp", "", {}), (2, "Coil", "", {})])

    def test_elements_filters_and_limit(self):
        names = lambda r: [e["name"] for e in r["elements"]]
        self.assertEqual(names(m.tool_elements(self.qet, folio=2)), ["Coil"])
        self.assertEqual(names(m.tool_elements(self.qet, folio=1)), ["Coil", "relay_coil", "lamp"])
        # case-insensitive substring of the definition's file name
        self.assertEqual(names(m.tool_elements(self.qet, name_contains="COIL")),
                         ["Coil", "relay_coil", "Coil"])
        self.assertEqual(names(m.tool_elements(self.qet, folio=1, name_contains="coil")),
                         ["Coil", "relay_coil"])
        r = m.tool_elements(self.qet, limit=2)
        self.assertEqual((r["count"], r["truncated"], len(r["elements"])), (4, True, 2))
        r = m.tool_elements(self.qet, limit=4)
        self.assertEqual((r["count"], r["truncated"], len(r["elements"])), (4, False, 4))

    def test_conductor_rows_exact(self):
        r = m.tool_conductors(self.qet)
        self.assertEqual((r["count"], r["truncated"]), (3, False))
        first, second, third = r["conductors"]
        self.assertEqual(first, {
            "folio": 1, "uuid": "", "num": "W1", "formula": "F", "cable": "C1", "bus": "B",
            "function": "L1", "color": "red", "section": "1.5", "type": "multi",
            # legacy ends resolved through the folio's terminal ids
            "key": f"1:{self.A}@0,-10,n--{self.A}@0,10,s"})
        # current ends: instance/terminal; an empty terminal falls back to its name
        self.assertEqual(second["key"], f"1:{self.B}/A1--{self.C}/{{t2}}")
        self.assertEqual((second["num"], second["cable"]), ("W2", "  "))
        self.assertEqual((third["folio"], third["key"]), (2, f"2:{self.D}/{{t1}}--{self.D}/{{t2}}"))

    def test_conductor_filters_and_limit(self):
        nums = lambda r: [c["num"] for c in r["conductors"]]
        self.assertEqual(nums(m.tool_conductors(self.qet, folio=1)), ["W1", "W2"])
        self.assertEqual(nums(m.tool_conductors(self.qet, folio=2)), [""])
        r = m.tool_conductors(self.qet, attribute="cable")
        self.assertEqual([c["value"] for c in r["conductors"]], ["C1", "  ", ""])
        # non_empty drops empty and whitespace-only values
        r = m.tool_conductors(self.qet, attribute="cable", non_empty=True)
        self.assertEqual([c["value"] for c in r["conductors"]], ["C1"])
        # non_empty alone, with no attribute, filters nothing
        self.assertEqual(m.tool_conductors(self.qet, non_empty=True)["count"], 3)
        self.assertNotIn("value", m.tool_conductors(self.qet)["conductors"][0])
        r = m.tool_conductors(self.qet, limit=2)
        self.assertEqual((r["count"], r["truncated"], len(r["conductors"])), (3, True, 2))
        r = m.tool_conductors(self.qet, limit=3)
        self.assertEqual((r["count"], r["truncated"], len(r["conductors"])), (3, False, 3))

    def test_default_limit_is_200_rows(self):
        els = "".join(f'<element uuid="{{{i}}}" type="x.elmt"/>' for i in range(201))
        wires = "".join(f'<conductor terminal1="{i}" terminal2="{i}"/>' for i in range(201))
        big = Path(self.tmp.name) / "big.qet"
        big.write_text(f'<project><diagram><elements>{els}</elements>'
                       f'<conductors>{wires}</conductors></diagram></project>')
        for tool, key in ((m.tool_elements, "elements"), (m.tool_conductors, "conductors")):
            with self.subTest(tool=key):
                r = tool(str(big))
                self.assertEqual((r["count"], r["truncated"], len(r[key])), (201, True, 200))

    def test_conductor_key_same_in_both_forms(self):
        # A wire in the numbered form before a save and the uuid form after
        # it (the first save of an older project) is the same wire: both
        # ends resolve to the placed symbol's terminal record. The record is
        # where the wire docks, 4 from the definition position.
        root = ET.fromstring(
            '<project><collection><category name="import"><category name="x">'
            '<element name="S.elmt"><definition><description>'
            '<terminal uuid="{T1}" x="0" y="0" orientation="s"/>'
            '<terminal uuid="{T2}" x="10" y="0" orientation="e"/>'
            '</description></definition></element>'
            '</category></category></collection>'
            '<diagram><elements><element uuid="{E}" type="embed://import/x/S.elmt">'
            '<terminals><terminal id="5" x="0" y="-4" orientation="2"/>'
            '<terminal id="6" x="6" y="0" orientation="1"/></terminals>'
            '</element></elements><conductors>'
            '<conductor terminal1="5" terminal2="6"/>'
            '<conductor element1="{e}" terminal1="{t1}" element2="{E}" terminal2="{T2}"/>'
            '</conductors></diagram></project>')
        numbered, by_uuid = [m._conductor_row(i, c, ix)["key"]
                             for i, c, ix in m._conductors(root)]
        self.assertEqual(numbered, "1:{E}@0,-4,2--{E}@6,0,1")
        self.assertEqual(by_uuid, numbered)

    def test_conductor_key_same_in_both_forms_in_a_column(self):
        """Terminals one above the other share their x: a uuid end must be
        matched on x, y and orientation together, not on any one of them.
        Found by the mutation audit: an "or" in place of "and" went
        unnoticed while every test's terminals differed in every coordinate."""
        root = ET.fromstring(
            '<project><collection><category name="import"><category name="x">'
            '<element name="C.elmt"><definition><description>'
            '<terminal uuid="{T1}" x="0" y="0" orientation="n"/>'
            '<terminal uuid="{T2}" x="0" y="20" orientation="n"/>'
            '<terminal uuid="{T3}" x="30" y="20" orientation="n"/>'
            '</description></definition></element>'
            '</category></category></collection>'
            '<diagram><elements><element uuid="{E}" type="embed://import/x/C.elmt">'
            '<terminals><terminal id="1" x="0" y="4" orientation="0"/>'
            '<terminal id="2" x="0" y="24" orientation="0"/>'
            '<terminal id="3" x="30" y="24" orientation="0"/></terminals>'
            '</element></elements><conductors>'
            '<conductor terminal1="2" terminal2="3"/>'
            '<conductor element1="{E}" terminal1="{T2}" element2="{E}" terminal2="{T3}"/>'
            '</conductors></diagram></project>')
        numbered, by_uuid = [m._conductor_row(i, c, ix)["key"]
                             for i, c, ix in m._conductors(root)]
        self.assertEqual(numbered, "1:{E}@0,24,0--{E}@30,24,0")
        self.assertEqual(by_uuid, numbered)

    def test_conductor_row_without_an_index(self):
        c = ET.fromstring('<conductor terminal1="7" terminal2="8"/>')
        self.assertEqual(m._conductor_row(3, c)["key"], "3:#7--#8")

    def test_plain_text(self):
        self.assertEqual(m._plain_text("<html><body><p>a &amp; <b>b</b></p>\n<p> c </p></body></html>"),
                         "a & b c")
        # no <body>: the whole string is the text
        self.assertEqual(m._plain_text("plain <b>x</b>"), "plain x")
        self.assertEqual(m._plain_text(None), "")


class ItemsTool(unittest.TestCase):
    """qet_items: every drawn item that is not a symbol or a wire, with its uuid."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.qet = str(Path(self.tmp.name) / "p.qet")
        Path(self.qet).write_text(
            '<project><diagram><elements>'
            '<element uuid="{el}" type="x.elmt" x="0" y="0"><dynamic_texts>'
            '<dynamic_elmt_text uuid="{et}" x="5" y="6" rotation="0" text_width="-1" frame="false"'
            ' font="Sans,9,-1" text_from="ElementInfo"><text>K1</text><info_name>label</info_name>'
            '</dynamic_elmt_text></dynamic_texts></element></elements><conductors/>'
            '<inputs><input uuid="{tx}" x="1" y="2" rotation="0" font="f" color="c"'
            ' text="&lt;html&gt;&lt;body&gt;note&lt;/body&gt;&lt;/html&gt;"/></inputs>'
            '<shapes><shape uuid="{sh}" type="Line" x1="0" y1="0" x2="9" y2="9" rotation="0">'
            '<pen color="c" style="s" widthF="1"/></shape></shapes>'
            '</diagram><diagram><elements/><conductors/>'
            '<images><image x="3" y="4" size="1" rotation="0"/></images>'
            '<tables><graphics_table uuid="{tb}" name="Parts" x="0" y="0" width="100" height="50"'
            ' display_n_row="10"/></tables>'
            '</diagram></project>')

    def tearDown(self):
        self.tmp.cleanup()

    def test_every_kind_with_its_uuid(self):
        r = m.tool_items(self.qet)
        self.assertEqual((r["count"], r["truncated"]), (5, False))
        self.assertEqual(r["items"], [
            {"kind": "text", "uuid": "{tx}", "folio": 1, "x": "1", "y": "2", "text": "note",
             "rotation": "0", "font": "f", "color": "c"},
            {"kind": "shape", "uuid": "{sh}", "folio": 1, "type": "Line", "from": ["0", "0"],
             "to": ["9", "9"], "line_color": "c", "line_style": "s", "line_width": "1",
             "fill": "none", "rotation": "0"},
            {"kind": "element_text", "uuid": "{et}", "folio": 1, "element": "{el}",
             "source": "ElementInfo", "bound_to": "label", "n": 1, "x": "5", "y": "6",
             "size": "9", "frame": "false", "rotation": "0", "width": "-1", "shows": "K1"},
            # saved before pictures carried a uuid: "" until the next save
            {"kind": "image", "uuid": "", "folio": 2, "x": "3", "y": "4", "scale": "1",
             "rotation": "0"},
            {"kind": "table", "uuid": "{tb}", "folio": 2, "name": "Parts", "x": "0", "y": "0",
             "width": "100", "height": "50", "rows_shown": "10"}])

    def test_filters_and_limit(self):
        kinds = lambda r: [i["kind"] for i in r["items"]]
        self.assertEqual(kinds(m.tool_items(self.qet, folio=2)), ["image", "table"])
        self.assertEqual(kinds(m.tool_items(self.qet, kind="shape")), ["shape"])
        self.assertEqual(kinds(m.tool_items(self.qet, folio=2, kind="shape")), [])
        r = m.tool_items(self.qet, limit=2)
        self.assertEqual((r["count"], r["truncated"], len(r["items"])), (5, True, 2))
        r = m.tool_items(self.qet, limit=5)
        self.assertEqual((r["count"], r["truncated"]), (5, False))
        with self.assertRaisesRegex(ValueError, "kind must be one of"):
            m.tool_items(self.qet, kind="wire")

    def test_default_limit_is_500(self):
        many = "".join(f'<input uuid="{{{i}}}" x="{i}" y="0" text="t"/>' for i in range(501))
        big = Path(self.tmp.name) / "big.qet"
        big.write_text(f'<project><diagram><inputs>{many}</inputs></diagram></project>')
        r = m.tool_items(str(big))
        self.assertEqual((r["count"], r["truncated"], len(r["items"])), (501, True, 500))


class DiffContracts(unittest.TestCase):
    """Exact qet_diff output, section by section, on hand-made pairs.

    The mutation audit found most of tool_diff's output unchecked: a dropped
    field of a move, of a conductor change or of a text record, a skipped
    comparison, or a list cap off by one all passed the suite."""

    A = "{aaaaaaaa-0000-4000-8000-000000000001}"
    B = "{bbbbbbbb-0000-4000-8000-000000000002}"
    C = "{cccccccc-0000-4000-8000-000000000003}"
    D = "{dddddddd-0000-4000-8000-000000000004}"

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.n = 0

    def tearDown(self):
        self.tmp.cleanup()

    def qet(self, body, title="T"):
        self.n += 1
        p = Path(self.tmp.name) / f"p{self.n}.qet"
        p.write_text(f'<project title="{title}">{body}</project>')
        return str(p)

    @staticmethod
    def el(uuid, x, y, label=None, comment=None, type_="x/coil.elmt", texts=""):
        info = "".join(f'<elementInformation show="1" name="{n}">{v}</elementInformation>'
                       for n, v in (("label", label), ("comment", comment)) if v is not None)
        u = f' uuid="{uuid}"' if uuid else ""
        return (f'<element{u} type="{type_}" x="{x}" y="{y}">'
                f'<elementInformations>{info}</elementInformations>'
                f'<dynamic_texts>{texts}</dynamic_texts></element>')

    @staticmethod
    def wire(e1, e2, **attrs):
        a = "".join(f' {k}="{v}"' for k, v in attrs.items())
        return f'<conductor element1="{e1}" terminal1="{{t1}}" element2="{e2}" terminal2="{{t2}}"{a}/>'

    def folio(self, elements="", wires="", extra="", **attrs):
        a = "".join(f' {k}="{v}"' for k, v in attrs.items())
        return (f'<diagram{a}><elements>{elements}</elements>'
                f'<conductors>{wires}</conductors>{extra}</diagram>')

    def test_elements_section_exact(self):
        A, B, C, D = self.A, self.B, self.C, self.D
        before = self.qet(self.folio(
            self.el(A, 10, 20, "K1", "c") + self.el(B, 0, 0) + self.el(C, 5, 5)
            + self.el("", 1, 1, type_="x/old.elmt")))
        after = self.qet(self.folio(
            self.el(A, 15, 20, "K2", "d") + self.el(B, 0, 0) + self.el(D, 7, 7)
            + self.el("", 2, 2, type_="x/old.elmt")))
        self.assertEqual(m.tool_diff(before, after)["elements"], {
            "before": 4, "after": 4,
            # no uuid: keyed on folio, position and name, so a move is remove + add
            "added": sorted([D, "1:2,2:old"]), "removed": sorted([C, "1:1,1:old"]),
            "moved": [{"uuid": A, "name": "coil", "folio": 1, "from": ["10", "20"],
                       "to": ["15", "20"], "delta": [5.0, 0.0]}],
            "moved_count": 1, "distinct_move_deltas": [[5.0, 0.0]],
            "relabelled": [{"uuid": A, "name": "coil", "from": "K1", "to": "K2"}],
            "info_changed": [{"uuid": A, "name": "coil", "from": {"label": "K1", "comment": "c"},
                              "to": {"label": "K2", "comment": "d"}}],
            "rotated": []})

    def test_an_unchanged_element_reports_nothing(self):
        p = self.qet(self.folio(self.el(self.A, 1, 2, "K1", "c")))
        e = m.tool_diff(p, p)["elements"]
        self.assertEqual((e["moved"], e["relabelled"], e["info_changed"], e["added"], e["removed"],
                          e["rotated"]), ([], [], [], [], [], []))

    def test_equal_angles_written_differently_are_the_same(self):
        self.assertEqual([m._angle(v) for v in ("-270", "90", "-90", "270", "360", "0", "450", "12.5", "", "x")],
                         ["90", "90", "270", "270", "0", "0", "90", "12.5", "", "x"])
        def field(rot):
            return self.el(self.A, 0, 0, texts=(
                f'<dynamic_elmt_text x="0" y="0" rotation="{rot}" text_from="UserText"><text>t</text>'
                '</dynamic_elmt_text>'))
        d = m.tool_diff(self.qet(self.folio(field("90"))), self.qet(self.folio(field("-270"))))
        self.assertEqual(d["element_texts"]["changed"], [])
        d = m.tool_diff(self.qet(self.folio(field("90"))), self.qet(self.folio(field("180"))))
        self.assertEqual(d["element_texts"]["changed"][0]["changed"], {"rotation": ["90", "180"]})

    def test_a_rotation_is_reported(self):
        """Rotating a symbol changes only its orientation (quarter turns); a diff
        that ignored it reported a rotation as no change at all."""
        rot = lambda o: self.qet(self.folio(
            self.el(self.A, 1, 2).replace('x="1"', f'orientation="{o}" x="1"')))
        d = m.tool_diff(rot(0), rot(1))
        self.assertEqual(d["elements"]["rotated"],
                         [{"uuid": self.A, "name": "coil", "folio": 1, "orientation": ["0", "1"]}])
        self.assertEqual((d["elements"]["moved"], d["elements"]["relabelled"]), ([], []))
        # no attribute is orientation 0, as QElectroTech reads it
        plain = self.qet(self.folio(self.el(self.A, 1, 2)))
        self.assertEqual(m.tool_diff(plain, rot(0))["elements"]["rotated"], [])

    def test_conductors_section_exact(self):
        A, B, C = self.A, self.B, self.C
        els = self.el(A, 0, 0) + self.el(B, 0, 0) + self.el(C, 0, 0)
        before = self.qet(self.folio(els, self.wire(A, B, num="W1", cable="C1")
                                     + self.wire(B, C, num="W2")))
        after = self.qet(self.folio(els, self.wire(A, B, num="W9", cable="C1", conductor_color="red")
                                    + self.wire(A, C, num="W3")))
        k = lambda x, y: f"1:{x}/{{t1}}--{y}/{{t2}}"
        self.assertEqual(m.tool_diff(before, after)["conductors"], {
            "before": 2, "after": 2, "keyed_by": "ends", "added": [k(A, C)], "removed": [k(B, C)],
            "changed": [{"key": k(A, B), "changed": {"num": ["W1", "W9"], "color": ["", "red"]}}],
            "changed_count": 1})

    def test_unresolvable_conductor_ends_are_flagged(self):
        p = self.qet(self.folio(self.el("", 0, 0),
                                '<conductor terminal1="3" terminal2="4" num="W1"/>'))
        c = m.tool_diff(p, p)["conductors"]
        self.assertEqual(c["unstable_keys"], 1)
        self.assertIn("renumbers on save", c["warning"])
        q = self.qet(self.folio(self.el(self.A, 0, 0) + self.el(self.B, 0, 0),
                                self.wire(self.A, self.B)))
        self.assertNotIn("unstable_keys", m.tool_diff(q, q)["conductors"])

    def test_list_caps(self):
        many = lambda n, x: "".join(self.el(f"{{{i:08d}-0000-4000-8000-000000000000}}", x, 0)
                                     for i in range(n))
        e = m.tool_diff(self.qet(self.folio(many(101, 0))), self.qet(self.folio(many(101, 5))))["elements"]
        self.assertEqual((len(e["moved"]), e["moved_count"]), (100, 101))
        e = m.tool_diff(self.qet(self.folio(many(51, 0))), self.qet(self.folio("")))["elements"]
        self.assertEqual((len(e["removed"]), e["before"]), (50, 51))
        e = m.tool_diff(self.qet(self.folio("")), self.qet(self.folio(many(51, 0))))["elements"]
        self.assertEqual(len(e["added"]), 50)

    def test_texts_shapes_images_by_uuid(self):
        def extras(x, color, uuid=True):
            u = lambda s: f' uuid="{{{s}}}"' if uuid else ""
            return (f'<inputs><input{u("t")} x="{x}" y="2" rotation="0" font="f" color="{color}"'
                    ' text="&lt;html&gt;&lt;body&gt;note&lt;/body&gt;&lt;/html&gt;"/></inputs>'
                    f'<shapes><shape{u("s")} type="Rectangle" x1="{x}" y1="0" x2="9" y2="9" rotation="0">'
                    f'<pen color="{color}" style="SolidLine" widthF="1"/><brush style="NoBrush"/></shape></shapes>'
                    f'<images><image{u("i")} x="{x}" y="3" size="1" rotation="0"/></images>')
        d = m.tool_diff(self.qet(self.folio(extra=extras(1, "#000"))),
                        self.qet(self.folio(extra=extras(4, "#f00"))))
        # A uuid-keyed item is labelled as it is after the edit; "changed" has both.
        self.assertEqual(d["texts"]["changed"], [{
            "item": {"folio": 1, "x": "4", "y": "2", "text": "note", "uuid": "{t}"},
            "changed": {"x": ["1", "4"], "color": ["#000", "#f00"]}}])
        self.assertEqual(d["shapes"]["changed"], [{
            "item": {"folio": 1, "type": "Rectangle", "from": ["4", "0"], "to": ["9", "9"], "uuid": "{s}"},
            "changed": {"from": [["1", "0"], ["4", "0"]], "line_color": ["#000", "#f00"]}}])
        self.assertEqual(d["images"]["changed"], [{
            "item": {"folio": 1, "x": "4", "y": "3", "uuid": "{i}"}, "changed": {"x": ["1", "4"]}}])
        for kind in ("texts", "shapes", "images"):
            self.assertEqual((d[kind]["keyed_by"], d[kind]["before"], d[kind]["after"]), ("uuid", 1, 1))
        # without uuids a move is a removal plus an addition, keyed by position
        d = m.tool_diff(self.qet(self.folio(extra=extras(1, "#000", uuid=False))),
                        self.qet(self.folio(extra=extras(4, "#000", uuid=False))))
        self.assertEqual((d["images"]["keyed_by"], len(d["images"]["added"]), len(d["images"]["removed"])),
                         ("position", 1, 1))

    def test_shape_style_fields(self):
        def shape(style, width, brush):
            return (f'<shapes><shape uuid="{{s}}" type="Line" x1="0" y1="0" x2="1" y2="1" rotation="90">'
                    f'<pen color="#000" style="{style}" widthF="{width}"/>{brush}</shape></shapes>')
        d = m.tool_diff(
            self.qet(self.folio(extra=shape("SolidLine", 1, '<brush style="NoBrush" color="#0f0"/>'))),
            self.qet(self.folio(extra=shape("DashLine", 2, '<brush style="SolidPattern" color="#0f0"/>'))))
        self.assertEqual(d["shapes"]["changed"][0]["changed"],
                         {"line_style": ["SolidLine", "DashLine"], "line_width": ["1", "2"],
                          "fill": ["none", "#0f0"]})

    def test_element_text_fields(self):
        def field(x, size, shows):
            return (f'<dynamic_elmt_text x="{x}" y="1" frame="false" rotation="0" text_width="-1" '
                    f'text_from="ElementInfo" font="Sans,{size},-1"><text>{shows}</text>'
                    '<info_name>label</info_name></dynamic_elmt_text>')
        d = m.tool_diff(self.qet(self.folio(self.el(self.A, 0, 0, texts=field(3, 9, "K1")))),
                        self.qet(self.folio(self.el(self.A, 0, 0, texts=field(6, 9, "K2")))))
        self.assertEqual(d["element_texts"]["changed"], [{
            "item": {"element": self.A, "source": "ElementInfo", "bound_to": "label", "n": 1},
            "changed": {"x": ["3", "6"], "shows": ["K1", "K2"]}}])

    def test_folio_and_project_fields(self):
        d = m.tool_diff(self.qet(self.folio(title="A", cols="10"), title="P"),
                        self.qet(self.folio(title="B", cols="10"), title="Q"))
        self.assertEqual(d["project"], {"changed": {"title": ["P", "Q"]}})
        self.assertEqual(d["folios"], {"before": 1, "after": 1, "keyed_by": "position",
                                       "changed": [{"folio": 1, "changed": {"title": ["A", "B"]}}]})
        d = m.tool_diff(self.qet(self.folio(title="A")),
                        self.qet(self.folio(title="B") + self.folio(title="C")))
        self.assertIn("folio count changed", d["folios"]["note"])
        d = m.tool_diff(self.qet(self.folio(title="A")),
                        self.qet(self.folio(title="A") + self.folio(title="C")))
        self.assertNotIn("note", d["folios"])

    def test_every_compared_field_is_reported_on_its_own(self):
        """Change one attribute at a time; each must show as exactly that field."""
        def changed_fields(before, after, kind):
            d = m.tool_diff(self.qet(self.folio(extra=before)), self.qet(self.folio(extra=after)))
            return [list(c["changed"]) for c in d[kind]["changed"]]
        def text(**v):
            a = {"rotation": "0", "font": "f", "color": "c", **v}
            return ('<inputs><input uuid="{t}" x="1" y="2" '
                    + " ".join(f'{k}="{w}"' for k, w in a.items()) + ' text="x"/></inputs>')
        def shape(rot):
            return (f'<shapes><shape uuid="{{s}}" type="Line" x1="0" y1="0" x2="1" y2="1" '
                    f'rotation="{rot}"><pen color="c" style="s" widthF="1"/></shape></shapes>')
        def image(**v):
            a = {"size": "1", "rotation": "0", **v}
            return ('<images><image uuid="{i}" x="0" y="0" '
                    + " ".join(f'{k}="{w}"' for k, w in a.items()) + '/></images>')
        for kind, before, after, field in (
                ("texts", text(), text(rotation="90"), "rotation"),
                ("texts", text(), text(font="g"), "font"),
                ("shapes", shape(0), shape(90), "rotation"),
                ("images", image(), image(rotation="90"), "rotation"),
                ("images", image(), image(size="2"), "scale")):
            with self.subTest(kind=kind, field=field):
                self.assertEqual(changed_fields(before, after, kind), [[field]])

    def test_every_element_text_field_attribute(self):
        def field(**v):
            a = {"x": "0", "y": "0", "frame": "false", "rotation": "0", "text_width": "-1",
                 "font": "Sans,9,-1", **v}
            return self.el(self.A, 0, 0, texts=(
                "<dynamic_elmt_text " + " ".join(f'{k}="{w}"' for k, w in a.items())
                + ' text_from="CompositeText"><text>K1</text>'
                '<composite_text>%{label}</composite_text></dynamic_elmt_text>'))
        for attr, value, reported in (("y", "5", "y"), ("rotation", "90", "rotation"),
                                      ("text_width", "40", "width"), ("frame", "true", "frame"),
                                      ("font", "Sans,12,-1", "size")):
            with self.subTest(attr=attr):
                d = m.tool_diff(self.qet(self.folio(field())),
                                self.qet(self.folio(field(**{attr: value}))))
                ch = d["element_texts"]["changed"]
                self.assertEqual([list(c["changed"]) for c in ch], [[reported]])
                # a composite field is identified by its formula
                self.assertEqual(ch[0]["item"]["bound_to"], "%{label}")

    def test_an_unchanged_conductor_is_not_listed_as_changed(self):
        A, B, C = self.A, self.B, self.C
        els = self.el(A, 0, 0) + self.el(B, 0, 0) + self.el(C, 0, 0)
        before = self.qet(self.folio(els, self.wire(A, B, num="W1") + self.wire(B, C, num="W2")))
        after = self.qet(self.folio(els, self.wire(A, B, num="W1") + self.wire(B, C, num="W5")))
        c = m.tool_diff(before, after)["conductors"]
        self.assertEqual([x["changed"] for x in c["changed"]], [{"num": ["W2", "W5"]}])

    def test_a_strip_without_its_data_is_skipped(self):
        p = self.qet(self.folio() + '<terminal_strips><terminal_strip/></terminal_strips>')
        self.assertEqual(m.tool_diff(p, p)["terminal_strips"]["before"], 0)

    def test_every_list_is_capped(self):
        U = lambda i: f"{{{i:08d}-0000-4000-8000-000000000000}}"
        many = lambda n, label: "".join(self.el(U(i), 0, 0, label) for i in range(n))
        e = m.tool_diff(self.qet(self.folio(many(51, "a"))),
                        self.qet(self.folio(many(51, "b"))))["elements"]
        self.assertEqual((len(e["relabelled"]), len(e["info_changed"])), (50, 50))
        turned = lambda o: "".join(self.el(U(i), 0, 0).replace('x="0"', f'orientation="{o}" x="0"')
                                   for i in range(51))
        e = m.tool_diff(self.qet(self.folio(turned(0))), self.qet(self.folio(turned(2))))["elements"]
        self.assertEqual(len(e["rotated"]), 50)
        els = many(102, None)
        wires = lambda n, num: "".join(self.wire(U(i), U(i + 1), num=num) for i in range(n))
        c = m.tool_diff(self.qet(self.folio(els, wires(101, "a"))),
                        self.qet(self.folio(els, wires(101, "b"))))["conductors"]
        self.assertEqual((len(c["changed"]), c["changed_count"]), (100, 101))
        c = m.tool_diff(self.qet(self.folio(els, wires(51, "a"))), self.qet(self.folio(els)))["conductors"]
        self.assertEqual(len(c["removed"]), 50)
        c = m.tool_diff(self.qet(self.folio(els)), self.qet(self.folio(els, wires(51, "a"))))["conductors"]
        self.assertEqual(len(c["added"]), 50)
        folios = lambda t: "".join(self.folio(title=f"{t}{i}") for i in range(51))
        f = m.tool_diff(self.qet(folios("a")), self.qet(folios("b")))["folios"]
        self.assertEqual(len(f["changed"]), 50)
        # _diff_keyed's own caps, through the text records
        texts = lambda n, x: "<inputs>" + "".join(
            f'<input uuid="{U(i)}" x="{x}" y="0" text="t"/>' for i in range(n)) + "</inputs>"
        t = m.tool_diff(self.qet(self.folio(extra=texts(51, 0))),
                        self.qet(self.folio(extra=texts(51, 1))))["texts"]
        self.assertEqual(len(t["changed"]), 50)
        t = m.tool_diff(self.qet(self.folio(extra=texts(51, 0))), self.qet(self.folio()))["texts"]
        self.assertEqual(len(t["removed"]), 50)
        t = m.tool_diff(self.qet(self.folio()), self.qet(self.folio(extra=texts(51, 0))))["texts"]
        self.assertEqual(len(t["added"]), 50)

    def test_conductors_by_uuid(self):
        """Every conductor has a uuid: a rewire is that conductor with new ends,
        not one removed and another added."""
        A, B, C = self.A, self.B, self.C
        els = self.el(A, 0, 0) + self.el(B, 0, 0) + self.el(C, 0, 0)
        before = self.qet(self.folio(els, self.wire(A, B, uuid="{w1}", num="W1")
                                     + self.wire(B, C, uuid="{w2}")))
        after = self.qet(self.folio(els, self.wire(A, C, uuid="{w1}", num="W1")
                                    + self.wire(A, B, uuid="{w3}")))
        k = lambda x, y: f"1:{x}/{{t1}}--{y}/{{t2}}"
        c = m.tool_diff(before, after)["conductors"]
        self.assertEqual(c["keyed_by"], "uuid")
        self.assertEqual(c["changed"], [{"key": k(A, C), "uuid": "{w1}",
                                         "changed": {"ends": [k(A, B), k(A, C)]}}])
        # added/removed are still named by their ends, as in the ends mode
        self.assertEqual((c["added"], c["removed"]), ([k(A, B)], [k(B, C)]))
        # one conductor without a uuid puts the whole comparison back on ends
        mixed = self.qet(self.folio(els, self.wire(A, B, uuid="{w1}", num="W1") + self.wire(B, C)))
        self.assertEqual(m.tool_diff(before, mixed)["conductors"]["keyed_by"], "ends")

    def test_folios_by_uuid(self):
        """With folio uuids a reorder is one 'reordered' entry, not every later
        folio changing title."""
        f = lambda u, t: self.folio(title=t, uuid=u)
        before = self.qet(f("{f1}", "One") + f("{f2}", "Two") + f("{f3}", "Three"))
        after = self.qet(f("{f2}", "Two") + f("{f1}", "One") + f("{f4}", "Four"))
        d = m.tool_diff(before, after)["folios"]
        self.assertEqual(d["keyed_by"], "uuid")
        self.assertEqual(d["changed"], [])
        self.assertEqual(d["reordered"], [{"uuid": "{f2}", "title": "Two", "from": 2, "to": 1},
                                          {"uuid": "{f1}", "title": "One", "from": 1, "to": 2}])
        self.assertEqual(d["added"], [{"folio": 3, "uuid": "{f4}", "title": "Four"}])
        self.assertEqual(d["removed"], [{"folio": 3, "uuid": "{f3}", "title": "Three"}])
        self.assertNotIn("note", d)
        renamed = self.qet(f("{f1}", "Uno") + f("{f2}", "Two") + f("{f3}", "Three"))
        self.assertEqual(m.tool_diff(before, renamed)["folios"]["changed"],
                         [{"folio": 1, "uuid": "{f1}", "changed": {"title": ["One", "Uno"]}}])

    def test_element_text_fields_by_uuid(self):
        """By their own uuid, deleting the first of two label fields is that
        field removed -- by position it read as the second one changing."""
        def fields(*items):
            return self.el(self.A, 0, 0, texts="".join(
                f'<dynamic_elmt_text uuid="{u}" x="{x}" y="0" text_from="UserText"><text>{t}</text>'
                '</dynamic_elmt_text>' for u, x, t in items))
        before = self.qet(self.folio(fields(("{e1}", 1, "a"), ("{e2}", 2, "b"))))
        after = self.qet(self.folio(fields(("{e2}", 2, "b"))))
        d = m.tool_diff(before, after)["element_texts"]
        self.assertEqual(d["keyed_by"], "uuid")
        self.assertEqual((d["changed"], [r["uuid"] for r in d["removed"]]), ([], ["{e1}"]))
        # the same pair without uuids: the position-based reading, flagged as such
        plain = lambda *items: self.qet(self.folio(self.el(self.A, 0, 0, texts="".join(
            f'<dynamic_elmt_text x="{x}" y="0" text_from="UserText"><text>{t}</text></dynamic_elmt_text>'
            for x, t in items))))
        d = m.tool_diff(plain((1, "a"), (2, "b")), plain((2, "b")))["element_texts"]
        self.assertEqual(d["keyed_by"], "position")

    def test_copied_symbols_keep_their_text_field_uuids(self):
        """Copying a symbol keeps its text fields' uuids (20 copies of one in
        2612_ats_singlephase.qet), so a field is its symbol's uuid plus its own:
        editing one copy's field must not be read as another copy's."""
        field = lambda x: ('<dynamic_elmt_text uuid="{same}" x="' + str(x) + '" y="0" '
                           'text_from="UserText"><text>t</text></dynamic_elmt_text>')
        pair = lambda xa, xb: self.qet(self.folio(self.el(self.A, 0, 0, texts=field(xa))
                                                  + self.el(self.B, 0, 0, texts=field(xb))))
        for before, after, which in ((pair(1, 1), pair(5, 1), self.A), (pair(1, 1), pair(1, 5), self.B)):
            with self.subTest(edited=which):
                d = m.tool_diff(before, after)["element_texts"]
                self.assertEqual(d["keyed_by"], "uuid")
                self.assertEqual([(c["item"]["element"], c["item"]["uuid"], c["changed"])
                                  for c in d["changed"]], [(which, "{same}", {"x": ["1", "5"]})])

    def test_a_field_on_a_symbol_without_a_uuid_is_matched_by_position(self):
        """The field has a uuid but its symbol has none (an older file): with no
        symbol to pair it with, the whole kind falls back to position."""
        field = lambda x: ('<dynamic_elmt_text uuid="{e1}" x="' + str(x) + '" y="0" '
                           'text_from="UserText"><text>t</text></dynamic_elmt_text>')
        on = lambda sym, x: self.qet(self.folio(self.el(sym, 0, 0, texts=field(x))))
        d = m.tool_diff(on("", 1), on("", 2))["element_texts"]
        self.assertEqual(d["keyed_by"], "position")
        self.assertEqual([c["changed"] for c in d["changed"]], [{"x": ["1", "2"]}])
        # the symbol has a uuid on one side only: each side is checked on its own
        for before, after in ((on("", 1), on(self.A, 1)), (on(self.A, 1), on("", 1))):
            with self.subTest(before=before, after=after):
                self.assertEqual(m.tool_diff(before, after)["element_texts"]["keyed_by"], "position")

    def test_repeated_uuids_fall_back_rather_than_merge(self):
        # the same field uuid twice inside one symbol
        twice = lambda x: self.qet(self.folio(self.el(self.A, 0, 0, texts="".join(
            f'<dynamic_elmt_text uuid="{{dup}}" x="{v}" y="0" text_from="UserText"><text>{v}</text>'
            '</dynamic_elmt_text>' for v in (x, 9)))))
        self.assertEqual(m.tool_diff(twice(1), twice(2))["element_texts"]["keyed_by"], "position")
        # two shapes sharing a uuid: both must still be counted
        shapes = self.qet(self.folio(extra='<shapes>' + ''.join(
            f'<shape uuid="{{s}}" type="Line" x1="{i}" y1="0" x2="1" y2="1"/>' for i in (0, 5)) + '</shapes>'))
        d = m.tool_diff(shapes, shapes)["shapes"]
        self.assertEqual((d["keyed_by"], d["before"]), ("position", 2))
        # two conductors sharing a uuid
        els = self.el(self.A, 0, 0) + self.el(self.B, 0, 0) + self.el(self.C, 0, 0)
        wires = self.qet(self.folio(els, self.wire(self.A, self.B, uuid="{w}")
                                    + self.wire(self.B, self.C, uuid="{w}")))
        c = m.tool_diff(wires, wires)["conductors"]
        self.assertEqual((c["keyed_by"], c["before"]), ("ends", 2))

    def test_uuid_matching_when_one_side_has_none_of_a_kind(self):
        """No folios, fields or wires on one side is not a reason to fall back
        to position: the other side's uuids are all there is to match."""
        empty = self.qet("")
        full = self.qet(self.folio(
            self.el(self.A, 0, 0, texts='<dynamic_elmt_text uuid="{e1}" x="0" y="0" '
                    'text_from="UserText"><text>t</text></dynamic_elmt_text>')
            + self.el(self.B, 0, 0), self.wire(self.A, self.B, uuid="{w1}"), uuid="{f1}"))
        d = m.tool_diff(empty, full)
        self.assertEqual((d["folios"]["keyed_by"], d["element_texts"]["keyed_by"],
                          d["conductors"]["keyed_by"]), ("uuid", "uuid", "uuid"))
        self.assertEqual(d["folios"]["added"], [{"folio": 1, "uuid": "{f1}", "title": ""}])
        # matched by uuid, a conductor is never on the renumbered-id footing
        self.assertNotIn("unstable_keys", d["conductors"])

    def test_a_folio_that_kept_its_place_is_not_reordered(self):
        f = lambda u, t: self.folio(title=t, uuid=u)
        before = self.qet(f("{f1}", "One") + f("{f2}", "Two") + f("{f3}", "Three"))
        after = self.qet(f("{f1}", "One") + f("{f3}", "Three") + f("{f2}", "Two"))
        self.assertEqual([r["uuid"] for r in m.tool_diff(before, after)["folios"]["reordered"]],
                         ["{f3}", "{f2}"])

    def test_folio_lists_are_capped(self):
        f = lambda i, t: self.folio(title=t, uuid=f"{{{i:04d}}}")
        many = lambda rng, t: "".join(f(i, t) for i in rng)
        d = m.tool_diff(self.qet(""), self.qet(many(range(51), "x")))["folios"]
        self.assertEqual(len(d["added"]), 50)
        d = m.tool_diff(self.qet(many(range(51), "x")), self.qet(""))["folios"]
        self.assertEqual(len(d["removed"]), 50)
        d = m.tool_diff(self.qet(many(range(52), "x")), self.qet(many(reversed(range(52)), "x")))["folios"]
        self.assertEqual(len(d["reordered"]), 50)

    def test_table_fields(self):
        def table(**v):
            a = {"x": "0", "y": "0", "width": "100", "height": "50", "display_n_row": "10", **v}
            return ('<tables><graphics_table uuid="{tb}" name="P" '
                    + " ".join(f'{k}="{w}"' for k, w in a.items()) + '/></tables>')
        for attr, reported in (("y", "y"), ("width", "width"), ("height", "height")):
            with self.subTest(attr=attr):
                d = m.tool_diff(self.qet(self.folio(extra=table())),
                                self.qet(self.folio(extra=table(**{attr: "7"}))))["tables"]
                self.assertEqual([list(c["changed"]) for c in d["changed"]], [[reported]])

    def test_tables(self):
        table = lambda x, rows: ('<tables><graphics_table uuid="{tb}" name="Parts" x="' + str(x)
                                 + f'" y="0" width="100" height="50" display_n_row="{rows}"/></tables>')
        d = m.tool_diff(self.qet(self.folio(extra=table(0, 10))),
                        self.qet(self.folio(extra=table(20, 12))))["tables"]
        self.assertEqual((d["keyed_by"], d["before"], d["after"]), ("uuid", 1, 1))
        self.assertEqual(d["changed"], [{"item": {"folio": 1, "name": "Parts", "uuid": "{tb}"},
                                         "changed": {"x": ["0", "20"], "rows_shown": ["10", "12"]}}])

    def test_terminal_strips(self):
        def strip(name, n):
            terms = "".join(f'<real_terminal/>' for _ in range(n))
            return ('<terminal_strips><terminal_strip><terminal_strip_data uuid="{st}">'
                    '<informations><information name="installation">I</information>'
                    '<information name="location">L</information>'
                    f'<information name="name">{name}</information></informations>'
                    f'</terminal_strip_data>{terms}</terminal_strip></terminal_strips>')
        d = m.tool_diff(self.qet(self.folio() + strip("X1", 2)), self.qet(self.folio() + strip("X1", 3)))
        self.assertEqual(d["terminal_strips"]["changed"],
                         [{"item": "I L X1", "changed": {"terminals": [2, 3]}}])


class Diff(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def project(self, name, *, texts=(), shapes=(), images=(), author="", version="1",
                strips=(), conductors=()):
        # An optional last member of each tuple is the item's uuid.
        uid = lambda u: f' uuid="{u[0]}"' if u else ""
        inputs = "".join(f'<input x="{x}" y="{y}" rotation="0" font="f"{uid(u)} '
                         f'text="&lt;html&gt;&lt;body&gt;&lt;p&gt;{t}&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;"/>'
                         for x, y, t, *u in texts)
        shp = "".join(f'<shape type="Rectangle" x1="{a}" y1="{b}" x2="{c}" y2="{d}"{uid(u)}>'
                      f'<pen color="{pc}" style="SolidLine" widthF="1"/>'
                      f'<brush color="#fff" style="SolidPattern"/></shape>'
                      for a, b, c, d, pc, *u in shapes)
        img = "".join(f'<image x="{x}" y="{y}" size="{s}" rotation="0"{uid(u)}/>'
                      for x, y, s, *u in images)
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

    def test_a_symbols_own_texts_are_not_free_texts(self):
        """Symbols in older files carry their own <inputs><input> texts;
        counting them made schema_indus.qet show 124 free texts where
        QElectroTech has 2."""
        d = self.dir
        a, b = d / "sym_a.qet", d / "sym_b.qet"
        body = ('<project><diagram><elements><element uuid="{e}" type="x">'
                '<inputs><input x="0" y="0" text="{t}"/></inputs></element></elements>'
                '<conductors/><inputs><input uuid="{f}" x="1" y="1" text="free"/></inputs>'
                '</diagram></project>')
        a.write_text(body.replace("{t}", "inside"))
        b.write_text(body.replace("{t}", "changed inside"))
        t = m.tool_diff(str(a), str(b))["texts"]
        self.assertEqual((t["before"], t["after"], t["added"], t["removed"], t["changed"]),
                         (1, 1, [], [], []))

    def test_edited_text_reads_as_removed_plus_added(self):
        a = self.project("a.qet", texts=[(1, 2, "note")])
        b = self.project("b.qet", texts=[(1, 2, "note EDITED")])
        d = m.tool_diff(a, b)["texts"]
        self.assertEqual([t["text"] for t in d["removed"]], ["note"])
        self.assertEqual([t["text"] for t in d["added"]], ["note EDITED"])

    U1 = "{11111111-1111-4111-8111-111111111111}"
    U2 = "{22222222-2222-4222-8222-222222222222}"

    def test_edited_text_with_a_uuid_is_a_change_to_that_text(self):
        a = self.project("a.qet", texts=[(1, 2, "note", self.U1)])
        b = self.project("b.qet", texts=[(1, 2, "note EDITED", self.U1)])
        d = m.tool_diff(a, b)["texts"]
        self.assertEqual(d["keyed_by"], "uuid")
        self.assertEqual((d["added"], d["removed"]), ([], []))
        self.assertEqual(d["changed"][0]["changed"]["text"], ["note", "note EDITED"])

    def test_moved_shape_with_a_uuid_is_a_change_to_that_shape(self):
        a = self.project("a.qet", shapes=[(0, 0, 5, 5, "#000000", self.U1),
                                          (9, 9, 12, 12, "#000000", self.U2)])
        b = self.project("b.qet", shapes=[(100, 0, 105, 5, "#000000", self.U1),
                                          (9, 9, 12, 12, "#000000", self.U2)])
        d = m.tool_diff(a, b)["shapes"]
        self.assertEqual((d["added"], d["removed"]), ([], []))
        self.assertEqual(len(d["changed"]), 1)
        self.assertEqual(d["changed"][0]["item"]["uuid"], self.U1)
        self.assertEqual(d["changed"][0]["changed"]["from"], [["0", "0"], ["100", "0"]])

    def test_a_file_without_uuids_against_its_resave_is_keyed_by_position(self):
        """The first save by a current QElectroTech adds uuids to a legacy
        file: that must not read as every item removed and re-added."""
        a = self.project("a.qet", texts=[(1, 2, "note")], images=[(3, 3, 1)])
        b = self.project("b.qet", texts=[(1, 2, "note", self.U1)], images=[(3, 3, 1, self.U2)])
        d = m.tool_diff(a, b)
        for k in ("texts", "images"):
            with self.subTest(section=k):
                self.assertEqual(d[k]["keyed_by"], "position")
                self.assertFalse(d[k]["added"] or d[k]["removed"] or d[k]["changed"])

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

    def test_empty_information_and_a_missing_one_are_the_same(self):
        """QElectroTech writes an empty label as <elementInformation name="label"/>
        and drops it on the next save, so comparing the raw bags made every
        unlabelled element of a re-saved project look edited."""
        def proj(infos):
            return (f'<project><diagram><elements><element uuid="{{e}}" type="x" x="0" y="0">'
                    f'<elementInformations>{infos}</elementInformations>'
                    f'</element></elements><conductors/></diagram></project>')
        a = self.dir / "a.qet"
        b = self.dir / "b.qet"
        c = self.dir / "c.qet"
        a.write_text(proj('<elementInformation show="1" name="label"></elementInformation>'
                          '<elementInformation show="1" name="comment">  </elementInformation>'))
        b.write_text(proj(''))
        c.write_text(proj('<elementInformation show="1" name="label">K1</elementInformation>'))
        self.assertEqual(m.tool_diff(str(a), str(b))["elements"]["info_changed"], [])
        # A value that really appears or disappears is still reported.
        changed = m.tool_diff(str(b), str(c))["elements"]["info_changed"]
        self.assertEqual([(x["from"], x["to"]) for x in changed], [({}, {"label": "K1"})])

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


class PathPolicy(unittest.TestCase):
    """The workspace confinement on model-supplied paths.

    Both AI security reviews on PR #980 flagged unrestricted filesystem
    reach as the headline MCP-side risk, one of them as the single blocker
    before merge. These check the control itself rather than the tools
    behind it, so they stay fast and hermetic.
    """

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name) / "workspace"
        self.root.mkdir()
        self.outside = Path(self.tmp.name) / "outside"
        self.outside.mkdir()
        (self.outside / "secret.qet").write_text("<project/>")
        (self.root / "ok.qet").write_text("<project/>")
        self._saved = dict(os.environ)
        os.environ["QET_MCP_WORKSPACE"] = str(self.root)
        os.environ.pop("QET_MCP_ALLOW_ANY_PATH", None)
        self.qet = fake_qet(Path(self.tmp.name) / "bin")
        os.environ["QET_BINARY"] = str(self.qet)

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._saved)
        self.tmp.cleanup()

    def test_read_inside_the_workspace_is_allowed(self):
        m.enforce_path_policy("qet_project_info", {"path": str(self.root / "ok.qet")})

    def test_read_outside_the_workspace_is_refused(self):
        with self.assertRaisesRegex(ValueError, "outside the workspace"):
            m.enforce_path_policy("qet_project_info",
                                  {"path": str(self.outside / "secret.qet")})

    def test_traversal_out_of_the_workspace_is_refused(self):
        sneaky = str(self.root / ".." / "outside" / "secret.qet")
        with self.assertRaisesRegex(ValueError, "outside the workspace"):
            m.enforce_path_policy("qet_project_info", {"path": sneaky})

    def test_symlink_escape_is_refused(self):
        """A link planted inside the workspace is judged by where it points.

        This is the case a string-prefix check gets wrong, which is why the
        policy resolves before comparing.
        """
        link = self.root / "innocent.qet"
        link.symlink_to(self.outside / "secret.qet")
        with self.assertRaisesRegex(ValueError, "outside the workspace"):
            m.enforce_path_policy("qet_project_info", {"path": str(link)})

    def test_write_outside_the_workspace_is_refused(self):
        with self.assertRaisesRegex(ValueError, "outside the workspace"):
            m.enforce_path_policy("qet_export", {
                "binary": str(self.qet),
                "project": str(self.root / "ok.qet"),
                "format": "pdf",
                "output": str(self.outside / "exfiltrated.pdf")})

    def test_existing_output_is_not_clobbered_without_overwrite(self):
        target = self.root / "existing.qet"
        target.write_text("precious")
        args = {"binary": str(self.qet), "output": str(target), "title": "T"}
        with self.assertRaisesRegex(ValueError, "already exists"):
            m.enforce_path_policy("qet_project_new", args)
        # ... and goes through once the caller says so explicitly
        m.enforce_path_policy("qet_project_new", dict(args, overwrite=True))
        self.assertEqual(target.read_text(), "precious", "policy must not itself write")

    def test_new_output_needs_no_overwrite_flag(self):
        m.enforce_path_policy("qet_project_new", {
            "binary": str(self.qet),
            "output": str(self.root / "brand_new.qet"), "title": "T"})

    def test_operation_level_file_paths_are_checked(self):
        """add_image/add_pdf_page carry their own path, one level down."""
        base = {"binary": str(self.qet),
                "project": str(self.root / "ok.qet"),
                "output": str(self.root / "out.qet")}
        outside_png = str(self.outside / "anything.png")
        for op in ({"op": "add_image", "folio": 0, "file": outside_png, "x": 0, "y": 0},
                   {"op": "add_pdf_page", "folio": 0, "file": outside_png,
                    "page": 1, "dpi": 150, "x": 0, "y": 0}):
            with self.subTest(op=op["op"]):
                with self.assertRaisesRegex(ValueError, "outside the workspace"):
                    m.enforce_path_policy("qet_edit", dict(base, operations=[op]))

    def test_several_roots_may_be_allowed(self):
        os.environ["QET_MCP_WORKSPACE"] = os.pathsep.join(
            [str(self.root), str(self.outside)])
        m.enforce_path_policy("qet_project_info", {"path": str(self.outside / "secret.qet")})

    def test_escape_hatch_disables_confinement(self):
        os.environ["QET_MCP_ALLOW_ANY_PATH"] = "1"
        self.assertEqual(m.workspace_roots(), [])
        m.enforce_path_policy("qet_project_info", {"path": "/etc/passwd"})

    def test_defaults_to_the_working_directory_not_to_everything(self):
        os.environ.pop("QET_MCP_WORKSPACE", None)
        roots = m.workspace_roots()
        self.assertEqual(roots, [Path(os.getcwd()).resolve()],
                         "an unset workspace must still confine, not open up")

    def test_tools_without_data_paths_are_untouched(self):
        m.enforce_path_policy("qet_diff", {})

    def test_every_write_tool_offers_the_overwrite_flag(self):
        """The policy and the schemas must not drift apart.

        A tool whose output the policy guards but whose schema never
        mentions "overwrite" is a tool no client can ever replace a file
        with; a tool that advertises the flag but is not in the policy is a
        tool that silently clobbers. Both are one forgotten line away, so
        the two lists are compared rather than trusted.
        """
        guarded = {name for name, spec in m._DATA_PATHS.items() if spec.get("write")}
        advertised = {t["name"] for t in m.TOOLS
                      if "overwrite" in t["inputSchema"].get("properties", {})}
        self.assertEqual(guarded, advertised - m._OVERWRITE_OWN_FILE)

    def test_every_data_path_argument_is_guarded(self):
        """The other direction: a tool whose schema takes a data path must be
        in the policy, or that path is read or written with no workspace
        check at all -- and nothing fails. binary and elements_dir have
        their own rule (BinaryPolicy)."""
        pathish = {"path", "project", "before", "after", "output", "directory"}
        for t in m.TOOLS:
            with self.subTest(tool=t["name"]):
                props = t["inputSchema"].get("properties", {})
                spec = m._DATA_PATHS.get(t["name"], {})
                guarded = set(spec.get("read", ())) | set(spec.get("write", ()))
                self.assertEqual(pathish & set(props), guarded & pathish,
                                 f"{t['name']}: data path arguments not in _DATA_PATHS")

    def test_the_policy_names_only_real_tools_and_arguments(self):
        by_name = {t["name"]: t for t in m.TOOLS}
        for name, spec in m._DATA_PATHS.items():
            with self.subTest(tool=name):
                self.assertIn(name, by_name, "policy guards a tool that does not exist")
                props = by_name[name]["inputSchema"].get("properties", {})
                for arg in tuple(spec.get("read", ())) + tuple(spec.get("write", ())):
                    self.assertIn(arg, props,
                                  f"{name} has no {arg!r} argument to guard")


class BinaryPolicy(unittest.TestCase):
    """The program the server launches is not the client's to choose (F063).

    "binary" used to be exempt from every check as "configuration", but it
    is a per-call argument: any executable file it named was run, with the
    client's own paths as arguments.
    """

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.root = base / "workspace"
        self.root.mkdir()
        (self.root / "ok.qet").write_text("<project/>")
        self.qet = fake_qet(base / "bin")
        self.other = fake_qet(base / "other")
        self._saved = dict(os.environ)
        os.environ["QET_MCP_WORKSPACE"] = str(self.root)
        os.environ["QET_BINARY"] = str(self.qet)
        # A PATH with no qelectrotech on it, so only what a test sets counts.
        os.environ["PATH"] = str(base / "empty")
        for var in ("QET_MCP_ALLOW_ANY_PATH", "QET_MCP_ALLOW_ANY_BINARY",
                    "QET_MCP_BINARIES", "QET_MCP_ELEMENTS"):
            os.environ.pop(var, None)

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._saved)
        self.tmp.cleanup()

    def call(self, **extra):
        args = dict({"project": str(self.root / "ok.qet")}, **extra)
        m.enforce_path_policy("qet_query", args)
        return args

    def test_left_out_it_is_filled_in(self):
        self.assertEqual(self.call()["binary"], str(self.qet.resolve()))

    def test_the_configured_one_is_accepted(self):
        self.assertEqual(self.call(binary=str(self.qet))["binary"], str(self.qet.resolve()))

    def test_any_other_program_is_refused(self):
        with self.assertRaisesRegex(ValueError, "not an allowed QElectroTech"):
            self.call(binary=str(self.other))

    def test_a_program_inside_the_workspace_is_refused_too(self):
        """Being inside the workspace makes a file readable, not runnable."""
        planted = fake_qet(self.root, "run.sh")
        with self.assertRaisesRegex(ValueError, "not an allowed QElectroTech"):
            self.call(binary=str(planted))

    def test_a_symlink_is_judged_by_its_target(self):
        link = self.root / "qelectrotech"
        link.symlink_to(self.other)
        with self.assertRaisesRegex(ValueError, "not an allowed QElectroTech"):
            self.call(binary=str(link))
        good = self.root / "also-qet"
        good.symlink_to(self.qet)
        self.call(binary=str(good))

    def test_others_can_be_listed(self):
        os.environ["QET_MCP_BINARIES"] = str(self.other)
        self.assertEqual(self.call(binary=str(self.other))["binary"], str(self.other.resolve()))

    def test_the_check_can_be_switched_off(self):
        os.environ["QET_MCP_ALLOW_ANY_BINARY"] = "1"
        self.assertEqual(self.call(binary=str(self.other))["binary"], str(self.other))

    def test_found_on_path(self):
        del os.environ["QET_BINARY"]
        os.environ["PATH"] = str(self.qet.parent)
        self.assertEqual(self.call()["binary"], str(self.qet.resolve()))

    def test_none_found_says_what_to_set(self):
        del os.environ["QET_BINARY"]
        with self.assertRaisesRegex(ValueError, "set QET_BINARY"):
            self.call()

    def test_every_tool_that_takes_binary_is_checked(self):
        """A tool whose schema offers "binary" but that the policy skips
        would run whatever it was given."""
        takes = {t["name"] for t in m.TOOLS
                 if "binary" in t["inputSchema"].get("properties", {})}
        self.assertEqual(takes, m._LAUNCHES_QET | set(m._LAUNCHES_QET_WITH))
        for name in takes:
            self.assertNotIn("binary", m._BY_NAME[name]["inputSchema"].get("required", []),
                             f"{name} still requires the client to name a binary")

    def test_elements_dir_outside_the_workspace_is_refused(self):
        with self.assertRaisesRegex(ValueError, "'elements_dir' is outside"):
            self.call(elements_dir=self.tmp.name)

    def test_elements_dir_can_be_listed(self):
        coll = Path(self.tmp.name) / "collection"
        coll.mkdir()
        os.environ["QET_MCP_ELEMENTS"] = str(coll)
        self.call(elements_dir=str(coll / "10_electric"))

    def test_an_install_finds_its_own_qet_and_collection(self):
        """Installed as <prefix>/share/qelectrotech/mcp/qet_mcp.py, the
        server needs no configuration at all."""
        import importlib.util
        prefix = Path(self.tmp.name) / "prefix"
        mcp = prefix / "share" / "qelectrotech" / "mcp"
        mcp.mkdir(parents=True)
        (prefix / "share" / "qelectrotech" / "elements").mkdir()
        shutil.copy2(HERE / "qet_mcp.py", mcp / "qet_mcp.py")
        exe = fake_qet(prefix / "bin")
        del os.environ["QET_BINARY"]
        spec = importlib.util.spec_from_file_location("qet_mcp_installed", mcp / "qet_mcp.py")
        inst = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(inst)
        args = {"project": str(self.root / "ok.qet")}
        inst.enforce_path_policy("qet_query", args)
        self.assertEqual(args["binary"], str(exe.resolve()))
        self.assertEqual(args["elements_dir"],
                         str((prefix / "share" / "qelectrotech" / "elements").resolve()))

    def installed_at(self, root: Path, mcp: Path):
        """Import a copy of the server placed at mcp/qet_mcp.py."""
        import importlib.util
        mcp.mkdir(parents=True)
        shutil.copy2(HERE / "qet_mcp.py", mcp / "qet_mcp.py")
        spec = importlib.util.spec_from_file_location(f"qet_mcp_{root.name}", mcp / "qet_mcp.py")
        inst = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(inst)
        return inst

    def test_the_windows_layout_is_found_too(self):
        """The Windows installers and the portable folder put the server in
        <root>/mcp, the program in <root>/bin as QElectroTech.exe, and the
        collection in <root>/elements."""
        root = Path(self.tmp.name) / "QElectroTech"
        (root / "elements").mkdir(parents=True)
        exe = fake_qet(root / "bin", "QElectroTech.exe")
        inst = self.installed_at(root, root / "mcp")
        del os.environ["QET_BINARY"]
        args = {"project": str(self.root / "ok.qet")}
        inst.enforce_path_policy("qet_query", args)
        self.assertEqual(args["binary"], str(exe.resolve()))
        self.assertEqual(args["elements_dir"], str((root / "elements").resolve()))

    def test_a_copy_saved_anywhere_is_not_an_install(self):
        """A downloaded copy sitting beside some bin/ folder must not go
        looking for QElectroTech there."""
        home = Path(self.tmp.name) / "home"
        fake_qet(home / "bin")
        inst = self.installed_at(home, home / "Downloads")
        self.assertIsNone(inst._installation())

    def test_a_folder_named_mcp_is_not_an_install(self):
        """Without a bin/ beside it, an mcp/ folder is just a folder."""
        inst = self.installed_at(Path(self.tmp.name) / "x", Path(self.tmp.name) / "x" / "mcp")
        self.assertIsNone(inst._installation())


class LaunchExecutable(unittest.TestCase):
    """Windows cannot run a lone copy of QElectroTech (F065): its DLLs sit
    beside the original. Everywhere else the private copy stays."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.src = base / "bin" / "qelectrotech"
        self.src.parent.mkdir()
        self.src.write_text("#!/bin/sh\nexit 0\n")
        self.sandbox = base / "sandbox"
        self.sandbox.mkdir()

    def tearDown(self):
        self.tmp.cleanup()

    def test_windows_runs_the_original(self):
        self.assertEqual(m._launch_executable(self.src, self.sandbox, True), self.src)
        self.assertEqual(list(self.sandbox.iterdir()), [], "nothing is copied on Windows")

    def test_windows_keeps_its_own_qt_platform(self):
        """The Windows packages have no offscreen plugin; asking for it
        leaves QElectroTech stuck at a message box."""
        env = m._launch_env({"PATH": "x"}, self.sandbox, True)
        self.assertNotIn("QT_QPA_PLATFORM", env)
        self.assertEqual(env["PATH"], "x")

    def test_elsewhere_runs_offscreen(self):
        env = m._launch_env({}, self.sandbox, False)
        self.assertEqual(env["QT_QPA_PLATFORM"], "offscreen")
        self.assertEqual(env["HOME"], str(self.sandbox))

    def test_settings_stay_in_the_sandbox_everywhere(self):
        """HOME and XDG move the settings only on Linux; QET_SETTINGS_DIR
        moves them on Windows and macOS too (#1178)."""
        for windows in (True, False):
            env = m._launch_env({}, self.sandbox, windows)
            self.assertEqual(env["QET_SETTINGS_DIR"], str(self.sandbox / ".config"))

    def test_elsewhere_runs_a_private_copy(self):
        exe = m._launch_executable(self.src, self.sandbox, False)
        self.assertEqual(exe.parent, self.sandbox)
        self.assertNotEqual(exe, self.src)
        self.assertEqual(exe.read_text(), self.src.read_text())


class ElementsDirSetting(unittest.TestCase):
    """elements_dir reaches QElectroTech as a settings file in the sandbox
    (#1178): the .ini that a QElectroTech knowing QET_SETTINGS_DIR reads on
    every system, and the .conf an older one reads on Linux."""

    def test_both_files_carry_the_collection(self):
        seen = {}

        def run(argv, **kwargs):
            cfg = Path(kwargs["env"]["QET_SETTINGS_DIR"]) / "QElectroTech"
            for f in cfg.iterdir():
                seen[f.name] = f.read_text(encoding="utf-8")
            return subprocess.CompletedProcess(argv, 0, "", "")

        with tempfile.TemporaryDirectory() as coll, \
                mock.patch.object(m.subprocess, "run", run):
            m._run_qet("/bin/true", ["x.qet"], elements_dir=coll)
        want = m._collection_setting(Path(coll))
        self.assertEqual(seen, {"QElectroTech.ini": want, "QElectroTech.conf": want})
        self.assertIn(f"common-collection-path={Path(coll).as_posix()}", want)

    def test_a_windows_path_has_forward_slashes(self):
        """Qt reads a backslash in the file as an escape."""
        from pathlib import PureWindowsPath
        text = m._collection_setting(PureWindowsPath(r"C:\x\custom"))
        self.assertIn("common-collection-path=C:/x/custom\n", text)
        self.assertNotIn("\\", text)


class ScriptingDisabledHint(unittest.TestCase):
    """QElectroTech may refuse to run scripts at all, and says so in French.

    Scripting is off by default from qelectrotech-source-mirror#984 on, so
    every tool here that drives QElectroTech through --run comes back empty
    until somebody sets QET_ENABLE_SCRIPTING=1 in the environment this
    server was started in. The refusal has to arrive as an instruction the
    caller can act on, not as exit code 3 and a paragraph of French, and
    not as one of the older guesses ("is it a build with --run support?")
    that happen to fit the same symptom.

    subprocess.run is faked, so these are about the reading of the result
    and cost no launch.
    """

    REFUSAL = ("Les scripts sont désactivés.\n\nPour l'activer : Configurer "
               "QElectroTech > Général > Projets, ou définir la variable "
               "d'environnement QET_ENABLE_SCRIPTING=1 pour une exécution "
               "sans interface (CI, traitement par lot).")

    def fake_run(self, returncode, stderr="", stdout=""):
        def run(argv, **kwargs):
            return subprocess.CompletedProcess(argv, returncode, stdout, stderr)
        return run

    def call(self, returncode, stderr="", **kw):
        saved = m.subprocess.run
        m.subprocess.run = self.fake_run(returncode, stderr)
        try:
            # /bin/true only has to exist and be executable: it is copied
            # into the sandbox and then never actually launched.
            return m._run_qet("/bin/true", ["x.qet"], **kw)
        finally:
            m.subprocess.run = saved

    def test_a_refused_script_says_which_variable_to_set(self):
        r = self.call(3, self.REFUSAL, script="qet.log('hi')")
        self.assertFalse(r["ok"])
        self.assertIn("QET_ENABLE_SCRIPTING=1", r["hint"])
        self.assertIn("env", r["hint"])

    def test_the_hint_survives_a_reworded_refusal(self):
        # Keyed on the variable name first, exit 3 as the fallback, so a
        # future build that words this differently still gets read right.
        r = self.call(3, "scripting is disabled", script="qet.log('hi')")
        self.assertIn("QET_ENABLE_SCRIPTING=1", r["hint"])

    def test_a_launch_with_no_script_is_not_blamed_on_scripting(self):
        # qet_export uses a plain CLI flag and keeps working with scripting
        # off, so its failures must never be explained this way.
        r = self.call(3, "Project not found: x.qet")
        self.assertNotIn("hint", r)

    def test_a_successful_script_gets_no_hint(self):
        r = self.call(0, "", script="qet.log('hi')")
        self.assertTrue(r["ok"])
        self.assertNotIn("hint", r)

    def test_the_specific_hint_is_not_overwritten_by_a_generic_one(self):
        """The two older guesses fit the same symptom and must yield to it.

        Scripting being off produces no capability report either, so
        tool_edit() and tool_project_new() would otherwise answer "is it a
        build with --run support?" -- sending the reader to check the one
        thing that is fine.
        """
        source = Path(m.__file__).read_text()
        for marker in ("the binary never ran the script",
                       "QElectroTech did not write the project"):
            with self.subTest(hint=marker[:30]):
                idx = source.index(marker)
                before = source[max(0, idx - 400):idx]
                self.assertIn('setdefault("hint"', before,
                              "this hint would clobber a more specific one")

    def test_every_script_driven_tool_is_named_in_the_hint(self):
        """The hint lists which tools need the variable; keep it true.

        A tool that starts driving QElectroTech through a script and is
        missing from that list leaves its caller reading a message that
        says the problem is somewhere else.
        """
        source = Path(m.__file__).read_text()
        # Tool handlers that pass script= to _run_qet.
        driven = set(re.findall(r"^def (tool_\w+)", source, re.M))
        script_driven = set()
        for name in driven:
            body = source[source.index(f"def {name}("):]
            body = body[:body.find("\ndef ") if "\ndef " in body else len(body)]
            if "script=script" in body or "script=\"\\n\".join(script)" in body:
                script_driven.add("qet_" + name[len("tool_"):])
        self.assertTrue(script_driven, "found no script-driven tools to check")
        hint = source[source.index("QET_ENABLE_SCRIPTING=1 to the"):][:900]
        for tool in sorted(script_driven):
            with self.subTest(tool=tool):
                self.assertIn(tool, hint)


NOTE_SCRIPT = (
    "// ==QETScript==\n"
    "// @name     Add a note\n"
    "// @tooltip  Puts a note on the folio on screen\n"
    "// @shortcut Ctrl+Alt+N\n"
    "// ==/QETScript==\n"
    "qet.addText(qet.currentFolio(), 'SCRIPT-NOTE', 40, 40);\n"
    "qet.log('added');\n")
ICON = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><rect width="16" height="16"/></svg>'


class ScriptHeader(unittest.TestCase):
    """The same cases as QElectroTech's tst_scriptheader: a header this
    accepts and QElectroTech refuses would install a script that never gets
    a button."""

    def h(self, lines):
        return m.parse_script_header(
            "// ==QETScript==\n" + lines + "// ==/QETScript==\nqet.log('x');\n", "a")

    def test_full(self):
        h = self.h("// @name     Add revision note\n// @icon     note.svg\n"
                   "// @tooltip  Puts a note\n// @shortcut Ctrl+Alt+R\n"
                   "// @context  selection\n// @api      1\n")
        self.assertNotIn("error", h)
        self.assertEqual((h["name"], h["icon"], h["shortcut"], h["context"]),
                         ("Add revision note", "note.svg", "Ctrl+Alt+R", "selection"))
        self.assertEqual(h["action_id"], "diagrameditor.script.a")

    def test_refused(self):
        cases = {
            "qet.log('x');\n": "no // ==QETScript== header",
        }
        for text, error in cases.items():
            self.assertEqual(m.parse_script_header(text, "a")["error"], error)
        for lines, error in [
                ("// @icon x.svg\n", "@name is required"),
                ("// @name A\n// @shortcutt Ctrl+K\n", "unknown header key @shortcutt"),
                ("// @name A\n// @context wires\n",
                 "@context must be one of: canvas, selection, conductor"),
                ("// @name A\n// @api 2\n",
                 "@api 2 is not supported by this version (1 is)")]:
            with self.subTest(lines=lines):
                self.assertEqual(self.h(lines)["error"], error)

    def test_only_the_block(self):
        h = m.parse_script_header(
            "// ==QETScript==\n// @name Real\n// ==/QETScript==\n"
            "// @name Not this one\n// @bogus key\n", "a")
        self.assertNotIn("error", h)
        self.assertEqual(h["name"], "Real")


class ScriptStore(unittest.TestCase):
    """install / list / read / remove against a scripts folder of its own."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name) / "scripts"
        self.env = mock.patch.dict(os.environ, {"QET_MCP_SCRIPTS_DIR": str(self.dir),
                                                "QET_ENABLE_SCRIPTING": "1"})
        self.env.start()

    def tearDown(self):
        self.env.stop()
        self.tmp.cleanup()

    def test_round_trip(self):
        r = m.tool_script_install("add-note", NOTE_SCRIPT)
        self.assertTrue(r["ok"])
        self.assertEqual((self.dir / "add-note.js").read_text(), NOTE_SCRIPT)
        listed = m.tool_script_list()
        self.assertEqual([s["id"] for s in listed["scripts"]], ["add-note"])
        self.assertEqual(listed["scripts"][0]["shortcut"], "Ctrl+Alt+N")
        self.assertEqual(m.tool_script_read("add-note")["source"], NOTE_SCRIPT)
        m.tool_script_remove("add-note")
        self.assertFalse((self.dir / "add-note.js").exists())
        self.assertEqual(m.tool_script_list()["scripts"], [])

    def test_icon_stored_and_removed_with_it(self):
        src = NOTE_SCRIPT.replace("// @tooltip", "// @icon     iconic.svg\n// @tooltip")
        m.tool_script_install("iconic", src, icon_svg=ICON)
        self.assertEqual((self.dir / "iconic.svg").read_text(), ICON)
        self.assertEqual(m.tool_script_read("iconic")["icon_svg"], ICON)
        m.tool_script_remove("iconic")
        self.assertEqual(list(self.dir.iterdir()), [])

    def test_shared_icon_kept_while_used(self):
        src = NOTE_SCRIPT.replace("// @tooltip", "// @icon     one.svg\n// @tooltip")
        m.tool_script_install("one", src, icon_svg=ICON)
        m.tool_script_install("two", src)            # names one.svg, which exists
        m.tool_script_remove("one")
        self.assertTrue((self.dir / "one.svg").exists())
        m.tool_script_remove("two")
        self.assertFalse((self.dir / "one.svg").exists())

    def test_refusals(self):
        for sid in ("../evil", "Upper", "", "a/b", "-x", "x" * 65, None):
            with self.subTest(id=sid), self.assertRaisesRegex(ValueError, "'id' must be"):
                m.tool_script_install(sid, NOTE_SCRIPT)
        with self.assertRaisesRegex(ValueError, "would refuse this header: @name"):
            m.tool_script_install("a", "// ==QETScript==\n// ==/QETScript==\n")
        with self.assertRaisesRegex(ValueError, "must say '// @icon a.svg'"):
            m.tool_script_install("a", NOTE_SCRIPT, icon_svg=ICON)
        with self.assertRaisesRegex(ValueError, "not in"):
            m.tool_script_install("a", NOTE_SCRIPT.replace(
                "// @tooltip", "// @icon     missing.svg\n// @tooltip"))
        src = NOTE_SCRIPT.replace("// @tooltip", "// @icon     a.svg\n// @tooltip")
        for bad in ("<html/>", "<svg", "x" * (65 * 1024)):
            with self.subTest(svg=bad[:10]), self.assertRaises(ValueError):
                m.tool_script_install("a", src, icon_svg=bad)
        self.assertFalse(self.dir.exists(), "a refused install wrote something")

    def test_overwrite_needs_asking(self):
        m.tool_script_install("a", NOTE_SCRIPT)
        with self.assertRaisesRegex(ValueError, "already stored"):
            m.tool_script_install("a", NOTE_SCRIPT.replace("Add a note", "Changed"))
        m.tool_script_install("a", NOTE_SCRIPT.replace("Add a note", "Changed"),
                              overwrite=True)
        self.assertEqual(m.tool_script_list()["scripts"][0]["name"], "Changed")

    def test_needs_consent(self):
        """Installing stores code the user will run by clicking: the same
        switch as editing a project, or nothing is written."""
        with mock.patch.dict(os.environ, {"QET_ENABLE_SCRIPTING": ""}):
            with self.assertRaisesRegex(ValueError, "QET_ENABLE_SCRIPTING=1"):
                m.tool_script_install("a", NOTE_SCRIPT)
            m.tool_script_list()                      # reading needs no consent
        m.tool_script_install("a", NOTE_SCRIPT)
        with mock.patch.dict(os.environ, {"QET_ENABLE_SCRIPTING": ""}):
            with self.assertRaisesRegex(ValueError, "QET_ENABLE_SCRIPTING=1"):
                m.tool_script_remove("a")
        self.assertTrue((self.dir / "a.js").exists())

    def test_list_reports_refused_files(self):
        self.dir.mkdir(parents=True)
        (self.dir / "bad.js").write_text("qet.log(1);\n")
        r = m.tool_script_list()
        self.assertEqual(r["refused"], [{"file": "bad.js",
                                         "error": "no // ==QETScript== header"}])

    def test_default_folder_is_qets_data_dir(self):
        """QStandardPaths::AppDataLocation plus the organisation and
        application names main.cpp sets, on each platform."""
        from pathlib import PurePosixPath, PureWindowsPath
        f = m.default_scripts_dir
        self.assertEqual(f("posix", "linux", {"XDG_DATA_HOME": "/x/share"}, "/home/u"),
                         PurePosixPath("/x/share/QElectroTech/QElectroTech/scripts"))
        self.assertEqual(f("posix", "linux", {}, "/home/u"),
                         PurePosixPath("/home/u/.local/share/QElectroTech/QElectroTech/scripts"))
        self.assertEqual(f("nt", "win32", {"APPDATA": r"C:\Users\u\AppData\Roaming"}, r"C:\Users\u"),
                         PureWindowsPath(r"C:\Users\u\AppData\Roaming\QElectroTech\QElectroTech\scripts"))
        self.assertEqual(f("nt", "win32", {}, r"C:\Users\u"),
                         PureWindowsPath(r"C:\Users\u\AppData\Roaming\QElectroTech\QElectroTech\scripts"))
        self.assertEqual(f("posix", "darwin", {}, "/Users/u"),
                         PurePosixPath("/Users/u/Library/Application Support/QElectroTech/QElectroTech/scripts"))
        with mock.patch.dict(os.environ, {"QET_MCP_SCRIPTS_DIR": "/elsewhere"}):
            self.assertEqual(m.scripts_dir(), Path("/elsewhere"))

    def test_install_does_not_need_a_binary_without_a_test(self):
        """Only a test run launches QElectroTech; storing a file must not be
        refused because no QElectroTech is configured."""
        with mock.patch.dict(os.environ, {"QET_BINARY": ""}), \
                mock.patch.object(m, "resolve_binary", return_value=None):
            args = {"id": "a", "source": NOTE_SCRIPT}
            m.enforce_path_policy("qet_script_install", args)
            with self.assertRaisesRegex(ValueError, "no QElectroTech found"):
                m.enforce_path_policy("qet_script_install",
                                      {"id": "a", "source": NOTE_SCRIPT,
                                       "test_project": self.tmp.name + "/p.qet"})


@needs_examples
class ScriptToolsIntegration(unittest.TestCase):
    """Through a real QElectroTech with stored scripts (script buttons)."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name) / "scripts"
        self.env = mock.patch.dict(os.environ, {"QET_MCP_SCRIPTS_DIR": str(self.dir),
                                                "QET_ENABLE_SCRIPTING": "1"})
        self.env.start()
        self.project = Path(self.tmp.name) / "perceuse.qet"
        shutil.copy2(Path(EXAMPLES) / "perceuse.qet", self.project)

    def tearDown(self):
        self.env.stop()
        self.tmp.cleanup()

    def test_api_lists_the_calls(self):
        r = m.tool_script_api(BINARY)
        self.assertTrue(r["ok"], r)
        self.assertIn("addText", " ".join(r["calls"]))
        self.assertGreater(r["call_count"], 100)

    def test_test_runs_on_a_copy(self):
        before = self.project.read_bytes()
        r = m.tool_script_test(BINARY, str(self.project), NOTE_SCRIPT)
        self.assertTrue(r["ok"], r)
        self.assertEqual([t["text"] for t in r["diff"]["texts"]["added"]], ["SCRIPT-NOTE"])
        self.assertIn("added", "\n".join(r["log"]))
        self.assertEqual(self.project.read_bytes(), before)

    def test_error_reported_with_its_line(self):
        r = m.tool_script_test(BINARY, str(self.project),
                               NOTE_SCRIPT + "qet.noSuchCall();\n")
        self.assertFalse(r["ok"])
        self.assertTrue(r["errors"], r)
        self.assertIn(":8:", r["errors"][0])       # line 8 of the script as given

    def test_failed_test_stores_nothing(self):
        r = m.tool_script_install("broken", NOTE_SCRIPT + "throw new Error('no');\n",
                                  test_project=str(self.project), binary=BINARY)
        self.assertFalse(r["ok"])
        self.assertFalse(self.dir.exists() and any(self.dir.iterdir()))
        r = m.tool_script_install("good", NOTE_SCRIPT,
                                  test_project=str(self.project), binary=BINARY)
        self.assertTrue(r["ok"], r)
        self.assertTrue((self.dir / "good.js").is_file())



@unittest.skipIf(os.name == "nt", "the fake QElectroTech is a Unix socket")
class LiveClient(unittest.TestCase):
    """The qet_live_* client against a stand-in for QElectroTech's
    LiveServer: one JSON line in, one JSON line out, token on every line."""

    def setUp(self):
        import socket, threading
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.scripts = root / "data" / "scripts"
        self.scripts.mkdir(parents=True)
        self.env = mock.patch.dict(os.environ, {"QET_MCP_SCRIPTS_DIR": str(self.scripts),
                                                "QET_ENABLE_SCRIPTING": "1"})
        self.env.start()
        self.sock_path = str(root / "s")
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.bind(self.sock_path)
        self.server.listen(4)
        self.seen = []

        def serve():
            while True:
                try:
                    conn, _ = self.server.accept()
                except OSError:
                    return
                with conn:
                    data = b""
                    while not data.endswith(b"\n"):
                        chunk = conn.recv(4096)
                        if not chunk:
                            break
                        data += chunk
                    req = json.loads(data)
                    self.seen.append(req)
                    ok = req.get("token") == "T0K"
                    conn.sendall((json.dumps({"ok": ok, "id": req.get("id"),
                                              "echo": req.get("cmd")}) + "\n").encode())
        threading.Thread(target=serve, daemon=True).start()

    def tearDown(self):
        self.server.close()
        self.env.stop()
        self.tmp.cleanup()

    def session(self):
        (self.scripts.parent / "live-session.json").write_text(
            json.dumps({"version": 1, "socket": self.sock_path, "token": "T0K"}))

    def test_no_session_says_what_to_switch_on(self):
        with self.assertRaisesRegex(ValueError, "Continuer"):
            m.tool_live_status()

    def test_requests_carry_the_token_and_the_script_id(self):
        self.session()
        self.assertEqual(m.tool_live_status(), {"ok": True, "echo": "status"})
        m.tool_live_run_stored("mark")
        m.tool_live_run_script("qet.log(1);", "Note")
        self.assertEqual([r["token"] for r in self.seen], ["T0K"] * 3)
        # "id" numbers the request; the stored script travels as "script"
        # (they once shared "id", and every stored run asked for script "1")
        self.assertEqual(self.seen[1]["script"], "mark")
        self.assertEqual((self.seen[2]["source"], self.seen[2]["name"]), ("qet.log(1);", "Note"))

    def test_running_needs_consent_status_does_not(self):
        self.session()
        with mock.patch.dict(os.environ, {"QET_ENABLE_SCRIPTING": ""}):
            m.tool_live_status()
            with self.assertRaisesRegex(ValueError, "QET_ENABLE_SCRIPTING=1"):
                m.tool_live_run_script("qet.log(1);")
            with self.assertRaisesRegex(ValueError, "'id' must be"):
                os.environ["QET_ENABLE_SCRIPTING"] = "1"
                m.tool_live_run_stored("../x")
        self.assertEqual(len(self.seen), 1)

    def test_screenshot_is_sent_as_an_image(self):
        """The picture must reach the assistant as an MCP image, not as a
        long base64 string inside the text."""
        self.session()
        with mock.patch.object(m, "_live_call",
                               return_value={"ok": True, "width": 2, "height": 1,
                                             "png_base64": "iVBORw0K"}):
            reply = m.handle({"jsonrpc": "2.0", "id": 9, "method": "tools/call",
                              "params": {"name": "qet_live_screenshot", "arguments": {}}})
        content = reply["result"]["content"]
        self.assertEqual(content[0], {"type": "image", "data": "iVBORw0K",
                                      "mimeType": "image/png"})
        self.assertNotIn("iVBORw0K", content[1]["text"])

    def test_call_once_prints_an_image_as_a_data_uri(self):
        import io
        self.session()
        out = io.StringIO()
        with mock.patch.object(m, "_live_call",
                               return_value={"ok": True, "png_base64": "iVBORw0K"}):
            code = m.call_once(["qet_live_screenshot"], stdout=out)
        self.assertEqual(code, 0)
        self.assertEqual(out.getvalue().splitlines()[0], "data:image/png;base64,iVBORw0K")

    def test_commands_and_folios_are_sent_as_asked(self):
        self.session()
        m.tool_live_command("diagrameditor.zoom_fit")
        m.tool_live_show_folio(2)
        m.tool_live_undo_last()
        self.assertEqual([(r["cmd"], r.get("action"), r.get("folio")) for r in self.seen],
                         [("command", "diagrameditor.zoom_fit", None),
                          ("show_folio", None, 2), ("undo_last", None, None)])
        with self.assertRaises(ValueError):
            m.tool_live_show_folio("2")

    def test_stale_session_file(self):
        (self.scripts.parent / "live-session.json").write_text(
            json.dumps({"socket": self.sock_path + "-gone", "token": "T0K"}))
        with self.assertRaisesRegex(ValueError, "could not reach"):
            m.tool_live_status()


class PathPolicyOverStdio(unittest.TestCase):
    """Proves the policy is actually wired into the dispatcher.

    The checks above call enforce_path_policy() directly; this one goes
    through a real server process, which is the only thing that shows a
    tool call is gated rather than merely gate-able.
    """

    def rpc(self, message, env_extra):
        env = dict(os.environ, **env_extra)
        proc = subprocess.run([sys.executable, str(HERE / "qet_mcp.py")],
                              input=json.dumps(message) + "\n",
                              capture_output=True, text=True, timeout=30, env=env)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        for line in proc.stdout.splitlines():
            if line.strip():
                obj = json.loads(line)
                if obj.get("id") == message.get("id"):
                    return obj
        self.fail("no reply for the request")

    def test_a_tool_call_reaching_outside_the_workspace_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "ws"
            root.mkdir()
            victim = Path(tmp) / "elsewhere.qet"
            victim.write_text('<project title="not yours"><diagram title="D"/></project>')

            reply = self.rpc({"jsonrpc": "2.0", "id": 7, "method": "tools/call",
                              "params": {"name": "qet_project_info",
                                         "arguments": {"path": str(victim)}}},
                             {"QET_MCP_WORKSPACE": str(root)})
            result = reply["result"]
            self.assertTrue(result.get("isError"), result)
            self.assertIn("outside the workspace", result["content"][0]["text"])

    def test_a_tool_call_cannot_choose_the_program_to_run(self):
        """F063's reproduction: a script in the workspace, named as binary."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.qet").write_text('<project title="x"><diagram title="D"/></project>')
            planted = fake_qet(root, "argv.py")
            reply = self.rpc({"jsonrpc": "2.0", "id": 9, "method": "tools/call",
                              "params": {"name": "qet_export",
                                         "arguments": {"binary": str(planted),
                                                       "project": str(root / "a.qet"),
                                                       "format": "pdf",
                                                       "output": str(root / "o.pdf")}}},
                             {"QET_MCP_WORKSPACE": str(root),
                              "QET_BINARY": str(fake_qet(root / "bin"))})
            result = reply["result"]
            self.assertTrue(result.get("isError"), result)
            self.assertIn("not an allowed QElectroTech", result["content"][0]["text"])

    def test_the_same_call_succeeds_inside_the_workspace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "ws"
            root.mkdir()
            proj = root / "mine.qet"
            proj.write_text('<project title="mine"><diagram title="D"/></project>')

            reply = self.rpc({"jsonrpc": "2.0", "id": 8, "method": "tools/call",
                              "params": {"name": "qet_project_info",
                                         "arguments": {"path": str(proj)}}},
                             {"QET_MCP_WORKSPACE": str(root)})
            result = reply["result"]
            self.assertFalse(result.get("isError"), result)
            self.assertIn("mine", result["content"][0]["text"])


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


class ContinuityFolioGuard(unittest.TestCase):
    """qet.checkContinuity() answers a folio index it has no folio for with an
    empty list, which looks like a clean folio; refuse it before launch."""

    def test_an_index_with_no_folio_is_refused_before_launch(self):
        with tempfile.TemporaryDirectory() as d:
            qet = Path(d) / "p.qet"
            qet.write_text("<project><diagram/><diagram/></project>")
            for folio in (2, 5, -1):
                with self.subTest(folio=folio):
                    with self.assertRaisesRegex(ValueError, r"2 folio\(s\), indexed 0 to 1"):
                        m.tool_continuity("/nonexistent", str(qet), folio=folio)

    def test_the_descriptions_say_how_folios_are_counted(self):
        props = {t["name"]: t["inputSchema"]["properties"] for t in m.TOOLS}
        self.assertIn("counted from 0", props["qet_continuity"]["folio"]["description"])
        self.assertIn("counted from 1", props["qet_conductors"]["folio"]["description"])


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

    def test_a_wire_numbering_rule_is_not_a_wire(self):
        """A folio's conductor numbering rule is saved as <autonum><conductor>;
        counting every <conductor> tag reported it as a wire with no ends."""
        with tempfile.TemporaryDirectory() as tmp:
            rule = ('<autonum><conductor><part increase="1" type="unit" value="3"/>'
                    '</conductor></autonum>')
            wire = '<conductors><conductor terminal1="1" terminal2="2" num="W1"/></conductors>'
            a, b = Path(tmp) / "a.qet", Path(tmp) / "b.qet"
            a.write_text(f'<project><diagram>{rule}<elements/>{wire}</diagram></project>')
            b.write_text(f'<project><diagram><elements/>{wire}</diagram></project>')
            self.assertEqual(m.tool_project_info(str(a))["folios"][0]["conductors"], 1)
            self.assertEqual(m.tool_project_info(str(a))["conductor_count"], 1)
            self.assertEqual([c["num"] for c in m.tool_conductors(str(a))["conductors"]], ["W1"])
            # Removing only the rule changes no wire.
            c = m.tool_diff(str(a), str(b))["conductors"]
            self.assertEqual((c["added"], c["removed"]), ([], []))


# ==========================================================================
# protocol
# ==========================================================================

class Protocol(unittest.TestCase):
    def rpc(self, *messages, workspace=None):
        # The server confines data paths to a workspace (see PathPolicy), so
        # a test driving it over real stdio has to declare one, exactly as a
        # real MCP host config does.
        env = dict(os.environ)
        if workspace is not None:
            env["QET_MCP_WORKSPACE"] = str(workspace)
        proc = subprocess.run([sys.executable, str(HERE / "qet_mcp.py")],
                              input="\n".join(json.dumps(x) for x in messages) + "\n",
                              capture_output=True, text=True, timeout=30, env=env)
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
                {"jsonrpc": "2.0", "id": 4, "method": "ping"},
                workspace=tmp)
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

    def call(self, *args, stdin="", workspace=None):
        env = dict(os.environ)
        if workspace is not None:
            env["QET_MCP_WORKSPACE"] = str(workspace)
        return subprocess.run([sys.executable, str(HERE / "qet_mcp.py"), "--call", *args],
                              input=stdin, capture_output=True, text=True,
                              timeout=30, env=env)

    def test_call_flag_runs_one_tool(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "a.qet"
            p.write_text('<project title="Via --call"><diagram title="D"/></project>')
            by_argv = self.call("qet_project_info", json.dumps({"path": str(p)}),
                                workspace=tmp)
            by_stdin = self.call("qet_project_info", "-",
                                 stdin=json.dumps({"path": str(p)}), workspace=tmp)
        for proc in (by_argv, by_stdin):
            self.assertEqual(proc.returncode, 0, proc.stderr)
            self.assertEqual(json.loads(proc.stdout)["title"], "Via --call")

    def test_call_flag_keeps_the_workspace_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "ws"
            root.mkdir()
            victim = Path(tmp) / "elsewhere.qet"
            victim.write_text('<project title="not yours"><diagram title="D"/></project>')
            proc = self.call("qet_project_info", json.dumps({"path": str(victim)}),
                             workspace=root)
        self.assertEqual(proc.returncode, 1)
        self.assertIn("outside the workspace", proc.stdout)
        self.assertNotIn("not yours", proc.stdout)

    def test_call_flag_exit_codes(self):
        tool_error = self.call("qet_project_info", '{"path": "/no/such.qet"}')
        self.assertEqual(tool_error.returncode, 1)
        for args in (("nope",), ("qet_project_info", "{not json"),
                     ("qet_project_info", "[1]"), ()):
            proc = self.call(*args)
            self.assertEqual(proc.returncode, 2, args)
            self.assertEqual(proc.stdout, "", args)


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

    def test_add_conductor_by_terminal_uuid(self):
        """The coil's file lists A2 before A1, and its index order puts A1
        first: wiring by uuid reaches the terminals named, whatever their
        index, and an unknown uuid stops the run with a note."""
        info = m.tool_element_info(str(Path(ELEMENTS) / COIL.removeprefix("common://")))
        by_name = {t["name"]: t for t in info["terminals"]}
        a1, a2 = by_name["A1"], by_name["A2"]
        self.assertTrue(a1["uuid"] and a2["uuid"])
        self.assertEqual((a1["index"], a2["index"]), (0, 1))
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_element", "id": "b", "folio": 0, "path": COIL, "x": 300, "y": 100},
            {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": a2["uuid"],
             "to": "$b", "to_terminal": a1["uuid"]}]))
        self.assertTrue(all(o["succeeded"] for o in r["operations"]))
        wires = [c for c in ET.parse(r["output"]).getroot().iter("conductor")
                 if c.get("terminal1") and c.get("element1")]
        self.assertEqual(len(wires), 1)
        ends = {(wires[0].get("element1"), wires[0].get("terminal1")),
                (wires[0].get("element2"), wires[0].get("terminal2"))}
        a_uuid, b_uuid = (o["result"] for o in r["operations"][:2])
        self.assertEqual(ends, {(a_uuid, a2["uuid"]), (b_uuid, a1["uuid"])})

        r = self.sb.edit(base, [
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_conductor", "folio": 0, "from": "$a",
             "from_terminal": "{00000000-0000-4000-8000-000000000001}",
             "to": "$a", "to_terminal": a1["uuid"]}])
        self.assertFalse(r["ok"])
        self.assertTrue(r["stopped_early"])
        self.assertIn("from_terminal: no terminal", json.dumps(r["operations"][1]))

        # both ends wrong: both reported, each by its argument
        r = self.sb.edit(base, [
            {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
            {"op": "add_conductor", "folio": 0, "from": "$a",
             "from_terminal": "{00000000-0000-4000-8000-000000000001}",
             "to": "$a", "to_terminal": "{00000000-0000-4000-8000-000000000002}"}])
        note = r["operations"][1].get("note", "")
        self.assertIn("from_terminal: no terminal", note)
        self.assertIn("to_terminal: no terminal", note)
    def test_folio_ref_survives_an_insert_before_it(self):
        """add_folio names a folio "$f"; inserting another at position 0
        moves it from index 1 to 2. "$f" must still name it."""
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            {"op": "insert_folio", "position": 0},
            {"op": "set_folio_title", "folio": "$f", "title": "MINE"}]))
        self.assertEqual(r["operations"][0]["result"], 1)
        titles = [f["title"] for f in m.tool_project_info(r["output"])["folios"]]
        self.assertEqual(len(titles), 3)
        self.assertEqual(titles[2], "MINE", titles)
        self.assertNotIn("MINE", titles[:2])

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

    def _hub(self):
        """e0's terminal 0 carries two conductors, to e1 and to e2. Returns
        the saved project and the two conductors' uuids."""
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_folio", "id": "f"},
            *[{"op": "add_element", "id": f"e{i}", "folio": "$f", "path": COIL, "x": 100 + i * 200, "y": 100}
              for i in range(3)],
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e1", "to_terminal": 0},
            {"op": "add_conductor", "folio": "$f", "from": "$e0", "from_terminal": 0, "to": "$e2", "to_terminal": 0}]))
        uuids = [c["uuid"] for c in m.tool_conductors(r["output"])["conductors"]]
        self.assertEqual(len(uuids), 2)
        self.assertTrue(all(uuids), "new conductors carry a saved uuid")
        return r["output"], uuids

    def test_conductor_by_uuid_where_two_meet_at_a_terminal(self):
        """The terminal e0/0 carries both conductors, which element + terminal
        cannot name; each uuid names one, whichever it is."""
        project, uuids = self._hub()
        for gone, kept in ((uuids[0], uuids[1]), (uuids[1], uuids[0])):
            with self.subTest(deleted=gone):
                r = self.ok(self.sb.edit(project, [
                    {"op": "delete_conductor", "folio": 1, "conductor": gone}], out="out2.qet"))
                left = [c["uuid"] for c in m.tool_conductors(r["output"])["conductors"]]
                self.assertEqual(left, [kept])

    def test_unknown_conductor_uuid_is_reported(self):
        project, _ = self._hub()
        r = self.sb.edit(project, [{"op": "delete_conductor", "folio": 1,
                                    "conductor": "{11111111-2222-4333-8444-555555555555}"}],
                         out="out2.qet")
        self.assertFalse(r["ok"])
        self.assertIn("no conductor", r["operations"][0].get("note", ""))

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

    def test_drawing_items_are_in_the_database_and_addressable_by_uuid(self):
        base = self.sb.new()
        r = self.ok(self.sb.edit(base, [
            {"op": "add_text", "folio": 0, "text": "note", "x": 50, "y": 50},
            {"op": "add_shape", "folio": 0, "shape": "rectangle", "x1": 10, "y1": 10, "x2": 200, "y2": 120}]))
        root = ET.parse(r["output"]).getroot()
        shape_uuid = next(root.iter("shape")).get("uuid")
        text_uuid = next(root.iter("input")).get("uuid")
        rows = m.tool_query(BINARY, r["output"],
                           "SELECT uuid, kind, folio, description FROM drawing_item_view "
                           "ORDER BY kind")["rows"]
        self.assertEqual([(x["uuid"], x["kind"], x["folio"]) for x in rows],
                         [(shape_uuid, "shape", 1), (text_uuid, "text", 1)])
        self.assertEqual(rows[1]["description"], "note")

        # A second shape placed above the first shifts its index to 1; the
        # uuid still names it.
        r2 = self.ok(self.sb.edit(r["output"], [
            {"op": "add_shape", "folio": 0, "shape": "line", "x1": 0, "y1": 0, "x2": 5, "y2": 0},
            {"op": "set_shape", "folio": 0, "index": shape_uuid, "property": "fill", "value": "#00ff00"}],
            out="out2.qet"))
        d = r2["diff"]["shapes"]
        self.assertEqual(d["keyed_by"], "uuid")
        self.assertEqual([c["item"]["uuid"] for c in d["changed"]], [shape_uuid])
        self.assertEqual(len(d["added"]), 1)

    def _old_file(self, same_shape_uuid=False):
        """A folio with a text and two shapes, as a version without
        drawing-item uuids wrote it -- or, with same_shape_uuid, hand-edited
        so that both shapes claim the same one."""
        r = self.ok(self.sb.edit(self.sb.new(), [
            {"op": "add_text", "folio": 0, "text": "note", "x": 50, "y": 50},
            {"op": "add_shape", "folio": 0, "shape": "rectangle", "x1": 10, "y1": 10, "x2": 200, "y2": 120},
            {"op": "add_shape", "folio": 0, "shape": "line", "x1": 0, "y1": 0, "x2": 5, "y2": 0}]))
        tree = ET.parse(r["output"])
        shapes = list(tree.getroot().iter("shape"))
        for item in [*tree.getroot().iter("input"), *shapes]:
            del item.attrib["uuid"]
        if same_shape_uuid:
            for shape in shapes:
                shape.set("uuid", SHARED_UUID)
        tree.write(self.sb.p("old.qet"), encoding="utf-8", xml_declaration=True)
        return self.sb.p("old.qet")

    def _db_uuids(self, project):
        rows = m.tool_query(BINARY, project, "SELECT uuid, kind FROM drawing_item_view")["rows"]
        return sorted((x["uuid"], x["kind"]) for x in rows)

    @staticmethod
    def _file_uuids(project):
        root = ET.parse(project).getroot()
        return sorted((item.get("uuid"), kind)
                      for tag, kind in (("shape", "shape"), ("input", "text"))
                      for item in root.iter(tag))

    def test_a_file_without_drawing_uuids_gets_the_same_ones_on_every_open(self):
        old = self._old_file()
        first = self._db_uuids(old)
        self.assertEqual(len(first), 3)
        self.assertEqual(len({u for u, _ in first}), 3, "each item gets its own")
        self.assertEqual(self._db_uuids(old), first)

    def test_the_first_save_writes_those_uuids_and_a_reopen_keeps_them(self):
        old = self._old_file()
        derived = self._db_uuids(old)
        saved = self.ok(self.sb.edit(old, [{"op": "add_folio"}], out="saved.qet"))["output"]
        self.assertEqual(self._file_uuids(saved), derived)
        self.assertEqual(self._db_uuids(saved), derived)
        # Saving the same old file again writes the same uuids (#754).
        again = self.ok(self.sb.edit(old, [{"op": "add_folio"}], out="again.qet"))["output"]
        self.assertEqual(self._file_uuids(again), derived)

    def test_a_uuid_repeated_in_a_hand_edited_file_names_one_item_only(self):
        old = self._old_file(same_shape_uuid=True)
        shapes = [u for u, kind in self._db_uuids(old) if kind == "shape"]
        self.assertEqual(len(set(shapes)), 2)
        self.assertIn(SHARED_UUID, shapes)
        self.assertEqual([u for u, kind in self._db_uuids(old) if kind == "shape"], shapes)

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

    def test_continuity_findings_carry_the_folio_number_qet_elements_uses(self):
        base = self.sb.new(folios=2)
        r = self.ok(self.sb.edit(base, [
            {"op": "add_element", "id": "a", "folio": 1, "path": COIL, "x": 100, "y": 100}]))
        found = m.tool_continuity(BINARY, r["output"], folio=1, elements_dir=ELEMENTS)["findings"]
        self.assertTrue(found)
        self.assertEqual({(f["folio"], f["folio_number"]) for f in found}, {(1, 2)})
        self.assertEqual({e["folio"] for e in m.tool_elements(r["output"])["elements"]}, {2})

    def test_continuity_finds_a_report_link_colour_mismatch(self):
        """Reproduces qelectrotech/qelectrotech-source-mirror#974: a
        folio-jump conductor drawn in two different colours on either side
        of a next_report/previous_report link.

        Cannot be produced through the scripting API (or, by the same
        logic, the GUI's own property editor) at all any more, and that is
        itself worth locking down: setConductorProperty() already applies
        to the whole potential via relatedPotentialConductors(true), which
        already follows a report link -- verified directly that setting
        "b"'s colour after the link exists changes "a"'s too, not just
        "b"'s. link_elements() also now refuses to create a mismatched
        link in the first place (see test_link_elements_refuses_a_
        mismatched_report_link). So the only way this state can still
        exist is a file QElectroTech did not itself produce -- simulated
        here the same way test_continuity_detects_a_tampered_potential
        does, by patching a saved file's XML directly."""
        base = self.sb.new(folios=2)
        ops = [{"op": "add_element", "id": "next", "folio": 0, "path": NEXT_REPORT, "x": 300, "y": 100},
               {"op": "add_element", "id": "prev", "folio": 1, "path": PREVIOUS_REPORT, "x": 300, "y": 100},
               {"op": "link_elements", "folio": 0, "element": "$next",
                "to_folio": 1, "to": "$prev"},
               {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
               {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0,
                "to": "$next", "to_terminal": 0},
               {"op": "set_conductor", "folio": 0, "element": "$a", "terminal": 0,
                "property": "color", "value": "#0000ff"},
               {"op": "add_element", "id": "b", "folio": 1, "path": COIL, "x": 100, "y": 100},
               {"op": "add_conductor", "folio": 1, "from": "$b", "from_terminal": 0,
                "to": "$prev", "to_terminal": 0}]
        r = self.ok(self.sb.edit(base, ops))
        xml = Path(r["output"]).read_text(encoding="utf-8")
        # both conductors read color="#0000ff" at this point (setting "a"'s
        # propagated to "b" too, across the link) -- patch just the second
        # occurrence to simulate a file QElectroTech itself never produced.
        first = xml.find('color="#0000ff"')
        second = xml.find('color="#0000ff"', first + 1)
        self.assertNotEqual(second, -1, "both sides of the link should carry the value")
        tampered = xml[:second] + 'color="#55aa00"' + xml[second + len('color="#0000ff"'):]
        tampered_path = self.sb.p("tampered_report_link.qet")
        Path(tampered_path).write_text(tampered, encoding="utf-8")

        result = m.tool_continuity(BINARY, tampered_path, elements_dir=ELEMENTS)
        mismatches = [f for f in result["findings"] if f["kind"] == "report_link_mismatch"]
        self.assertEqual(len(mismatches), 1)
        self.assertEqual(mismatches[0]["property"], "color")
        self.assertEqual(sorted(mismatches[0]["values"]), ["#0000ff", "#55aa00"])
        self.assertEqual(mismatches[0]["severity"], "warning",
                         "unenforced by the app -- a real gap, but not proof of external "
                         "tampering the way potential_mismatch's \"error\" is")
        self.assertEqual(result["warnings"], 1)

    def test_continuity_no_false_positive_when_report_link_colours_match(self):
        base = self.sb.new(folios=2)
        ops = [{"op": "add_element", "id": "next", "folio": 0, "path": NEXT_REPORT, "x": 300, "y": 100},
               {"op": "add_element", "id": "prev", "folio": 1, "path": PREVIOUS_REPORT, "x": 300, "y": 100},
               {"op": "link_elements", "folio": 0, "element": "$next",
                "to_folio": 1, "to": "$prev"},
               {"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
               {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0,
                "to": "$next", "to_terminal": 0},
               {"op": "set_conductor", "folio": 0, "element": "$a", "terminal": 0,
                "property": "color", "value": "#0000ff"},
               {"op": "add_element", "id": "b", "folio": 1, "path": COIL, "x": 100, "y": 100},
               {"op": "add_conductor", "folio": 1, "from": "$b", "from_terminal": 0,
                "to": "$prev", "to_terminal": 0}]
        # "b"'s conductor is left at whatever it inherited from "a" across
        # the already-established link (see ConductorCreator::
        # existingPotential()'s AllReport branch) -- no explicit colour set.
        r = self.ok(self.sb.edit(base, ops))
        result = m.tool_continuity(BINARY, r["output"], elements_dir=ELEMENTS)
        mismatches = [f for f in result["findings"] if f["kind"] == "report_link_mismatch"]
        self.assertEqual(mismatches, [])

    def test_link_elements_refuses_a_mismatched_report_link(self):
        """LinkElementCommand::redo() would otherwise pop
        PotentialSelectorDialog -- a plain QDialog::exec() -- when linking
        two report elements whose existing conductors disagree, which
        hangs forever under headless --run with nobody there to answer
        it. Measured directly: confirmed hanging before this guard was
        added, confirmed clean (refused, no hang) after."""
        base = self.sb.new(folios=2)
        ops = [{"op": "add_element", "id": "a", "folio": 0, "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "next", "folio": 0, "path": NEXT_REPORT, "x": 300, "y": 100},
               {"op": "add_conductor", "folio": 0, "from": "$a", "from_terminal": 0,
                "to": "$next", "to_terminal": 0},
               {"op": "set_conductor", "folio": 0, "element": "$a", "terminal": 0,
                "property": "color", "value": "#0000ff"},
               {"op": "add_element", "id": "b", "folio": 1, "path": COIL, "x": 100, "y": 100},
               {"op": "add_element", "id": "prev", "folio": 1, "path": PREVIOUS_REPORT, "x": 300, "y": 100},
               {"op": "add_conductor", "folio": 1, "from": "$b", "from_terminal": 0,
                "to": "$prev", "to_terminal": 0},
               {"op": "set_conductor", "folio": 1, "element": "$b", "terminal": 0,
                "property": "color", "value": "#55aa00"},
               {"op": "link_elements", "folio": 0, "element": "$next",
                "to_folio": 1, "to": "$prev"}]
        r = self.sb.edit(base, ops, out="refuse.qet")
        self.assertFalse(r["ok"])
        self.assertFalse(r["operations"][-1]["succeeded"])

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
class UuidIndexLookups(unittest.TestCase):
    """qet.tableIndex(), qet.elementTextIndex() and qet.folioIndex(): a uuid in, the index the
    other calls take out, as qet.textIndex() does for free texts. The index is
    what shifts when an earlier item is deleted; the uuid is what holds."""

    def run_script(self, project, body):
        r = m._run_qet(BINARY, [str(project)], elements_dir=ELEMENTS or None,
                       script=body + '\nqet.log("OUT " + JSON.stringify(out));', tail=200_000)
        line = next((ln for ln in (r["stdout"] + "\n" + r["stderr"]).splitlines() if "OUT " in ln), None)
        self.assertIsNotNone(line, r.get("stderr", "")[-500:])
        return json.loads(line.split("OUT ", 1)[1])

    def test_table_index_follows_the_table_across_a_deletion(self):
        root = ET.parse(Path(EXAMPLES) / "industrial.qet").getroot()
        folio, table = next((i, d.find("tables/graphics_table")) for i, d in enumerate(root.iter("diagram"))
                            if d.find("tables/graphics_table") is not None)
        twin = ET.fromstring(ET.tostring(table))
        twin.set("uuid", "{11111111-2222-4333-8444-555555555555}")
        twin.set("x", str(float(table.get("x")) + 900))
        list(root.iter("diagram"))[folio].find("tables").append(twin)
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "two_tables.qet"
            ET.ElementTree(root).write(p, encoding="utf-8")
            first, second = table.get("uuid"), twin.get("uuid")
            out = self.run_script(p, f"""
                var out = {{}};
                out.first = qet.tableIndex({folio}, "{first}");
                out.second = qet.tableIndex({folio}, "{second}");
                out.bogus = qet.tableIndex({folio}, "{{00000000-0000-4000-8000-000000000000}}");
                out.not_uuid = qet.tableIndex({folio}, "table");
                out.list = qet.tables({folio});
                qet.deleteTable({folio}, out.first);
                out.second_after = qet.tableIndex({folio}, "{second}");
                out.first_after = qet.tableIndex({folio}, "{first}");""")
        self.assertEqual(sorted([out["first"], out["second"]]), [0, 1])
        # each index names the right table: the twin sits 900 further right
        at = lambda i: float(re.search(r"at \(([-\d.]+),", out["list"][i]).group(1))
        self.assertEqual(at(out["second"]) - at(out["first"]), 900)
        self.assertEqual((out["bogus"], out["not_uuid"]), (-1, -1))
        self.assertEqual((out["second_after"], out["first_after"]), (0, -1))

    def test_qet_edit_deletes_then_moves_tables_by_uuid(self):
        """Delete one table, then move the other, both by uuid. By index the
        second op would name the wrong table: deleting table 0 shifts table 1."""
        text = (Path(EXAMPLES) / "industrial.qet").read_text(encoding="utf-8")
        root = ET.fromstring(text)
        folio, table = next((i, d.find("tables/graphics_table")) for i, d in enumerate(root.iter("diagram"))
                            if d.find("tables/graphics_table") is not None)
        twin = ET.fromstring(ET.tostring(table))
        twin.set("uuid", "{11111111-2222-4333-8444-555555555555}")
        twin.set("x", str(float(table.get("x")) + 900))
        # Insert into the raw text: re-serialising the whole file with
        # ElementTree rewrites the embedded SVG logo's namespace, which
        # QElectroTech then saves without its declaration.
        start = text.index(f'uuid="{table.get("uuid")}"')
        end = text.index("</graphics_table>", start) + len("</graphics_table>")
        text = text[:end] + ET.tostring(twin, encoding="unicode") + text[end:]
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "two.qet", Path(tmp) / "out.qet"
            src.write_text(text, encoding="utf-8")
            r = m.tool_edit(BINARY, str(src), [
                {"op": "delete_table", "folio": folio, "table": table.get("uuid")},
                {"op": "set_table_position", "folio": folio, "table": twin.get("uuid"), "x": 120, "y": 340}],
                str(out), elements_dir=ELEMENTS or None)
            self.assertTrue(r["ok"], r.get("hint"))
            left = list(ET.parse(out).getroot().iter("diagram"))[folio].findall("tables/graphics_table")
        self.assertEqual([(t.get("uuid"), float(t.get("x")), float(t.get("y"))) for t in left],
                         [(twin.get("uuid"), 120.0, 340.0)])

    def test_qet_edit_follows_a_folio_by_uuid_across_a_removal(self):
        """Remove folio 0, then retitle what was folio 2 by its uuid. By index
        the second op would retitle the wrong folio. The file is saved
        without folio uuids, so qet_project_info shows them only once a
        first qet_edit has saved it."""
        src = Path(EXAMPLES) / "tableau_domestique.qet"
        self.assertEqual({f["uuid"] for f in m.tool_project_info(str(src))["folios"]}, {""})
        with tempfile.TemporaryDirectory() as tmp:
            once, out = Path(tmp) / "once.qet", Path(tmp) / "out.qet"
            r = m.tool_edit(BINARY, str(src), [{"op": "set_folio", "folio": 0, "property": "author",
                                                "value": "x"}], str(once), elements_dir=ELEMENTS or None)
            self.assertTrue(r["ok"], r.get("hint"))
            before = m.tool_project_info(str(once))["folios"]
            self.assertTrue(all(m._UUID_RE.fullmatch(f["uuid"]) for f in before))
            third = before[2]
            r = m.tool_edit(BINARY, str(once), [
                {"op": "remove_folio", "folio": before[0]["uuid"]},
                {"op": "set_folio", "folio": third["uuid"], "property": "title", "value": "moved"}],
                str(out), elements_dir=ELEMENTS or None)
            self.assertTrue(r["ok"], r.get("hint"))
            after = m.tool_project_info(str(out))["folios"]
        self.assertEqual([f["uuid"] for f in after], [f["uuid"] for f in before[1:]])
        self.assertEqual([f["title"] for f in after],
                         [f["title"] for f in before[1:2]] + ["moved"] + [f["title"] for f in before[3:]])

    def test_qet_edit_edits_one_copys_field_by_uuid(self):
        """Two copies of a symbol share a field uuid; addressing it with the
        symbol changes that copy's field only."""
        root = ET.parse(Path(EXAMPLES) / "2612_ats_singlephase.qet").getroot()
        owners = {}
        for i, d in enumerate(root.iter("diagram")):
            for el in d.iter("element"):
                for t in el.findall("dynamic_texts/dynamic_elmt_text"):
                    if t.get("uuid"):
                        owners.setdefault((i, t.get("uuid")), []).append(el.get("uuid"))
        (folio, field), (a, b) = next((k, v[:2]) for k, v in owners.items() if len(v) >= 2)
        with tempfile.TemporaryDirectory() as tmp:
            src, out = Path(tmp) / "in.qet", Path(tmp) / "out.qet"
            shutil.copy(Path(EXAMPLES) / "2612_ats_singlephase.qet", src)
            r = m.tool_edit(BINARY, str(src), [
                {"op": "set_element_text", "folio": folio, "element": b, "index": field,
                 "property": "x", "value": "77"}], str(out), elements_dir=ELEMENTS or None)
            self.assertTrue(r["ok"], r.get("hint"))
            x_of = lambda path, el_uuid: next(
                t.get("x") for el in ET.parse(path).getroot().iter("element") if el.get("uuid") == el_uuid
                for t in el.findall("dynamic_texts/dynamic_elmt_text") if t.get("uuid") == field)
            self.assertEqual(float(x_of(out, b)), 77.0)
            self.assertEqual(x_of(out, a), x_of(src, a))

    def test_element_text_index_needs_the_element_as_well(self):
        """Copies of a symbol share their text fields' uuids (2612_ats_singlephase.qet):
        the same field uuid resolves on each copy to that copy's own field."""
        root = ET.parse(Path(EXAMPLES) / "2612_ats_singlephase.qet").getroot()
        owners = {}
        for i, d in enumerate(root.iter("diagram")):
            for el in d.iter("element"):
                for t in el.iter("dynamic_elmt_text"):
                    if t.get("uuid"):
                        owners.setdefault((i, t.get("uuid")), []).append((el.get("uuid"), t.findtext("text")))
        (folio, field), copies = next((k, v) for k, v in owners.items()
                                      if len(v) >= 2 and len({s for _, s in v}) >= 2)
        (a, shows_a), (b, shows_b) = copies[:2]
        out = self.run_script(Path(EXAMPLES) / "2612_ats_singlephase.qet", f"""
            var out = {{}};
            out.ia = qet.elementTextIndex({folio}, "{a}", "{field}");
            out.ib = qet.elementTextIndex({folio}, "{b}", "{field}");
            out.la = qet.elementTexts({folio}, "{a}");
            out.lb = qet.elementTexts({folio}, "{b}");
            out.bogus = qet.elementTextIndex({folio}, "{a}", "{{00000000-0000-4000-8000-000000000000}}");
            out.no_element = qet.elementTextIndex({folio}, "{{00000000-0000-4000-8000-000000000001}}", "{field}");""")
        self.assertGreaterEqual(out["ia"], 0)
        self.assertGreaterEqual(out["ib"], 0)
        self.assertIn(f"shows='{shows_a}'", out["la"][out["ia"]])
        self.assertIn(f"shows='{shows_b}'", out["lb"][out["ib"]])
        self.assertEqual((out["bogus"], out["no_element"]), (-1, -1))

    def test_folio_index_follows_the_folio_across_a_removal(self):
        """A file saved without folio uuids (tableau_domestique.qet) still has
        one per folio once loaded, the same on every load; the index of the
        third folio moves when the first is removed, its uuid does not."""
        body = """
            var out = {uuids: []};
            for (var i = 0; i < qet.folioCount(); i++) out.uuids.push(qet.folioUuid(i));
            out.found = out.uuids.map(function (u) { return qet.folioIndex(u); });
            out.bogus = qet.folioIndex("{00000000-0000-4000-8000-000000000000}");
            out.not_uuid = qet.folioIndex("folio 1");
            out.out_of_range = qet.folioUuid(qet.folioCount());
            qet.removeFolio(0);
            out.third_after = qet.folioIndex(out.uuids[2]);
            out.first_after = qet.folioIndex(out.uuids[0]);"""
        project = Path(EXAMPLES) / "tableau_domestique.qet"
        self.assertFalse(any(d.get("uuid") for d in ET.parse(project).getroot().iter("diagram")))
        out = self.run_script(project, body)
        self.assertEqual(len(out["uuids"]), 5)
        self.assertEqual(len(set(out["uuids"])), 5)
        self.assertTrue(all(m._UUID_RE.fullmatch(u) for u in out["uuids"]))
        self.assertEqual(out["found"], [0, 1, 2, 3, 4])
        self.assertEqual((out["bogus"], out["not_uuid"], out["out_of_range"]), (-1, -1, ""))
        self.assertEqual((out["third_after"], out["first_after"]), (1, -1))
        self.assertEqual(self.run_script(project, body)["uuids"], out["uuids"])

    def test_a_new_folio_keeps_its_uuid_through_a_save(self):
        with tempfile.TemporaryDirectory() as tmp:
            saved = Path(tmp) / "saved.qet"
            out = self.run_script(Path(EXAMPLES) / "tableau_domestique.qet", f"""
                var out = {{}};
                out.index = qet.addFolio();
                out.uuid = qet.folioUuid(out.index);
                out.saved = qet.save({json.dumps(str(saved))});""")
            self.assertTrue(out["saved"])
            in_file = [d.get("uuid") for d in ET.parse(saved).getroot().iter("diagram")]
        self.assertEqual(in_file[out["index"]], out["uuid"])


@needs_examples
class CorpusIntegration(unittest.TestCase):
    def test_folio_counts_match_what_qelectrotech_itself_holds(self):
        """Element and conductor counts per folio, from the file, against
        QElectroTech's own counts after loading it -- over every example.
        schema_indus.qet caught a numbering rule counted as a wire."""
        script = ('var o = [];'
                  'for (var f = 0; f < qet.folioCount(); f++)'
                  '  o.push([qet.elementCount(f), qet.conductorCount(f)]);'
                  'qet.log("COUNTS " + JSON.stringify(o));')
        checked = 0
        for f in sorted(Path(EXAMPLES).glob("*.qet")):
            with self.subTest(project=f.name):
                r = m._run_qet(BINARY, [str(f)], elements_dir=ELEMENTS or None,
                               script=script, tail=200_000)
                line = next(ln for ln in (r["stdout"] + "\n" + r["stderr"]).splitlines()
                            if "COUNTS " in ln)
                qet = json.loads(line.split("COUNTS ", 1)[1])
                mcp = [[x["elements"], x["conductors"]]
                       for x in m.tool_project_info(str(f))["folios"]]
                self.assertEqual(mcp, qet)
                checked += 1
        self.assertGreater(checked, 20)

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

    def test_a_folio_number_from_qet_elements_fails_with_the_index_to_use(self):
        sb = Sandbox()
        try:
            src = shutil.copy(Path(EXAMPLES) / "grafcet.qet", sb.p("g.qet"))
            first = m.tool_elements(src, folio=1)["elements"][0]
            self.assertEqual(first["folio"], 1)
            move = {"op": "move_element", "element": first["uuid"], "dx": 10, "dy": 0}
            r = m.tool_edit(BINARY, src, [dict(move, folio=first["folio"])], sb.p("wrong.qet"),
                            elements_dir=ELEMENTS or None)
            self.assertFalse(r["ok"])
            self.assertIn('"folio": 0 here', r["hint"])
            r = m.tool_edit(BINARY, src, [dict(move, folio=0)], sb.p("right.qet"),
                            elements_dir=ELEMENTS or None)
            self.assertTrue(r["ok"], r.get("hint"))
            self.assertEqual(r["diff"]["elements"]["distinct_move_deltas"], [[10.0, 0.0]])
        finally:
            sb.close()

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
                    self.assertEqual(r["diff"]["elements"]["info_changed"], [])
        finally:
            sb.close()


if __name__ == "__main__":
    print(f"binary   : {BINARY or '(none: integration tests skipped)'}")
    print(f"elements : {ELEMENTS or '(none)'}")
    print(f"examples : {EXAMPLES or '(none)'}")
    unittest.main(verbosity=2)
