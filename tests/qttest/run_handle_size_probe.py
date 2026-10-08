#!/usr/bin/env python3
"""Link the handle-size integration probe using an existing Ninja app build.

Reuses app objects without duplicating the app or introducing a core-library
refactor. No application source or existing executable is replaced. The
generated manifest, probe binary, reports and export fixtures stay in build/.
"""
import argparse
import pathlib
import re
import shutil
import subprocess
import sys

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
manifest = (build / 'build.ninja').read_text(encoding='utf-8')
match = re.search(r'^build (\S*?/sources/main\.cpp\.(?:obj|o)):', manifest, re.MULTILINE)
if not match:
    sys.exit('Cannot find the application main object in the Ninja build.')
main_object = match.group(1)
probe_object = main_object.replace('main.cpp.', 'handle_size_probe.cpp.')
probe_binary = binary.with_name('handle_size_probe' + binary.suffix)
def escape(path):
    return pathlib.Path(path).as_posix().replace('$', '$$').replace(':', '$:').replace(' ', '$ ')
main_source = source.parents[2] / 'sources' / 'main.cpp'
manifest = manifest.replace(main_object, probe_object)
manifest = manifest.replace(escape(main_source), escape(source))
if binary.suffix:
    manifest = manifest.replace(binary.name, probe_binary.name)
else:
    old_target = escape(binary.name if binary.parent == build else binary)
    new_target = escape(probe_binary.name if probe_binary.parent == build else probe_binary)
    manifest = manifest.replace('build ' + old_target + ':', 'build ' + new_target + ':')
    old_file = binary.name if binary.parent == build else binary.as_posix()
    new_file = probe_binary.name if probe_binary.parent == build else probe_binary.as_posix()
    manifest = re.sub(r'^  TARGET_FILE = ' + re.escape(old_file) + r'$',
                      '  TARGET_FILE = ' + new_file, manifest, flags=re.MULTILINE)
probe_manifest = build / 'handle-size-probe.ninja'
probe_manifest.write_text(manifest, encoding='utf-8')
ninja = shutil.which('ninja')
if not ninja:
    sys.exit('Ninja is required for the integration probe.')
target = probe_binary.name if probe_binary.parent == build else str(probe_binary)
subprocess.run([ninja, '-C', str(build), '-f', probe_manifest.name, target], check=True)
output = build / 'handle-size'
output.mkdir(exist_ok=True)
report = output / 'results.txt'
result = subprocess.run([str(probe_binary), str(fixture), str(report), str(output)], timeout=90)
if report.exists():
    print(report.read_text(encoding='utf-8', errors='replace'))
sys.exit(result.returncode)
