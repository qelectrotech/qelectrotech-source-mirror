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
"""Build QET's two icon themes from the PNG and SVG sources in ico/.

Run from the repository root:

    python3 misc/make_icon_themes.py

It writes:

    ico/themes/qet/index.theme           the light theme, all existing art
    ico/themes/qet-dark/index.theme      the dark theme, inherits "qet"
    ico/themes/qet-dark/<size>/*.png     light-ink copies of the line-art icons
    ico/themes/qet-dark/scalable/*.svg   the same for the SVG icons
    ico/icon-themes.qrc                  resource file listing both themes

The light theme does not copy any file. The .qrc aliases the existing
ico/<size>/<name>.png files into the theme layout Qt's icon loader
expects, so QIcon::fromTheme("name") finds them. The dark theme only holds
the icons that need a dark variant: black line art. Colored icons are not
touched; the dark theme inherits them from the light one.

Qt inherits by name, not by size: once a name has any file in the dark
theme, the parent theme is never consulted for that name, and a size the
dark theme lacks is served by scaling the nearest dark file. An icon that
is line art at 22 pixels and colored at 128 (the printer) would then come
out as the 22 pixel copy scaled up on a dark palette. So for every name
the dark theme holds, the .qrc also aliases the light files of the sizes
the dark theme does not have, when they read on the dark window (3:1,
measured as tests/qttest/tst_qeticons.cpp does); a light file that does
not is left out, and Qt scales the nearest dark copy as before.

An icon counts as line art when fewer than 20% of its visible pixels are
saturated. Its dark copy keeps hue and alpha and inverts lightness, scaled
so pure black becomes (220, 220, 220), the dark palette's text color.

An icon drawn as a light object is left alone even when it is not
saturated: a page sheet, a printer body, the white half of the background
swatch. Such art already reads on a dark toolbar, and inverting it would
turn the white body black (the PDF import icon, GitHub #919). The test is
the share of visible pixels that are near white: 30% or more and the icon
inherits from the light theme untouched.

Requires Pillow. Idempotent: running it twice changes nothing.
"""

import colorsys
import os
import re
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install Pillow")

ROOT = Path(__file__).resolve().parent.parent
ICO = ROOT / "ico"
THEMES = ICO / "themes"
QRC = ICO / "icon-themes.qrc"

# Fixed-size folders that hold toolbar, menu and dialog icons. The 24x16
# flags, the 22x22/color swatches and the application icons are not theme
# icons and stay on their resource paths.
SIZES = ["16x16", "22x22", "32x32", "48x48", "128x128"]

# Table entries in sources/qeticons.cpp that pair a 16 pixel file with a
# 22 pixel file of another name. The theme needs one name per icon, so
# the 22 pixel file is exposed under the 16 pixel name as well. The folio
# icons that used to be listed here are SVGs now (ico/scalable/).
ALIASES = {
    "22x22/conductor2.png": "conductor-reset",
}

# SVG icons referenced from sources/qeticons.cpp. ico/scalable/ holds the
# ones drawn for QET as vectors; one file serves every size.
SVGS = [
    "scalable/diagram.svg",
    "scalable/folio-delete.svg",
    "scalable/folio-new.svg",
    "scalable/folio-properties.svg",
    "scalable/label.svg",
    "scalable/pdf-import.svg",
    "breeze-icons/scalable/apps/hidef/edit-opacity.svg",
    "breeze-icons/scalable/apps/hidef/image-flip-horizontal-symbolic.svg",
    "breeze-icons/scalable/apps/hidef/image-flip-vertical-symbolic.svg",
    "breeze-icons/scalable/apps/hidef/draw-bezier-curves.svg",
    "breeze-icons/scalable/apps/hidef/transform-crop.svg",
    "generated/ellipse-to-bezier.svg",
    "generated/rect-to-bezier.svg",
    "generated/rect-to-polyline.svg",
]

