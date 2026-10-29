# Building and checking this contribution

The combined source has not yet been built. Start with the repository's
`INSTALL.md` and `CONTRIBUTING.md` for the supported compiler, Qt/private headers,
SQLite and dependency setup. Fetch submodules in a real Git checkout. The upload
batches are an overlay on the target repository, not a standalone source tree.

## Build configuration

Enable the retained custom regression targets with `-DRS13_TESTS=ON`.
Keep `-DPACKAGE_TESTS=ON` to build the official tests too. `BUILD_WITH_KF=OFF`
is available when KDE Frameworks are not configured. Use an out-of-source build.
For example, after configuring the required compiler and dependency paths:

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_KF=OFF -DPACKAGE_TESTS=ON -DRS13_TESTS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

These are proposed commands, not commands successfully executed during packaging.
Run the additional suites described in `CONTRIBUTING.md` as applicable. CTest does
not run the opt-in `rs13_tests` through `rs26_tests` targets automatically.

The supplied original Windows build used Qt 6.4.2, MinGW 13.1.0 x64 POSIX,
CMake/Ninja, SQLite, and release flags `-O1 -DNDEBUG`. That recorded configuration
is not proof of compatibility of the merged source. Use matching compiler/Qt
runtime DLLs for whichever configuration is actually tested.

## Latest custom Windows UI suites

Build `rs25_tests`, `rs20_tests`, `rs26_tests`, `rs14_tests` and `qelectrotech`.
Their executable directory needs the usual Qt/MinGW libraries and plugins.
For the custom PDF import check it additionally needs:

```text
<binary-directory>/pdf-runtime/python.exe
<binary-directory>/pdf-runtime/... Python and PyMuPDF dependencies ...
<binary-directory>/tools/pdf_references25.py
```

The original portable package contains a Python 3.12.10 / PyMuPDF 1.26.5 runtime
in its `pdf-runtime` folder. For testing a rebuilt application, that runtime
folder can be copied beside the rebuilt EXEs; do not copy it into Git source.
Alternatively provision an equivalent Python runtime with PyMuPDF installed.
The test runner copies the helper script from this repository into the binary
directory. It does not download dependencies or copy the original application EXE.
Application resources such as the element collection, languages and title blocks
must also be available as required by the normal application deployment.

On Windows, with the matching Qt binaries available, run:

```text
python tests/custom-2.6/run_regressions.py --bin-dir build --qt-bin C:/Qt/6.4.2/mingw_64/bin
```

Replace these paths with the actual locations. For multi-configuration builds,
the binary directory may be `build/Debug`. Add `--platform windows` for native
Windows-font screenshots; the default is `offscreen`. Results go in a new folder
under the binary directory, or an empty directory supplied with `--output-dir`.

The order is rs25, rs20, rs26, rs14: rs25 creates the bilingual fixture used by
rs26. Initial inputs are copied from `tests/custom-2.6/fixtures`. The runner stops
on a failed suite. Retained older suites remain available for additional testing;
their input arguments differ, so consult their main functions before running.

## Independent PDF helper check

In a Python environment with PyMuPDF installed, use:

```text
python tools/pdf_references25.py tests/custom-2.6/fixtures/PDF_import_fixture.pdf build/pdf-helper-check
```

The fixture should yield two page images and a manifest containing two internal
links. This helper check was executed during packaging with PyMuPDF 1.26.7; it
does not exercise the C++ UI importer or establish cross-platform application
compatibility.

## Before marking the pull request ready

Record the exact merged revision, compiler/Qt/build configuration, test commands,
results and skipped tests. Exercise Undo/Redo, save/reopen, translated/source text,
PDF navigation and rendering, conductor patterns, and newer official persistence
features. Compare the original and merged behaviour. Keep supplied old logs
separate from newly executed results. Physical printing requires its own check.
