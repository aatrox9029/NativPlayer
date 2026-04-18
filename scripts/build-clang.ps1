param(
    [switch]$HeadlessOnly,
    [switch]$SkipHeadlessRun,
    [string]$TraceFile = ''
)

$ErrorActionPreference = 'Stop'
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-clang'
$DebugDir = Join-Path $BuildDir 'debug'
$PlayerExe = Join-Path $BuildDir 'nativplayer.exe'
$HeadlessExe = Join-Path $BuildDir 'nativplayer_headless_testkit.exe'
$HeadlessLatestExe = Join-Path $BuildDir 'nativplayer_headless_testkit_latest.exe'
$HeadlessRunExe = Join-Path $BuildDir ("vp_suite_runner_{0}.exe" -f ([Guid]::NewGuid().ToString('N')))
$BuildRuntimeMpv = Join-Path $BuildDir 'mpv-2.dll'
$BuildLogo = Join-Path $BuildDir 'logo.png'
$BuildButtonStyleDir = Join-Path $BuildDir 'button-style'
$AppIconScript = Join-Path $PSScriptRoot 'sync-app-icon.ps1'
$AppIconResourceScript = Join-Path $Root 'resources\app_icon.rc'
$BuildAppIconResource = Join-Path $BuildDir 'nativplayer_icon.res'

function Write-TraceStep {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if ([string]::IsNullOrWhiteSpace($TraceFile)) {
        return
    }

    Add-Content -LiteralPath $TraceFile -Value ("{0} {1}" -f ([DateTime]::Now.ToString('o')), $Message)
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
if (-not (Test-Path $DebugDir)) {
    New-Item -ItemType Directory -Path $DebugDir | Out-Null
}

$TransientPatterns = @(
    '*_tmp.exe',
    '*_codex.exe',
    '*_uiaudit_*.exe',
    'vp_validation_runner*.exe',
    'vp_validation_runner*.txt',
    'vp_validation_runner*.meta.txt',
    'vp_flicker_validation*.exe',
    'vp_flicker_validation*.txt',
    'vp_flicker_validation*.meta.txt',
    'vp_suite_*.exe',
    'vp_suite_*.tmp',
    'vp_suite_runner.exe'
)

$StaleRuntimeCopies = @(
    (Join-Path $BuildDir 'libmpv-2.dll'),
    (Join-Path $BuildDir 'mpv-2.dll')
)

foreach ($pattern in $TransientPatterns) {
    Get-ChildItem -Path $BuildDir -Filter $pattern -File -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
}

foreach ($staleRuntimeCopy in $StaleRuntimeCopies) {
    Remove-Item -LiteralPath $staleRuntimeCopy -Force -ErrorAction SilentlyContinue
}
Remove-Item -LiteralPath $BuildLogo -Force -ErrorAction SilentlyContinue

$CommonFlags = @(
    '/std:c++20',
    '/EHsc',
    '/DUNICODE',
    '/D_UNICODE',
    '/DWIN32_LEAN_AND_MEAN',
    '/DNOMINMAX',
    '/I', $Root,
    '/I', (Join-Path $Root 'include')
)

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $escapeArgument = {
        param([string]$Value)
        if ($null -eq $Value) {
            return '""'
        }

        if ($Value -notmatch '[\s"]') {
            return $Value
        }

        $escaped = $Value -replace '(\\*)"', '$1$1\"'
        $escaped = $escaped -replace '(\\+)$', '$1$1'
        return '"' + $escaped + '"'
    }

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = (($Arguments | ForEach-Object { & $escapeArgument $_ }) -join ' ')
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $startInfo.WorkingDirectory = $Root

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    $output = @()
    if (-not [string]::IsNullOrEmpty($stdout)) {
        $output += $stdout.TrimEnd("`r", "`n")
    }
    if (-not [string]::IsNullOrEmpty($stderr)) {
        $output += $stderr.TrimEnd("`r", "`n")
    }
    return @{
        ExitCode = $process.ExitCode
        Output = ($output -join [Environment]::NewLine)
    }
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
        if ($peOffset -lt 0) {
            throw "Invalid PE header offset: $Path"
        }

        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "PE signature is missing: $Path"
        }

        return $reader.ReadUInt16()
    } finally {
        $stream.Dispose()
    }
}

