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

    ico/themes/qet/index.theme                  the light theme, all existing art
    ico/themes/qet-dark/index.theme             the dark theme, inherits "qet"
    ico/themes/qet-dark/<size>/*.png            light-ink copies of the line-art icons
    ico/themes/qet-dark/scalable/*.svg          the same for the SVG icons
    ico/themes/qet-dark/breeze*/                the same for the Breeze SVGs
    ico/themes/qet-dark/traced*/                the same for the traced SVGs
    ico/themes/generated/scalable-22/*.svg      QET's 24 pixel canvas cut down to its 22 pixel art
    ico/themes/generated/breeze-24/*.svg        Breeze 22 pixel art on a 24 pixel canvas
    ico/themes/generated/traced-24/*.svg        traced 22 pixel art on a 24 pixel canvas
    ico/themes/generated/traced-large-24/*.svg  the smooth 22 pixel design on that canvas
    ico/icon-themes.qrc                         resource file listing both themes

An icon listed in BREEZE comes from the KDE Breeze theme instead of its
PNG files. misc/import_breeze_icons.py copies Breeze's 16, 22 and 32
pixel art into ico/breeze/. The icon's PNG files are left out of the
theme, so Qt never picks them over the SVG.

Qt draws an SVG at the size asked for. Art drawn on a 16 pixel grid is
sharp at 16, 32, 64 and 128 pixels, where each of its pixels covers
whole screen pixels; at 24 its 1 pixel lines fall between pixels and
blur. So each size folder of the theme aliases the art whose grid
divides that size:

    16x16     16 pixel art
    22x22     22 pixel art
    scalable  22 pixel art on a 24 pixel canvas, for Fusion's toolbar
    32x32     32 pixel art, else 16 pixel art doubled
    48x48     the 24 pixel canvas doubled
    64x64     32 pixel art doubled, else 16 pixel art
    128x128   32 pixel art, else 16 pixel art

On a 2x screen Qt asks for twice the pixels and takes the scalable file
unless a folder is declared for that scale. A 16 pixel menu icon would
then be the 24 pixel canvas drawn at 32 pixels, blurred. So each size
folder has a twin with Scale=2 (16x16@2 and so on) that aliases the art
for twice the pixels, by the same rule.

Where Breeze has no 16 pixel drawing, QET adds one in
ico/breeze/added/16/, on Breeze's grid and in its colors, so every icon
has art drawn for 16 pixels. Without one, the 32 pixel art is halved,
which blurs. A drawing there also replaces a Breeze 16 pixel drawing in
another style than the 22 pixel art (transform-crop).

32 pixel art drawn in another style than the 22 pixel art (a blue
folder for a line-art one) is not used; the 16 pixel art keeps the
icon's look.

Qt takes a Fixed folder only at its exact size and the scalable file for
every other size, so a size not listed, such as the 50 pixels of the
elements panel, is still scaled.

An SVG of ico/qet/scalable/ is 22 pixel art on a 24 pixel canvas: sharp in
the toolbar and blurred at 16 pixels, in a menu. One with a 16 pixel
drawing of the same name in ico/qet/scalable/16/ (the Align icons) is
served as a Breeze icon is: the 16 pixel drawing, the 22 pixel art cut
out of the canvas, and the file itself at 24 and 48 pixels.

An icon with files in ico/qet/traced/ is QET's own PNG art redrawn as SVG.
ico/qet/traced/16 and ico/qet/traced/22 hold the PNG copied pixel by
pixel, so the icon looks as it did at those sizes. ico/qet/traced/large16
and ico/qet/traced/large22 hold the same two designs as smooth vectors,
for the larger sizes and for 2x screens. A smooth vector still has its
straight edges on the grid it was drawn on, so it follows the same rule
as the Breeze art:

    16x16     the 16 pixel copy
    22x22     the 22 pixel copy
    24x24     the 22 pixel copy on a 24 pixel canvas
    a multiple of 24 pixels (48, 96)           the smooth 22 pixel design
                                               on a 24 pixel canvas
    another multiple of 16 (32, 64, 128...)    the smooth 16 pixel design
    44 pixels and scalable                     the smooth 22 pixel design

The Scale=2 folders follow the table at twice their size. Five icons
have a 32 pixel design, used in place of both smooth vectors.

A settings page icon (PAGES) is colored art shown at 64 and 128 pixels:
one SVG in the scalable folder serves it. The printer and settings pages
use the Breeze art QET's PNGs were exported from (BREEZE_PAGES).

An SVG is recolored for the dark theme through its text color (see
dark_svg). A colored SVG, one without a text color, gets no dark copy
and is inherited from the light theme, as a colored PNG is.

The light theme does not copy any file. The .qrc aliases the existing
ico/<size>/<name>.png files into the theme layout Qt's icon loader
expects, so QIcon::fromTheme("name") finds them. The dark theme only holds
the icons that need a dark variant: black line art. Colored icons are not
touched; the dark theme inherits them from the light one.

Qt inherits by name, not by size: once a name has any file in the dark
theme, the parent theme is never consulted for that name, and a size the
dark theme lacks is served by scaling the nearest dark file. An icon that
is line art at 22 pixels and colored at 128 would then come
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
GENERATED = THEMES / "generated"

# Fixed-size folders that hold toolbar, menu and dialog icons. The 24x16
# flags, the 22x22/color swatches and the application icons are not theme
# icons and stay on their resource paths.
SIZES = ["16x16", "22x22", "32x32", "48x48", "64x64", "128x128"]

# The folder that holds the traced icons' 22 pixel art on a 24 pixel
# canvas. It has no PNGs. Breeze icons use the scalable folder for this;
# a traced icon's scalable file is its smooth vector.
CANVAS_SIZE = "24x24"

# Table entries in sources/qeticons.cpp that pair a 16 pixel file with a
# 22 pixel file of another name. The theme needs one name per icon, so
# the 22 pixel file is exposed under the 16 pixel name as well. The folio
# and conductor icons that used to be listed here are SVGs now.
ALIASES = {}

# SVG icons referenced from sources/qeticons.cpp. ico/qet/scalable/ holds the
# ones drawn for QET as vectors; one file serves every size.
SVGS = [
    "qet/scalable/align-horizontal-center.svg",
    "qet/scalable/align-horizontal-left.svg",
    "qet/scalable/align-horizontal-right.svg",
    "qet/scalable/align-vertical-bottom.svg",
    "qet/scalable/align-vertical-center.svg",
    "qet/scalable/align-vertical-top.svg",
    "qet/scalable/diagram.svg",
    "qet/scalable/draw-fillet.svg",
    "qet/scalable/folio-delete.svg",
    "qet/scalable/folio-new.svg",
    "qet/scalable/folio-properties.svg",
    "qet/scalable/label.svg",
    "qet/scalable/pdf-import.svg",
    "qet/scalable/snap-to-grid.svg",
]

# Icons served by Breeze SVGs in ico/breeze/, QET name to Breeze name.
# The Breeze name is the file a symbolic link resolves to, so QET icons
# that share art share one copy (misc/import_breeze_icons.py).
BREEZE = {
    "application-exit": "application-exit",
    "applications-development-translation": "amarok_change_language",
    "arrow-left": "go-previous",
    "arrow-right": "go-next",
    "configure": "configure",
    "dialog-cancel": "dialog-cancel",
    "dialog-ok": "dialog-ok-apply",
    "document-close": "document-close",
    "document-export": "document-export",
    "document-import": "document-import",
    "document-new": "document-new",
    "document-open": "document-open",
    "document-open-recent": "document-open-recent",
    "document-print": "document-print",
    "document-print-frame": "document-print",
    "document-save": "document-save",
    "document-save-all": "document-save-all",
    "document-save-as": "document-save-as",
    "draw-bezier-curves": "draw-bezier-curves",
    "edit-clear": "edit-clear",
    "edit-clear-locationbar-ltr": "edit-clear-locationbar-ltr",
    "edit-copy": "edit-copy",
    "edit-cut": "edit-cut",
    "edit-delete": "edit-delete",
    "edit-download": "edit-download",
    "edit-opacity": "edit-opacity",
    "edit-paste": "edit-paste",
    "edit-redo": "edit-redo",
    "edit-rename": "document-edit",
    "edit-select-all": "edit-select-all",
    "edit-select-invert": "edit-select-invert",
    "edit-select-none": "edit-select-none",
    "edit-table-cell-merge": "edit-table-cell-merge",
    "edit-table-cell-split": "edit-table-cell-split",
    "edit-table-delete-column": "edit-table-delete-column",
    "edit-table-delete-row": "edit-table-delete-row",
    "edit-table-insert-column-left": "edit-table-insert-column-left",
    "edit-table-insert-column-right": "edit-table-insert-column-right",
    "edit-table-insert-row-above": "edit-table-insert-row-above",
    "edit-table-insert-row-under": "edit-table-insert-row-under",
    "edit-undo": "edit-undo",
    "flip": "object-flip-vertical",
    "folder": "folder",
    "folder-new": "folder-new",
    "folder-open": "folder-open",
    "folder-properties": "document-properties",
    "format-text-bold": "format-text-bold",
    "format-text-italic": "format-text-italic",
    "format-text-subscript": "format-text-subscript",
    "format-text-superscript": "format-text-superscript",
    "format-text-underline": "format-text-underline",
    "go-bottom": "go-bottom",
    "go-down": "go-down",
    "go-down-double": "go-down-skip",
    "go-home": "go-home",
    "go-top": "go-top",
    "go-up": "go-up",
    "go-up-double": "go-up-skip",
    "grid": "view-grid",
    "help-contents": "help-contents",
    "help-donate": "love-amarok",
    "image-flip-horizontal": "object-flip-horizontal",
    "image-flip-vertical": "object-flip-vertical",
    "image-x-eps": "application-postscript",
    "insert-image": "insert-image",
    "item-cancel": "dialog-cancel",
    "item-copy": "edit-copy",
    "item-move": "transform-move",
    "kdenlive-show-video": "video-symbolic",
    "line": "draw-line",
    "list-add": "list-add",
    "list-remove": "paint-none",
    "masquer": "view-hidden",
    "mirror": "object-flip-horizontal",
    "object-group": "object-group",
    "object-locked": "document-encrypted",
    "object-rotate-right": "object-rotate-right",
    "object-unlocked": "document-decrypt",
    "polygon": "draw-polygon",
    "preferences-desktop-user": "preferences-desktop-user",
    "rectangle": "draw-rectangle",
    "restaurer": "view-visible",
    "select": "edit-select",
    "single_page": "snap-page",
    "start": "media-playback-start",
    "table-of-content": "gtk-index",
    "text": "insert-text",
    "transform-crop": "transform-crop",
    "transform-rotate": "transform-rotate",
    "transform-scale": "transform-scale",
    "two_pages": "view-pages-facing",
    "user-busy": "im-user-busy",
    "user-online": "im-user-online",
    "view-fullscreen": "view-fullscreen",
    "view-pim-journal": "view-calendar-journal",
    "view-refresh": "view-refresh",
    "view-restore": "view-restore",
    "view_fit_width": "zoom-fit-width",
    "window-close": "window-close",
    "window-new": "window-new",
    "zoom-draw": "zoom-fit-best",
    "zoom-in": "zoom-in",
    "zoom-original": "zoom-original",
    "zoom-out": "zoom-out",
}

# Sizes Breeze draws its action icons at, the folders of ico/breeze/.
BREEZE_SIZES = [16, 22, 32]

# Settings page icons: colored art shown at 64 and 128 pixels only, so one
# scalable SVG serves every size. QET name to Breeze name and the Breeze
# size it is copied from (ico/breeze/<size>/<name>.svg). QET's PNGs of
# these were exports of the same Breeze art.
BREEZE_PAGES = {
    "printer": ("printer", 64),
    "settings": ("systemsettings", 48),
}

# Page icons, QET name to SVG, relative to ico/.
PAGES = {name: f"breeze/{size}/{breeze}.svg" for name, (breeze, size) in BREEZE_PAGES.items()}
PAGES["configure-shortcuts"] = "qet/pages/configure-shortcuts.svg"

# The app icon's theme name. Its PNGs, one per size, stay in
# ico/breeze-icons/<size>/apps/, where the packaging scripts install them from.
APP_ICON = "qelectrotech"

# Where the generator puts the 22 pixel Breeze art moved onto a 24 pixel
# canvas (see on_24_canvas).
BREEZE_24 = GENERATED / "breeze-24"

# Icons traced from QET's PNG art: every name with a file in ico/qet/traced/16.
TRACED = sorted(svg.stem for svg in (ICO / "qet" / "traced" / "16").glob("*.svg"))

# Where the generator puts the 22 pixel art cut out of the ico/qet/scalable/
# files that have a 16 pixel drawing (see grid_entries).
SCALABLE_22 = GENERATED / "scalable-22"

# Where the generator puts the 22 pixel traced art on a 24 pixel canvas:
# the pixel copy and the smooth vector.
TRACED_24 = GENERATED / "traced-24"
TRACED_LARGE_24 = GENERATED / "traced-large-24"

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


TEXT_COLOR = re.compile(r"(\.ColorScheme-(?:Text|ButtonText)\s*\{[^}]*?color:\s*)#[0-9a-fA-F]{6}")
TEXT_CLASS = re.compile(r'class="[^"]*\bColorScheme-(?:Text|ButtonText)\b')


def dark_svg(text):
    """The dark copy of an SVG, or None when the art is colored.

    A Breeze SVG colors its parts through a stylesheet: ColorScheme-Text
    for the ink, ColorScheme-NegativeText for red and so on. Only the text
    colors change; a red delete icon stays red. A file whose stylesheet
    declares the text color but draws nothing in it is colored. An SVG
    without such a stylesheet has its currentColor replaced.
    """
    if "ColorScheme-" in text:
        if not TEXT_CLASS.search(text):
            return None
        dark, count = TEXT_COLOR.subn(rf"\g<1>{SVG_INK}", text)
        return dark if count else None
    if "currentColor" in text:
        return text.replace("currentColor", SVG_INK)
    return None


def svg_size(tag):
    """The canvas width of an <svg> tag: its viewBox, else its width."""
    box = re.search(r'\sviewBox="0 0 (\d+) \d+"', tag)
    width = re.search(r'\swidth="(\d+)(?:px)?"', tag)
    return int(box.group(1)) if box else int(width.group(1)) if width else None


def on_24_canvas(text, path):
    """The 22 pixel art moved 1 pixel right and down on a 24 pixel canvas,
    as Breeze's own 24 pixel icons are, so Fusion's 24 pixel toolbar slot
    draws it unscaled."""
    root = re.search(r"<svg\b[^>]*>", text)
    size = svg_size(root.group(0)) if root else None
    if size != 22:
        sys.exit(f"{path}: expected a 22x22 canvas")
    tag = re.sub(r'\s(viewBox|width|height)="[^"]*"', "", root.group(0))
    tag = tag[:-1] + ' viewBox="0 0 24 24">'
    body = text[root.end():]
    defs = re.match(r"\s*<defs\b.*?</defs>", body, re.S)
    head = defs.group(0) if defs else ""
    body = body[len(head):]
    end = body.rindex("</svg>")
    return (text[:root.start()] + tag + head + '\n  <g transform="translate(1,1)">'
            + body[:end] + "  </g>\n" + body[end:])


def color_classes(path):
    """The Breeze color classes (Text, Accent, NegativeText...) an SVG draws with."""
    text = (ICO / path).read_text(encoding="utf-8")
    return set(re.findall(r'class="[^"]*?\bColorScheme-(\w+)', text))


def breeze_entries(breeze_name):
    """(theme folder, source path relative to ico/) for one Breeze icon,
    following the table in the module docstring."""
    art = {size: f"breeze/{size}/{breeze_name}.svg" for size in BREEZE_SIZES
           if (ICO / "breeze" / str(size) / f"{breeze_name}.svg").exists()}
    # QET's own 16 pixel drawing, where Breeze has none or draws the icon
    # in another style at 16 (transform-crop: a crop mark, a dashed box
    # at 22). Breeze's 32 pixel art then goes too, so 32 and up keep the
    # 22 pixel look.
    added = ICO / "breeze" / "added" / "16" / f"{breeze_name}.svg"
    if added.exists():
        if 16 in art:
            art.pop(32, None)
        art[16] = str(added.relative_to(ICO))
    # Breeze sometimes draws the 32 pixel icon in another style, a blue
    # folder where the smaller ones are line art. Its color classes then
    # differ from the 22 pixel art's; the 16 pixel art keeps the look.
    if 32 in art and 16 in art and color_classes(art[32]) != color_classes(art[22]):
        del art[32]
    canvas = str((BREEZE_24 / f"{breeze_name}.svg").relative_to(ICO))
    small = art.get(16, art.get(32))
    large = art.get(32, art.get(16))
    return [("16x16", small), ("22x22", art[22]), ("scalable", canvas),
            ("32x32", large), ("48x48", canvas), ("64x64", large), ("128x128", large),
            ("16x16@2", large), ("22x22@2", art[22]), ("32x32@2", large),
            ("48x48@2", canvas), ("64x64@2", large), ("128x128@2", large)]


def without_canvas(text, path):
    """The 22 pixel art of a 24 pixel canvas file: the same drawing seen
    through a 22 pixel window, the reverse of on_24_canvas."""
    root = re.search(r"<svg\b[^>]*>", text)
    if not root or svg_size(root.group(0)) != 24:
        sys.exit(f"{path}: expected a 24x24 canvas")
    tag = re.sub(r'\s(viewBox|width|height)="[^"]*"', "", root.group(0))
    tag = tag[:-1] + ' viewBox="1 1 22 22" width="22" height="22">'
    return text[:root.start()] + tag + text[root.end():]


def grid_entries(name):
    """(theme folder, source path relative to ico/) for an ico/qet/scalable/
    icon with a 16 pixel drawing. The scalable folder is not listed; it
    holds the file already."""
    small = f"qet/scalable/16/{name}.svg"
    art = str((SCALABLE_22 / f"{name}.svg").relative_to(ICO))
    canvas = f"qet/scalable/{name}.svg"
    return [("16x16", small), ("22x22", art), ("32x32", small), ("48x48", canvas),
            ("64x64", small), ("128x128", small),
            ("16x16@2", small), ("22x22@2", art), ("32x32@2", small),
            ("48x48@2", canvas), ("64x64@2", small), ("128x128@2", small)]


def traced_entries(name):
    """(theme folder, source path relative to ico/) for one traced icon,
    following the table in the module docstring."""
    pixels = {16: f"qet/traced/16/{name}.svg", 22: f"qet/traced/22/{name}.svg",
              24: str((TRACED_24 / f"{name}.svg").relative_to(ICO))}
    small, large = f"qet/traced/large16/{name}.svg", f"qet/traced/large22/{name}.svg"
    canvas = TRACED_LARGE_24 / f"{name}.svg"
    # An icon with a 32 pixel design has no canvas copy (see main).
    canvas = str(canvas.relative_to(ICO)) if canvas.exists() else large
    entries = []
    for folder in SIZES + [CANVAS_SIZE]:
        size = int(folder.split("x")[0])
        for scale, suffix in ((1, ""), (2, "@2")):
            px = size * scale
            entries.append((folder + suffix,
                            pixels.get(px) or (canvas if px % 24 == 0 else
                                               small if px % 16 == 0 else large)))
    return entries + [("scalable", large)]


def dark_path(source):
    """Where the dark copy of a Breeze or traced SVG goes: one copy per
    source file, however many theme entries alias it."""
    if source.startswith("qet/scalable/16/"):
        return "themes/qet-dark/scalable-16/" + Path(source).name
    return "themes/qet-dark/" + source.removeprefix("themes/generated/").removeprefix("qet/")


def write_canvases(folder, sources, convert=on_24_canvas):
    """Write the 24 pixel canvas copy of each 22 pixel file in sources
    (name to path) into folder and drop the copies no longer wanted.
    Returns the number of files written or removed."""
    changed = 0
    wanted = set()
    for name, source in sorted(sources.items()):
        target = folder / f"{name}.svg"
        changed += write_if_changed(target, convert(source.read_text(encoding="utf-8"), source))
        wanted.add(target)
    for stale in sorted(folder.glob("*.svg")):
        if stale not in wanted:
            stale.unlink()
            changed += 1
    return changed


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
            if d.endswith("@2"):
                lines.append("Scale=2")
        lines.append("")
    return "\n".join(lines)


def main():
    light = []   # (alias, source) pairs, paths relative to ico/
    dark = []    # paths relative to ico/, files exist on disk
    dark_aliases = []   # (alias, source) pairs in the dark theme
    changed = 0
    line_art = colored = light_art = 0

    grid = sorted(svg.stem for svg in (ICO / "qet" / "scalable" / "16").glob("*.svg"))
    for size in SIZES:
        folder = ICO / size
        for png in sorted(folder.glob("*.png")):
            if png.stem in BREEZE or png.stem in TRACED or png.stem in grid or png.stem in PAGES:
                continue
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

    # The app icon: Nuri's PNGs, used as they are. Colored art, so the
    # dark theme inherits it.
    for size in SIZES:
        rel = f"breeze-icons/{size}/apps/{APP_ICON}.png"
        if (ICO / rel).exists():
            light.append((f"themes/qet/{size}/{APP_ICON}.png", rel))

    svgs = [((ICO / rel).name.replace("-symbolic", ""), rel) for rel in SVGS]
    svgs += [(f"{name}.svg", rel) for name, rel in PAGES.items()]
    for name, rel in svgs:
        light.append((f"themes/qet/scalable/{name}", rel))
        text = dark_svg((ICO / rel).read_text(encoding="utf-8"))
        if text is None:
            # Colored art: like a colored PNG, the dark theme inherits it.
            colored += 1
            continue
        changed += write_if_changed(THEMES / "qet-dark" / "scalable" / name, text)
        dark.append(f"themes/qet-dark/scalable/{name}")

    # Breeze and traced icons: the 24 pixel canvas copies, then one light
    # and at most one dark alias per theme folder, each to a shared file.
    overlap = sorted(set(BREEZE) & set(TRACED))
    if overlap:
        sys.exit("both a Breeze and a traced icon: " + ", ".join(overlap))
    changed += write_canvases(BREEZE_24, {name: ICO / "breeze" / "22" / f"{name}.svg"
                                          for name in set(BREEZE.values())})
    changed += write_canvases(TRACED_24, {name: ICO / "qet" / "traced" / "22" / f"{name}.svg"
                                          for name in TRACED})
    smooth = {name: ICO / "qet" / "traced" / "large22" / f"{name}.svg" for name in TRACED}
    changed += write_canvases(TRACED_LARGE_24, {
        name: path for name, path in smooth.items()
        if 'viewBox="0 0 22 22"' in path.read_text(encoding="utf-8")})
    changed += write_canvases(SCALABLE_22, {name: ICO / "qet" / "scalable" / f"{name}.svg" for name in grid},
                              without_canvas)
    entries = [(qet_name, breeze_entries(breeze_name)) for qet_name, breeze_name in BREEZE.items()]
    entries += [(name, grid_entries(name)) for name in grid]
    entries += [(name, traced_entries(name)) for name in TRACED]
    dark_files = {}   # dark copy path -> written, one per source file
    for qet_name, folders in sorted(entries):
        for folder, source in folders:
            light.append((f"themes/qet/{folder}/{qet_name}.svg", source))
            copy = dark_path(source)
            if copy not in dark_files:
                text = dark_svg((ICO / source).read_text(encoding="utf-8"))
                dark_files[copy] = text is not None
                if text is not None:
                    changed += write_if_changed(ICO / copy, text)
            if dark_files[copy]:
                dark_aliases.append((f"themes/qet-dark/{folder}/{qet_name}.svg", copy))
    dark += [copy for copy, written in dark_files.items() if written and copy not in dark]

    # Complete each dark name with the light files of its other sizes
    # that read on a dark window (see the module docstring): aliases, no
    # copies. A colored SVG counts as reading, as it does in the test.
    folders = SIZES + [CANVAS_SIZE] + [f"{d}@2" for d in SIZES + [CANVAS_SIZE]] + ["scalable"]
    dark_sizes = {}
    for path in dark + [alias for alias, _ in dark_aliases]:
        size, name = path.split("/")[2:4]
        if size in folders:
            dark_sizes.setdefault(name, set()).add(size)
    for alias, source in light:
        size, name = alias.split("/")[2:]
        if name in dark_sizes and size not in dark_sizes[name] \
                and (source.endswith(".svg")
                     or reads_on_dark(Image.open(ICO / source).convert("RGBA"))):
            dark_aliases.append((f"themes/qet-dark/{size}/{name}", source))

    # Drop dark files from an earlier run that are no longer generated, so
    # a reclassified icon falls back to the light theme instead of keeping
    # a stale copy.
    for stale in sorted((THEMES / "qet-dark").rglob("*")):
        if stale.is_file() and stale.name != "index.theme" \
                and str(stale.relative_to(ICO)) not in dark:
            stale.unlink()
            changed += 1

    changed += write_if_changed(THEMES / "qet" / "index.theme",
                                index_theme("qet", "QElectroTech icons", folders))
    changed += write_if_changed(THEMES / "qet-dark" / "index.theme",
                                index_theme("qet-dark", "QElectroTech icons for dark palettes",
                                            folders, inherits="qet"))

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
          f"{len(SVGS)} SVGs, {len(BREEZE)} Breeze icons, {len(TRACED)} traced icons; {changed} files written or removed")


if __name__ == "__main__":
    os.chdir(ROOT)
    main()
