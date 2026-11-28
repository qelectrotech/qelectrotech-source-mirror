"""GRAFCET charts of FluidSIM 6 circuits, drawn with the GRAFCET symbols of the import (folder grafcet/).

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

FluidSIM stores a GRAFCET as circuit objects Grafcet.Step, Grafcet.Transition, Grafcet.Action, Grafcet.MultiPin
(branch) and Grafcet.SyncRule (double line: parallel branch or synchronisation), joined through <connect> like
wires. Imported are:
- every step with its number (initial step: flags="initial"), macro steps (M<name>), macro entry and exit steps
  (E<name>, S<name>) and enclosing steps with their own symbols;
- every transition with its receptivity in the notation of IEC 60848 (FluidSIM "*" -> "AND" dot, "+" -> "OR",
  "RE x" -> rising edge, "FE x" -> falling edge, "[...]" -> condition, "-SF1.signal" -> "-SF1"), and a jump
  under it when FluidSIM names a target step;
- every action: continuous (actionType 0), conditional (1), stored on activation (2), on deactivation (3), on
  firing (4, approximated as on activation), on event (5); assignments as "A := 1"; chained actions belong to
  the step of the first one; forcing orders (actionType 6) as a continuous action "G12{8, 9, 11}";
- the connections; double lines become a parallel branch or a synchronisation.

The symbols come with the import (folder grafcet/): the GRAFCET symbols of the QElectroTech collection redrawn on
the 10 grid, with named terminals (in, out, action, step, 1, 2), so steps and transitions can be wired by name.
They get their own folder in the imported project, apart from the symbols of the collection.
"""
import re

G = "fluidsim_grafcet/"
FILES = {
    "initial": G + "20202000_step.elmt", "step": G + "20202001_step.elmt",
    "enclosing": G + "20202002_step.elmt", "macro": G + "20202003_step.elmt",
    "transition": G + "20202060_transition.elmt",
    "0": G + "20202020_action.elmt", "1": G + "20202021_action.elmt", "2": G + "20202022_action.elmt",
    "3": G + "20202023_action.elmt", "4": G + "20202022_action.elmt", "5": G + "20202024_action.elmt",
    "parallel": G + "20202070_parallel_branch.elmt", "sync": G + "20202071_synchronization.elmt",
    "jump": G + "20202090_jump.elmt",
}
# FluidSIM connector -> terminal of the QElectroTech symbol
TERMINAL = {"GC_S_in": "in", "GC_S_out": "out", "GC_S_A": "action", "GC_T_in": "in", "GC_T_out": "out"}


def receptivity(text):
    """A FluidSIM condition in the notation of IEC 60848."""
    s = (text or "").strip()
    s = s.replace(".signal", "")
    s = re.sub(r"\[(.*?)\]", r"\1", s)
    s = re.sub(r"(^|[\s(*+])RE\s+", "\\1\u2191", s)
    s = re.sub(r"(^|[\s(*+])FE\s+", "\\1\u2193", s)
    s = s.replace("*", " \u00b7 ").replace("+", " \u2228 ")
    return re.sub(r"\s+", " ", s).strip()


def values(o):
    return {e.get("ident"): e.get("value") or "" for e in o if e.tag in ("const", "parameter")}


def terminals(elmt_path):
    """{terminal name: (uuid, x, y)} of a symbol."""
    text = elmt_path.read_text(encoding="utf-8")
    out = {}
    for tag in re.findall(r"<terminal\b[^>]*>", text):
        name = re.search(r'\sname="([^"]*)"', tag)
        uid = re.search(r'uuid="\{([^}]+)\}"', tag)
        x = float(re.search(r'\sx="([-\d.]+)"', tag).group(1))
        y = float(re.search(r'\sy="([-\d.]+)"', tag).group(1))
        if name and uid:
            out[name.group(1)] = (uid.group(1), x, y)
    return out


class _Placeholder:
    """A stand-in circuit object for the jump symbol under a transition."""

    def __init__(self, ident):
        self.ident = ident

    def get(self, key, default=None):
        return self.ident if key == "ident" else default