function Get-PortableExecutableMachineName {
    param(
        [Parameter(Mandatory = $true)]
        [UInt16]$Machine
    )

    switch ($Machine) {
        0x014C { return 'x86' }
        0x8664 { return 'x64' }
        0xAA64 { return 'arm64' }
        default { return ('unknown(0x{0:X4})' -f $Machine) }
    }
}

function Assert-PortableExecutableMachine {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [UInt16]$ExpectedMachine,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path $Path)) {
        throw "$Label not found: $Path"
    }

    $actualMachine = Get-PortableExecutableMachine -Path $Path
    if ($actualMachine -ne $ExpectedMachine) {
        throw "$Label must be x64. Actual architecture: $(Get-PortableExecutableMachineName -Machine $actualMachine)"
    }
}

function Assert-X64Toolchain {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory
    )

    $probeSource = Join-Path $BuildDirectory 'toolchain_arch_probe.cpp'
    $probeObject = Join-Path $BuildDirectory 'toolchain_arch_probe.obj'
    @'
#ifndef _M_X64
#error NativPlayer requires an x64 clang-cl toolchain. Open an x64 developer shell before building.
#endif
int main() { return 0; }
'@ | Set-Content -LiteralPath $probeSource -Encoding Ascii

    try {
        $probeResult = Invoke-NativeCommand -FilePath 'clang-cl' -Arguments @('/nologo', '/c', '/TP', "/Fo$probeObject", $probeSource)
        if ($probeResult.ExitCode -ne 0) {
            throw ($probeResult.Output | Out-String)
        }
    } finally {
        Remove-Item $probeSource, $probeObject -Force -ErrorAction SilentlyContinue
    }
}

function Get-SourcesFromCMakeSet {
    param(
        [string]$CMakeText,
        [string]$SetName
    )

    $match = [regex]::Match($CMakeText, "set\($SetName\s+(.*?)\)", [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) {
        throw "Unable to locate source set '$SetName' in CMakeLists.txt"
    }

    return ($match.Groups[1].Value -split "`r?`n" |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and -not $_.StartsWith('#') })
}

function Remove-StalePlayerFallbacks {
    param(
        [string]$BuildDirectory,
        [string]$KeepPath
    )

    $fallbacks = Get-ChildItem -Path $BuildDirectory -Filter 'nativplayer_latest*.exe' -File -ErrorAction SilentlyContinue
    foreach ($fallback in $fallbacks) {
        if ($KeepPath -and [System.StringComparer]::OrdinalIgnoreCase.Equals($fallback.FullName, $KeepPath)) {
            continue
        }
        try {
            Remove-Item -LiteralPath $fallback.FullName -Force
        } catch {
            Write-Warning "Unable to delete stale fallback artifact '$($fallback.Name)'."
        }
    }
}

function Remove-StaleHeadlessFallbacks {
    param(
        [string]$BuildDirectory,
        [string]$KeepPath
    )

    $fallbacks = Get-ChildItem -Path $BuildDirectory -Filter 'nativplayer_headless_testkit_latest_*.exe' -File -ErrorAction SilentlyContinue
    foreach ($fallback in $fallbacks) {
        if ($KeepPath -and [System.StringComparer]::OrdinalIgnoreCase.Equals($fallback.FullName, $KeepPath)) {
            continue
        }
        try {
            Remove-Item -LiteralPath $fallback.FullName -Force
        } catch {
            Write-Warning "Unable to delete stale headless fallback artifact '$($fallback.Name)'."
        }
    }
}

function Get-PlayerFallbackPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory
    )

    return Join-Path $BuildDirectory ("nativplayer_latest_{0}.exe" -f ([DateTime]::UtcNow.ToString('yyyyMMddHHmmssfff')))
}

