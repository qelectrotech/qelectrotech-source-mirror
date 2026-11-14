"""Build an integration probe from the application's own Ninja build.

The probe takes the place of sources/main.cpp in a copy of build.ninja: it is
compiled with the application's flags and linked with its objects, under its
own name, and no application source or existing executable is replaced. The
copy of the manifest stays in the build directory.

Ninja writes paths with / on Unix and with \\ on Windows (MSVC and MinGW), so
both are matched.
"""
import contextlib
import os
import pathlib
import re
import shutil
import subprocess
import sys


def timeout(seconds):
    """A time limit of a probe, multiplied by QET_TEST_TIMEOUT_SCALE when it
    is set, as the C++ tests do (qettesttimeout.h)."""
    try:
        scale = int(os.environ.get('QET_TEST_TIMEOUT_SCALE', ''))
    except ValueError:
        scale = 1
    return seconds * scale if scale > 1 else seconds


def _escape(path):
    """A path as Ninja writes it: $, : and space escaped with $."""
    return path.replace('$', '$$').replace(':', '$:').replace(' ', '$ ')


def build_probe(build, binary, source, manifest_name):
    """Build the probe `source` (a .cpp) in place of main.cpp; return its path.

    Every probe runs Ninja in the application's build folder, and two Ninja
    runs there at once would write its log and dependency files together.
    Only this build step is locked: the probes themselves run side by side.
    """
    with _build_lock(build):
        return _build_probe(build, binary, source, manifest_name)


def _build_probe(build, binary, source, manifest_name):
    manifest = (build / 'build.ninja').read_text(encoding='utf-8')
    match = re.search(r'^build (\S*?[/\\]sources[/\\]main\.cpp\.(?:obj|o)):',
                      manifest, re.MULTILINE)
    if not match:
        sys.exit('Cannot find the application main object in the Ninja build.')
    name = source.stem
    main_object = match.group(1)
    probe_object = main_object.replace('main.cpp.', name + '.cpp.')
    probe_binary = binary.with_name(name + binary.suffix)
    main_source = source.parents[2] / 'sources' / 'main.cpp'
    manifest = manifest.replace(main_object, probe_object)
    for old, new in {(main_source.as_posix(), source.as_posix()),
                     (str(main_source), str(source))}:
        manifest = manifest.replace(_escape(old), _escape(new))
    if binary.suffix:
        # qelectrotech.exe and qelectrotech.pdb, but not the manifest
        # qelectrotech.exe.manifest the MSVC link reads.
        manifest = re.sub(r'(?<![\w.])' + re.escape(binary.stem) + r'\.(exe|pdb)(?![\w.])',
                          name + r'.\1', manifest)
    else:
        old_target = _escape(binary.name if binary.parent == build else binary.as_posix())
        new_target = _escape(probe_binary.name if probe_binary.parent == build
                             else probe_binary.as_posix())
        manifest = manifest.replace('build ' + old_target + ':', 'build ' + new_target + ':')
        old_file = binary.name if binary.parent == build else binary.as_posix()
        new_file = probe_binary.name if probe_binary.parent == build else probe_binary.as_posix()
        manifest = re.sub(r'^  TARGET_FILE = ' + re.escape(old_file) + r'$',
                          '  TARGET_FILE = ' + new_file, manifest, flags=re.MULTILINE)
    probe_manifest = build / manifest_name
    probe_manifest.write_text(manifest, encoding='utf-8')
    ninja = shutil.which('ninja')
    if not ninja:
        sys.exit('Ninja is required for the integration probe.')
    target = probe_binary.name if probe_binary.parent == build else str(probe_binary)
    subprocess.run([ninja, '-C', str(build), '-f', probe_manifest.name, target], check=True)
    return probe_binary


@contextlib.contextmanager
def _build_lock(build):
    """Hold an exclusive lock on build/probe-ninja.lock until the block ends."""
    with open(build / 'probe-ninja.lock', 'a+b') as handle:
        if os.name == 'nt':
            import msvcrt
            while True:
                handle.seek(0)
                try:
                    msvcrt.locking(handle.fileno(), msvcrt.LK_LOCK, 1)
                    break
                except OSError:
                    continue  # LK_LOCK gives up after ten one-second tries
            try:
                yield
            finally:
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
        else:
            import fcntl
            fcntl.flock(handle, fcntl.LOCK_EX)
            try:
                yield
            finally:
                fcntl.flock(handle, fcntl.LOCK_UN)