SATURATED_FRACTION = 0.20   # at or above this an icon is "colored"
WHITE_LIGHTNESS = 0.85      # a visible pixel this light counts as white
WHITE_FRACTION = 0.30       # at or above this an icon is "light art"
INK = 220 / 255.0           # lightness of pure black after inversion
SVG_INK = "#dcdcdc"
DARK_WINDOW = (53, 53, 53)  # QET::Palette::fusionDark() window color
DARK_RATIO = 3.0            # what tst_qeticons requires of a dark theme file


def visible_pixels(image):
    src = image.load()
    return [src[x, y] for y in range(image.height) for x in range(image.width)
            if src[x, y][3] > 64]


def is_line_art(image):
    pixels = visible_pixels(image)
    if not pixels:
        return False
    saturated = 0
    for r, g, b, _ in pixels:
        _, _, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
        if s > 0.25 and max(r, g, b) > 60:
            saturated += 1
    return saturated / len(pixels) < SATURATED_FRACTION


def has_light_fill(image):
    """True when the icon is mostly white: a page, a sheet, a light body."""
    pixels = visible_pixels(image)
    if not pixels:
        return False
    white = sum(1 for r, g, b, _ in pixels
                if colorsys.rgb_to_hls(r / 255, g / 255, b / 255)[1] > WHITE_LIGHTNESS)
    return white / len(pixels) >= WHITE_FRACTION


def relative_luminance(rgb):
    def linear(c):
        c /= 255.0
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = rgb
    return 0.2126 * linear(r) + 0.7152 * linear(g) + 0.0722 * linear(b)


