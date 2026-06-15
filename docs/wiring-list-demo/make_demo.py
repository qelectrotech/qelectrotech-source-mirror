#!/usr/bin/env python3
"""Generate a throwaway demo project that renders wire_list_view in the GUI.

PROOF OF CONCEPT — NOT FOR PRODUCTION USE.

Copies examples/industrial.qet to examples/wire_list_demo.qet and repoints the
existing "Nomenclature" folio table at the wire_list_view, so you can open the
file and see the from-to wiring list rendered as a table (without using the new
"Add a wiring list" menu action).

Run from the repository root:
    python3 docs/wiring-list-demo/make_demo.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "examples", "industrial.qet")
DST = os.path.join(ROOT, "examples", "wire_list_demo.qet")

WIRING_QUERY = ("SELECT wire, from_point, from_folio, to_point, to_folio "
                "FROM wire_list_view ORDER BY wire")


def main() -> int:
    if not os.path.exists(SRC):
        print(f"source not found: {SRC}", file=sys.stderr)
        return 1
    with open(SRC, encoding="utf-8") as f:
        s = f.read()

    def repl(m: "re.Match") -> str:
        return "<query>" + WIRING_QUERY + "</query>" \
            if "element_nomenclature_view" in m.group(1) else m.group(0)

    s2, _ = re.subn(r"<query>(.*?)</query>", repl, s, flags=re.S)
    s2 = s2.replace('<project title="Example project"',
                    '<project title="WIRE-LIST-DEMO (proof of concept)"', 1)

    if WIRING_QUERY not in s2:
        print("no element_nomenclature_view table found to repoint", file=sys.stderr)
        return 2
    with open(DST, "w", encoding="utf-8") as f:
        f.write(s2)
    print(f"wrote {DST}")
    print("Open it in QElectroTech and go to the Nomenclature folio.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
