# Decision Analysis: QtTest Project-Load Target Deweighting

Date: 2026-09-21
Status: historical decision analysis; Option A was later implemented and reviewed.

Outcome note, 2026-09-22:
- Option A was implemented as the qttest-private CMake registration function `add_qet_project_load_qtest(...)` in `tests/qttest/CMakeLists.txt`.
- Only `tst_contactcrossrefprojectionservice` was migrated to that helper.
- The refactor reduced registration/maintenance weight for future project-load tests. It intentionally did not reduce compile/link weight, because the real `QETProject` load still depends on the application object graph and QET resources.
- Focused verification passed after implementation and review: the relevant six-test CTest set stayed green.

## Scope

This note analyzes how to reduce the build weight of project-load QtTests, starting with `tst_contactcrossrefprojectionservice`. It does not change CMake, production code, tests, fixtures, persistence, UI, rendering, `LinkElementCommand`, or Device/Function-Core behavior.

## Evidence

- `tests/qttest/CMakeLists.txt` registers most tests as narrow binaries. Header-only or small-unit tests compile only their direct sources, while CLI-based integration tests depend on the already built `qelectrotech` executable.
- `tst_contactcrossrefprojectionservice` is different: it creates `QET_QTTEST_APP_SRC` from `QET_SRC_FILES`, filters only `sources/main.cpp`, globs all `sources/*.h` directories as include directories, and compiles `qelectrotech.qrc` plus `ico/icon-themes.qrc`.
- The target links broad application dependencies: `Qt::Test`, `pugixml::pugixml`, `SingleApplication::SingleApplication`, `SQLite3::SQLite3`, `${KF_PRIVATE_LIBRARIES}`, and `${QET_PRIVATE_LIBRARIES}`.
- Top-level `CMakeLists.txt` builds the application from `${QET_RES_FILES}`, `${QET_SRC_FILES}`, translations, `qelectrotech.qrc`, and `ico/icon-themes.qrc`. The test target currently mirrors most of that app build to get a real `QETProject` load.
- `tst_contactcrossrefprojectionservice.cpp` loads a real `.qet` fixture through `QETProject`, constructs `QApplication`, disables backups/non-interactive dialogs, and tests the read-only projection service over the loaded runtime object graph.
- Earlier verification recorded in `Brain.md` says the direct project-load test crashed without QET resources; adding the QET resource files made the test pass.

## Why It Is Heavy

The current target is not just testing `ContactCrossRefProjectionService`. It compiles almost the complete QElectroTech application object graph so that `QETProject` can load diagrams, embedded elements, graphics items, titleblocks/resources, database/report-adjacent code, and UI-linked classes without missing symbols or resources. That makes the test realistic, but it is expensive and easy to copy into an oversized pattern for future projection tests.

## Options

### Option A: Shared Project-Load QtTest App Library

Create one internal test-support target that owns the existing heavy project-load wiring: `QET_SRC_FILES` minus `main.cpp`, resource files, include directories, and broad libraries. Link `tst_contactcrossrefprojectionservice` and future project-load projection tests to that target.

Affected later files:
- `tests/qttest/CMakeLists.txt`
- possibly a small `tests/qttest/project_load_test_support.*` helper if setup code is centralized

Advantages:
- Removes repeated heavy CMake blocks before they spread.
- Keeps behavior closest to the current green test.
- Fits the "QET remains chassis" direction because tests still exercise the real loaded QET object graph.

Risks:
- Build weight is centralized, not truly reduced.
- A shared target can become a dumping ground if every future test links it by default.
- Depending on CMake/object-library details, resource and AUTOGEN behavior must be verified carefully on macOS and other platforms.

Compatibility:
- High, because the current source/resource/library set stays materially the same.

Acceptance criteria:
- Existing `tst_contactcrossrefprojectionservice` behavior unchanged.
- Focused CTest set remains green.
- New CMake helper target is private to qttest and clearly named for project-load tests only.
- No production source, XML schema, UI behavior, or fixture changes.

Abort rules:
- Stop if resource initialization changes project-load behavior.
- Stop if the helper target requires production code changes.
- Stop if Windows/macOS bundle/resource behavior becomes ambiguous.

### Option B: Dedicated Test-Support Loader Helper Only

