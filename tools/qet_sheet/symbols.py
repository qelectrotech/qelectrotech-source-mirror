"""Elements of the collections: collection path -> file, terminals with
position, name and uuid.

A sheet places a symbol by one of its terminals ("terminal 3 of this symbol
lies at x/y"), so that wires run without a bend. Conductors are attached to
the terminal's uuid, which is unique even when two terminals share a name.

The collections are found through the environment, as the import dialog of
QElectroTech sets it:
  QET_COMMON_ELEMENTS_DIR   common://   (the collection shipped with QET)
  QET_CUSTOM_ELEMENTS_DIR   custom://   (the user collection)
  QET_COMPANY_ELEMENTS_DIR  company://  (the company collection, optional)
"""
import functools
import os
import re
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))


def _default_common():
    """The collection of the source tree (tools/qet_sheet -> elements) or of
    an installation (<data>/import/qet_sheet -> <data>/elements)."""
    path = os.path.normpath(os.path.join(HERE, "..", "..", "elements"))
    return path if os.path.isdir(path) else ""


COMMON = os.environ.get("QET_COMMON_ELEMENTS_DIR") or _default_common()
CUSTOM = os.environ.get("QET_CUSTOM_ELEMENTS_DIR", "")
COMPANY = os.environ.get("QET_COMPANY_ELEMENTS_DIR", "")


def file(path):
    """The file of a collection path (common://, custom://, company://)."""
    for scheme, root in (("common://", COMMON), ("custom://", CUSTOM), ("company://", COMPANY)):
        if path.startswith(scheme):
            if not root:
                raise FileNotFoundError(f"{path}: the {scheme[:-3]} collection is not set "
                                        f"(QET_{scheme[:-3].upper()}_ELEMENTS_DIR)")
            return os.path.join(root, path[len(scheme):])
    return path


class Terminal:
    __slots__ = ("name", "x", "y", "orientation", "uuid", "index")

    def __init__(self, name, x, y, orientation, uuid, index):
        self.name, self.x, self.y = name, x, y
        self.orientation, self.uuid, self.index = orientation, uuid, index


class Symbol:
    def __init__(self, path):
        self.path = path
        root = ET.parse(file(path)).getroot()
        self.link_type = root.get("link_type") or "simple"
        # centre of the symbol relative to its insertion point
        self.center_x = float(root.get("width", 0)) / 2 - float(root.get("hotspot_x", 0))
        raw = [(t.get("name") or "", float(t.get("x")), float(t.get("y")), t.get("orientation"),
                t.get("uuid"))
               for t in root.iter("terminal") if t.get("x") is not None]
        # QET's index order: top to bottom, then left to right
        raw.sort(key=lambda t: (round(t[2], 1), round(t[1], 1)))
        self.terminals = [Terminal(n, x, y, o, u, i) for i, (n, x, y, o, u) in enumerate(raw)]
        # index of the texts in file order (as QET's set_element_text counts
        # them): user fields and the label
        dyn = list(root.iter("dynamic_text"))
        self.field_index = [i for i, t in enumerate(dyn) if t.get("text_from") == "UserText"]
        self.field_default = [(t.findtext("text") or "") for t in dyn
                              if t.get("text_from") == "UserText"]
        self.fields = len(self.field_index)
        self.label_index = next((i for i, t in enumerate(dyn) if t.get("text_from") == "ElementInfo"
                                 and (t.findtext("info_name") or "") == "label"), 0)
        # where the label stands (x, y, font size, alignment), to move it out
        # of the way of a wire (Sheet.place)
        lab = dyn[self.label_index] if dyn and self.label_index < len(dyn) else None
        if lab is not None and lab.get("text_from") == "ElementInfo":
            size = re.search(r",([\d.]+),", lab.get("font") or "")
            self.label_position = (float(lab.get("x", 0)), float(lab.get("y", 0)),
                                   float(size.group(1)) if size else 9.0,
                                   lab.get("Halignment") or "AlignLeft")
        else:
            self.label_position = None

    def terminal(self, which):
        """which: a name ('A1', '3'), an index (int) or ('name', n) for the
        n-th terminal of that name."""
        if isinstance(which, int):
            return self.terminals[which]
        if isinstance(which, tuple):
            name, n = which
            return [t for t in self.terminals if t.name == name][n]
        found = [t for t in self.terminals if t.name == which]
        if not found:
            raise KeyError(f"{self.path}: no terminal {which!r} "
                           f"(has {[t.name for t in self.terminals]})")
        return found[0]


@functools.lru_cache(maxsize=None)
def symbol(path):
    return Symbol(path)


C = "common://10_electric/10_allpole/"
COIL = C + "310_relays_contactors_contacts/01_coils/bobine3.elmt"          # A1 top, A2 bottom, master
NO_CONTACT = (C + "310_relays_contactors_contacts/02_contacts_cross_referencing/"
              "01_auxiliary_contacts/con_simple.elmt")                    # slave
NC_CONTACT = (C + "310_relays_contactors_contacts/02_contacts_cross_referencing/"
              "01_auxiliary_contacts/con_simple_nf.elmt")                 # slave
TERMINAL = C + "130_terminals_terminal_strips/borne_continuite2.elmt"       # top/bottom
CIRCUIT_BREAKER_3P = (C + "200_fuses_protective_gears/12_magneto_thermal_circuit_breakers/"
                      "dis_mag_term_3f-2.elmt")
RCBO = C + "200_fuses_protective_gears/50_residual_current_circuit_breaker/int_diff5.elmt"
MOTOR_AC = C + "391_consumers_actuators/10_engines/moteur_mono.elmt"
MOTOR_DC = C + "391_consumers_actuators/10_engines/moteur_dc.elmt"
NTC = C + "395_electronics_semiconductors/01_resistors/ntc.elmt"
COMING = C + "100_folio_referencing/01coming_arrow.elmt"                    # terminal on the right
GOING = C + "100_folio_referencing/02going_arrow.elmt"                      # terminal on the left
