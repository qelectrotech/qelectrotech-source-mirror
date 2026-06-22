# Building QElectroTech on Windows (Qt5 / MSYS2)

This guide reproduces the toolchain used by the project's own CI
(`.github/workflows/windows-build.yml`). It is the most reliable way to build
this fork locally, including the custom Wire Catalogue feature under
`sources/custom/`.

> **Why MSYS2 + Qt5?** This repo configures `find_package(QT NAMES Qt5)` — it
> targets **Qt5, not Qt6**. The CI builds it with the MSYS2 **UCRT64**
> environment, so that is what we mirror here.

---

## 1. Install MSYS2

1. Download and run the installer from <https://www.msys2.org>.
2. Accept the default install path (`C:\msys64`).
3. When it finishes, open **"MSYS2 UCRT64"** from the Start menu
   (the purple icon — **not** "MSYS2 MSYS" or "MINGW64").

Update the package database once:

```bash
pacman -Syu
```

If it asks to close the terminal, close it, reopen **MSYS2 UCRT64**, and run
`pacman -Syu` again.

---

## 2. Install build dependencies

Paste this into the **MSYS2 UCRT64** shell (matches the CI dependency list):

```bash
pacman -S --needed \
  git \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkg-config \
  mingw-w64-ucrt-x86_64-qt5-base \
  mingw-w64-ucrt-x86_64-qt5-svg \
  mingw-w64-ucrt-x86_64-qt5-tools \
  mingw-w64-ucrt-x86_64-qt5-translations \
  mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-kwidgetsaddons \
  mingw-w64-ucrt-x86_64-kcoreaddons \
  mingw-w64-ucrt-x86_64-extra-cmake-modules
```

(The CI also installs `nsis` and `angleproject`; those are only for building
the Windows installer and ANGLE rendering — not needed to compile and run.)

---

## 3. Get the source ready

The repo is already cloned at `E:\Qelectrotech`. In MSYS2 that path is
`/e/Qelectrotech`. The two build-critical submodules (`pugixml`,
`SingleApplication`) are already initialised. If you ever re-clone, run:

```bash
cd /e/Qelectrotech
git submodule update --init pugixml SingleApplication
```

> The large `elements` submodule (the schematic symbol library) is **not**
> required to compile or launch the app — only to place real components. You can
> init it later with `git submodule update --init elements`.

---

## 4. Configure and build

```bash
cd /e/Qelectrotech
mkdir -p build && cd build

cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/ucrt64 \
  -DQt5_DIR=/ucrt64/lib/cmake/Qt5 \
  -DQT_VERSION_MAJOR=5 \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
  -DBUILD_TESTING=OFF \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_CXX_FLAGS="-DQET_EXPORT_PROJECT_DB" \
  -DSQLite3_INCLUDE_DIR=/ucrt64/include \
  -DSQLite3_LIBRARY=/ucrt64/lib/libsqlite3.dll.a \
  ..

ninja
```

- `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` + `-DQT_VERSION_MAJOR=5` force the Qt5
  path even if a Qt6 cmake package is present.
- First build takes several minutes; subsequent `ninja` runs are incremental.
- The executable lands in `build/` as `qelectrotech.exe`.

---

## 5. Run it

The app needs Qt's DLLs and plugins next to (or visible to) the exe. Easiest:

```bash
cd /e/Qelectrotech/build
windeployqt qelectrotech.exe   # copies required Qt DLLs/plugins beside the exe
./qelectrotech.exe
```

`windeployqt` ships with `qt5-tools`. After running it once, you can also launch
`qelectrotech.exe` by double-clicking it from Explorer.

---

## 6. Verifying the Wire Catalogue build

The custom code compiles as part of the normal build (it is registered in
`cmake/qet_compilation_vars.cmake`). To confirm it was actually compiled:

```bash
ninja -v 2>&1 | grep wirecatalogue
```

You should see `wirecataloguedb.cpp`, `wirecataloguemodel.cpp`,
`wirespecdialog.cpp`, and `wirecatalogueui.cpp` being compiled.

> Note: the panel is not yet reachable from a menu — the `mainwindow.cpp` hook
> is the next step. Until then, a successful compile of the four files above is
> the verification that Phase 1–2 code is sound.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `cmake: command not found` | You opened the wrong shell. Use **MSYS2 UCRT64**. |
| CMake picks Qt6 / wrong Qt | Ensure `-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON -DQT_VERSION_MAJOR=5` are present; or `rm -rf /ucrt64/lib/cmake/Qt6`. |
| `Could NOT find SQLite3` | Install `mingw-w64-ucrt-x86_64-sqlite3`; keep the `-DSQLite3_*` flags above. |
| `KF5...` / ECM errors | Install `kwidgetsaddons`, `kcoreaddons`, `extra-cmake-modules`. |
| Missing `pugixml` / `SingleApplication` headers | `git submodule update --init pugixml SingleApplication`. |
| App starts then exits / "plugin qwindows" error | Run `windeployqt qelectrotech.exe` from the build dir. |
| `QSQLITE driver not loaded` at runtime | `windeployqt` must copy the `sqldrivers/` plugin; re-run it, or ensure `qt5-base` is installed. |
