# Custom 2.6 contribution

This contribution collects the final source state of a custom QElectroTech build.
It contains the improvements retained through Custom 2.6, not a sequence of
historical release packages. "Custom 2.6" is a local release label, not the
official QElectroTech application version; the upstream version is unchanged.

## Features proposed for review

- Selective bulk formatting of title-block text cells, with Undo.
- Editable table titles, column headings and manual cell text in the active
  drawing language while retaining source-language text.
- Relay/contact display options: cross or contact symbols, per-device overrides,
  type defaults, line width and independent placement.
- Label clearance previews and bounded movement, including minimum/exact spacing,
  axis constraints, alignment groups, locks and optional automatic correction.
- Manual reference destinations, PDF navigation/export controls, and a separate
  Python helper for importing PDF page artwork and internal navigation.
- Retained table/BOM formatting, formulas, combined tables, defaults and sizing;
  language editing/placement; text selection/alignment; conductor appearance and
  protective-earth patterns; print fixes and previously introduced caching.

See [the user guide](USER_GUIDE.txt) for menu steps and limitations. It describes
the supplied original Custom 2.6 build; screenshots and the full example bundle
are separate review evidence. The latest regression tests' input projects are
under `tests/custom-2.6/fixtures`.

## Source provenance and integration

- Original upstream base: `b7aefe9971f41eff262070c03b35e0421e341f31` (26 October 2026).
- Prepared against official `master`: `b339900fb0500b2e708ab3c78fcdaae6e025fbcc` (29 October 2026).
- The complete custom source was compared with its original base. Its changes
  were then three-way merged with the official source at the target revision.
- Line endings were normalized to LF for changed text files before merging.
- Manual overlap resolutions preserve official translation installation and
  test registrations. Conductor rendering uses custom dash settings when
  enabled and the newer official dash lengths otherwise; both needed includes
  were retained. The custom pattern test precedes the official MSVC test-PATH
  setup so that setup also covers the new test.
- Unchanged source, dependency snapshots, expanded license links, old package
  scripts, generated logs and binaries are not part of the source upload.

Older numbers in retained test/header filenames are implementation names, not
separate releases being submitted. Removing them would remove code or regression
coverage still used by this final version.

## Validation status

This is a **draft contribution pending a fresh full build and regression run**.
The merged C++ application has not been compiled or executed during packaging.
The portable EXE supplied by the author predates this integration and is not a
binary of the merged source.

Packaging checks included merge-marker scans, C++ syntax-tree comparison against
both input versions (not a compiler/type check), XML and archive checks, and a
PDF-helper run against the supplied fixture: 2 pages and 2 internal links using
PyMuPDF 1.26.7. The new test runner's help/argument interface was checked.
No full C++ test-suite pass is claimed for the integrated source.

[SUPPLIED_VALIDATION.txt](SUPPLIED_VALIDATION.txt) preserves the author's report
for the original 2.6 build. Its pass counts and timings were not independently
reproduced and must not be interpreted as results for this integrated tree.
See [BUILD_AND_TEST.md](BUILD_AND_TEST.md) for the remaining checks.

## Known integration work for review

- The custom PDF importer currently expects a Windows portable Python runtime
  beside the executable. It is separate from the official optional QtPdf page
  importer. Cross-platform runtime discovery and packaging need review.
- The custom regression targets are opt-in (`RS13_TESTS`). The latest UI suites
  are run separately; they are not automatically included in CTest.
- Translation coverage, code style, serialization compatibility (including newer
  upstream formats), and interaction with newer official features need review.
- Physical printer output was not tested. Fixed symbol text and wire labels are
  excluded from automatic label spacing; unresolved placements remain unchanged.

The contribution is broad. It can be reviewed as a draft first and split into
feature pull requests in consultation with the maintainers.
