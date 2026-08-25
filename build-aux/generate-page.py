#!/usr/bin/env python3
"""
generate-page.py — Generates gh-pages/index.html for QElectroTech development builds.

Called from package.yml

Environment variables required:
  DATE, SHORT, REPO, SHA, RUN_URL, RUN_NUMBER, RELEASE_TAG

Optional (Windows - omitted entirely if empty):
    INSTALLER_URL, PORTABLE_URL, MSI_URL

Optional (macOS - omitted entirely if empty):
  DMG_ARM64_URL, DMG_X8664_URL

Optional (Linux AppImage - omitted entirely if empty):
  APPIMAGE_X8664_URL, APPIMAGE_AARCH64_URL

Optional (Linux Snap - omitted entirely if empty):
  SNAP_AMD64_URL, SNAP_ARM64_URL

Optional (Linux Flatpak - omitted entirely if empty):
  FLATPAK_X8664_URL, FLATPAK_AARCH64_URL
"""
import os

date         = os.environ.get("DATE", "")
short        = os.environ.get("SHORT", "")
repo         = os.environ.get("REPO", "")
sha          = os.environ.get("SHA", "")
run_url      = os.environ.get("RUN_URL", "")
run_number   = os.environ.get("RUN_NUMBER", "")
release_tag  = os.environ.get("RELEASE_TAG", "")

installer_url = os.environ.get("INSTALLER_URL", "")
portable_url  = os.environ.get("PORTABLE_URL", "")
msi_url       = os.environ.get("MSI_URL", "")

appimage_aarch64_url = os.environ.get("APPIMAGE_AARCH64_URL", "")
appimage_x8664_url   = os.environ.get("APPIMAGE_X8664_URL", "")

dmg_arm64_url  = os.environ.get("DMG_ARM64_URL", "")
dmg_x8664_url  = os.environ.get("DMG_X8664_URL", "")

snap_amd64_url = os.environ.get("SNAP_AMD64_URL", "")
snap_arm64_url = os.environ.get("SNAP_ARM64_URL", "")

flatpak_x8664_url   = os.environ.get("FLATPAK_X8664_URL", "")
flatpak_aarch64_url = os.environ.get("FLATPAK_AARCH64_URL", "")

msi_block = ""
if msi_url:
    msi_block = f"""
<a class="btn btn-msi" href="{msi_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Windows Installer .msi<small>.msi &mdash; for enterprise / GPO deployment</small></span>
</a>"""

# --- Windows ----------------------------------------------------------------------
windows_installer_btn = ""
if installer_url:
    windows_installer_btn = f"""
<a class="btn btn-primary" href="{installer_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Windows Installer<small>.exe &mdash; recommended, includes all dependencies</small></span>
</a>"""

windows_portable_btn = ""
if portable_url:
    windows_portable_btn = f"""
<a class="btn btn-secondary" href="{portable_url}">
<span class="btn-icon">&#128230;</span>
<span class="btn-text">Windows Portable<small>.zip &mdash; no installation required, extract and run &quot;Lancer QET.bat&quot;</small></span>
</a>"""

windows_block = ""
if installer_url or portable_url or msi_url:
    windows_block = f"""
<div class="card">
<h2>&#127993; Windows &mdash; x86_64 </h2>
<div class="downloads">
{windows_installer_btn}
{msi_block}
{windows_portable_btn}
</div>
</div>"""

# --- macOS  -------------------------------------------
macos_arm64_btn = ""
if dmg_arm64_url:
    macos_arm64_btn = f"""
<a class="btn btn-primary" href="{dmg_arm64_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">macOS Apple Silicon (arm64)<small>.dmg &mdash; for M1/M2/M3/M4 Macs</small></span>
</a>"""

macos_x8664_btn = ""
if dmg_x8664_url:
    macos_x8664_btn = f"""
<a class="btn btn-secondary" href="{dmg_x8664_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">macOS Intel (x86_64)<small>.dmg &mdash; for Intel-based Macs</small></span>
</a>"""

