# How to contribute
 
I'm really glad you're reading this,
because we need volunteer developers to help
this project come to fruition.


Here are some important resources:

* [Qet code style](https://qelectrotech.org/wiki_new/doc/qt_creator#on_ajoute_le_style_de_code_qet)
* [git Documentation](https://git-scm.com/doc)

## Testing

### Build the tests

Start with the dependencies and platform-specific setup in
[INSTALL.md](INSTALL.md). Tests additionally require the Qt6 Test module.
The commands below use CMake/CTest 3.20 or newer (`--test-dir` needs 3.20),
a C++17 compiler, and the Qt6 development packages, including GuiPrivate,
Svg and LinguistTools. Configuration may download dependencies with
FetchContent, including Catch2, so an initial build needs network access.

Run the following commands from the repository root. Keep build products in
a separate directory and use Debug for the regression tests:

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_TESTS=ON
cmake --build build --config Debug --parallel
```

`PACKAGE_TESTS` defaults to `ON`; specify it explicitly when reusing a build
directory that previously disabled tests. `BUILD_WITH_KF` defaults to `ON`.
If KDE Frameworks are unavailable, add `-DBUILD_WITH_KF=OFF` to the configure
command, as described in INSTALL.md. Use a separate build directory when
changing compiler, Qt kit, or generator.

### Run the tests with CTest

After a successful build, list the registered tests, run them, or select a
test by name with `-R`:

```sh
ctest --test-dir build -C Debug -N
ctest --test-dir build -C Debug --output-on-failure
ctest --test-dir build -C Debug -R '^tst_qetstrings$' --output-on-failure
```

`-N` only lists tests; it does not execute them. `-C Debug` selects the
configuration for multi-configuration generators such as Visual Studio.
For single-configuration generators such as Ninja and Unix Makefiles,
`CMAKE_BUILD_TYPE=Debug` selects it at configure time. CTest runs the tests
registered by CMake; it does not build their executables.

Some tests create Qt widgets and need a graphical environment. On a Linux
machine without a display, use Xvfb as the Linux CI does:

```sh
xvfb-run -a ctest --test-dir build -C Debug --output-on-failure
```

Tests that depend on optional features may not be registered. In particular,
several scripting tests require the Qt6 Qml module at configure time.
Review the test list and any skipped tests before reporting what was covered.

### Windows

Follow INSTALL.md for SQLite3 and the MSVC or MinGW toolchain. The compiler
and Qt kit must match. For MSVC, use a Visual Studio developer PowerShell;
the example below assumes Visual Studio 2022 and a 64-bit MSVC Qt kit.
Replace the example Qt path with your installation, and supply any SQLite3
paths or toolchain file required by your setup:

```powershell
$qtPrefix = 'C:/Qt/6.x.x/msvc2022_64'
cmake -S . -B build-msvc -G 'Visual Studio 17 2022' -A x64 `
    "-DCMAKE_PREFIX_PATH=$qtPrefix" -DBUILD_WITH_KF=OFF -DPACKAGE_TESTS=ON
cmake --build build-msvc --config Debug --parallel
$env:Path = "$qtPrefix/bin;$env:Path"
ctest --test-dir build-msvc -C Debug --output-on-failure
```

Putting the matching Qt `bin` directory on `PATH` lets test executables find
Qt DLLs. Other shared dependencies must also be available on `PATH`.
For MinGW with Ninja, use the matching MinGW Qt kit and compiler environment,
omit `-A x64`, select `-G Ninja`, and set `-DCMAKE_BUILD_TYPE=Debug`.
The Linux shell regression scripts below do not run natively on Windows.

### Tests outside the CTest suite

The Catch2 executable `C_unittests` is built with `PACKAGE_TESTS=ON`, but is
currently not registered with CTest. Run it separately from the repository
root after building:

```sh
./build/tests/catch/C_unittests
```

This executable creates a Qt GUI application too. On headless Linux, use:

```sh
xvfb-run -a ./build/tests/catch/C_unittests
```

For the Visual Studio example above, use this PowerShell command after
setting up `PATH`:

```powershell
& ./build-msvc/tests/catch/Debug/C_unittests.exe
```

The Python MCP suite is also separate from CTest. Its unit and protocol
tests can be run without a QElectroTech binary. Some fixtures launch POSIX
shell scripts, so use a Unix-like environment for this suite:

```sh
python3 misc/qet-mcp/test_qet_mcp.py
```

Use `python` instead of `python3` if that is your interpreter's command.
Integration tests need a built QElectroTech and the environment variables
documented in [the MCP testing guide](misc/qet-mcp/README.md#testing),
including `QET_ENABLE_SCRIPTING=1` for script-driven tests. Missing integration
prerequisites cause tests to be skipped, so a successful unit run alone does
not establish integration coverage.

### Linux regression prerequisites

In addition to the build dependencies in INSTALL.md, headless widget tests
need `xvfb-run`, Xvfb and `xauth`. The existing
[Linux workflow](.github/workflows/linux-build.yml) lists the packages used
by CI, including the QtSvg development package and SQLite driver for QtSql.
Package names vary by distribution.

The IPC open-forwarding regression requires Bash, Xvfb, Openbox and xdotool.
Use a Qt6 Debug build; the documented reproduction used Qt 6.10.2. It uses
`examples/industrial.qet` by default and starts its own display, so run it
directly rather than wrapping it in `xvfb-run`:

```sh
tests/ipc-regression/run.sh --binary build/qelectrotech
```

This test is separate from CTest. Exit code `0` means the scenario survived,
`1` means a crash, and `2` means inconclusive or unusable input. An inconclusive
run is not a pass. See [the IPC guide](tests/ipc-regression/README.md).

The quit-during-modal regression needs gdb with Python support and a binary
with the symbols used by the test (use an unstripped Debug build). It uses
Qt's offscreen platform and needs no X server. It is registered with CTest
on Linux and can be selected with:

```sh
ctest --test-dir build -C Debug -R '^modal_quit_regression$' --output-on-failure
```

Missing gdb, missing Python support in gdb, or a stripped binary can result
in exit code `77`, which CTest treats as skipped. See
[the modal-quit guide](tests/modal-quit-regression/README.md).

When submitting changes, state the build configuration, commands actually
run, results, and any skipped or unavailable tests. If commands were only
checked against the build files or documentation, say so explicitly.

## Submitting changes

Always write a clear log message for your commits.
One-line messages are fine for small changes,
but bigger changes should look like this:

    $ git commit -m "A brief summary of the commit
    > 
    > A paragraph describing what changed and its impact."

* It is always appropriate to keep the commits small.
* For major changes it is recommended to use branches.

### Interactive Staging
https://git-scm.com/book/en/v2/Git-Tools-Interactive-Staging

issue: you have modified a class but you want to write it in 2 commits

 ´git add -p´ or ´git add -i´


    /qet> git add -i


               staged     unstaged path
      1:    unchanged        +1/-1 sources/diagram.cpp

    *** Commands ***
      1: status       2: update       3: revert       4: add untracked
      5: patch        6: diff         7: quit         8: help


    What now> 5


               staged     unstaged path
      1:    unchanged        +1/-1 sources/diagram.cpp
    Patch update>> 1
               staged     unstaged path
    * 1:    unchanged        +1/-1 sources/diagram.cpp
    Patch update>>
    diff --git a/sources/diagram.cpp b/sources/diagram.cpp
    index bffca653f..9bd2280f7 100644
    --- a/sources/diagram.cpp
    +++ b/sources/diagram.cpp
    @@ -103,9 +103,9 @@ Diagram::Diagram(QETProject *project) :
        connect(&border_and_titleblock,
                &BorderTitleBlock::titleBlockFolioChanged,
                this, &Diagram::updateLabels);
    -       connect(this, &Diagram::diagramActivated,
    +       foo(do_a);
    -       adjust(diagramActivated);
    +       bar(do_c);
        adjustSceneRect();
     }


    (1/1) Stage this hunk [y,n,q,a,d,s,e,?]? s


    Split into 2 hunks.
    @@ -103,5 +103,5 @@
        connect(&border_and_titleblock,
                &BorderTitleBlock::titleBlockFolioChanged,
                this, &Diagram::updateLabels);
    -       connect(this, &Diagram::diagramActivated,
    +       foo(do_a);


    (1/2) Stage this hunk [y,n,q,a,d,j,J,g,/,e,?]? y


    @@ -107,5 +107,5 @@
                this, &Diagram::loadElmtFolioSeq);
    -       adjust(diagramActivated);
    +       bar(do_c);
        adjustSceneRect();
     }


    (2/2) Stage this hunk [y,n,q,a,d,K,g,/,e,?]? n


    *** Commands ***
      1: status       2: update       3: revert       4: add untracked
      5: patch        6: diff         7: quit         8: help
    What now>What now>7
    Bye.

    git commit -m "Mod Signal Slot to funsion"

## Coding conventions

Start reading our code and you'll get the hang of it.
We optimize for readability:

  * We use tabs to indent, and interpret tabs as taking up to 8 spaces.
    see https://qelectrotech.org/wiki_new/doc/qt_creator#on_ajoute_le_style_de_code_qet
  * We try to keep to at most 80 characters per line.
  * Try to make your code understandable.
    You may put comments in, but comments invariably tend to stale out when
    the code they were describing changes.
    Often splitting a function into two
    makes the intention of the code much clearer.
    
Thanks,
QElectroTech
