"""Optional mapping of sPlan parts to elements of a collection.

sPlan knows no articles: a part is a group of strokes with a library name
("CPU", "power supply"). Which device it really is, is printed on the
symbol or only shown in a picture, and the library name may even be wrong.
That decision is left to a person, with a reason, in a mapping file (JSON).
The import then translates it like a compiler: the same schematic and the
same mapping give the same project. --mapping-template writes the skeleton
with every device, its printed texts and its terminals.

Without a mapping file nothing changes: every part is drawn with a standard
symbol of the common collection or as a device box.

The file:
{
  "paths":     {"PLC": "custom://plc/cpu_"},           # abbreviations
  "templates": {"CPU": { ...like a device, without "when"... }},
  "devices": [
    {"when": {"sheet": 6, "tag": "A2"},        # everything given must match; no sheet:
                                               # any sheet. Also "symbol" (library name),
                                               # "value", "text" (printed on the symbol)
     "template": "CPU",                        # optional, own entries override it
     "manufacturer": "...", "reference": "...", "type": "...",
     "evidence": "why this article",           # listed in open_questions.txt
     "standard": true,                         # keep the standard symbol, set the data only
     "channels": {                             # else: an element per sPlan terminal
        ".0": {"element": "PLC:DI_1", "terminal": "DI", "address": "%I0.0"},
        "L+": {"element": "PLC:supply", "terminal": "L1", "group": "supply"},  # one group,
                                                                                # one element
        "1.2": {"skip": true, "reason": "network cable, no terminal"}}}
  ]
}
"""
import json
import os
import re


class Rule:
    def __init__(self, d, paths):
        self.d, self.paths = d, paths
        self.when = d.get("when", {})
        self.manufacturer, self.reference = d.get("manufacturer", ""), d.get("reference", "")
        self.type, self.evidence = d.get("type", ""), d.get("evidence", "")
        self.standard = bool(d.get("standard"))
        self.channels = d.get("channels") or {}
        self.used = 0

    def path(self, short):
        """"PLC:DI_1" -> custom://plc/cpu_DI_1.elmt"""
        if "://" not in short and ":" in short:
            k, rest = short.split(":", 1)
            if k in self.paths:
                short = self.paths[k] + rest
        return short if short.endswith(".elmt") else short + ".elmt"

    def info(self):
        i = {}
        if self.manufacturer:
            i["manufacturer"] = self.manufacturer
        if self.reference:
            i["manufacturer_reference"] = self.reference
        if self.type:
            i["designation"] = self.type
        return i

    def matches(self, part, sheet):
        w = self.when
        if "sheet" in w and int(w["sheet"]) != sheet:
            return False
        if "tag" in w and w["tag"] != part.tag:
            return False
        if "symbol" in w and w["symbol"].lower() != (part.symbol or "").lower():
            return False
        if "value" in w and w["value"].lower() != (part.value or "").lower():
            return False
        if "text" in w and not any(w["text"].lower() in t.lower() for t, _ in part.texts):
            return False
        return bool(w)


class Mapping:
    def __init__(self, path=None):
        self.rules, self.source = [], path
        if not path:
            return
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        paths = data.get("paths", {})
        templates = data.get("templates", {})
        for d in data.get("devices", []):
            if "template" in d:
                base = dict(templates[d["template"]])
                channels = dict(base.get("channels") or {})
                channels.update(d.get("channels") or {})  # the device adds or overrides terminals
                base.update(d)
                base["channels"] = channels
                d = base
            self.rules.append(Rule(d, paths))

    def find(self, part, sheet):
        for r in self.rules:
            if r.matches(part, sheet):
                r.used += 1
                return r
        return None

    def unused(self):
        return [r for r in self.rules if not r.used]


def groups(part, rule, questions, number):
    """Distributes the sPlan terminals of the part over elements.
    Returns [(element path, [(terminal, element terminal, address)])] in the
    order of the mapping file, and the terminals without an entry (they stay
    in a box of their own, so that no connection is lost)."""
    by_name = {}
    for t in part.terminals:
        by_name.setdefault(t.name, []).append(t)
    grouped, order = {}, []
    for name, k in rule.channels.items():
        for t in by_name.pop(name, []):
            if k.get("skip"):
                questions.append(f"Sheet {number}: {part.tag}:{name} not drawn: "
                                 f"{k.get('reason', 'as the mapping says')}.")
                continue
            key = (rule.path(k["element"]), k.get("group") or f"_{name}_{id(t)}")
            if key not in grouped:
                grouped[key] = []
                order.append(key)
            grouped[key].append((t, k["terminal"], k.get("address", "")))
    rest = [t for ts in by_name.values() for t in ts]
    return [(key[0], grouped[key]) for key in order], rest


def write_template(path, contents):
    """The skeleton of a mapping file: every device with its library name,
    value, printed texts and terminals with where they lead."""
    devices = []
    for number, c in contents:
        for part in c.parts:
            if not part.terminals:
                continue
            names = {t.name for t in part.terminals}
            devices.append({
                "when": ({"sheet": number, "tag": part.tag} if part.tag
                         else {"sheet": number, "symbol": part.symbol}),
                "_sPlan": {"symbol": part.symbol, "value": part.value,
                           "printed": [t for t, _ in part.texts if t not in names][:40],
                           "terminals": {t.name: (t.net.name or ", ".join(
                               str(u) for u in t.net.terminals if u is not t)) if t.net else ""
                               for t in part.terminals}},
                "manufacturer": "", "reference": "", "type": "", "evidence": "",
                "channels": {}})
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"paths": {}, "templates": {}, "devices": devices}, f, ensure_ascii=False,
                  indent=1)
    return path


def default_path(target_qet):
    return re.sub(r"\.qet$", "", target_qet) + "_mapping.json"


def load_for(target_qet, explicit=None):
    p = explicit or default_path(target_qet)
    return Mapping(p if os.path.exists(p) else None)
