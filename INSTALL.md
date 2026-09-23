# Building QElectroTech from source

QElectroTech is built with **CMake** and requires the  **current Qt6 only**; Qt5 and the
old `qmake`-based build described in earlier versions of this file have
been retired.

## 1. Getting the source

QET uses git submodules, so clone recursively:

```sh
git clone --recursive https://github.com/qelectrotech/qelectrotech-source-mirror.git
cd qelectrotech-source-mirror
```

If you already have a non-recursive clone:

```sh
git submodule update --init --recursive
```

## 2. Requirements

| Component | Status | Notes |
|---|---|---|
| CMake ≥ 3.5 | required | CMake 4.3+ is also fine, see note below |
| C++17 compiler | required | GCC or Clang on Unix-like platforms; MSVC or MinGW-w64 g++ on Windows — see [Choosing a compiler](#3-choosing-a-compiler-unix) / [Building on Windows](#6-building-on-windows-msvc--mingw) |
| Qt6 base + widgets | required | |
| Qt6 **GuiPrivate** headers | required | needed for clickable PDF hyperlinks; **hard build failure** at CMake generate time if missing, see below |
| Qt Linguist tools (`lrelease`) | required | compiles the tracked `.ts` files into `.qm` as part of every normal build |
| pugixml | handled automatically | fetched and built via CMake FetchContent if not already present on the system — see [pugixml](#8-pugixml) below |
| Qt Test module | required if building tests | `PACKAGE_TESTS` is `ON` by default; QtTest ships as part of the base Qt6 dev packages listed below on every platform, no extra package needed |
| KDE Frameworks (KF6) | optional | see [Building without KDE Frameworks](#9-building-without-kde-frameworks) |
| QtPdf module | optional | see [PDF page import](#7-pdf-page-import-qtpdf) |


## 3. Building (out-of-source build)

Always build in a separate directory from the source tree — in-source builds
are not supported.

```sh
mkdir build
cd build
cmake ..
cmake --build . --parallel
```

For a Unix-like platform you'll usually also want an explicit build type,
since CMake's default (empty) build type means no optimization and no
debug info either:

```sh
cmake .. -DCMAKE_BUILD_TYPE=Release
```

To install (default prefix is `/usr/local`; adjust with
`-DCMAKE_INSTALL_PREFIX=...` at the configure step if needed):

```sh
sudo cmake --install .
```

Useful configure-time options (pass as `-D<OPTION>=ON/OFF`):

| Option | Default | Effect |
|---|---|---|
| `PACKAGE_TESTS` | `ON` | build the test suite (needs the Qt Test module) |
| `BUILD_WITH_KF` | distro-dependent | build against KDE Frameworks; see below |
| `QET_ENABLE_PCH` | `OFF` | precompiled headers for faster rebuilds (needs CMake ≥ 3.16) |

## 3.1 Choosing a compiler (Unix)

Both **GCC** and **Clang** are supported on Unix-like platforms; any
reasonably recent release with solid C++17 support works. Neither is
hard-coded — CMake picks up whichever `cc`/`c++` (or `CC`/`CXX` environment
variables) resolve to by default, and you can force one explicitly at
configure time:

```sh
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
# or
cmake .. -DCMAKE_C_COMPILER=gcc   -DCMAKE_CXX_COMPILER=g++
```

or equivalently, by exporting `CC`/`CXX` before calling `cmake`. The
per-distro package lists below install GCC by default (via
`build-essential`/`gcc-c++`/the base toolchain); see each section for the
Clang alternative.

## 4. Distribution packages

Exact package names vary by release/branch; if a name below doesn't resolve,
search your package manager (`apt search`, `pkg search`, `brew search`) for
the closest match.

### Debian / Ubuntu

```sh
sudo apt install \
    build-essential cmake ninja-build git \
    qt6-base-dev qt6-base-private-dev qt6-tools-dev qt6-tools-dev-tools \

    libkf6coreaddons-dev libkf6widgetsaddons-dev
```

`qt6-base-private-dev` is **not** pulled in automatically by `qt6-base-dev`
but is mandatory for the build (see
[Qt6 private headers](#6-qt6-private-headers-mandatory) below).

For Clang instead of GCC:

```sh
sudo apt install clang
```

Optional, for PDF page import:

```sh
sudo apt install libqt6pdf6-dev
```

Optional, to use a system pugixml instead of letting CMake fetch it:

```sh
sudo apt install libpugixml-dev
```

### Fedora / RHEL

```sh
sudo dnf install \
    cmake gcc-c++ git \
    qt6-qtbase-devel qt6-qtbase-private-devel qt6-qttools-devel \
    kf6-kcoreaddons-devel kf6-kwidgetsaddons-devel
```

For Clang instead of GCC: `sudo dnf install clang`.

Optional, for PDF page import: `qt6-qtwebengine-devel` provides `QtPdf` on
some Fedora releases — check `dnf provides '*/QPdfDocument'` if unsure.

Optional, for a system pugixml: `sudo dnf install pugixml-devel`.

### FreeBSD

```sh
pkg install \
    cmake git \
    qt6-base qt6-tools \
    kf6-kcoreaddons kf6-kwidgetsaddons
```

(from ports: `devel/qt6-base`, `devel/qt6-tools`,
`devel/kf6-kcoreaddons`, `x11-toolkits/kf6-kwidgetsaddons`.) Qt6's
`GuiPrivate` headers ship as part of `qt6-base` on FreeBSD, no separate
package is needed. `QtPdf` is not packaged on FreeBSD at the time of writing
— PDF page import will simply be disabled (see below).

Clang is the FreeBSD base system compiler and needs nothing extra; GCC is
available via `pkg install gcc` if you'd rather use it.

Optional, for a system pugixml: `pkg install pugixml` (`devel/pugixml`).

### macOS

Using [Homebrew](https://brew.sh):

```sh
brew install cmake qt ninja
```

Homebrew's `qt` formula is Qt6 and includes the private headers, so no
extra package is required there. KDE Frameworks are not practically
available via Homebrew, so macOS builds are normally done **without KF**
(`-DBUILD_WITH_KF=OFF`) — see the caveat below.

Apple's Clang (from the Xcode Command Line Tools, `xcode-select --install`)
is the default and needs no extra package; GCC is available via
`brew install gcc` if you specifically want it instead.

Optional, for a system pugixml: `brew install pugixml`.

### Windows

See [Building on Windows](#6-building-on-windows-msvc--mingw) below — the
package sources differ enough from the Unix-like platforms above (no system
package manager and Qt aren't provided the same way) that it gets
its own section.

## 5. pugixml

QET links against [pugixml](https://pugixml.org) for XML parsing.
`cmake/fetch_pugixml.cmake` handles this automatically: if a suitable
pugixml isn't already available on the system, CMake fetches and builds it
from source via `FetchContent` as part of the normal configure step — you
don't need to do anything to get a working build.

If you'd rather avoid that network fetch and use your distribution's own
pugixml package instead, install it before running `cmake ..` (package
names are listed per platform in the [Distribution
packages](#4-distribution-packages) section above — e.g. `libpugixml-dev`
on Debian/Ubuntu, `pugixml-devel` on Fedora).

## 6. Building on Windows (MSVC / MinGW)

Both toolchains QET's CMake build targets on Windows are covered here:
**MSVC** (Visual Studio 2019/2022) and **MinGW-w64** (gcc). Unlike the
Unix-like platforms above, there's no single system package manager, so
Qt and (optionally) KDE Frameworks each need to be sourced
separately per toolchain.

One piece of good news either way: unlike Debian/Fedora, the official Qt
Windows kits (both MSVC and MinGW) **already include the GuiPrivate private
headers** — there's no separate "private headers" package to remember on
Windows (see [Qt6 private headers](#7-qt6-private-headers-mandatory) below
for why this matters).

`QtPdf` is available as an optional component in the Qt Online Installer
(look for "Qt PDF" under the Qt 6.4+ node); if you skip it, PDF page import
is silently disabled exactly as on the other platforms. KDE Frameworks are
not prebuilt for Windows by any of the routes below, so Windows builds
normally use `-DBUILD_WITH_KF=OFF` (see the
[autosave caveat](#9-building-without-kde-frameworks)).

### MSVC (Visual Studio 2019 / 2022)

1. Install Visual Studio with the "Desktop development with C++" workload,
   and install Qt6 for MSVC (e.g. the `msvc2019_64` or `msvc2022_64` kit)
   via the [Qt Online Installer](https://www.qt.io/download-qt-installer).
2. Configure and build from an "x64 Native Tools Command Prompt for VS":
   ```bat
   mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 ^
       -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64" ^
       -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
       -DBUILD_WITH_KF=OFF
   cmake --build . --config Release --parallel
   ```
   Ninja works too, in place of the Visual Studio generator, if you prefer
   (`-G Ninja -DCMAKE_BUILD_TYPE=Release`, run from the same VS developer
   prompt so `cl.exe` is on `PATH`).

### MinGW-w64 (gcc)

Two common ways to get a MinGW toolchain that matches a Qt6 MinGW kit: Qt's
own bundled MinGW (installed alongside a `mingw_64` Qt kit via the Qt
Online Installer), or [MSYS2](https://www.msys2.org).

Using MSYS2, from a **MINGW64** shell:
```sh
pacman -S --needed \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools \
    mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-sqlite3

mkdir build && cd build
cmake .. -G Ninja -DBUILD_WITH_KF=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

If `libbacktrace` is installed in your MSYS2 environment, pass the following
cached CMake variables when configuring. They are only necessary in that
case, because `FindBacktrace` needs them to locate the library:

```sh
cmake .. -G Ninja -DBUILD_WITH_KF=OFF -DCMAKE_BUILD_TYPE=Release \
    -DBacktrace_INCLUDE_DIR=/c/msys64/clang64/include \
    -DBacktrace_LIBRARY=/c/msys64/clang64/lib/libbacktrace.a
```

`Backtrace_INCLUDE_DIR` must point to the directory containing `backtrace.h`,
and `Backtrace_LIBRARY` to the `libbacktrace.a` file. The paths above are the
usual locations for the MSYS2 `clang64` environment; adjust them if your
installation uses a different prefix.

(KF6 isn't packaged in MSYS2 either, hence `-DBUILD_WITH_KF=OFF` again.)

Using the Qt Online Installer's bundled MinGW kit instead: point
`CMAKE_PREFIX_PATH` at that kit (e.g. `C:\Qt\6.x.x\mingw_64`) and make sure
its bundled `g++.exe` comes first on `PATH`, or pass
`-DCMAKE_C_COMPILER`/`-DCMAKE_CXX_COMPILER` explicitly so CMake doesn't pick
up a different MinGW installation.

## 7. Qt6 private headers (mandatory)

QET uses the private `QPdfEngine` API (`<private/qpdf_p.h>`) to produce
clickable hyperlinks in PDF export. This requires Qt's `GuiPrivate` target,
which on several distributions (notably Debian/Ubuntu) is shipped in a
separate "private headers" package rather than the normal `-dev` package —
e.g. `qt6-base-private-dev` on Debian/Ubuntu, or
`qt6-qtbase-private-devel` on Fedora. Without it, CMake's `find_package`
step succeeds but configuration later fails at generate time with:

```
Imported target "Qt6::GuiPrivate" includes non-existent path
```

If you hit this, install your distribution's Qt6 private-headers package
(on Windows this is a non-issue — see [Building on
Windows](#6-building-on-windows-msvc--mingw) above).

## 8. PDF page import (QtPdf)

The toolbar's "Add a PDF" feature (`QPdfDocument::pagePointSize()`) needs
the separate `QtPdf` Qt module, available since Qt 6.4.

**This is optional and soft-fails**: if `QtPdf` isn't found, or is present
but older than 6.4, CMake prints a status message and simply disables the
feature (`QET_HAS_QTPDF` stays `FALSE`) — it does **not** stop the build.
Some Qt6 distributions don't ship `QtPdf` at all (it lives outside Qt's
core module set, alongside QtWebEngine), so this is expected on several
platforms, including FreeBSD as of this writing.

## 9. Building without KDE Frameworks

`BUILD_WITH_KF=OFF` builds QET against a bundled fallback UI layer instead
of KDE Frameworks (`kcoreaddons`/`kwidgetsaddons`). This is a fully
supported configuration and is, in practice, the default on platforms where
KF6 isn't readily packaged (macOS and Windows in particular).

Almost everything works identically either way. The one known caveat:
**autosave behavior is not as polished without KF**, since QET relies on
KCoreAddons' autosave/backup-file handling when it's available. Expect
autosave to work, but less robustly than on a KF-enabled build.

## 10. Translations (developer operation)

Everything in this section is a **maintainer/translator task**, not
something a normal build or a packager needs to touch.

The `.qm` files shipped with the application are compiled from the tracked
`.ts` files automatically as part of every normal build (via `lrelease`) —
regular builds and packages get up-to-date translated UI strings for free,
with no developer action needed.

Refreshing the `.ts` files themselves from the source code (`lupdate`) is
the actual developer operation: it's exposed as its own opt-in build
target, separate from the normal build:

```sh
cmake --build . --target update_translations
```

This is **not** run automatically and should not be added to routine or
parallel CI builds: it rewrites the tracked `.ts` files as a side effect,
and running it concurrently with the normal `lrelease` compilation step (as
would happen under `-j`/parallel builds) can corrupt a `.ts` file mid-write,
causing `lrelease` to fail with "Premature end of document". Run it on its
own, review the diff, and commit the updated `.ts` files separately.
