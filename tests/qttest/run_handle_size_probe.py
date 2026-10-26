#!/usr/bin/env python3
"""Link the handle-size integration probe using an existing Ninja app build.

Reuses app objects without duplicating the app or introducing a core-library
refactor. No application source or existing executable is replaced. The
generated manifest, probe binary, reports and export fixtures stay in build/.
"""
import argparse
import pathlib
import subprocess
import sys

import ninja_probe

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True)
parser.add_argument('--binary', required=True)
parser.add_argument('--source', required=True)
parser.add_argument('--fixture', required=True)
args = parser.parse_args()
build = pathlib.Path(args.build).resolve()
binary = pathlib.Path(args.binary).resolve()
source = pathlib.Path(args.source).resolve()
fixture = pathlib.Path(args.fixture).resolve()
probe_binary = ninja_probe.build_probe(build, binary, source, 'handle-size-probe.ninja')
output = build / 'handle-size'
output.mkdir(exist_ok=True)
report = output / 'results.txt'
result = subprocess.run([str(probe_binary), str(fixture), str(report), str(output)], timeout=90)
if report.exists():
    print(report.read_text(encoding='utf-8', errors='replace'))
sys.exit(result.returncode)