function Get-HeadlessFallbackPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory,
        [Parameter(Mandatory = $true)]
        [string]$RunnerPath
    )

    if (Test-Path $RunnerPath) {
        $runnerItem = Get-Item -LiteralPath $RunnerPath
        return Join-Path $BuildDirectory ("nativplayer_headless_testkit_latest_{0}.exe" -f $runnerItem.LastWriteTimeUtc.ToString('yyyyMMddHHmmssfff'))
    }

    return Join-Path $BuildDirectory ("nativplayer_headless_testkit_latest_{0}.exe" -f ([DateTime]::UtcNow.ToString('yyyyMMddHHmmssfff')))
}

function Move-BuildDiagnosticsToDebugDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory,
        [Parameter(Mandatory = $true)]
        [string]$DebugDirectory
    )

    if (-not (Test-Path $DebugDirectory)) {
        New-Item -ItemType Directory -Path $DebugDirectory | Out-Null
    }

    foreach ($pattern in @('*.txt', '*.log', '*.json')) {
        $artifacts = Get-ChildItem -Path $BuildDirectory -Filter $pattern -File -ErrorAction SilentlyContinue
        foreach ($artifact in $artifacts) {
            $destination = Join-Path $DebugDirectory $artifact.Name
            if ([System.StringComparer]::OrdinalIgnoreCase.Equals($artifact.FullName, $destination)) {
                continue
            }
            Move-Item -LiteralPath $artifact.FullName -Destination $destination -Force
        }
    }
}

function Invoke-PlayerBuild {
    param(
        [string[]]$CompilerFlags,
        [string[]]$Sources,
        [string[]]$ResourceInputs,
        [string]$PrimaryOutputPath
    )

    $linkerFlags = @('/link', '/MACHINE:X64', 'user32.lib', 'gdi32.lib', 'gdiplus.lib', 'comdlg32.lib', 'comctl32.lib', 'shell32.lib', 'ole32.lib',
        'advapi32.lib', 'shlwapi.lib', 'dbghelp.lib', 'bcrypt.lib', 'winhttp.lib', 'winmm.lib', 'msimg32.lib', 'uxtheme.lib')

    $primaryResult = Invoke-NativeCommand -FilePath 'clang-cl' -Arguments ($CompilerFlags + $Sources + $ResourceInputs + @("/Fe:$PrimaryOutputPath") + $linkerFlags)
    if ($primaryResult.ExitCode -eq 0) {
        Remove-StalePlayerFallbacks -BuildDirectory (Split-Path -Parent $PrimaryOutputPath) -KeepPath $null
        return $PrimaryOutputPath
    }

    if ($primaryResult.Output -notmatch 'permission denied') {
        throw ($primaryResult.Output | Out-String)
    }

    $fallbackOutputPath = Get-PlayerFallbackPath -BuildDirectory (Split-Path -Parent $PrimaryOutputPath)
    Remove-StalePlayerFallbacks -BuildDirectory (Split-Path -Parent $fallbackOutputPath) -KeepPath $null
    $fallbackResult = Invoke-NativeCommand -FilePath 'clang-cl' -Arguments ($CompilerFlags + $Sources + $ResourceInputs + @("/Fe:$fallbackOutputPath") + $linkerFlags)
    if ($fallbackResult.ExitCode -ne 0) {
        throw ($fallbackResult.Output | Out-String)
    }
    Write-Warning "Locked '$([System.IO.Path]::GetFileName($PrimaryOutputPath))'; wrote latest player build to '$fallbackOutputPath'."
    return $fallbackOutputPath
}

function Get-PlayerResourceInputs {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory
    )

    if (-not (Test-Path $AppIconScript)) {
        throw "App icon sync script not found: $AppIconScript"
    }
    if (-not (Test-Path $AppIconResourceScript)) {
        throw "App icon resource script not found: $AppIconResourceScript"
    }

    & powershell -ExecutionPolicy Bypass -File $AppIconScript | Out-Null
    $resourceCompile = Invoke-NativeCommand -FilePath 'llvm-rc' -Arguments @('/fo', $BuildAppIconResource, $AppIconResourceScript)
    if ($resourceCompile.ExitCode -ne 0) {
        throw ($resourceCompile.Output | Out-String)
    }

    return @($BuildAppIconResource)
}

