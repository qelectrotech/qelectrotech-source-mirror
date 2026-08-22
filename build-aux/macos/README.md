# macOS packaging

Two scripts, one purpose each:

- **`package_dmg.sh`** — configure, build, bundle, optionally sign/notarize,
  and produce a DMG from whatever source tree is already checked out.
  Used by both CI and local runs.
- **`update_and_package.sh`** — convenience wrapper for local use only:
  pulls the latest source, then calls `package_dmg.sh`. **Never used by CI.**

## Prerequisites

- Xcode command line tools
- Homebrew, with `qt@5` and/or `qt` (Qt6) installed, plus `cmake`, `ninja`
- A git checkout of this repository

## Quick local test (unsigned)

```bash
./build-aux/macos/package_dmg.sh
```

Produces an unsigned DMG for your Mac's native architecture. Fine for
checking the app builds and launches; not something to distribute.

## How the build actually works

`package_dmg.sh` does five things, in order:

1. **Locates Qt** via Homebrew (`qt@5` or `qt`, depending on `--qt-version`).
2. **Detects the real minimum macOS version directly from Qt itself**,
   rather than a hardcoded guess. `QtCore`'s own build-version load command
   records the actual floor the installed Qt build requires — this is the
   real, binding constraint for the whole bundle, since `dyld` refuses to
   load Qt on anything older regardless of what we claim in our own
   `CMAKE_OSX_DEPLOYMENT_TARGET`. Handles both load command formats Qt
   ships across its supported versions (`LC_BUILD_VERSION`'s `minos` field,
   used by current Qt6 builds, and the older `LC_VERSION_MIN_MACOSX`'s
   `version` field, still used by some Qt5 builds) — parsed with awk-based
   load-command-context tracking rather than a flat grep, since both blocks
   can contain lines that look similar out of context (notably,
   `LC_BUILD_VERSION` blocks also list build-tool versions like the linker's
   own version number, which must not be confused with the OS minimum).
3. **Configures and builds via CMake** (`cmake -S ... -B ...`), producing a
   genuine `.app` bundle — `CMakeLists.txt` has an `elseif(APPLE)` branch on
   the main executable target using the `MACOSX_BUNDLE` keyword and an
   explicit `install(TARGETS ... BUNDLE DESTINATION .)` rule, both required
   for CMake to produce bundle output at all rather than a flat Linux-style
   install tree.
4. **Reorganizes CMake's install output into proper bundle structure.**
   `CMakeLists.txt`'s install rules for the resource directories (`lang`,
   `elements`, `examples`, `titleblocks`) and the app icon are not yet
   bundle-aware — they land in a flat layout alongside the bundle rather
   than inside `Contents/Resources/`. This script moves them into place
   after the fact. (There's a documented, more "correct" alternative using
   CMake generator expressions — `$<TARGET_BUNDLE_CONTENT_DIR:...>` — that
   would fix this at the CMake layer instead; deliberately not done yet,
   since it's a larger, riskier change than fixing it here. Worth
   revisiting if this reorganization step ever becomes a maintenance
   burden.)
5. **Runs `macdeployqt`**, optionally signs and notarizes, creates the DMG
   (with a retry loop — `hdiutil create` has shown occasional transient
   "Resource busy" failures on CI runners), and optionally signs/notarizes
   the DMG itself.

Once `CMakeLists.txt` gains `qt_generate_deploy_app_script()` (Qt 6.3+;
tracked separately, not yet done), step 5's explicit `macdeployqt` call
becomes unnecessary — `cmake --install` would trigger it automatically.

## Local build with your own certificate already in your keychain

```bash
./build-aux/macos/package_dmg.sh --sign
```

Finds the first "Developer ID Application" identity in your default
keychain automatically. Override with `QET_SIGNING_IDENTITY` if you have
more than one and want a specific one.

## Local build with notarization

Requires credentials stored once via:

```bash
xcrun notarytool store-credentials "org.qelectrotech" \
  --apple-id <your-apple-id> --team-id <your-team-id> --password <app-specific-password>
```

Then:

```bash
./build-aux/macos/package_dmg.sh --sign --notarize
```

You'll be prompted to confirm before notarization actually runs (it's slow
and consumes Apple's rate limits). Use a different stored profile name via
`QET_NOTARIZE_PROFILE` if needed.

## Repeated local builds, always against the latest source

```bash
./build-aux/macos/update_and_package.sh --sign --notarize
```

## CI signing and notarization setup

CI never reads your local keychain — it needs its own copy of the
certificate and API credentials, provided as GitHub Actions secrets:

