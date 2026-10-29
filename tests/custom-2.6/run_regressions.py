#!/usr/bin/env python3
"""Run the supplied Custom 2.6 Windows UI suites against a rebuilt application.

This runner does not install dependencies, download files or build QElectroTech.
See docs/custom-2.6/BUILD_AND_TEST.md for the required binary/runtime layout.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-dir', type=Path, required=True)
    parser.add_argument('--qt-bin', type=Path)
    parser.add_argument('--platform', default='offscreen', choices=['offscreen', 'windows'])
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('These latest UI suites require the Windows PDF runtime layout.')
    source = Path(__file__).resolve().parents[2]
    binary = args.bin_dir.resolve()
    suites = ['rs25_tests', 'rs20_tests', 'rs26_tests', 'rs14_tests']
    required = [binary / (name + '.exe') for name in suites]
    required += [binary / 'pdf-runtime/python.exe']
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        parser.error('Build the suites and prepare the runtime first. Missing: ' + ', '.join(missing))
    if args.output_dir:
        output = args.output_dir.resolve()
        if output.exists() and any(output.iterdir()):
            parser.error('--output-dir must be empty, to preserve prior results.')
        output.mkdir(parents=True, exist_ok=True)
    else:
        output = Path(tempfile.mkdtemp(prefix='custom-2.6-checks-', dir=binary))
    (binary / 'tools').mkdir(exist_ok=True)
    shutil.copy2(source / 'tools/pdf_references25.py', binary / 'tools/pdf_references25.py')
    fixtures = Path(__file__).resolve().parent / 'fixtures'
    for name in ['Relay_input_2.6.qetz', 'Bilingual_headers_2.5.qetz', 'PDF_import_fixture.pdf']:
        shutil.copy2(fixtures / name, output / name)
    env = os.environ.copy()
    paths = [str(binary)]
    if args.qt_bin:
        paths.append(str(args.qt_bin.resolve()))
    paths.append(env.get('PATH', ''))
    env['PATH'] = os.pathsep.join(paths)
    env['QT_QPA_PLATFORM'] = args.platform
    for variable, directory in [('QET_SETTINGS_DIR', 'settings'), ('APPDATA', 'appdata'),
                                ('LOCALAPPDATA', 'localappdata'), ('TEMP', 'temp'), ('TMP', 'temp')]:
        destination = output / directory
        destination.mkdir(exist_ok=True)
        env[variable] = str(destination)
    print('Results:', output, flush=True)
    for name in suites:
        log_path = output / (name + '.log')
        with log_path.open('w', encoding='utf-8') as log:
            result = subprocess.run([str(binary / (name + '.exe')), str(output), str(source)],
                                    cwd=source, env=env, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=300)
        print(name, 'exit', result.returncode, '-', log_path, flush=True)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