function Sync-ButtonStyleAssets {
    $sourceDirectory = Join-Path $Root 'button-style'
    if (-not (Test-Path $sourceDirectory)) {
        Remove-Item -LiteralPath $BuildButtonStyleDir -Recurse -Force -ErrorAction SilentlyContinue
        return
    }

    Remove-Item -LiteralPath $BuildButtonStyleDir -Recurse -Force -ErrorAction SilentlyContinue
    Copy-Item -LiteralPath $sourceDirectory -Destination $BuildButtonStyleDir -Recurse -Force
}

function Write-ArtifactSummary {
    param(
        [string]$PlayerPath,
        [string]$HeadlessPath
    )

    $artifactPath = Join-Path $Root 'nativplayer-artifact.txt'
    $lines = @(
        $PlayerPath
    )
    if (Test-Path $PlayerPath) {
        $playerItem = Get-Item -LiteralPath $PlayerPath
        $lines += "LastWriteTime=$($playerItem.LastWriteTime.ToString('o'))"
        $lines += "Length=$($playerItem.Length)"
        $lines += "Machine=$(Get-PortableExecutableMachineName -Machine (Get-PortableExecutableMachine -Path $PlayerPath))"
    }
    if (Test-Path $HeadlessPath) {
        $headlessItem = Get-Item -LiteralPath $HeadlessPath
        $lines += "HeadlessLastWriteTime=$($headlessItem.LastWriteTime.ToString('o'))"
        $lines += "HeadlessLength=$($headlessItem.Length)"
        $lines += "HeadlessMachine=$(Get-PortableExecutableMachineName -Machine (Get-PortableExecutableMachine -Path $HeadlessPath))"
    }
    Set-Content -LiteralPath $artifactPath -Value $lines
}

$CMakeText = Get-Content (Join-Path $Root 'CMakeLists.txt') -Raw
$PlayerSources = Get-SourcesFromCMakeSet -CMakeText $CMakeText -SetName 'VELO_PLAYER_SOURCES'
$CommonSources = Get-SourcesFromCMakeSet -CMakeText $CMakeText -SetName 'VELO_COMMON_SOURCES'
$TestSources = @('tests\headless_test_kit.cpp') + $CommonSources

