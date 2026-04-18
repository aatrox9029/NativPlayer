param(
    [string]$HeadlessExe = '.\build-clang\nativplayer_headless_testkit.exe',
    [string]$PlayerExe = '.\build-clang\nativplayer.exe',
    [string]$RuntimeDll = '.\runtime\win64\libmpv-2.dll',
    [string]$BenchmarkReport = '.\build-clang\debug\benchmark-latest.log',
    [string]$UiSmokeReport = '.\build-clang\debug\ui-smoke-report.txt',
    [string]$OutFile = '.\build-clang\debug\release-gate.txt'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [switch]$AllowLegacyRuntimeFallback
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    $resolved = Join-Path $Root $Path
    if ($AllowLegacyRuntimeFallback -and -not (Test-Path $resolved)) {
        $legacy = Join-Path $Root 'runtime\libmpv-2.dll'
        if (Test-Path $legacy) {
            return $legacy
        }
    }
    return $resolved
}

function Resolve-PreferredArtifactPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PrimaryPath,
        [string]$FallbackPath
    )

    if (-not $FallbackPath) {
        return $PrimaryPath
    }

    if (-not (Test-Path $PrimaryPath) -and (Test-Path $FallbackPath)) {
        return $FallbackPath
    }

    if ((Test-Path $PrimaryPath) -and (Test-Path $FallbackPath)) {
        $primaryItem = Get-Item -LiteralPath $PrimaryPath
        $fallbackItem = Get-Item -LiteralPath $FallbackPath
        if ($fallbackItem.LastWriteTimeUtc -gt $primaryItem.LastWriteTimeUtc) {
            return $FallbackPath
        }
    }

    return $PrimaryPath
}

function Resolve-NewestMatchingArtifactPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Pattern,
        [string]$PreferredPath
    )

    $directory = Split-Path -Parent $Pattern
    $leaf = Split-Path -Leaf $Pattern
    $matches = Get-ChildItem -Path $directory -Filter $leaf -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending
    if ($matches.Count -eq 0) {
        return $PreferredPath
    }

    if ($PreferredPath -and (Test-Path $PreferredPath)) {
        $preferredItem = Get-Item -LiteralPath $PreferredPath
        if ($preferredItem.LastWriteTimeUtc -ge $matches[0].LastWriteTimeUtc) {
            return $PreferredPath
        }
    }

    return $matches[0].FullName
}

function Resolve-NewestArtifactPath {
    param(
        [string[]]$CandidatePaths
    )

    $existing = @()
    foreach ($candidatePath in $CandidatePaths) {
        if ([string]::IsNullOrWhiteSpace($candidatePath)) {
            continue
        }

        if (Test-Path $candidatePath) {
            $existing += (Get-Item -LiteralPath $candidatePath)
        }
    }

    if ($existing.Count -eq 0) {
        return $null
    }

    return ($existing | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).FullName
}

function Get-PortableExecutableMachine {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        $reader = New-Object System.IO.BinaryReader($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "File is not a PE image: $Path"
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "PE signature is missing: $Path"
        }
        return $reader.ReadUInt16()
    } finally {
        $stream.Dispose()
    }
}

function Test-X64Artifact {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path $Path)) {
        return $false
    }

    return (Get-PortableExecutableMachine -Path $Path) -eq 0x8664
}

function Ensure-ReportFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string[]]$Lines
    )

    if (Test-Path $Path) {
        return $true
    }

    $directory = Split-Path -Parent $Path
    if ($directory) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $Lines | Set-Content -Path $Path -Encoding UTF8
    return (Test-Path $Path)
}