def convert(folio, objects, connections, collection, scale, centre):
    """Operations for qet_edit; @return (ops, count, not_imported)."""
    ops, not_imported = [], []
    count = {"steps": 0, "transitions": 0, "actions": 0, "connections": 0}
    gc = {o.get("ident"): o for o in objects if (o.get("object-ref") or "").startswith("Grafcet.")}
    if not gc:
        return ops, count, not_imported
    if not all((collection / f).is_file() for f in FILES.values()):
        not_imported.append("GRAFCET: the GRAFCET symbols are missing next to the import")
        return ops, count, not_imported
    term = {k: terminals(collection / v) for k, v in FILES.items()}
    placed = {}     # FluidSIM ident -> (id, kind)

    def add(o, kind, x, y, label=None):
        oid = f"g{folio}_{o.get('ident')}"
        ops.append({"op": "add_element", "id": oid, "folio": folio, "path": "common://" + FILES[kind],
                    "x": round(x, 1), "y": round(y, 1)})
        if label:
            ops.append({"op": "set_label", "folio": folio, "element": "$" + oid, "label": label})
        placed[o.get("ident")] = (oid, kind)
        return oid

    for ident, o in gc.items():
        kind = o.get("object-ref")
        w = values(o)
        mx, my = centre(o)
        if kind == "Grafcet.Step":
            flags = set(w.get("flags", "").split())
            name = w.get("name", "")
            if "macro" in flags:
                symbol, label = "macro", "M" + name
            elif "enclosing" in flags:
                symbol, label = "enclosing", name
            else:
                symbol = "initial" if "initial" in flags else "step"
                label = ("E" if "mentry" in flags else "S" if "mexit" in flags else "") + name
            unknown = flags - {"initial", "alink", "enclosing", "macro", "mentry", "mexit"}
            if unknown:
                not_imported.append(f"step {name}: {' '.join(sorted(unknown))} (imported as a plain step)")
            add(o, symbol, mx * scale, my * scale, label)
            count["steps"] += 1
        elif kind == "Grafcet.Transition":
            oid = add(o, "transition", mx * scale, my * scale, receptivity(w.get("condition")) or "1")
            if w.get("cond-descr"):
                ops.append({"op": "set_info", "folio": folio, "element": "$" + oid, "key": "comment",
                            "value": w["cond-descr"]})
            target = (w.get("target-info") or "").strip()
            if target:
                # jump (FluidSIM target-info): the jump symbol under the transition, terminal on terminal
                jid = add(_Placeholder(o.get("ident") + "_jump"), "jump", mx * scale, my * scale + 30, target)
                ops.append({"op": "add_conductor", "folio": folio, "from": "$" + oid,
                            "from_terminal": "{" + term["transition"]["out"][0] + "}", "to": "$" + jid,
                            "to_terminal": "{" + term["jump"][next(iter(term["jump"]))][0] + "}"})
                count["connections"] += 1
            count["transitions"] += 1
        elif kind == "Grafcet.Action":
            atype = w.get("actionType", "0")
            if atype == "6":
                # forcing order: G12{8, 9, 11}, G12{*}, G12{}, G12{INIT}
                text = "G%s{%s}" % (w.get("sub-grafcet", "").strip(), w.get("steps", "").strip())
                add(o, "0", float(o.get("x1", 0)) * scale, my * scale, text)
                count["actions"] += 1
                continue
            if atype not in FILES:
                not_imported.append(f"action of type {atype} ({w.get('var')}, {w.get('steps')})")
                continue
            var = w.get("var", "").strip()
            text = var if atype in ("0", "1") else f"{var} := {w.get('allocation') or '1'}"
            x1 = float(o.get("x1", 0))
            oid = add(o, atype, x1 * scale, my * scale, text)
            cond = receptivity(w.get("condition"))
            if atype == "5" and cond and not cond.startswith(("\u2191", "\u2193")):
                cond = f"\u2191({cond})"
            if atype in ("1", "5") and cond:
                ops.append({"op": "add_element_text", "folio": folio, "element": "$" + oid, "source": "text",
                            "value": cond, "x": 84, "y": -42})
            if atype == "4":
                not_imported.append(f"action on firing \u201c{text}\u201d imported as an action on activation")
            if w.get("alloc-descr"):
                ops.append({"op": "set_info", "folio": folio, "element": "$" + oid, "key": "comment",
                            "value": w["alloc-descr"]})
            count["actions"] += 1
        elif kind in ("Grafcet.MultiPin", "Grafcet.SyncRule", "Grafcet.SubGrafcet"):
            pass
        else:
            not_imported.append(kind)

    # nets: branches and double lines join all their connectors
    parent = {}

    def root(k):
        while parent.setdefault(k, k) != k:
            parent[k] = parent[parent[k]]
            k = parent[k]
        return k

    def node(ref):
        oid, _, a = ref.partition(".")
        o = gc.get(oid)
        if o is None:
            return None
        if o.get("object-ref") in ("Grafcet.MultiPin", "Grafcet.SyncRule"):
            return ("node", oid)
        return (oid, a)

    for v in connections:
        a, b = node(v.get("con1", "")), node(v.get("con2", ""))
        if a and b:
            parent[root(a)] = root(b)

    # actions at a step: all actions hanging (also through other actions) on GC_S_A
    action_step = {}
    for k in list(parent):
        if k[0] == "node":
            continue
        if gc[k[0]].get("object-ref") == "Grafcet.Step" and k[1] == "GC_S_A":
            r = root(k)
            for k2 in list(parent):
                if k2[0] != "node" and root(k2) == r and gc[k2[0]].get("object-ref") == "Grafcet.Action":
                    action_step[k2[0]] = k[0]
    for aid, sid in action_step.items():
        if aid in placed and sid in placed:
            (ea, kind_a), (es, kind_s) = placed[aid], placed[sid]
            if "action" not in term[kind_s] or "step" not in term[kind_a]:
                continue
            ops.append({"op": "add_conductor", "folio": folio, "from": "$" + es,
                        "from_terminal": "{" + term[kind_s]["action"][0] + "}", "to": "$" + ea,
                        "to_terminal": "{" + term[kind_a]["step"][0] + "}"})
            count["connections"] += 1

    # sequence: steps and transitions per net
    nets = {}
    for k in list(parent):
        if k[0] == "node" or k[1] not in TERMINAL or k[1] == "GC_S_A" or k[0] not in placed:
            continue
        nets.setdefault(root(k), []).append(k)
    for r, members in nets.items():
        if len(members) < 2:
            continue
        upper = [g for g in members if g[1].endswith("_out")]     # end here from above
        lower = [g for g in members if g[1].endswith("_in")]      # start here downwards
        bar = next((k[1] for k in list(parent) if k[0] == "node" and root(k) == r
                    and gc[k[1]].get("object-ref") == "Grafcet.SyncRule"), None)

        def position(g):
            return centre(gc[g[0]])

        def wire(g, target_id, target_terminal):
            eid, kind = placed[g[0]]
            name = TERMINAL[g[1]]
            if name not in term[kind]:
                return
            ops.append({"op": "add_conductor", "folio": folio, "from": "$" + eid,
                        "from_terminal": "{" + term[kind][name][0] + "}", "to": "$" + target_id,
                        "to_terminal": "{" + target_terminal + "}",
                        "route": "avoid"})   # a jump back and the like around steps and transitions
            count["connections"] += 1

        if bar is not None and (len(upper) > 1 or len(lower) > 1):
            o = gc[bar]
            kind = "sync" if len(upper) > len(lower) else "parallel"
            mx, my = centre(o)
            bid = add(o, kind, mx * scale, my * scale)
            t = term[kind]
            for g in upper + lower:
                gx = position(g)[0]
                if kind == "parallel":
                    name = "in" if g in upper else ("1" if gx < mx else "2")
                else:
                    name = "out" if g in lower else ("1" if gx < mx else "2")
                wire(g, bid, t[name][0])
            continue
        # else one after the other from top to bottom
        members.sort(key=lambda g: (position(g)[1], position(g)[0]))
        for g1, g2 in zip(members, members[1:]):
            eid2, kind2 = placed[g2[0]]
            name2 = TERMINAL[g2[1]]
            if name2 in term[kind2]:
                wire(g1, eid2, term[kind2][name2][0])
    return ops, count, not_imported
