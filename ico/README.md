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
| `breeze-icons/` | App and file-type icons in the Breeze style | Nuri | No | CC-BY-ND-3.0 |
| `256x256/` | QET logo for windows, the tray and the About menu; app icon | Nuri | No | CC-BY-ND-3.0 |
| `mac_icon/`, `windows_icon/` | App and file-type icons for macOS and Windows | Nuri | No | CC-BY-ND-3.0 |
| `splash.png` | Splash screen | Nuri | No | CC-BY-ND-3.0 |

`icon-themes.qrc` lists the theme files for the Qt resource system and
is written by `misc/make_icon_themes.py`. Do not edit it by hand.

`copyright` is the Debian copyright file for the whole project. It
holds the full license and credit for each icon.

## Adding an icon

1. Put the SVG in `qet/scalable/` and add it to `SVGS` in
   `misc/make_icon_themes.py`. For a Breeze icon, add its name to
   `BREEZE` there instead and run `misc/import_breeze_icons.py`.
2. Run `python3 misc/make_icon_themes.py`.
3. Load it with `QIcon::fromTheme("<name>")`.
4. Credit it in `copyright`.
