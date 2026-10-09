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
"""
mirror_source_language.py — give every source-language string its own
translation in the source language's .ts file.

    python3 misc/i18n/mirror_source_language.py lang/qet_en.ts
    python3 misc/i18n/mirror_source_language.py --check lang/qet_en.ts

WHY THIS EXISTS

The strings in the code are English (French until #1390), and they are
both the translation key and the text the English UI shows. An empty
entry in lang/qet_en.ts falls back to the code text at run time, so the
source language never strictly needs a translation. The price is that an
English wording fix in the code is a key change, which orphans the
translation of that string in every other language file.

With every source string mirrored into its own .ts file, the source
language is served from its .qm like any other language, and its wording
can be corrected in the .ts alone. The key in the code then only has to
stay stable.

Only the source language's own file may be mirrored: a run on any other
file would copy English into it. The script therefore refuses a file
whose <TS> header names a language different from its sourcelanguage
(lang/qet_fr.ts says language="fr_FR" sourcelanguage="en").

WHAT IT DOES

For every message whose translation is empty, the source text is copied
into the translation and the entry is finished. Nothing else changes:

  - a translation with text is never touched, whether it is a French
    rendering of a source in another language, identical to the source, or marked
    unfinished (a translator's work in progress), with one exception:
    an unfinished translation identical to its source is finished.
    That is what lupdate's same-text heuristic leaves behind when a new
    string repeats an existing one, and it is what a mirror run would
    have written anyway;
  - vanished and obsolete entries are left to lupdate;
  - in a plural message only the empty <numerusform>s are filled;
  - the file stays byte-identical outside the rewritten <translation>
    elements: header, locations, comments, indentation, escaping.

The source text is copied as lupdate wrote it, escaping included, so
&apos; stays &apos;. A <source> with a child element (lupdate's <byte/>)
is refused rather than guessed at.

Run it after `cmake --build . --target update_translations`, before
committing. Forgetting it breaks nothing: an empty entry still falls
back to the code text, and the next run fills it. `--check` exits 1
when a run would change the file, so it can gate a commit.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
import xml.etree.ElementTree as ET

MESSAGE_RE = re.compile(r"<message(?P<attrs>[^>]*)>(?P<body>.*?)</message>", re.S)
SOURCE_RE = re.compile(r"<source>(?P<text>.*?)</source>", re.S)
TRANSLATION_RE = re.compile(
    r"<translation(?P<attrs>[^>]*)>(?P<body>.*?)</translation>", re.S)
FORM_RE = re.compile(r"<numerusform>(?P<text>.*?)</numerusform>", re.S)
TYPE_RE = re.compile(r'\s*type="(?P<type>[^"]*)"')

# lupdate's own layout: <translation> at 8 spaces, <numerusform> at 12.
FORM_INDENT = " " * 12
CLOSE_INDENT = " " * 8


class SourceError(ValueError):
    """A <source> that cannot be copied verbatim."""


class LanguageError(ValueError):
    """The file is a translation into another language, not a mirror."""


TS_HEADER_RE = re.compile(r"<TS\b[^>]*>")
ATTR_RE = r'\b{}="([^"]*)"'


def _base_language(code: str) -> str:
    return code.replace("-", "_").split("_")[0].lower()


def check_languages(text: str) -> None:
    """Refuse a .ts whose language is not its source language.

    A file without a sourcelanguage attribute is accepted: lupdate writes
    one only when it knows the source language, and the fixtures and old
    files have none.
    """
    header = TS_HEADER_RE.search(text)
    if header is None:
        return
    lang = re.search(ATTR_RE.format("language"), header.group(0))
    source = re.search(ATTR_RE.format("sourcelanguage"), header.group(0))
    if lang is None or source is None:
        return
    if _base_language(lang.group(1)) != _base_language(source.group(1)):
        raise LanguageError(
            f'the file is a translation into "{lang.group(1)}" of '
            f'"{source.group(1)}" text; only the source language\'s own '
            "file can be mirrored")


@dataclass
class Stats:
    mirrored_messages: int = 0
    mirrored_forms: int = 0
    finished_identical: int = 0
    unfinished_cleared: int = 0
    kept_translated: int = 0
    kept_identical: int = 0
    kept_vanished: int = 0
    kept_unfinished_with_text: int = 0

    def changed(self) -> bool:
        return self.mirrored_messages > 0 or self.finished_identical > 0

    def summary(self) -> str:
        return (
            f"mirrored {self.mirrored_messages} message(s), "
            f"{self.mirrored_forms} plural form(s), "
            f"finished {self.finished_identical} identical unfinished, "
            f"cleared {self.unfinished_cleared} unfinished flag(s); kept "
            f"{self.kept_translated} translated, {self.kept_identical} identical, "
            f"{self.kept_vanished} vanished, "
            f"{self.kept_unfinished_with_text} unfinished with text")


def _translation_type(attrs: str) -> str:
    m = TYPE_RE.search(attrs)
    return m.group("type") if m else ""


def _finish(whole: str, body: str, tm: re.Match, tattrs: str,
            stats: Stats) -> str:
    """Drop the unfinished flag of a translation equal to its source."""
    stats.finished_identical += 1
    stats.unfinished_cleared += 1
    start = whole.index(body) + tm.start()
    old = tm.group(0)
    new = old.replace(f"<translation{tattrs}>",
                      f"<translation{TYPE_RE.sub('', tattrs)}>", 1)
    return whole[:start] + new + whole[start + len(old):]


def _mirror_message(match: re.Match, forms: int, stats: Stats) -> str:
    whole = match.group(0)
    attrs, body = match.group("attrs"), match.group("body")
    numerus = 'numerus="yes"' in attrs
    sm = SOURCE_RE.search(body)
    tm = TRANSLATION_RE.search(body)
    if sm is None or tm is None:
        return whole
    source = sm.group("text")
    if "<" in source:
        raise SourceError(
            "a <source> contains a child element and cannot be copied: "
            + source[:60])
    tattrs, tbody = tm.group("attrs"), tm.group("body")
    ttype = _translation_type(tattrs)
    if ttype in ("vanished", "obsolete"):
        stats.kept_vanished += 1
        return whole

    if numerus:
        found = FORM_RE.findall(tbody)
        empty = [f for f in found if f == ""]
        if found and not empty:
            if ttype == "unfinished" and all(f == source for f in found):
                return _finish(whole, body, tm, tattrs, stats)
            if ttype == "unfinished":
                stats.kept_unfinished_with_text += 1
            elif all(f == source for f in found):
                stats.kept_identical += 1
            else:
                stats.kept_translated += 1
            return whole
        if found:
            new_body = FORM_RE.sub(
                lambda f: f.group(0) if f.group("text") != ""
                else f"<numerusform>{source}</numerusform>", tbody)
            stats.mirrored_forms += len(empty)
        else:
            new_body = "\n" + "".join(
                f"{FORM_INDENT}<numerusform>{source}</numerusform>\n"
                for _ in range(forms)) + CLOSE_INDENT
            stats.mirrored_forms += forms
    else:
        if tbody != "":
            if ttype == "unfinished" and tbody == source:
                return _finish(whole, body, tm, tattrs, stats)
            if ttype == "unfinished":
                stats.kept_unfinished_with_text += 1
            elif tbody == source:
                stats.kept_identical += 1
            else:
                stats.kept_translated += 1
            return whole
        new_body = source

    stats.mirrored_messages += 1
    if ttype == "unfinished":
        stats.unfinished_cleared += 1
    kept_attrs = TYPE_RE.sub("", tattrs)
    new_translation = f"<translation{kept_attrs}>{new_body}</translation>"
    new_message_body = body[:tm.start()] + new_translation + body[tm.end():]
    return f"<message{attrs}>{new_message_body}</message>"


def mirror_text(text: str, forms: int = 2) -> tuple[str, Stats]:
    """Return the mirrored .ts text and what was done to it."""
    check_languages(text)
    stats = Stats()
    new = MESSAGE_RE.sub(lambda m: _mirror_message(m, forms, stats), text)
    return new, stats


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Copy each empty translation's source text into the "
                    "translation of a source-language .ts file.")
    parser.add_argument("ts", type=Path, help="the .ts file, e.g. lang/qet_en.ts")
    parser.add_argument("--check", action="store_true",
                        help="change nothing; exit 1 if a run would change the file")
    parser.add_argument("--forms", type=int, default=2,
                        help="plural forms to write when a plural message has "
                             "none (default 2, as in English and French)")
    args = parser.parse_args(argv)

    with open(args.ts, encoding="utf-8", newline="") as f:
        text = f.read()
    try:
        new, stats = mirror_text(text, args.forms)
        ET.fromstring(new.encode("utf-8"))
    except (SourceError, LanguageError, ET.ParseError) as e:
        print(f"{args.ts}: {e}", file=sys.stderr)
        return 2
    print(f"{args.ts}: {stats.summary()}")
    if args.check:
        return 1 if stats.changed() else 0
    if new != text:
        with open(args.ts, "w", encoding="utf-8", newline="") as f:
            f.write(new)
    return 0


if __name__ == "__main__":
    sys.exit(main())
