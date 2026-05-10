#!/usr/bin/env python3
"""
Patch QET64.nsi for GitHub Actions Windows build.

Usage:
    python3 patch_nsi.py <nsi_path> <version> <files_win_path>

Arguments:
    nsi_path      : path to QET64.nsi (modified in place)
    version       : full version string e.g. 0.100.1-r8819-abc1234_x86_64-win64
    files_win_path: absolute Windows path to nsis_root/files/
                    e.g. D:\\a\\repo\\nsis_root\\files
"""

import sys
import re


def fix_mixed_paths(nsi, files_win):
    """
    After injecting the absolute path, some sub-paths may still contain
    forward slashes (e.g. files\\elements/10_electric).
    Normalize all slashes inside quoted strings that contain our absolute path.
    """
    marker = files_win.lower()

    def fix_quoted(m):
        content = m.group(1)
        if marker in content.lower():
            content = content.replace('/', '\\')
        return '"' + content + '"'

    return re.sub(r'"([^"\r\n]*)"', fix_quoted, nsi)


def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <nsi_path> <version> <files_win_path>")
        sys.exit(1)

    nsi_path  = sys.argv[1]
    version   = sys.argv[2]
    files_win = sys.argv[3].rstrip('\\/')

    print(f"Patching : {nsi_path}")
    print(f"Version  : {version}")
    print(f"Files dir: {files_win}")

    with open(nsi_path, encoding='utf-8', errors='replace') as f:
        nsi = f.read()

    # 1. Patch SOFT_VERSION
    nsi = re.sub(
        r'!define SOFT_VERSION\s+.*',
        f'!define SOFT_VERSION "{version}"',
        nsi
    )
    print("  [1] SOFT_VERSION patched")

    # 2. Rename QElectroTech.exe -> qelectrotech.exe (literal value, not NSIS var)
    nsi = nsi.replace('/bin/QElectroTech.exe', '/bin/qelectrotech.exe')
    print("  [2] Exe name patched")

    # 3. Replace relative ./files/ paths with absolute Windows path + backslash
    sep = '\\'
    abs_files = files_win + sep
    nsi = nsi.replace('./files/', abs_files)
    nsi = nsi.replace('.\\files\\', abs_files)
    nsi = nsi.replace('.\\\\files\\\\', abs_files)
    print(f"  [3] Paths ./files/ -> {abs_files}")

    # 4. Normalize mixed slashes in injected absolute paths
    #    e.g. D:\path\files\elements/10_electric -> D:\path\files\elements\10_electric
    nsi = fix_mixed_paths(nsi, files_win)
    print("  [4] Mixed slashes normalized in absolute paths")

    with open(nsi_path, 'w', encoding='utf-8') as f:
        f.write(nsi)

    # Verifications
    version_found = re.search(r'!define SOFT_VERSION\s+"([^"]+)"', nsi)
    if version_found:
        print(f"  OK SOFT_VERSION = {version_found.group(1)}")
    else:
        print("  ERROR: SOFT_VERSION not found after patch")
        sys.exit(1)

    count = nsi.count(abs_files)
    print(f"  OK {count} occurrences of '{abs_files}'")

    mixed = re.findall(r'[A-Z]:[^"\s\r\n]*files[^"\s\r\n]*/[^"\s\r\n]*', nsi)
    if mixed:
        print(f"  WARNING: {len(mixed)} mixed paths remaining:")
        for m in mixed[:5]:
            print(f"    {m}")
    else:
        print("  OK no mixed paths remaining")


if __name__ == '__main__':
    main()
