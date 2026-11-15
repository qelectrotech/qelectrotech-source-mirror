# Icons

QElectroTech loads most icons by name from an icon theme, for example
`QIcon::fromTheme("document-save")`. An icon theme is a folder of icons
sorted by size, with an `index.theme` file that lists the sizes. QET
has two themes, `qet` for light windows and `qet-dark` for dark ones.
`misc/make_icon_themes.py` builds both from the source folders below.

| Folder | Contents | Source | Built by a script | License |
|---|---|---|---|---|
| `qet/traced/` | QET's icons as SVG, at 16 and 22 px, with smooth versions for larger sizes | Traced from QET's former PNG icons, or drawn by contributors | No | GPL-2.0-or-later |
| `qet/scalable/` | QET's SVG icons drawn on a 24 px canvas; `16/` holds 16 px versions | Drawn by contributors | No | GPL-2.0-or-later |
| `qet/pages/` | Settings page icons | Drawn by contributors | No | GPL-2.0-or-later |
| `breeze/` | KDE Breeze icons, in folders by pixel size | KDE Breeze | `misc/import_breeze_icons.py` | LGPL-3.0-or-later |
| `breeze/added/` | 16 px versions of Breeze icons that Breeze lacks or draws in another style | Drawn by contributors in the Breeze style | No | LGPL-3.0-or-later |
| `flags/` | Language flags | Damien Sorel's flag pack | No | Public domain |
| `colors/` | Color swatches for the element editor | Contributed in 2020 | No | GPL-2.0-or-later |
| `themes/` | The `qet` and `qet-dark` themes; `generated/` holds intermediate files | Built from the folders above | `misc/make_icon_themes.py` | Same as the source file |
| `app/hicolor/` | App and file-type icons, in folders by pixel size; installed to `share/icons/hicolor/` on Linux | Nuri, in the Breeze style | No | CC-BY-ND-3.0 |
| `app/256x256/` | QET logo for windows, the tray and the About menu; app icon | Nuri | No | CC-BY-ND-3.0 |
| `app/mac/`, `app/windows/` | App and file-type icons for macOS and Windows | Nuri | No | CC-BY-ND-3.0 |
| `app/splash.png` | Splash screen | Nuri | No | CC-BY-ND-3.0 |

`icon-themes.qrc` lists the theme files for the Qt resource system and
is written by `misc/make_icon_themes.py`. Do not edit it by hand.

`copyright` is the Debian copyright file for the whole project. It
holds the full license and credit for each icon.

## Drawing an icon

QET's icons share one look: a drawing in one ink color, with color only
where it carries a meaning. `tests/qttest/tst_qeticons.cpp` checks every
icon of the theme against three of the rules below: it uses only the
palette's colors, its outline reaches 3:1, and it loads by name. A test
cannot tell what a color means: the person who draws the icon and the
reviewer check that.

### Ink

The ink is the color of the drawing itself, its outlines and lines:
#4d4d4d. Mark it as Breeze's text color, so the dark theme can turn it
into #dcdcdc:

```svg
<style type="text/css" id="current-color-scheme">
  .ColorScheme-Text { color:#4d4d4d; }
</style>
<path class="ColorScheme-Text" fill="currentColor" d="..."/>
```

### Color

Use a color only for one of these meanings, in this value, through its
Breeze class:

| Color | Breeze class | Meaning | Example |
|---|---|---|---|
| Red, #db4b59 | `ColorScheme-NegativeText` | Removes, deletes, closes or stops something | `edit-delete` |
| Green, #239d57 | `ColorScheme-PositiveText` | Creates or adds something | `element-new` |
| Blue, #1892d2 | `ColorScheme-Highlight` | Marks the part an action works on, or the way it moves | `raise` |

White, #ffffff, may fill a shape drawn in ink or in one of these colors,
for example a check mark on a green disc.

These values reach 3:1 against both window colors with almost no margin.
Do not make them lighter or darker. `misc/import_breeze_icons.py` gives
Breeze icons these values in place of Breeze's own.

Note: an icon that shows a real wire, terminal or conductor may draw it
in its IEC 60445 color, for example light blue for neutral or
green-and-yellow for protective earth. The color must still reach 3:1.
List such an icon in `kWireIcons` in `tests/qttest/tst_qeticons.cpp`, so
the palette check accepts its wire colors.

### Contrast

An icon must stand out from the window in both themes. WCAG 2.2, success
criterion 1.4.11 (non-text contrast), asks 3:1 between a graphic and the
color next to it. The window colors are #efefef in the light theme and
#353535 in the dark one. At 24 pixels, at least half of an icon's
outline (its visible pixels next to a transparent one) must be drawn in a
color that reaches 3:1 against the window; the test allows 2.95:1 for
the rounding of a drawn pixel. The ink and the three colors above meet
it. Logos keep their own colors: WCAG does not cover logotypes.

### Size

Draw on a 24 pixel canvas and keep the art inside the middle 22 pixels.
Put lines on whole pixels, so they stay sharp in the toolbar. Such an
icon blurs at 16 pixels, in a menu; draw a 16 pixel version of it in
`qet/scalable/16/` under the same name.

### Names

Use the name of the freedesktop Icon Naming Specification when one fits,
such as `document-save` or `edit-delete`. Otherwise use a lowercase name
with hyphens, such as `folio-new`.

### Loading

Load every icon by name, with `QIcon::fromTheme()`. Only the app and
file-type icons (`app/`), the flags (`flags/`) and the color swatches
(`colors/`) load from a file: they show their own colors.

### Icons drawn before these rules

`kOffPalette` and `kLowContrast` in `tests/qttest/tst_qeticons.cpp` list
the icons that do not follow these rules yet, and `kLoadedFromFile` the
files the program still loads directly. Take an icon off its list when
you fix it; the test fails until you do.

## Adding an icon

1. Draw it as described in [Drawing an icon](#drawing-an-icon). Put the
   SVG in `qet/scalable/` and add it to `SVGS` in
   `misc/make_icon_themes.py`. For a Breeze icon, add its name to
   `BREEZE` there instead and run `misc/import_breeze_icons.py`.
2. Run `python3 misc/make_icon_themes.py`.
3. Load it with `QIcon::fromTheme("<name>")`.
4. Credit it in `copyright`.
