param(
    [switch]$BuildPlayer,
    [switch]$SkipHeadlessRun,
    [string]$CompilerPath = ''
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-clang'
$InstallerDir = Join-Path $Root 'installer'
$InstallerScript = Join-Path $InstallerDir 'NativPlayer.iss'
$InstallerOutputDir = Join-Path $BuildDir 'installer'
$PlayerPrimaryExe = Join-Path $BuildDir 'nativplayer.exe'
$HeadlessPrimaryExe = Join-Path $BuildDir 'nativplayer_headless_testkit.exe'
$ButtonStyleDir = Join-Path $BuildDir 'button-style'
$BuildScript = Join-Path $PSScriptRoot 'build-latest-exe.ps1'
$LogoFile = Join-Path $Root 'logo.png'
$SetupIconFile = Join-Path $Root 'resources\app.ico'
$ReadmeEnFile = Join-Path $Root 'release\README.en.md'
$ReadmeZhTwFile = Join-Path $Root 'release\README.zh-TW.md'
$ReadmeZhCnFile = Join-Path $Root 'release\README.zh-CN.md'

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
    return Get-ChildItem -Path $directory -Filter $Pattern -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

function Assert-PathExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path $Path)) {
        throw "$Label not found: $Path"
    }
}

function Resolve-IsccPath {
    param(
        [string]$RequestedPath
    )

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        if (Test-Path $RequestedPath) {
            return (Resolve-Path -LiteralPath $RequestedPath).Path
        }
        throw "Specified Inno Setup compiler not found: $RequestedPath"
    }

    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "ISCC.exe not found. Install Inno Setup 6 or pass -CompilerPath."
}

function Get-AppVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string]$MetadataFile
    )

    $metadata = Get-Content -LiteralPath $MetadataFile -Raw
    $match = [regex]::Match($metadata, 'AppVersion\(\)\s*\{\s*return L"([^"]+)"')
    if (-not $match.Success) {
        throw "Unable to determine app version from: $MetadataFile"
    }
    return $match.Groups[1].Value
}

function Convert-ToIsccDefine {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    return "/D{0}={1}" -f $Name, $Value
}

if ($BuildPlayer) {
    Assert-PathExists -Path $BuildScript -Label 'Build script'
    $buildArgs = @(
        '-ExecutionPolicy', 'Bypass',
        '-File', $BuildScript
    )
    if ($SkipHeadlessRun) {
        $buildArgs += '-SkipHeadlessRun'
    }
    & powershell @buildArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$playerExe = Resolve-LatestArtifactPath -PrimaryPath $PlayerPrimaryExe -Pattern 'nativplayer_latest*.exe'
$headlessExe = Resolve-LatestArtifactPath -PrimaryPath $HeadlessPrimaryExe -Pattern 'nativplayer_headless_testkit_latest*.exe'
$appVersion = Get-AppVersion -MetadataFile (Join-Path $Root 'app\app_metadata.cpp')
$isccPath = Resolve-IsccPath -RequestedPath $CompilerPath

Assert-PathExists -Path $InstallerScript -Label 'Installer script'
Assert-PathExists -Path $playerExe -Label 'Player EXE'
Assert-PathExists -Path $headlessExe -Label 'Headless Automation Test Kit EXE'
Assert-PathExists -Path $ButtonStyleDir -Label 'Button style directory'
Assert-PathExists -Path $LogoFile -Label 'Logo file'
Assert-PathExists -Path $SetupIconFile -Label 'Setup icon file'
Assert-PathExists -Path $ReadmeEnFile -Label 'English release readme'
Assert-PathExists -Path $ReadmeZhTwFile -Label 'Traditional Chinese release readme'
Assert-PathExists -Path $ReadmeZhCnFile -Label 'Simplified Chinese release readme'

New-Item -ItemType Directory -Path $InstallerOutputDir -Force | Out-Null

$defines = @(
    (Convert-ToIsccDefine -Name 'MyAppVersion' -Value $appVersion),
    (Convert-ToIsccDefine -Name 'PlayerExe' -Value $playerExe),
    (Convert-ToIsccDefine -Name 'HeadlessExe' -Value $headlessExe),
    (Convert-ToIsccDefine -Name 'ButtonStyleDir' -Value $ButtonStyleDir),
    (Convert-ToIsccDefine -Name 'LogoFile' -Value $LogoFile),
    (Convert-ToIsccDefine -Name 'SetupIconFile' -Value $SetupIconFile),
    (Convert-ToIsccDefine -Name 'ReadmeEnFile' -Value $ReadmeEnFile),
    (Convert-ToIsccDefine -Name 'ReadmeZhTwFile' -Value $ReadmeZhTwFile),
    (Convert-ToIsccDefine -Name 'ReadmeZhCnFile' -Value $ReadmeZhCnFile),
    (Convert-ToIsccDefine -Name 'OutputDir' -Value $InstallerOutputDir)
)

& $isccPath @defines $InstallerScript
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$installerPath = Join-Path $InstallerOutputDir 'NativPlayer-Setup.exe'
Assert-PathExists -Path $installerPath -Label 'Installer EXE'
Write-Output $installerPath
