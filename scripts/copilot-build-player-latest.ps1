param()

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$cmakeText = Get-Content '.\CMakeLists.txt' -Raw
$match = [regex]::Match($cmakeText, 'set\(VELO_PLAYER_SOURCES\s+(.*?)\)', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $match.Success) {
    throw 'Unable to locate VELO_PLAYER_SOURCES in CMakeLists.txt.'
}

$sources = $match.Groups[1].Value -split "`r?`n" |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -and -not $_.StartsWith('#') }

$buildDir = '.\build-clang'
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

$timestamp = Get-Date -Format 'yyyyMMddHHmmss'
$latestExe = Join-Path $buildDir ("nativplayer_latest_{0}.exe" -f $timestamp)
$primaryExe = Join-Path $buildDir 'nativplayer.exe'

$args = @(
    '/std:c++20',
    '/EHsc',
    '/DUNICODE',
    '/D_UNICODE',
    '/DWIN32_LEAN_AND_MEAN',
    '/DNOMINMAX',
    '/I', '.',
    '/I', '.\include'
) + $sources + @(
    "/Fe:$latestExe",
    '/link',
    '/MACHINE:X64',
    'user32.lib',
    'gdi32.lib',
    'gdiplus.lib',
    'comdlg32.lib',
    'comctl32.lib',
    'shell32.lib',
    'ole32.lib',
    'advapi32.lib',
    'shlwapi.lib',
    'dbghelp.lib',
    'bcrypt.lib',
    'winhttp.lib',
    'winmm.lib',
    'msimg32.lib',
    'uxtheme.lib'
)

& clang-cl @args
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

try {
    Copy-Item -LiteralPath $latestExe -Destination $primaryExe -Force
} catch {
}

if (Test-Path '.\runtime\win64\libmpv-2.dll') {
    Copy-Item '.\runtime\win64\libmpv-2.dll' '.\build-clang\libmpv-2.dll' -Force
} elseif (Test-Path '.\runtime\libmpv-2.dll') {
    Copy-Item '.\runtime\libmpv-2.dll' '.\build-clang\libmpv-2.dll' -Force
}

if (Test-Path '.\runtime\mpv-2.dll') {
    Copy-Item '.\runtime\mpv-2.dll' '.\build-clang\mpv-2.dll' -Force
}

if (Test-Path '.\build-clang\button-style') {
    Remove-Item '.\build-clang\button-style' -Recurse -Force -ErrorAction SilentlyContinue
}
if (Test-Path '.\button-style') {
    Copy-Item '.\button-style' '.\build-clang\button-style' -Recurse -Force
}

Write-Output $latestExe
