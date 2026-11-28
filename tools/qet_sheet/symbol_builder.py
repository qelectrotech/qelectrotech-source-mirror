"""Elements the importers write themselves into the user collection.

- device_box(): a labelled box with the terminals of a device that has no
  matching standard symbol, terminals on top and at the bottom;
- support(): the small helpers a redrawn sheet needs and the common
  collection does not have: vertical cross-reference arrows, the T-junctions
  that join a device to a potential rail, and a corner for routed wires.

Both go below custom://<IMPORT_FOLDER>/ (QET_CUSTOM_ELEMENTS_DIR), from where
QElectroTech embeds them into the project like any element.
"""
import os
import uuid
from xml.sax.saxutils import escape

from . import symbols as S

IMPORT_FOLDER = "Imported"
STYLE = 'style="line-style:normal;line-weight:normal;filling:none;color:black" antialias="false"'
_NS = uuid.UUID("6f1b8a52-3c1e-4d0b-9a57-2f6c1e0d4b11")


def _write(rel, xml, overwrite=False):
    """Writes custom://IMPORT_FOLDER/rel unless it exists; returns its path."""
    if not S.CUSTOM:
        raise FileNotFoundError("the user collection is not set (QET_CUSTOM_ELEMENTS_DIR)")
    rel = f"{IMPORT_FOLDER}/{rel}"
    target = os.path.join(S.CUSTOM, *rel.split("/"))
    if overwrite or not os.path.exists(target):
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "w", encoding="utf-8", newline="\n") as f:
            f.write(xml)
    return "custom://" + rel


def _uuid(*parts):
    """A stable uuid: the same element written twice keeps its terminals."""
    return "{" + str(uuid.uuid5(_NS, "/".join(parts))) + "}"


def _label(x, y, key):
    return (f'<dynamic_text x="{x}" y="{y}" z="2" text_width="-1" Halignment="AlignLeft" '
            'Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="false" '
            f'text_from="ElementInfo" uuid="{_uuid(key, "label")}" '
            'font="Liberation Sans,10,-1,5,50,0,0,0,0,0,Regular">'
            '<text></text><info_name>label</info_name></dynamic_text>')


def _text(x, y, text, size=6, rotation=0):
    return (f'<text x="{x}" y="{y}" size="{size}" text="{escape(text)}" rotation="{rotation}" '
            f'color="#000000" font="Liberation Sans,{size},-1,5,50,0,0,0,0,0,Regular"/>')


def _definition(key, name, width, height, hotspot_x, hotspot_y, body, link_type="simple",
                infos=""):
    return (f'<definition version="0.100.0" type="element" link_type="{link_type}" '
            f'width="{width}" height="{height}" hotspot_x="{hotspot_x}" hotspot_y="{hotspot_y}">\n'
            f'<uuid uuid="{_uuid(key)}"/>\n<names><name lang="en">{escape(name)}</name></names>\n'
            f'<elementInformations>{infos}<elementInformation name="label" show="1">'
            '</elementInformation></elementInformations>\n'
            '<informations>Written by the QElectroTech importers (tools/qet_sheet)</informations>\n'
            f'<description>{body}</description>\n</definition>\n')


