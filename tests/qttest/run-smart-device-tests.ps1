param(
    [string]$MsysRoot = 'C:\msys64',
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\..\build-debug'),
    [int]$Jobs = 8,
    [switch]$SkipApplicationBuild
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
$oldPath = $env:Path
$oldPlatform = $env:QT_QPA_PLATFORM
$oldFontDir = $env:QT_QPA_FONTDIR
$oldArtifacts = $env:QET_SMART_DEVICE_ARTIFACT_DIR
try {
    $env:Path = "$MsysRoot\ucrt64\bin;$MsysRoot\usr\bin;$env:Path"
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Push-Location $BuildDir
    try {
        if (-not $SkipApplicationBuild) {
            & qmake-qt5 (Join-Path $repo 'qelectrotech.pro') -spec win32-g++ CONFIG+=debug CONFIG+=no_kf5 CONFIG-=release DEFINES+=BUILD_WITHOUT_KF
            if ($LASTEXITCODE) { throw 'Application qmake failed' }
            & mingw32-make "-j$Jobs"
            if ($LASTEXITCODE) { throw 'Application build failed' }
        }
    } finally { Pop-Location }

    $testDir = Join-Path $BuildDir 'smart-device-tests'
    New-Item -ItemType Directory -Force -Path $testDir | Out-Null
    Push-Location $testDir
    try {
        & qmake-qt5 (Join-Path $PSScriptRoot 'smart_device.pro') "QET_BUILD_DIR=$($BuildDir.Replace('\', '/'))" CONFIG+=debug CONFIG-=release
        if ($LASTEXITCODE) { throw 'Test qmake failed' }
        & mingw32-make "-j$Jobs"
        if ($LASTEXITCODE) { throw 'Test build failed' }
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_QPA_FONTDIR = Join-Path $env:WINDIR 'Fonts'
        $env:QET_SMART_DEVICE_ARTIFACT_DIR = Join-Path $testDir 'artifacts'
        & .\debug\tst_smart_device.exe -o results.txt,txt
        $result = $LASTEXITCODE
        Get-Content results.txt
        if ($result) { throw "Smart device tests failed: $result" }
    } finally { Pop-Location }
} finally {
    $env:Path = $oldPath
    $env:QT_QPA_PLATFORM = $oldPlatform
    $env:QT_QPA_FONTDIR = $oldFontDir
    $env:QET_SMART_DEVICE_ARTIFACT_DIR = $oldArtifacts
}
