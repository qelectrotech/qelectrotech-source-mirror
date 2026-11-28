"""A QElectroTech project built as a list of edit operations, then made by a headless QElectroTech.

    plan = Plan("Title", folder, author="")
    folio = plan.folio("Page title", "plant", "location", "1")
    a = folio.place("custom://EPLAN import/EPDZ/a.elmt", 200, 300, label="-K1")
    b = folio.place("custom://EPLAN import/EPDZ/b.elmt", 200, 500, label="-S1")
    folio.wire(a, 1, b, 0)
    project, result = plan.build("project.qet")

Parts are placed by their insertion point; conductors join terminals given by index (QElectroTech's order:
top to bottom, then left to right) and are written with the terminal's uuid, which names it even where two
terminals share a name. The operations are those of qet_mcp's tool_edit (misc/qet-mcp/qet_mcp.py).
"""
import functools
import html
import os
import re
import xml.etree.ElementTree as ET

import qet_backend

BLANK_TITLE_BLOCK = "eplan_import_blank"


def collection_dir(scheme):
    """Folder of a collection: "custom" (QET_CUSTOM_ELEMENTS_DIR), "common" (QET_COMMON_ELEMENTS_DIR) or
    "company" (QET_COMPANY_ELEMENTS_DIR); None when it is not set."""
    value = os.environ.get(f"QET_{scheme.upper()}_ELEMENTS_DIR", "").strip()
    return value or None


def element_file(path):
    """custom://, common:// or company:// path -> file."""
    for scheme in ("custom", "common", "company"):
        prefix = scheme + "://"
        if path.startswith(prefix):
            root = collection_dir(scheme)
            if root is None:
                raise RuntimeError(f"{path}: QET_{scheme.upper()}_ELEMENTS_DIR is not set")
            return os.path.join(root, *path[len(prefix):].split("/"))
    return path


class Terminal:
    __slots__ = ("name", "x", "y", "orientation", "uuid", "index")

    def __init__(self, name, x, y, orientation, uuid, index):
        self.name, self.x, self.y, self.orientation, self.uuid, self.index = name, x, y, orientation, uuid, index


class Symbol:
    """An element file: its link type, its terminals in QElectroTech's index order and its label text."""

    def __init__(self, path):
        self.path = path
        root = ET.parse(element_file(path)).getroot()
        self.link_type = root.get("link_type") or "simple"
        raw = [(t.get("name") or "", float(t.get("x")), float(t.get("y")), t.get("orientation"), t.get("uuid"))
               for t in root.iter("terminal") if t.get("x") is not None]
        raw.sort(key=lambda t: (round(t[2], 1), round(t[1], 1)))
        self.terminals = [Terminal(n, x, y, o, u, i) for i, (n, x, y, o, u) in enumerate(raw)]
        # index of the texts in file order, as QElectroTech's set_element_text counts them
        dyn = list(root.iter("dynamic_text"))
        self.label_index = next((i for i, t in enumerate(dyn) if t.get("text_from") == "ElementInfo"
                                 and (t.findtext("info_name") or "") == "label"), None)

    def terminal(self, which):
        """which: an index, or a name (the first terminal of that name)."""
        if isinstance(which, int):
            return self.terminals[which]
        for t in self.terminals:
            if t.name == which:
                return t
        raise KeyError(f"{self.path}: no terminal {which!r} (has {[t.name for t in self.terminals]})")


@functools.lru_cache(maxsize=None)
def symbol(path):
    return Symbol(path)


def _rotated(x, y, degrees):
    """Where a terminal lies after a clockwise turn (QElectroTech, y down)."""
    return {0: (x, y), 90: (-y, x), 180: (-x, -y), 270: (y, -x)}[degrees % 360]


class Part:
    """A placed element; terminal(which) gives its terminal, pos(which) where it lies on the folio."""

    def __init__(self, folio, ident, path, x, y, rotation=0):
        self.folio, self.ident, self.path, self.x, self.y = folio, ident, path, x, y
        self.symbol, self.rotation = symbol(path), rotation % 360
        self.label = ""

    @property
    def ref(self):
        return f"${self.ident}"

    def terminal(self, which):
        return self.symbol.terminal(which)

    def pos(self, which):
        t = self.terminal(which)
        dx, dy = _rotated(t.x, t.y, self.rotation)
        return (round(self.x + dx, 1), round(self.y + dy, 1))


class Folio:
    def __init__(self, plan, index, title, plant, location, number):
        self.plan, self.index, self.title = plan, index, title
        self.ops, self.n = [], 0
        f = index
        self.ops += [
            {"op": "set_folio_title", "folio": f, "title": title},
            {"op": "set_folio", "folio": f, "property": "plant", "value": plant},
            {"op": "set_folio", "folio": f, "property": "locmach", "value": location},
            {"op": "set_folio", "folio": f, "property": "folio", "value": str(number)},
            {"op": "set_folio_border", "folio": f, "property": "preset", "value": "a3-landscape"},
            {"op": "set_folio_border", "folio": f, "property": "columns", "value": "10"},
            {"op": "set_folio_border", "folio": f, "property": "column-width", "value": "156"},
            {"op": "set_folio_border", "folio": f, "property": "display-rows", "value": "false"},
            {"op": "set_conductor_default", "folio": f, "property": "displaytext", "value": "false"}]

    def _id(self, ident=None):
        self.n += 1
        return f"f{self.index}_{ident or 'e'}_{self.n}"

    def place(self, path, x, y, label="", infos=None, rotation=0):
        """Place the element @p path with its insertion point at x/y; infos: element informations
        {manufacturer, manufacturer_reference, designation, ...}."""
        p = Part(self, self._id(), path, round(x, 1), round(y, 1), rotation)
        p.label = label or ""
        self.ops.append({"op": "add_element", "folio": self.index, "path": path, "x": p.x, "y": p.y,
                         "id": p.ident})
        if rotation % 360:
            self.ops.append({"op": "rotate_element", "folio": self.index, "element": p.ref,
                             "angle": rotation % 360})
        self.ops.append({"op": "set_label", "folio": self.index, "element": p.ref, "label": p.label})
        for k, v in (infos or {}).items():
            if v:
                self.ops.append({"op": "set_info", "folio": self.index, "element": p.ref, "key": k,
                                 "value": str(v)})
        return p

    def wire(self, a, terminal_a, b, terminal_b):
        self.ops.append({"op": "add_conductor", "folio": self.index,
                         "from": a.ref, "from_terminal": a.terminal(terminal_a).uuid,
                         "to": b.ref, "to_terminal": b.terminal(terminal_b).uuid})

    def link(self, master, *slaves):
        """Link a master to its slaves (coil -> contacts), on this folio or others."""
        for s in slaves:
            self.plan.links.append((s.folio.index, s, master.folio.index, master))