macos_block = ""
if dmg_arm64_url or dmg_x8664_url:
    macos_block = f"""
<div class="card">
<h2>&#127838; macOS </h2>
<div class="downloads">
{macos_arm64_btn}
{macos_x8664_btn}
</div>
</div>"""

# --- Linux / AppImage  -----------
appimage_x8664_btn = ""
if appimage_x8664_url:
    appimage_x8664_btn = f"""
<a class="btn btn-primary" href="{appimage_x8664_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux x86_64 AppImage<small>.AppImage &mdash; chmod +x and run, no installation required</small></span>
</a>"""

appimage_aarch64_btn = ""
if appimage_aarch64_url:
    appimage_aarch64_btn = f"""
<a class="btn btn-secondary" href="{appimage_aarch64_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux aarch64 AppImage<small>.AppImage &mdash; chmod +x and run, no installation required</small></span>
</a>"""

appimage_block = ""
if appimage_x8664_url or appimage_aarch64_url:
    appimage_block = f"""
<div class="card">
<h2>&#128039; Linux &mdash; AppImage </h2>
<div class="downloads">
{appimage_x8664_btn}
{appimage_aarch64_btn}
</div>
</div>"""

# --- Linux / Snap -----------
# not yet published to the Snap Store -- these are raw, unsigned bundles,
# which need --dangerous to install manually.
snap_amd64_btn = ""
if snap_amd64_url:
    snap_amd64_btn = f"""
<a class="btn btn-primary" href="{snap_amd64_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux amd64 Snap<small>.snap &mdash; sudo snap install --dangerous ./&lt;file&gt;.snap</small></span>
</a>"""

snap_arm64_btn = ""
if snap_arm64_url:
    snap_arm64_btn = f"""
<a class="btn btn-secondary" href="{snap_arm64_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux arm64 Snap<small>.snap &mdash; sudo snap install --dangerous ./&lt;file&gt;.snap</small></span>
</a>"""

snap_block = ""
if snap_amd64_url or snap_arm64_url:
    snap_block = f"""
<div class="card">
<h2>&#128230; Linux &mdash; Snap</h2>
<div class="warning">
&#9888;&#65039; Not yet published to the Snap Store &mdash; this is a raw, unsigned
bundle. Requires the <code>--dangerous</code> flag to install manually, since
it isn't signed by the Store.
</div>
<div class="downloads">
{snap_amd64_btn}
{snap_arm64_btn}
</div>
</div>"""

# --- Linux / Flatpak -----------
# a raw .flatpak bundle is a normal, supported distribution format -- no
# special flags or caveats needed to install one.
flatpak_x8664_btn = ""
if flatpak_x8664_url:
    flatpak_x8664_btn = f"""
<a class="btn btn-primary" href="{flatpak_x8664_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux x86_64 Flatpak<small>.flatpak &mdash; flatpak install ./&lt;file&gt;.flatpak</small></span>
</a>"""

flatpak_aarch64_btn = ""
if flatpak_aarch64_url:
    flatpak_aarch64_btn = f"""
<a class="btn btn-secondary" href="{flatpak_aarch64_url}">
<span class="btn-icon">&#11015;</span>
<span class="btn-text">Linux aarch64 Flatpak<small>.flatpak &mdash; flatpak install ./&lt;file&gt;.flatpak</small></span>
</a>"""

flatpak_block = ""
if flatpak_x8664_url or flatpak_aarch64_url:
    flatpak_block = f"""
<div class="card">
<h2>&#128230; Linux &mdash; Flatpak</h2>
<div class="downloads">
{flatpak_x8664_btn}
{flatpak_aarch64_btn}
</div>
</div>"""

