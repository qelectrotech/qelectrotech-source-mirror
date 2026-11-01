# Instructions for AI coding agents

QElectroTech (QET) is a free schematic editor written in C++17 with Qt 6 and
built with CMake. Project files are XML. These instructions are for AI agents
working in this repository, and they summarise the project's guides. If a
guide and this file disagree, the guide wins, so read it fresh for the task:

- Any change: [Fix or add with an AI assistant](https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/contribute_with_ai_assistant)
- Anything posted on GitHub: [Posting with an AI assistant](https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/posting_with_ai_assistant)
- Translations: [Translate with an AI assistant](https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/translating_with_ai)
- C++ code: [Code style](https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/code_style)
- Building: [Building from Source](https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/building)
- Testing: [CONTRIBUTING.md](CONTRIBUTING.md)

## The person decides

- The person you work for reads and approves everything before it goes out.
  Never post, comment, close or merge without being told to for that post.
- Stop and ask before you delete files or install software.
- Maintainers want short, plain posts written for the user's point of view.
  Never claim something was tested when it was not.

## Before you change anything

- Search open and closed issues and pull requests. The feature may exist, or
  the bug may already be fixed.
- Reproduce a bug on the current code first.
- Show a short plan (files to change, any new setting) and wait for an OK.
- Work on a branch of the person's fork, made from the current upstream
  `master`.
- The interface and the code use different words: wire = conductor,
  page = folio, symbol = element.

## Rules for every change

- A change to existing behaviour gets an on/off setting in the settings
  dialog, off by default, so existing projects look the same.
- Old project files must still open and look unchanged.
- Anything the user does to a project goes through a `QUndoCommand` on the
  project's undo stack. Merge ids are listed in
  `dev_doc/ID_of_QUndoCommand.txt`.
- Match the code around you. If a file differs from the code style, the file
  wins inside that file.
- Never reformat lines you are not changing. Never run `clang-format -i` on a
  whole file: the `.clang-format` in the repository does not match the code.
  The code style page gives a matching configuration, to use on your own lines
  only.
- Tabs, 80 columns, `Type *name`, `m_` for new members, `enum class` for new
  enums.
- UTF-8 without a byte-order mark and Unix line endings. New files start with
  the licence block from the code style page.
- Make one logical change per commit, and say why in the message.
- Add tests for new functionality, in `tests/qttest` or `tests/catch`.

## Interface text

- Every string the user can see goes through `tr()`, written in English. Never
  write French or another language in the source.
- Use placeholders such as `%1` and `%n`, never glued-together sentences.
- Use the names menus, buttons and dialogs already have. Check the existing
  `tr()` text instead of guessing.

## Build and test

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_TESTS=ON
cmake --build build --parallel
xvfb-run -a ctest --test-dir build --output-on-failure
```

Leave out `xvfb-run -a` if there is a display. Add `-DBUILD_WITH_KF=OFF` if KDE
Frameworks are not available. Use fewer parallel jobs if the build runs out of
memory. For Microsoft's compiler, run the tests in `RelWithDebInfo`, not
`Debug`.

## Check before a pull request

For a code change, run [qet-verify](https://github.com/ispyisail/qet-verify) on
your last commit and fix every `FAIL`. A `SKIP` is not a pass.

```sh
../qet-verify/verify.sh --qet-dir <QET folder> --binary <built qelectrotech>
```

Then try the change on a real project, and review your own diff the way a
maintainer would. A change the user can see needs a before and an after
screenshot, taken from the same build with one thing changed. Use an example
project, never a customer's or an employer's drawing.

## Translations

- Updating a language touches only `lang/qet_<code>.ts`. A new language touches
  five places: follow the table in the translation guide.
- Run `lupdate sources -ts lang/qet_<code>.ts -source-language en_US` from the
  current `master`. For a new language, add `-target-language <code>`, as the
  translation guide does.
- Keep placeholders, HTML tags, links, the `&` of Alt-key letters and a
  trailing `...` or `…`.
- Leave every translation you write marked `type="unfinished"`. The person
  marks a string done after reading it.
- Run the check with `qet_ts_tool.py` from the translation guide, and
  `lrelease`.
- Never commit a `.qm` file. The build makes it.

## Pull requests

- Open it against `master` of `qelectrotech/qelectrotech-source-mirror`, and
  follow `.github/pull_request_template.md`.
- Start the title with a verb such as Fix, Add or Document. Spell out
  abbreviations, and use no `feat:` or `fix:` tags.
- Stay within the length limits of the posting guide, and include everything
  it requires of a description, such as the qet-verify receipt pasted exactly
  as the script printed it.
- State the build configuration, the commands actually run, their results, and
  anything skipped or not tested.
- Say that an AI assistant helped, and who read the result. For a translation,
  say how many strings are reviewed and how many are still unfinished.
- After a review, push new commits. Do not force-push an open pull request.
