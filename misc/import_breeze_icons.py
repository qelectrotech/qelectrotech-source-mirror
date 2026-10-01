#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""Copy the Breeze icons listed in misc/make_icon_themes.py into ico/breeze/.

Run from the repository root with a checkout of
https://github.com/KDE/breeze-icons:

    python3 misc/import_breeze_icons.py /path/to/breeze-icons
    python3 misc/make_icon_themes.py

For each Breeze name in BREEZE it writes the art Breeze draws at 16, 22
and 32 pixels, unchanged:

    ico/breeze/16/<name>.svg
    ico/breeze/22/<name>.svg
    ico/breeze/32/<name>.svg

Many Breeze names are symbolic links to another file. The copy holds the
linked file's content under the name BREEZE gives, so list the link's
target in BREEZE and several QET icons share one copy. Every name needs
22 pixel art; 16 and 32 are copied where Breeze has them.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_icon_themes import BREEZE, BREEZE_SIZES, ICO  # noqa: E402

CATEGORIES = ["actions", "places", "mimetypes", "status", "devices", "apps", "preferences"]
OUT = ICO / "breeze"


def find(checkout, name, size):
    for category in CATEGORIES:
        path = checkout / "icons" / category / str(size) / f"{name}.svg"
        if path.exists():
            return path.resolve()
    return None


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    checkout = Path(sys.argv[1])
    names = sorted(set(BREEZE.values()))
    wanted = set()
    for name in names:
        if not find(checkout, name, 22):
            sys.exit(f"{name}: no 22 pixel icon in {checkout}")
        for size in BREEZE_SIZES:
            source = find(checkout, name, size)
            if source:
                target = OUT / str(size) / f"{name}.svg"
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.read_bytes())
                wanted.add(target)
    # Drop icons no longer listed, so ico/breeze/ holds only what the theme uses.
    for old in sorted(OUT.glob("*/*.svg")):
        if old not in wanted:
            old.unlink()
    print(f"{len(names)} Breeze icons, {len(wanted)} files written to {OUT.relative_to(ICO.parent)}")


if __name__ == "__main__":
    main()