class Plan:
    def __init__(self, title, folder, author=""):
        self.title, self.folder, self.author = title, folder, author
        self.folios, self.links = [], []
        self.sources, self.targets = [], []     # (folio, name, Part): outgoing / incoming report arrows
        self.late = []                          # operations that must follow the links
        self.open = []                          # outgoing arrows left without a partner

    def folio(self, title, plant, location, number):
        f = Folio(self, len(self.folios), title, plant, location, number)
        self.folios.append(f)
        return f

    def _report_links(self):
        """Each outgoing arrow with the next incoming arrow of the same name on a later folio (else an earlier
        one); each target once. Then the master/slave links."""
        ops, free, self.open = [], list(self.targets), []
        for f, name, q in sorted(self.sources, key=lambda a: a[0]):
            candidates = [z for z in free if z[1] == name and z[0] != f]
            later = [z for z in candidates if z[0] > f]
            choice = (min(later, key=lambda z: z[0]) if later
                      else (min(candidates, key=lambda z: -z[0]) if candidates else None))
            if choice is None:
                self.open.append((f, name))
                continue
            free.remove(choice)
            ops.append({"op": "link_elements", "folio": f, "element": q.ref, "to_folio": choice[0],
                        "to": choice[2].ref})
        for f, slave, mf, master in self.links:
            ops.append({"op": "link_elements", "folio": f, "element": slave.ref, "to_folio": mf, "to": master.ref})
        return ops

    def build(self, name, timeout=3000):
        """Make the project @p name in the plan's folder: a new project with one folio per page, the
        operations applied by tool_edit. -> (path, tool_edit's answer)."""
        q = qet_backend.mcp()
        binary = qet_backend.binary()
        os.environ.setdefault("QET_SCRIPT_TIMEOUT_MS", str(timeout * 1000))
        ops = []
        for f in self.folios:
            ops += f.ops
        ops += self._report_links()
        ops += self.late
        blank = os.path.join(self.folder, "_eplan_import_blank.qet")
        out = os.path.join(self.folder, name)
        # a new project holds at most 200 folios; more are added in front of the other operations
        new = q.tool_project_new(binary, blank, title=self.title, folios=min(200, max(1, len(self.folios))),
                                 author=self.author, overwrite=True, timeout=timeout)
        if not new.get("ok", True) and not os.path.isfile(blank):
            raise RuntimeError(f"QElectroTech could not make a new project: {new}")
        if len(self.folios) > 200:
            ops = [{"op": "add_folio"}] * (len(self.folios) - 200) + ops
        result = q.tool_edit(binary, blank, ops, out, timeout=timeout)
        try:
            os.remove(blank)
        except OSError:
            pass
        if not os.path.isfile(out):
            raise RuntimeError(f"QElectroTech did not write {out}: {result.get('hint') or result.get('error')}")
        embed_blank_title_block(out)
        return out, result


def embed_blank_title_block(qet):
    """Give every folio of @p qet an empty title block template embedded in the project: the frame and title
    block of the EPLAN page are drawn as folio graphics instead. Written into the file, not through the
    scripting API, so that nothing goes into the user's title block folder."""
    with open(qet, encoding="utf-8") as f:
        s = f.read()
    template = (f'<titleblocktemplate name="{BLANK_TITLE_BLOCK}">'
                '<information>Empty: the frame and title block come from the EPLAN page</information>'
                '<logos/><grid rows="1;" cols="r100%;"><field row="0" col="0" name="" displaylabel="false" '
                'align="left" valign="center" fontsize="6" hadjust="false"><value/><label/></field></grid>'
                '</titleblocktemplate>')
    if re.search(r"<titleblocktemplates\s*/>", s):
        s = re.sub(r"<titleblocktemplates\s*/>", f"<titleblocktemplates>{template}</titleblocktemplates>", s, 1)
    elif "<titleblocktemplates>" in s:
        s = s.replace("<titleblocktemplates>", "<titleblocktemplates>" + template, 1)
    else:
        s = re.sub(r"(<project\b[^>]*>)", lambda m: m.group(1) + f"\n    <titleblocktemplates>{template}"
                   "</titleblocktemplates>", s, 1)

    def diagram(m):
        tag = re.sub(r'\s(titleblocktemplate|titleblocktemplateCollection)="[^"]*"', "", m.group(0))
        return tag[:-1] + (f' titleblocktemplate="{html.escape(BLANK_TITLE_BLOCK, quote=True)}"'
                           ' titleblocktemplateCollection="embedded">')
    s = re.sub(r"<diagram\b[^>]*>", diagram, s)
    with open(qet, "w", encoding="utf-8") as f:
        f.write(s)
