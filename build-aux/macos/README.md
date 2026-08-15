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
| `MACOS_CERTIFICATE_P12_BASE64` | Your Developer ID Application `.p12`, base64-encoded (`base64 -i cert.p12`) |
| `MACOS_CERTIFICATE_PASSWORD` | The password you set when exporting the `.p12` |
| `NOTARY_API_KEY_BASE64` | An App Store Connect API key `.p8`, base64-encoded |
| `NOTARY_KEY_ID` | That API key's Key ID |
| `NOTARY_ISSUER_ID` | That API key's Issuer ID |

If these secrets aren't set, `package_dmg.sh` still runs fine with
`--sign`/`--notarize` simply omitted from the workflow — it produces an
unsigned DMG rather than failing.

**Testing this yourself before touching the real certificate**: generate a
self-signed test certificate via Keychain Access ("Certificate Assistant →
Create a Certificate"), export and base64-encode it the same way, and add
it to secrets on your own fork. This exercises the entire signing pipeline
(keychain import, `codesign`, cleanup) without involving the real Developer
ID at all. Notarization itself will still fail with a self-signed
certificate — Apple's notarization service specifically requires a real
Developer ID — but everything up to that point is fully testable this way.

## Known gaps / TODO

- Version extraction assumes `sources/qetversion.cpp`'s
  `return QVersionNumber{...}` format doesn't change: single
  `VERSION` file would be better.
- `CMAKE_OSX_DEPLOYMENT_TARGET` gets set to the minimum version supported by the installed (using brew) Qt framework.`Info.plist`'s `LSMinimumSystemVersion` 
  is adjusted accordingly. If support of a lower version should be necessary then the Qt framework has to be built separately.