Push-Location $Root
try {
    $BuiltHeadlessExe = $HeadlessRunExe
    $BuiltPlayerExe = $PlayerExe
    if (-not [string]::IsNullOrWhiteSpace($TraceFile)) {
        Set-Content -LiteralPath $TraceFile -Value @()
    }
    Write-TraceStep -Message 'build start'
    Assert-X64Toolchain -BuildDirectory $BuildDir
    Write-TraceStep -Message 'toolchain ok'
    $PlayerResourceInputs = Get-PlayerResourceInputs -BuildDirectory $BuildDir
    Write-TraceStep -Message ('player icon resource built ' + ($PlayerResourceInputs -join ', '))
    if (-not $HeadlessOnly) {
        $BuiltPlayerExe = Invoke-PlayerBuild -CompilerFlags $CommonFlags -Sources $PlayerSources -ResourceInputs $PlayerResourceInputs -PrimaryOutputPath $PlayerExe
        Write-TraceStep -Message ('player built ' + $BuiltPlayerExe)
    } else {
        $latestPlayerFallback = Get-ChildItem -Path $BuildDir -Filter 'nativplayer_latest_*.exe' -File -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTimeUtc -Descending |
            Select-Object -First 1 -ExpandProperty FullName
        $BuiltPlayerExe = if (Test-Path $PlayerExe) { $PlayerExe } elseif ($latestPlayerFallback) { $latestPlayerFallback } else { $PlayerExe }
        Write-TraceStep -Message ('player build skipped; using ' + $BuiltPlayerExe)
    }
    $headlessBuildResult = Invoke-NativeCommand -FilePath 'clang-cl' -Arguments ($CommonFlags + $TestSources + @("/Fe:$HeadlessRunExe", '/link', '/MACHINE:X64',
                'user32.lib', 'gdi32.lib', 'shell32.lib', 'ole32.lib', 'shlwapi.lib', 'advapi32.lib', 'bcrypt.lib', 'winhttp.lib', 'winmm.lib', 'msimg32.lib',
                'comdlg32.lib', 'comctl32.lib', 'uxtheme.lib'))
    if ($headlessBuildResult.ExitCode -ne 0) {
        throw ($headlessBuildResult.Output | Out-String)
    }
    Write-TraceStep -Message ('headless runner built ' + $HeadlessRunExe)
    try {
        Copy-Item $HeadlessRunExe $HeadlessLatestExe -Force
        $BuiltHeadlessExe = $HeadlessLatestExe
        Remove-StaleHeadlessFallbacks -BuildDirectory $BuildDir -KeepPath $null
        Write-TraceStep -Message ('headless latest copied ' + $BuiltHeadlessExe)
    } catch {
        $fallbackHeadlessExe = Get-HeadlessFallbackPath -BuildDirectory $BuildDir -RunnerPath $HeadlessRunExe
        Copy-Item $HeadlessRunExe $fallbackHeadlessExe -Force
        $BuiltHeadlessExe = $fallbackHeadlessExe
        Remove-StaleHeadlessFallbacks -BuildDirectory $BuildDir -KeepPath $BuiltHeadlessExe
        Write-Warning "Locked 'nativplayer_headless_testkit_latest.exe'; wrote latest headless build to '$fallbackHeadlessExe'."
        Write-TraceStep -Message ('headless latest fallback copied ' + $BuiltHeadlessExe)
    }
    try {
        Copy-Item $HeadlessRunExe $HeadlessExe -Force
        $BuiltHeadlessExe = $HeadlessExe
        Write-TraceStep -Message ('headless primary copied ' + $BuiltHeadlessExe)
    } catch {
        Write-Warning "Locked 'nativplayer_headless_testkit.exe'; wrote latest headless build to '$HeadlessLatestExe'."
        Write-TraceStep -Message 'headless primary locked'
    }
    $runtimeMpvPath = Join-Path $Root 'runtime\mpv-2.dll'
    if (Test-Path $runtimeMpvPath) {
        Copy-Item $runtimeMpvPath $BuildRuntimeMpv -Force
    }
    Sync-ButtonStyleAssets

    Assert-PortableExecutableMachine -Path $BuiltPlayerExe -ExpectedMachine 0x8664 -Label 'nativplayer.exe'
    Assert-PortableExecutableMachine -Path $HeadlessRunExe -ExpectedMachine 0x8664 -Label 'nativplayer_headless_testkit.exe'
    Assert-PortableExecutableMachine -Path $BuiltHeadlessExe -ExpectedMachine 0x8664 -Label 'nativplayer_headless_testkit deployed artifact'

    $runtimeCandidates = @(
        (Join-Path $Root 'runtime\win64\libmpv-2.dll'),
        (Join-Path $Root 'runtime\libmpv-2.dll')
    )
    $runtimeLibMpv = $runtimeCandidates |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1
    if ($runtimeLibMpv) {
        Assert-PortableExecutableMachine -Path $runtimeLibMpv -ExpectedMachine 0x8664 -Label 'runtime libmpv-2.dll'
        Write-TraceStep -Message ('architecture assertions passed with runtime ' + $runtimeLibMpv)
    } else {
        Write-TraceStep -Message 'architecture assertions passed without local libmpv-2.dll'
    }

    Write-ArtifactSummary -PlayerPath $BuiltPlayerExe -HeadlessPath $BuiltHeadlessExe
    Write-TraceStep -Message 'artifact summary written'
    if (-not $SkipHeadlessRun) {
        Write-TraceStep -Message 'headless run start'
        & $HeadlessRunExe
        Write-TraceStep -Message 'headless run complete'
    }
} finally {
    if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals($BuiltHeadlessExe, $HeadlessRunExe)) {
        Remove-Item $HeadlessRunExe -Force -ErrorAction SilentlyContinue
    }
    Write-TraceStep -Message 'build end'
    Pop-Location
    Move-BuildDiagnosticsToDebugDirectory -BuildDirectory $BuildDir -DebugDirectory $DebugDir
}