def device_box(rel, kind, top, bottom, manufacturer="", reference="", description="", pitch=30,
               overwrite=False):
    """A device box with terminals on top and at the bottom.

    top/bottom: [(name, meaning)]; the terminals are `pitch` apart from
    x = pitch on. Returns the custom:// path; an existing file is kept (its
    terminal uuids are in projects already drawn) unless overwrite."""
    n = max(len(top), len(bottom), 1)
    width = ((n + 1) * pitch + 9) // 10 * 10
    height = 80
    body = [f'<rect x="0" y="0" width="{width}" height="{height}" {STYLE}/>',
            _text(6, height // 2 + 4, kind, 8)]
    for side, y, orientation in ((top, 0, "n"), (bottom, height, "s")):
        for i, (name, meaning) in enumerate(side):
            x = pitch * (i + 1)
            body.append(f'<terminal uuid="{_uuid(rel, orientation, str(i), name)}" '
                        f'name="{escape(name)}" x="{x}" y="{y}" orientation="{orientation}" '
                        'type="Generic"/>')
            ty, tm = (y + 10, y + 18) if orientation == "n" else (y - 12, y - 4)
            body.append(_text(x + 3, ty, name, 6))
            if meaning:
                body.append(_text(x + 3, tm, meaning, 5))
    infos = "".join(f'<elementInformation name="{k}" show="1">{escape(v)}</elementInformation>'
                    for k, v in (("manufacturer", manufacturer),
                                 ("manufacturer_reference", reference),
                                 ("designation", reference), ("description", description)) if v)
    xml = _definition(rel, kind, width + 20, height + 40, 10, 30,
                      _label(0, -30, rel) + "".join(body), infos=infos)
    return _write(rel, xml, overwrite)


def _terminal(key, name, x, y, orientation):
    return (f'<terminal uuid="{_uuid(key, name)}" name="{name}" x="{x}" y="{y}" '
            f'orientation="{orientation}" type="Generic"/>')


def _line(x1, y1, x2, y2):
    return (f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" end1="none" end2="none" '
            f'length1="1.5" length2="1.5" {STYLE}/>')


def _arrow_label(key, x, y, align):
    return (f'<dynamic_text x="{x}" y="{y}" z="3" text_width="-1" Halignment="{align}" '
            'Valignment="AlignTop" frame="false" rotation="270" text_from="ElementInfo" '
            f'uuid="{_uuid(key, "label")}" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">'
            '<text></text><info_name>label</info_name></dynamic_text>')


_FILLED = 'style="line-style:normal;line-weight:normal;filling:black;color:black" antialias="true"'


def _support_xml(name):
    key = "support/" + name
    if name == "coming_arrow_vertical":
        # potential coming from above; the cross reference stands upright
        # above the tip, so arrows close to each other stay readable
        body = ('<polygon x1="-4" y1="-1" x2="4" y2="-1" x3="0" y3="9" ' + _FILLED + '/>'
                + _line(0, 9, 0, 12) + _arrow_label(key, -7, -3, "AlignLeft")
                + _terminal(key, "1", 0, 13, "s"))
        return _definition(key, "Coming arrow, vertical (from above)", 40, 30, 30, 10, body,
                           "previous_report")
    if name == "going_arrow_vertical":
        body = ('<polygon x1="-4" y1="-9" x2="4" y2="-9" x3="0" y3="2" ' + _FILLED + '/>'
                + _line(0, -9, 0, -12) + _arrow_label(key, -7, 4, "AlignRight")
                + _terminal(key, "1", 0, -13, "n"))
        return _definition(key, "Going arrow, vertical (downwards)", 30, 80, 15, 20, body,
                           "next_report")
    if name == "tap_down":
        # a T-junction on a rail (W-E), the branch going down (S)
        body = (_line(0, 0, 15, 0) + _line(0, 0, 7.5, 7.5)
                + _terminal(key, "W", 0, 0, "w") + _terminal(key, "E", 14.9, 0, "e")
                + _terminal(key, "S", 7.5, 7.5, "s"))
        return _definition(key, "Tap on a rail, downwards", 30, 30, 10, 20, body)
    if name == "tap_up":
        body = (_line(0, -7.5, 0, 0) + _line(-7.5, 0, 0, 0) + _line(0, -7.5, 7.5, 0)
                + _terminal(key, "N", 0, -7.4, "n") + _terminal(key, "W", -7.5, 0, "w")
                + _terminal(key, "E", 7.5, 0, "e"))
        return _definition(key, "Tap on a rail, upwards", 20, 40, 10, 30, body)
    if name == "corner":
        # a bend of a routed wire, with no connection dot: four terminals on
        # one point, so the wire can come in and leave in any direction
        body = (_terminal(key, "N", 0, 0, "n") + _terminal(key, "W", -0.1, 0.1, "w")
                + _terminal(key, "E", 0.1, 0.1, "e") + _terminal(key, "S", 0, 0.2, "s"))
        return _definition(key, "Wire corner", 10, 10, 5, 5, body)
    raise KeyError(name)


SUPPORT = ("coming_arrow_vertical", "going_arrow_vertical", "tap_down", "tap_up", "corner")


def support(name):
    """custom:// path of a support element, written on first use."""
    return _write(f"support/{name}.elmt", _support_xml(name))
