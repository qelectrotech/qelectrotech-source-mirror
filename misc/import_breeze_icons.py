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
and 32 pixels:

    ico/breeze/16/<name>.svg
    ico/breeze/22/<name>.svg
    ico/breeze/32/<name>.svg

The settings page icons in BREEZE_PAGES are copied from the one size
listed for each, to ico/breeze/<size>/<name>.svg.

Many Breeze names are symbolic links to another file. The copy holds the
linked file's content under the name BREEZE gives, so list the link's
target in BREEZE and several QET icons share one copy. Every name needs
22 pixel art; 16 and 32 are copied where Breeze has them.

Each copy takes QET's colors in place of Breeze's (QET_COLORS): the ink
and the red, green and blue of ico/README.md, "Drawing an icon".
"""

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_icon_themes import BREEZE, BREEZE_PAGES, BREEZE_SIZES, ICO  # noqa: E402

CATEGORIES = ["actions", "places", "mimetypes", "status", "devices", "apps", "preferences"]
OUT = ICO / "breeze"

# Breeze's color and the QET color that replaces it. Breeze's red, green
# and blue fall below 3:1 against one of QET's two window colors; QET's
# reach it on both. Breeze's darker ink would stand out beside QET's.
QET_COLORS = {
    "#232629": "#4d4d4d",  # ink, ColorScheme-Text
    "#da4453": "#db4b59",  # red, ColorScheme-NegativeText
    "#27ae60": "#239d57",  # green, ColorScheme-PositiveText
    "#3daee9": "#1892d2",  # blue, ColorScheme-Highlight
}


def with_qet_colors(text):
    """text with every Breeze color of QET_COLORS replaced by QET's."""
    for breeze, qet in QET_COLORS.items():
        text = re.sub(re.escape(breeze), qet, text, flags=re.IGNORECASE)
    return text


def copy(source, target):
    # Bytes in and out: the file keeps its line endings.
    target.parent.mkdir(parents=True, exist_ok=True)
    text = source.read_bytes().decode("utf-8")
    target.write_bytes(with_qet_colors(text).encode("utf-8"))


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
                copy(source, target)
                wanted.add(target)
    for name, size in BREEZE_PAGES.values():
        source = find(checkout, name, size)
        if not source:
            sys.exit(f"{name}: no {size} pixel icon in {checkout}")
        target = OUT / str(size) / f"{name}.svg"
        copy(source, target)
        wanted.add(target)
    # Drop icons no longer listed, so ico/breeze/ holds only what the theme uses.
    for old in sorted(OUT.glob("*/*.svg")):
        if old not in wanted:
            old.unlink()
    print(f"{len(names)} Breeze icons, {len(wanted)} files written to {OUT.relative_to(ICO.parent)}")


if __name__ == "__main__":
    main()