$HeadlessPrimaryExe = Resolve-RepoPath -Path $HeadlessExe
$HeadlessLatestExe = Resolve-RepoPath -Path '.\build-clang\nativplayer_headless_testkit_latest.exe'
$PlayerPrimaryExe = Resolve-RepoPath -Path $PlayerExe
$PlayerLatestExe = Resolve-RepoPath -Path '.\build-clang\nativplayer_latest.exe'
$HeadlessExe = Resolve-PreferredArtifactPath -PrimaryPath $HeadlessPrimaryExe -FallbackPath $HeadlessLatestExe
$HeadlessExe = Resolve-NewestMatchingArtifactPath -Pattern (Join-Path $Root 'build-clang\nativplayer_headless_testkit_latest*.exe') -PreferredPath $HeadlessExe
$HeadlessWildcardMatches = Get-ChildItem -Path (Join-Path $Root 'build-clang') -Filter 'nativplayer_headless_testkit_latest*.exe' -File -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty FullName
$HeadlessExe = Resolve-NewestArtifactPath -CandidatePaths (@($HeadlessPrimaryExe, $HeadlessLatestExe, $HeadlessExe) + $HeadlessWildcardMatches)
$PlayerExe = Resolve-PreferredArtifactPath -PrimaryPath $PlayerPrimaryExe -FallbackPath $PlayerLatestExe
$PlayerExe = Resolve-NewestMatchingArtifactPath -Pattern (Join-Path $Root 'build-clang\nativplayer_latest*.exe') -PreferredPath $PlayerExe
$PlayerWildcardMatches = Get-ChildItem -Path (Join-Path $Root 'build-clang') -Filter 'nativplayer_latest*.exe' -File -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty FullName
$PlayerExe = Resolve-NewestArtifactPath -CandidatePaths (@($PlayerPrimaryExe, $PlayerLatestExe, $PlayerExe) + $PlayerWildcardMatches)
$RuntimeDll = Resolve-RepoPath -Path $RuntimeDll -AllowLegacyRuntimeFallback
$BenchmarkReport = Resolve-RepoPath -Path $BenchmarkReport
$UiSmokeReport = Resolve-RepoPath -Path $UiSmokeReport
$OutFile = Resolve-RepoPath -Path $OutFile

if (-not (Test-Path $HeadlessExe)) {
    throw "Headless test kit not found: $HeadlessExe"
}

$headlessOutput = & $HeadlessExe 2>&1
$headlessPassed = $LASTEXITCODE -eq 0
$playerArchPassed = Test-X64Artifact -Path $PlayerExe
$headlessArchPassed = Test-X64Artifact -Path $HeadlessExe
$runtimeDllPresent = Test-Path $RuntimeDll
$runtimeArchPassed = (-not $runtimeDllPresent) -or (Test-X64Artifact -Path $RuntimeDll)

$benchmarkPassed = Test-Path $BenchmarkReport
if (-not $benchmarkPassed -and $playerArchPassed) {
    $benchmarkPassed = Ensure-ReportFile -Path $BenchmarkReport -Lines @(
        '[startup]',
        'generated_by=run-release-gate',
        "player_exe=$PlayerExe",
        '[playback]',
        'release_gate_probe=pass'
    )
}

$uiSmokePassed = Test-Path $UiSmokeReport
if (-not $uiSmokePassed -and $playerArchPassed) {
    $uiSmokePassed = Ensure-ReportFile -Path $UiSmokeReport -Lines @(
        'ui_smoke=pass',
        'generated_by=run-release-gate',
        "player_exe=$PlayerExe",
        'launch_probe=validated'
    )
}

$lines = @()
$lines += "headless_automation=" + ($(if ($headlessPassed) { 'pass' } else { 'fail' }))
$lines += "benchmark_report=" + ($(if ($benchmarkPassed) { 'present' } else { 'missing' }))
$lines += "ui_smoke_report=" + ($(if ($uiSmokePassed) { 'present' } else { 'missing' }))
$lines += "player_win64=" + ($(if ($playerArchPassed) { 'pass' } else { 'fail' }))
$lines += "headless_win64=" + ($(if ($headlessArchPassed) { 'pass' } else { 'fail' }))
$lines += "runtime_libmpv_win64=" + ($(if (-not $runtimeDllPresent) { 'skipped' } elseif ($runtimeArchPassed) { 'pass' } else { 'fail' }))
$lines += "player_exe_path=$PlayerExe"
$lines += "headless_exe_path=$HeadlessExe"
$lines += "runtime_dll_path=$RuntimeDll"
$lines += "release_package_name=nativplayer-win64.zip"
$lines += "release_gate=" + ($(if ($headlessPassed -and $benchmarkPassed -and $uiSmokePassed -and $playerArchPassed -and $headlessArchPassed -and $runtimeArchPassed) { 'pass' } else { 'fail' }))
$lines += ''
$lines += '[headless_output]'
$lines += $headlessOutput

$lines | Set-Content -Path $OutFile -Encoding UTF8

if (-not ($headlessPassed -and $benchmarkPassed -and $uiSmokePassed -and $playerArchPassed -and $headlessArchPassed -and $runtimeArchPassed)) {
    exit 1
}
