"""Import every FluidSIM circuit below a folder, each into a project of its own, and sum up the results.

Copyright 2006-2026 The QElectroTech Team
This file is part of QElectroTech, licensed under the GNU GPL v2 or later.

A check of the import over many circuits, e.g. the samples of a FluidSIM installation (folder circ/). Per circuit
fluidsim_import.py writes OUTPUT/<name>.qet and its report; the results go to OUTPUT/results.csv (one row per
circuit) and OUTPUT/summary.json.

    python3 fluidsim_batch.py --circ ".../FluidSIM6/circ" --output FOLDER [--qet ...] [--parallel 4] [--only pneu/TP101]
    python3 fluidsim_batch.py --circ ".../FluidSIM6/circ" --check-rotation

A development aid: it lives in the source tree only and is not installed.
"""
import argparse
import csv
import json
import math
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import fluidsim_import as fi  # noqa: E402
FIELDS = ["file", "import", "failed_operations", "parts", "wires", "links", "grafcet_steps", "message"]


def one(circ: Path, root: Path, target: Path, qet):
    row = dict.fromkeys(FIELDS, "")
    row["file"] = circ.as_posix()
    stem = re.sub(r"[^\w\-]+", "_", circ.relative_to(root).with_suffix("").as_posix())
    project = target / (stem + ".qet")
    report = target / (stem + ".json")
    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    command = [sys.executable, str(HERE / "fluidsim_import.py"), str(circ), str(project), "--report", str(report)]
    if qet:
        command += ["--qet", str(qet)]
    try:
        r = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace",
                           timeout=600, env=env)
    except subprocess.TimeoutExpired:
        row.update({"import": "error", "message": "import: timed out"})
        return row
    if r.returncode != 0 or not project.exists() or not report.exists():
        last = (r.stderr or r.stdout).strip().splitlines()
        row.update({"import": "error", "message": (last[-1] if last else "")[:300]})
        return row
    b = json.loads(report.read_text(encoding="utf-8"))
    row.update({"import": "ok", "parts": b["parts"], "wires": b["wires"], "links": b["links"],
                "failed_operations": b["failed_operations"], "grafcet_steps": b["steps"],
                "message": "; ".join(b.get("not_imported", [])[:3])[:300]})
    return row


def check_rotation(folder: Path):
    """Counts for SENSE = +1 and -1 how many wires at rotated parts fit exactly (development aid)."""
    hits = {1: 0, -1: 0}
    total = 0
    for f in sorted(folder.rglob("*.circ")):
        try:
            root = fi.read(f)
        except Exception:
            continue
        graphics = fi.section(root, "symbol-graphics")
        if graphics is None:
            continue
        symbols = {s.get("ident"): s for s in graphics.findall("symbol")}
        by_id = {o.get("ident"): o for o in fi.circuit_objects(root)}
        for v in root.iter("connect"):
            a, b = v.get("con1", ""), v.get("con2", "")
            oa, _, xa = a.partition(".")
            ob, _, xb = b.partition(".")
            if oa not in by_id or ob not in by_id:
                continue
            if not (fi.fe.number(by_id[oa], "rotation") or fi.fe.number(by_id[ob], "rotation")):
                continue
            points = (v.get("points") or "").split()
            if not points:
                continue
            lx, ly = (float(z) for z in points[-1].split(","))
            total += 1
            for sense in (1, -1):
                fi.fe.Transform.SENSE = sense
                pa, pb = fi.connector_position(symbols, by_id[oa], xa), fi.connector_position(symbols, by_id[ob], xb)
                if pa and pb and math.hypot(pa[0] + lx - pb[0], pa[1] + ly - pb[1]) < 1.0:
                    hits[sense] += 1
    fi.fe.Transform.SENSE = -1
    print(f"wires at rotated parts: {total}; fitting clockwise: {hits[1]}, counter-clockwise: {hits[-1]}")
    return hits


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--circ", type=Path, required=True, help="the folder with the circuits (searched below)")
    ap.add_argument("--check-rotation", action="store_true",
                    help="only count, for both senses of rotation, the wires at rotated parts that fit exactly")
    ap.add_argument("--output", type=Path, help="where the projects and results.csv go (not for --check-rotation)")
    ap.add_argument("--qet", type=Path, default=None, help="the QElectroTech executable")
    ap.add_argument("--parallel", type=int, default=4)
    ap.add_argument("--only", default="", help="only circuits whose path contains this")
    a = ap.parse_args(argv)
    if a.check_rotation:
        check_rotation(a.circ)
        return
    if a.output is None:
        ap.error("--output is needed")
    a.output.mkdir(parents=True, exist_ok=True)
    circuits = [c for c in sorted(a.circ.rglob("*.circ")) if a.only.replace("\\", "/") in c.as_posix()]
    results = []
    with ThreadPoolExecutor(max(1, a.parallel)) as pool:
        jobs = {pool.submit(one, c, a.circ, a.output, a.qet): c for c in circuits}
        for i, f in enumerate(as_completed(jobs), 1):
            try:
                row = f.result()
            except Exception as e:      # a fault of this tool must not stop the others
                row = dict.fromkeys(FIELDS, "")
                row.update({"file": jobs[f].as_posix(), "import": "error", "message": f"batch: {e}"})
            results.append(row)
            if i % 25 == 0:
                print(f"{i}/{len(circuits)}", flush=True)
    results.sort(key=lambda z: z["file"])
    with (a.output / "results.csv").open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, FIELDS, delimiter=";")
        w.writeheader()
        w.writerows(results)
    ok = [z for z in results if z["import"] == "ok"]
    summary = {
        "circuits": len(results), "imported": len(ok),
        "without_failed_operations": sum(1 for z in ok if str(z["failed_operations"]) == "0"),
        "with_grafcet": sum(1 for z in ok if str(z["grafcet_steps"]) not in ("", "0")),
    }
    (a.output / "summary.json").write_text(json.dumps(summary, indent=1), encoding="utf-8")
    print(json.dumps(summary, indent=1))


if __name__ == "__main__":
    main()
