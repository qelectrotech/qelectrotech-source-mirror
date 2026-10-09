#!/usr/bin/env python3
"""
Regression suite for mirror_source_language.py.

    python3 misc/i18n/test_mirror_source_language.py

The fixtures are a hand-written pair: mirror_before.ts is what lupdate
leaves behind, mirror_after.ts is what the script must produce from it,
byte for byte. Every message class the script distinguishes appears
once in the pair, so a change in behaviour shows up as a diff in a
known place.

The last test looks at the repository's own lang/qet_en.ts and fails
when it needs a mirror run: that is the maintenance rule from
INSTALL.md, kept honest.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import mirror_source_language as m  # noqa: E402

SCRIPT = HERE / "mirror_source_language.py"
BEFORE = HERE / "fixtures" / "mirror_before.ts"
AFTER = HERE / "fixtures" / "mirror_after.ts"
REPO_EN = HERE.parent.parent / "lang" / "qet_en.ts"
REPO_FR = HERE.parent.parent / "lang" / "qet_fr.ts"


def read(path: Path) -> str:
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def messages(text: str) -> dict[tuple[str, str], ET.Element]:
    """(context, source) -> <message> element."""
    out = {}
    for ctx in ET.fromstring(text.encode("utf-8")).iter("context"):
        name = ctx.findtext("name")
        for msg in ctx.iter("message"):
            out[(name, msg.findtext("source"))] = msg
    return out


TRANSLATION_RE = re.compile(r"<translation[^>]*>.*?</translation>", re.S)


def outside_translations(text: str) -> str:
    """The file with every <translation> element blanked out."""
    return TRANSLATION_RE.sub("<translation/>", text)


class MirrorText(unittest.TestCase):
    def test_nothing_to_do_is_byte_identical(self):
        after = read(AFTER)
        out, stats = m.mirror_text(after)
        self.assertEqual(out, after)
        self.assertEqual(stats.mirrored_messages, 0)
        self.assertEqual(stats.mirrored_forms, 0)
        self.assertEqual(stats.finished_identical, 0)
        self.assertEqual(stats.unfinished_cleared, 0)

    def test_before_becomes_after(self):
        out, stats = m.mirror_text(read(BEFORE))
        self.assertEqual(out, read(AFTER))
        self.assertEqual(stats.mirrored_messages, 8)
        self.assertEqual(stats.mirrored_forms, 3)
        self.assertEqual(stats.finished_identical, 2)
        self.assertEqual(stats.unfinished_cleared, 4)
        self.assertEqual(stats.kept_translated, 1)
        self.assertEqual(stats.kept_identical, 1)
        self.assertEqual(stats.kept_vanished, 2)
        self.assertEqual(stats.kept_unfinished_with_text, 1)

    def test_nothing_outside_translation_elements_changes(self):
        """Header, locations, comments, sources and indentation are untouched."""
        self.assertEqual(outside_translations(read(BEFORE)),
                         outside_translations(read(AFTER)))

    def test_idempotent(self):
        once, _ = m.mirror_text(read(BEFORE))
        twice, stats = m.mirror_text(once)
        self.assertEqual(once, twice)
        self.assertFalse(stats.changed())

    def test_untouched_classes(self):
        before, after = messages(read(BEFORE)), messages(read(AFTER))
        for key in [("Alpha", "Apply to Entire Project"),   # English source, translated
                    ("Alpha", "QElectroTech"),              # identical
                    ("Alpha", "Brouillon"),                 # unfinished with text
                    ("Alpha", "Ancien texte"),              # vanished with text
                    ("Alpha", "Autre ancien texte")]:       # vanished, empty
            self.assertEqual(ET.tostring(before[key]), ET.tostring(after[key]), key)
        self.assertEqual(after[("Alpha", "Brouillon")].find("translation").get("type"),
                         "unfinished")

    def test_escaping_is_copied_verbatim(self):
        """&apos; must stay &apos;, never become &amp;apos; (a re-escape bug)."""
        out, _ = m.mirror_text(read(BEFORE))
        for raw in ("<translation>&lt;b&gt;%1&lt;/b&gt; : %2</translation>",
                    "<translation>&amp;Aide</translation>",
                    "<translation>l&apos;élément &quot;x&quot;</translation>"):
            self.assertIn(raw, out)
        for key, msg in messages(out).items():
            tr = msg.find("translation")
            if key[0] == "Beta":
                self.assertEqual(tr.text, msg.findtext("source"), key)

    def test_unfinished_identical_to_source_is_finished(self):
        """lupdate's same-text heuristic leaves these behind after a refresh;
        39 of them appeared in lang/qet_fr.ts on 2026-10-07."""
        out, _ = m.mirror_text(read(BEFORE))
        msgs = messages(out)
        for key in [("Alpha", "Copier"), ("Alpha", "%n page(s)")]:
            self.assertIsNone(msgs[key].find("translation").get("type"), key)
        self.assertEqual(msgs[("Alpha", "Brouillon")].find("translation").get("type"),
                         "unfinished", "a different unfinished text stays unfinished")

    def test_multiline_and_spaces_kept(self):
        out, _ = m.mirror_text(read(BEFORE))
        self.assertIn("<translation> Ligne 1\nLigne 2\n</translation>", out)

    def test_plural_half_empty_fills_only_the_empty_form(self):
        out, _ = m.mirror_text(read(BEFORE))
        forms = [f.text for f in messages(out)[("Alpha", "%n élément(s)")]
                 .find("translation").iter("numerusform")]
        self.assertEqual(forms, ["%n élément", "%n élément(s)"])

    def test_plural_both_empty_fills_both_and_clears_unfinished(self):
        out, _ = m.mirror_text(read(BEFORE))
        tr = messages(out)[("Alpha", "%n conducteur(s)")].find("translation")
        self.assertIsNone(tr.get("type"))
        self.assertEqual([f.text for f in tr.iter("numerusform")],
                         ["%n conducteur(s)", "%n conducteur(s)"])

    def test_plural_without_forms_gets_default_forms(self):
        text = ('<TS><context><name>C</name><message numerus="yes">\n'
                '        <source>%n x</source>\n'
                '        <translation type="unfinished"></translation>\n'
                '    </message></context></TS>')
        out, stats = m.mirror_text(text, forms=3)
        self.assertEqual(stats.mirrored_forms, 3)
        self.assertEqual(out.count("<numerusform>%n x</numerusform>"), 3)
        self.assertNotIn("unfinished", out)
        ET.fromstring(out)

    def test_translation_into_another_language_is_refused(self):
        for header in ('<TS version="2.1" language="fr_FR" sourcelanguage="en">',
                       '<TS language="de" sourcelanguage="en_US">'):
            text = (header + '<context><name>C</name><message>\n'
                    '        <source>Open</source>\n'
                    '        <translation></translation>\n'
                    '    </message></context></TS>')
            with self.assertRaises(m.LanguageError, msg=header):
                m.mirror_text(text)

    def test_source_language_file_is_accepted(self):
        for header in ('<TS version="2.1" language="en_US" sourcelanguage="en">',
                       '<TS version="2.1" language="fr_FR">'):  # no sourcelanguage
            text = (header + '<context><name>C</name><message>\n'
                    '        <source>Open</source>\n'
                    '        <translation></translation>\n'
                    '    </message></context></TS>')
            out, stats = m.mirror_text(text)
            self.assertEqual(stats.mirrored_messages, 1, header)
            self.assertIn("<translation>Open</translation>", out)

    def test_source_with_child_element_is_refused(self):
        text = ('<TS><context><name>C</name><message>\n'
                '        <source>a<byte value="7"/>b</source>\n'
                '        <translation></translation>\n'
                '    </message></context></TS>')
        with self.assertRaises(m.SourceError):
            m.mirror_text(text)


class CommandLine(unittest.TestCase):
    def run_script(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), *args],
                              capture_output=True, text=True)

    def copy(self, src: Path) -> Path:
        d = tempfile.mkdtemp()
        dst = Path(d) / src.name
        dst.write_bytes(src.read_bytes())
        self.addCleanup(lambda: (dst.unlink(), os.rmdir(d)))
        return dst

    def test_check_exit_codes(self):
        self.assertEqual(self.run_script("--check", str(BEFORE)).returncode, 1)
        self.assertEqual(self.run_script("--check", str(AFTER)).returncode, 0)
        self.assertEqual(read(BEFORE).count("<translation></translation>"), 5,
                         "--check must not write")

    def test_rewrite_then_check_passes(self):
        tmp = self.copy(BEFORE)
        r = self.run_script(str(tmp))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("mirrored 8 message(s), 3 plural form(s)", r.stdout)
        self.assertEqual(read(tmp), read(AFTER))
        self.assertEqual(self.run_script("--check", str(tmp)).returncode, 0)

    def test_child_element_exits_2_and_leaves_file(self):
        tmp = self.copy(BEFORE)
        bad = read(tmp).replace("<source>Ouvrir le projet</source>",
                                '<source>a<byte value="7"/>b</source>')
        tmp.write_text(bad, encoding="utf-8", newline="")
        r = self.run_script(str(tmp))
        self.assertEqual(r.returncode, 2)
        self.assertIn("child element", r.stderr)
        self.assertEqual(read(tmp), bad)

    @unittest.skipUnless(REPO_EN.is_file(), "lang/qet_en.ts not found next to the tool")
    def test_repository_source_language_file_is_mirrored(self):
        """The maintenance rule: run the mirror after update_translations.

        Until 2026-10-09 this checked lang/qet_fr.ts, which #1390 had turned
        into an ordinary translation; it passed while the rule was wrong."""
        r = self.run_script("--check", str(REPO_EN))
        self.assertEqual(r.returncode, 0,
                         f"lang/qet_en.ts needs a mirror run:\n{r.stdout}")

    @unittest.skipUnless(REPO_FR.is_file(), "lang/qet_fr.ts not found next to the tool")
    def test_repository_translation_file_is_refused(self):
        """A run on the French translation would copy English into it."""
        before = REPO_FR.read_bytes()
        r = self.run_script(str(REPO_FR))
        self.assertEqual(r.returncode, 2, r.stdout)
        self.assertIn("only the source language", r.stderr)
        self.assertEqual(REPO_FR.read_bytes(), before)


if __name__ == "__main__":
    unittest.main()