Keep the current CMake target shape, but extract repeated runtime setup into a small test-only helper: project fixture path, `QApplication`/offscreen conventions, backup/message-box setup, invalid XML policy, and `QETProject` construction checks.

Affected later files:
- `tests/qttest/CMakeLists.txt` only if helper header/source registration is needed
- `tests/qttest/*` project-load tests

Advantages:
- Very low risk.
- Improves consistency without changing link topology.
- Good preparatory step if only one or two project-load tests exist.

Risks:
- Does not solve the heavy build target.
- Future tests may still duplicate the broad CMake block.

Compatibility:
- Very high.

Acceptance criteria:
- Helper is test-only and contains setup, not domain assertions.
- Existing tests remain green.
- No new dependency direction.

Abort rules:
- Stop if setup helper starts encoding domain semantics.

### Option C: Minimal Source-Subset Target

Replace `QET_SRC_FILES` minus `main.cpp` with a curated subset needed by `QETProject` load plus `ContactCrossRefProjectionService`.

Affected later files:
- `tests/qttest/CMakeLists.txt`
- possibly `cmake/qet_compilation_vars.cmake` only if a reusable source group is introduced later

Advantages:
- Real build-time reduction if successful.
- Forces understanding of the true project-load dependency set.

Risks:
- High churn: `QETProject` load touches diagrams, graphics items, elements, titleblocks, terminal strips, database hooks, resources, and UI-adjacent code.
- Easy to create fragile per-platform link failures.
- Can drift from the real application object graph and weaken the value of projection tests.

Compatibility:
- Medium to low until proven across platforms.

Acceptance criteria:
- Measurably smaller compile/link set.
- Same fixture loads and same assertions pass.
- No stubs that alter behavior.
- Cross-platform configure/link remains clean.

Abort rules:
- Stop if minimality requires stubbing production behavior.
- Stop if missing-symbol chasing becomes broader than the test-only slice.

### Option D: CLI-Only Black-Box Tests for Projection Behaviors

Avoid direct `QETProject` loading in QtTests and verify future behavior via the built `qelectrotech` binary, as existing CLI tests do.

Affected later files:
- `tests/qttest/*` CLI tests
- no project-load service test target growth

Advantages:
- Light per-test build targets.
- Matches existing CLI test style and uses the real binary.

Risks:
- Not suitable for the read-only projection service unless a CLI/export hook is added, which is explicitly out of scope for the current service prototype.
- Would push toward production CLI/export changes just to observe internals.

Compatibility:
- Good for exported behavior, poor for internal projection-service API tests.

Acceptance criteria:
- Only use for behaviors already exposed by existing CLI commands.

Abort rules:
- Stop if a new CLI seam is needed solely for tests.

### Option E: Production Core Split

Refactor project-load/domain classes into a smaller production library and link both app and tests against it.

Affected later files:
- broad production CMake and source ownership boundaries

Advantages:
- Long-term cleanest dependency model.

Risks:
- This is an architecture decision, not a testtarget cleanup.
- High blast radius and outside current constraints.

Compatibility:
- Unknown without a separate architecture phase.

Acceptance criteria:
- Requires explicit future architecture approval.

Abort rules:
- Do not start under the current testtarget cleanup brief.

## Recommendation

Recommendation: choose Option A first, with Option B allowed as a small companion if setup duplication starts appearing.

Confidence: Likely.

Reason: Option A preserves the already verified behavior while preventing more copy-pasted full-app CMake blocks. It matches the current project direction: QET remains the runtime chassis, and projection services are tested against the real loaded model. It does not pretend that `QETProject` load is lightweight today, and it avoids the production refactor implied by Options C/E.

## Open Decisions

- Should the shared target be an object library, a static test-support library, or a CMake function that registers project-load tests with the same source/resource bundle?
- Should runtime setup remain local in each test for visibility, or move into a tiny test-only helper?
- How many future project-load tests are expected? If only one more, a minimal helper may be enough; if several, a shared target is safer.
- Should a later architecture phase try to split a genuine non-UI project core, or keep projection tests on the real app graph for now?

## Next Proposed Task

Review future project-load qttests against this outcome: use `add_qet_project_load_qtest(...)` only when a test genuinely needs a real loaded `QETProject`. Keep lighter unit and CLI tests on their narrower existing patterns.
