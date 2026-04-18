param(
    [switch]$SkipHeadlessRun
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildScript = Join-Path $scriptRoot 'build-clang.ps1'
$root = Split-Path -Parent $scriptRoot
$buildDir = Join-Path $root 'build-clang'
$playerExe = Join-Path $buildDir 'nativplayer.exe'
$headlessExe = Join-Path $buildDir 'nativplayer_headless_testkit.exe'

function Resolve-LatestArtifact {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PrimaryPath,
        [Parameter(Mandatory = $true)]
        [string]$FallbackPattern
    )

    if (Test-Path $PrimaryPath) {
        return $PrimaryPath
    }

    $directory = Split-Path -Parent $PrimaryPath
    $fallback = Get-ChildItem -Path $directory -Filter $FallbackPattern -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1 -ExpandProperty FullName
    return $fallback
}

if (-not (Test-Path $buildScript)) {
    throw "Build script not found: $buildScript"
}

$invokeArgs = @(
    '-ExecutionPolicy', 'Bypass',
    '-File', $buildScript
)

if ($SkipHeadlessRun) {
    $invokeArgs += '-SkipHeadlessRun'
}

& powershell @invokeArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if (-not (Test-Path $buildDir)) {
    throw "Build output directory not found: $buildDir"
}

$resolvedPlayerExe = Resolve-LatestArtifact -PrimaryPath $playerExe -FallbackPattern 'nativplayer_latest*.exe'
$resolvedHeadlessExe = Resolve-LatestArtifact -PrimaryPath $headlessExe -FallbackPattern 'nativplayer_headless_testkit_latest*.exe'

if ([string]::IsNullOrWhiteSpace($resolvedPlayerExe)) {
    throw "Build finished but no player EXE was produced in: $buildDir"
}

Write-Output $resolvedPlayerExe
if (-not [string]::IsNullOrWhiteSpace($resolvedHeadlessExe)) {
    Write-Output $resolvedHeadlessExe
}