html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>QElectroTech &ndash; Development Builds</title>
<style>
*,*::before,*::after{{box-sizing:border-box;margin:0;padding:0}}
body{{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:#f0f4f8;color:#2d3748;min-height:100vh}}
header{{background:linear-gradient(135deg,#1a365d 0%,#2b6cb0 100%);color:white;padding:48px 24px 40px;text-align:center}}
header h1{{font-size:2.2em;letter-spacing:-0.5px;margin-bottom:8px}}
header p{{opacity:.8;font-size:1.05em}}
main{{max-width:680px;margin:40px auto;padding:0 20px 60px}}
.card{{background:white;border-radius:12px;padding:28px;margin-bottom:24px;box-shadow:0 2px 12px rgba(0,0,0,.08)}}
.card h2{{font-size:1em;text-transform:uppercase;letter-spacing:.06em;color:#718096;margin-bottom:16px}}
.meta{{font-size:.875em;color:#4a5568;line-height:1.8;margin-bottom:20px}}
.meta a{{color:#2b6cb0;text-decoration:none}}
.meta a:hover{{text-decoration:underline}}
.badge{{display:inline-block;background:#ebf8ff;color:#2b6cb0;border-radius:4px;font-size:.8em;font-weight:600;padding:2px 8px;margin-left:6px;vertical-align:middle}}
.warning{{background:#fffbeb;border-left:4px solid #f6ad55;border-radius:4px;padding:12px 16px;font-size:.875em;color:#744210;margin-bottom:24px;line-height:1.5}}
.warning a{{color:#c05621}}
.downloads{{display:flex;flex-direction:column;gap:12px}}
.btn{{display:flex;align-items:center;gap:12px;padding:14px 20px;border-radius:8px;font-size:.95em;font-weight:600;text-decoration:none;transition:transform .1s,box-shadow .1s}}
.btn:hover{{transform:translateY(-1px);box-shadow:0 4px 12px rgba(0,0,0,.15)}}
.btn-primary{{background:#2b6cb0;color:white}}
.btn-msi{{background:#6b46c1;color:white}}
.btn-secondary{{background:#edf2f7;color:#2d3748}}
.btn-icon{{font-size:1.3em}}
.btn-text small{{display:block;font-weight:400;font-size:.8em;opacity:.75;margin-top:1px}}
footer{{text-align:center;font-size:.8em;color:#a0aec0;padding:32px 0 0}}
footer a{{color:#718096;text-decoration:none}}
</style>
</head>
<body>
<header>
<h1>&#9889; QElectroTech</h1>
<p>Development Builds</p>
</header>
<main>
<div class="card">
<h2>Build info</h2>
<div class="meta">
&#128197; &nbsp;<strong>{date}</strong><br>
&#128256; &nbsp;Commit <a href="https://github.com/{repo}/commit/{sha}"><code>{short}</code></a><br>
&#128295; &nbsp;<a href="{run_url}">CI Run #{run_number}</a>
<span class="badge">development</span>
</div>
<div class="warning">
&#9888;&#65039; This is a development version generated automatically from the newest commit on the master branch, It might introduce new features which you might want, but it may also exhibit new bugs that have not yet been identified yet.
For production use, download a <a href="https://github.com/{repo}/releases">stable release</a>.
</div>
<a class="btn btn-secondary" href="https://github.com/{repo}/releases/tag/{release_tag}">
<span class="btn-icon">&#128230;</span>
<span class="btn-text">All development version binaries on GitHub<small>Every platform &mdash; release page with checksums</small></span>
</a>
</div>
{windows_block}
{macos_block}
{appimage_block}
{snap_block}
{flatpak_block}
</main>
<footer>
Auto-generated by GitHub Actions &nbsp;&middot;&nbsp;
<a href="https://github.com/{repo}">Source on GitHub</a> &nbsp;&middot;&nbsp;
<a href="https://qelectrotech.org">qelectrotech.org</a> &nbsp;&middot;&nbsp;
<a href="docs/">API documentation</a>
</footer>
</body>
</html>"""

os.makedirs("gh-pages", exist_ok=True)
with open("gh-pages/index.html", "w", encoding="utf-8") as f:
    f.write(html)

print("index.html written OK")