def reads_on_dark(image):
    """True when the icon's mean visible color reaches DARK_RATIO against
    the dark window, the test tst_qeticons applies to every dark file."""
    pixels = visible_pixels(image)
    if not pixels:
        return False
    n = len(pixels)
    mean = (sum(p[0] for p in pixels) // n, sum(p[1] for p in pixels) // n,
            sum(p[2] for p in pixels) // n)
    lighter, darker = sorted((relative_luminance(mean), relative_luminance(DARK_WINDOW)),
                             reverse=True)
    return (lighter + 0.05) / (darker + 0.05) >= DARK_RATIO


def invert_lightness(image):
    """Invert lightness, keeping hue, saturation and alpha.

    The ink in these icons is dark grey rather than black, so a plain
    inversion would leave it mid grey on a dark toolbar. The darkest
    visible pixel is taken as the ink and mapped to INK; white maps to
    black; everything between scales linearly.
    """
    src = image.load()
    darkest = 1.0
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = src[x, y]
            if a > 64:
                darkest = min(darkest, colorsys.rgb_to_hls(r / 255, g / 255, b / 255)[1])
    span = max(1.0 - darkest, 1e-6)
    out = Image.new("RGBA", image.size)
    dst = out.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = src[x, y]
            h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
            l = INK * (1.0 - (l - darkest) / span)
            l = min(max(l, 0.0), 1.0)
            r2, g2, b2 = colorsys.hls_to_rgb(h, l, s)
            dst[x, y] = (round(r2 * 255), round(g2 * 255), round(b2 * 255), a)
    return out


def write_if_changed(path, data):
    """Write text or bytes only when the content differs, so git stays quiet."""
    mode = "rb" if isinstance(data, bytes) else "r"
    if path.exists():
        with open(path, mode, **({} if mode == "rb" else {"encoding": "utf-8"})) as f:
            if f.read() == data:
                return False
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = "wb" if isinstance(data, bytes) else "w"
    with open(path, mode, **({} if mode == "wb" else {"encoding": "utf-8"})) as f:
        f.write(data)
    return True


def png_bytes(image):
    from io import BytesIO
    buf = BytesIO()
    image.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def index_theme(name, comment, dirs, inherits=None):
    lines = ["[Icon Theme]", f"Name={name}", f"Comment={comment}"]
    if inherits:
        lines.append(f"Inherits={inherits}")
    lines.append("Directories=" + ",".join(dirs))
    lines.append("")
    for d in dirs:
        lines.append(f"[{d}]")
        if d == "scalable":
            lines += ["Size=22", "Type=Scalable", "MinSize=8", "MaxSize=256"]
        else:
            lines += [f"Size={d.split('x')[0]}", "Type=Fixed"]
        lines.append("")
    return "\n".join(lines)


def main():
    light = []   # (alias, source) pairs, paths relative to ico/
    dark = []    # paths relative to ico/, files exist on disk
    changed = 0
    line_art = colored = light_art = 0

    for size in SIZES:
        folder = ICO / size
        for png in sorted(folder.glob("*.png")):
            rel = f"{size}/{png.name}"
            names = [png.stem]
            if rel in ALIASES:
                names.append(ALIASES[rel])
            image = Image.open(png).convert("RGBA")
            art = is_line_art(image)
            if art and has_light_fill(image):
                art = False
                light_art += 1
            elif art:
                line_art += 1
                dark_image = None
            else:
                colored += 1
            for name in names:
                light.append((f"themes/qet/{size}/{name}.png", rel))
                if art:
                    if dark_image is None:
                        dark_image = png_bytes(invert_lightness(image))
                    target = THEMES / "qet-dark" / size / f"{name}.png"
                    changed += write_if_changed(target, dark_image)
                    dark.append(f"themes/qet-dark/{size}/{name}.png")

    for rel in SVGS:
        src = ICO / rel
        name = src.name.replace("-symbolic", "")
        light.append((f"themes/qet/scalable/{name}", rel))
        text = src.read_text(encoding="utf-8")
        text = re.sub(r"color:#[0-9a-fA-F]{6}", f"color:{SVG_INK}", text)
        text = text.replace("currentColor", SVG_INK)
        target = THEMES / "qet-dark" / "scalable" / name
        changed += write_if_changed(target, text)
        dark.append(f"themes/qet-dark/scalable/{name}")

    # Complete each dark name with the light files of its other sizes
    # that read on a dark window (see the module docstring): aliases, no
    # copies.
    dark_sizes = {}
    for path in dark:
        size, name = path.split("/")[2:]
        dark_sizes.setdefault(name, set()).add(size)
    dark_aliases = []
    for alias, source in light:
        size, name = alias.split("/")[2:]
        if name in dark_sizes and size not in dark_sizes[name] \
                and reads_on_dark(Image.open(ICO / source).convert("RGBA")):
            dark_aliases.append((f"themes/qet-dark/{size}/{name}", source))

    # Drop dark files from an earlier run that are no longer generated, so
    # a reclassified icon falls back to the light theme instead of keeping
    # a stale copy.
    for stale in sorted((THEMES / "qet-dark").glob("*/*")):
        if stale.is_file() and stale.name != "index.theme" \
                and str(stale.relative_to(ICO)) not in dark:
            stale.unlink()
            changed += 1

    dirs = SIZES + ["scalable"]
    changed += write_if_changed(THEMES / "qet" / "index.theme",
                                index_theme("qet", "QElectroTech icons", dirs))
    changed += write_if_changed(THEMES / "qet-dark" / "index.theme",
                                index_theme("qet-dark", "QElectroTech icons for dark palettes",
                                            dirs, inherits="qet"))

    qrc = ["<!DOCTYPE RCC>", "<!-- Generated by misc/make_icon_themes.py. Do not edit. -->",
           '<RCC version="1.0">', '    <qresource prefix="/ico">',
           "        <file>themes/qet/index.theme</file>",
           "        <file>themes/qet-dark/index.theme</file>"]
    for alias, source in light:
        qrc.append(f'        <file alias="{alias}">{source}</file>')
    for path in dark:
        qrc.append(f"        <file>{path}</file>")
    for alias, source in dark_aliases:
        qrc.append(f'        <file alias="{alias}">{source}</file>')
    qrc += ["    </qresource>", "</RCC>", ""]
    changed += write_if_changed(QRC, "\n".join(qrc))

    print(f"{line_art} line-art icons, {light_art} light icons, {colored} colored icons, "
          f"{len(SVGS)} SVGs; {changed} files written or removed")


if __name__ == "__main__":
    os.chdir(ROOT)
    main()