| Secret | What it is |
|---|---|
| `MACOS_CERTIFICATE_P12_BASE64` | Your Developer ID Application `.p12`, base64-encoded |
| `MACOS_CERTIFICATE_PASSWORD` | The password you set when exporting the `.p12` |
| `NOTARY_API_KEY_BASE64` | An App Store Connect API key `.p8`, base64-encoded |
| `NOTARY_KEY_ID` | That API key's Key ID |
| `NOTARY_ISSUER_ID` | That API key's Issuer ID |

If these secrets aren't set, `package_dmg.sh` still runs fine with
`--sign`/`--notarize` simply omitted from the workflow — it produces an
unsigned DMG rather than failing.

**Testing this yourself with a throwaway certificate**: OpenSSL (on Linux
or macOS) can generate a fully valid, `codesign`-compatible self-signed
test certificate — no real Mac or Apple Developer account required to
validate the *mechanism*. Generate one named literally
`Developer ID Application: Test User (...)` (the script searches for that
exact string pattern, matching how real Developer ID certificates are
named), export as `.p12`, base64-encode, add to secrets **on your own
fork**, never Laurent's real repo. Notarization itself will still fail
against a self-signed cert — Apple's notarization service specifically
requires a real Developer ID — but signing and the whole keychain-import
mechanism are fully testable this way.

** Status: signing mechanism validated up through certificate/keychain
import; not yet confirmed end-to-end against a real Developer ID
certificate.** Every unsigned-path run (version detection, bundle
structure, DMG creation) is confirmed working across the full
arm64/x86_64 × Qt5/Qt6 matrix. The signing path was debugged extensively
with a self-signed test certificate — working through several real
certificate-format compatibility issues (see below) — but was not carried
through to a final confirmed `security import` + `codesign` success before
other work took priority. **This needs to be either finished with the test
certificate, or validated directly with Laurent's real one, before
`--sign`/`--notarize` should be trusted in a real release pipeline.**

### A note on certificate format, if signing issues come up again

If `security import` ever reports `Unknown format in import` against a
`.p12` that OpenSSL itself parses without complaint, this is very likely a
PKCS#12 encryption-scheme compatibility issue, not a corrupted file.
OpenSSL 3.x's default export encryption (AES-256+PBKDF2) is not readable
by `security import` on macOS versions before 15 — but macOS 15+ actually
*added* support for it (per Apple's own DTS engineers). Given GitHub's
`macos-latest` runner has moved to macOS 26, the modern OpenSSL default
should now work without any special flags at all — the various
`-legacy`/`-certpbe PBE-SHA1-3DES` workarounds explored during earlier
debugging address an *older* runner-compatibility problem that may no
longer actually apply on current infrastructure. If you hit this error
again, check `sw_vers` on the actual runner first before reaching for a
legacy-crypto flag.

## Runner matrix

`macos-package.yml` pairs architecture and runner explicitly (not a
crossed matrix dimension, to avoid needing `exclude:` entries for the two
nonsensical combinations):

| `arch` | `runner` |
|---|---|
| `arm64` | `macos-latest` (native Apple Silicon) |
| `x86_64` | `macos-26-intel` (genuine Intel hardware, not cross-compiled) |

**`macos-26-intel` is a time-limited option** — it's GitHub's last planned
Intel macOS image, expected to be retired around August 2027 (Apple is
discontinuing x86_64 support macOS-side). After that, continued x86_64
support would need cross-compilation from an Apple Silicon host instead
(technically straightforward for the compiler itself via
`-DCMAKE_OSX_ARCHITECTURES=x86_64`, but requires a second, Rosetta-emulated
Homebrew installation to get x86_64 Qt libraries — meaningfully more
fragile than native compilation). Not an issue today; worth planning for
before mid-2027.

**Supporting macOS versions older than what Homebrew's current Qt bottles
require** (a genuinely separate question from the runner architecture
above) would need building Qt from source in-house — a large, ongoing
maintenance commitment (multi-hour builds, becoming responsible for
tracking Qt's own security patches). 

## Known gaps / TODO

- Signing/notarization path needs final end-to-end confirmation (see above).
- `qt_generate_deploy_app_script()` CMake integration not yet added
  (would simplify step 5, not required for correctness).
- The install-output reorganization (step 4) is a pragmatic script-side fix
  rather than the more "correct" CMake generator-expression approach —
  documented as Option A vs. Option B in project discussion; revisit if
  the reorganization logic becomes a maintenance burden.
- Not extensively tested on real end-user hardware beyond structural
  verification of the produced bundle.
