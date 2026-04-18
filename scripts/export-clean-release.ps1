param()

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-clang'
$ReleaseRoot = Join-Path $BuildDir 'release-clean'
$OpenSourceDir = Join-Path $ReleaseRoot 'nativplayer-open-source-en'
$UserDir = Join-Path $ReleaseRoot 'nativplayer-win64-user'
$InstallerDir = Join-Path $ReleaseRoot 'installer'
$DefaultLanguageHintFileName = 'nativplayer.default-language.txt'

function Resolve-LatestArtifactPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PrimaryPath,
        [Parameter(Mandatory = $true)]
        [string]$Pattern
    )

    if (Test-Path $PrimaryPath) {
        return $PrimaryPath
    }

    $directory = Split-Path -Parent $PrimaryPath
    $candidate = Get-ChildItem -Path $directory -Filter $Pattern -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1 -ExpandProperty FullName
    return $candidate
}

function Reset-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Copy-IfExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,
        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        return
    }

    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

$PlayerExe = Resolve-LatestArtifactPath -PrimaryPath (Join-Path $BuildDir 'nativplayer.exe') -Pattern 'nativplayer_latest*.exe'
$HeadlessExe = Resolve-LatestArtifactPath -PrimaryPath (Join-Path $BuildDir 'nativplayer_headless_testkit.exe') -Pattern 'nativplayer_headless_testkit_latest*.exe'

if ([string]::IsNullOrWhiteSpace($PlayerExe) -or -not (Test-Path $PlayerExe)) {
    throw "Player EXE not found. Build the project first."
}
if ([string]::IsNullOrWhiteSpace($HeadlessExe) -or -not (Test-Path $HeadlessExe)) {
    throw "Headless Automation Test Kit not found. Build the project first."
}

Reset-Directory -Path $ReleaseRoot
Reset-Directory -Path $OpenSourceDir
Reset-Directory -Path $UserDir
Reset-Directory -Path $InstallerDir

$openSourceDirectories = @(
    '.vscode',
    'app',
    'button-style',
    'common',
    'config',
    'diagnostics',
    'include',
    'installer',
    'localization',
    'platform',
    'playback',
    'release',
    'resources',
    'scripts',
    'tests',
    'third_party',
    'ui'
)

$openSourceFiles = @(
    'CMakeLists.txt',
    'README.md',
    'logo.png',
    'setup.bat',
    'quick_build.bat'
)

foreach ($directory in $openSourceDirectories) {
    Copy-IfExists -Source (Join-Path $Root $directory) -Destination (Join-Path $OpenSourceDir $directory)
}
foreach ($file in $openSourceFiles) {
    Copy-IfExists -Source (Join-Path $Root $file) -Destination (Join-Path $OpenSourceDir $file)
}

Copy-Item -LiteralPath (Join-Path $Root 'release\README.open-source.en.md') -Destination (Join-Path $OpenSourceDir 'README.md') -Force

Copy-Item -LiteralPath $PlayerExe -Destination (Join-Path $UserDir 'nativplayer.exe') -Force
Copy-Item -LiteralPath $HeadlessExe -Destination (Join-Path $UserDir 'nativplayer_headless_testkit.exe') -Force
Copy-IfExists -Source (Join-Path $BuildDir 'button-style') -Destination (Join-Path $UserDir 'button-style')
Copy-Item -LiteralPath (Join-Path $Root 'release\README.en.md') -Destination (Join-Path $UserDir 'README.md') -Force
Set-Content -LiteralPath (Join-Path $UserDir $DefaultLanguageHintFileName) -Value 'en-US' -Encoding Ascii

$installerArtifacts = Get-ChildItem -Path (Join-Path $BuildDir 'installer') -Filter '*.exe' -File -ErrorAction SilentlyContinue
foreach ($artifact in $installerArtifacts) {
    Copy-Item -LiteralPath $artifact.FullName -Destination (Join-Path $InstallerDir $artifact.Name) -Force
}

$summary = @(
    "OPEN_SOURCE_DIR=$OpenSourceDir",
    "USER_DIR=$UserDir",
    "PLAYER_EXE=$(Join-Path $UserDir 'nativplayer.exe')",
    "HEADLESS_EXE=$(Join-Path $UserDir 'nativplayer_headless_testkit.exe')",
    "INSTALLER_DIR=$InstallerDir",
    "LIBMPV_INCLUDED=false"
)
Set-Content -LiteralPath (Join-Path $ReleaseRoot 'release-clean-summary.txt') -Value $summary -Encoding UTF8

Write-Output $OpenSourceDir
Write-Output $UserDir
