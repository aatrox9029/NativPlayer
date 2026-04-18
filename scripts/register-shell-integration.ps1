param(
    [string]$ExePath = (Join-Path $PSScriptRoot '..\build-clang\nativplayer.exe')
)

$ErrorActionPreference = 'Stop'

$resolvedExe = (Resolve-Path $ExePath).Path
$appProgId = 'NativPlayer.Media'
$supportedExtensions = @('.mp4', '.mkv', '.avi', '.mov', '.wmv', '.flv', '.webm', '.m4v', '.mpg', '.mpeg')

function Set-RegistryValue {
    param(
        [string]$Path,
        [string]$Name,
        [string]$Value
    )

    if (-not (Test-Path $Path)) {
        New-Item -Path $Path -Force | Out-Null
    }

    if ($Name -eq '(default)') {
        Set-Item -Path $Path -Value $Value -Force
        return
    }

    New-ItemProperty -Path $Path -Name $Name -Value $Value -PropertyType String -Force | Out-Null
}

$basePath = 'HKCU:\Software\Classes'
Set-RegistryValue -Path (Join-Path $basePath $appProgId) -Name '(default)' -Value 'NativPlayer Media'
Set-RegistryValue -Path (Join-Path $basePath "$appProgId\DefaultIcon") -Name '(default)' -Value ('"{0}",0' -f $resolvedExe)
Set-RegistryValue -Path (Join-Path $basePath "$appProgId\shell\open\command") -Name '(default)' -Value ('"{0}" "%1"' -f $resolvedExe)

foreach ($extension in $supportedExtensions) {
    $extensionPath = Join-Path $basePath $extension
    Set-RegistryValue -Path $extensionPath -Name '(default)' -Value $appProgId
    Set-RegistryValue -Path (Join-Path $extensionPath 'OpenWithProgids') -Name $appProgId -Value ''

    $verbPath = Join-Path $basePath "$extension\shell\NativPlayer"
    Set-RegistryValue -Path $verbPath -Name '(default)' -Value 'Open with NativPlayer'
    Set-RegistryValue -Path (Join-Path $verbPath 'command') -Name '(default)' -Value ('"{0}" "%1"' -f $resolvedExe)
}

$folderVerbPath = Join-Path $basePath 'Directory\shell\NativPlayer'
Set-RegistryValue -Path $folderVerbPath -Name '(default)' -Value 'Play folder with NativPlayer'
Set-RegistryValue -Path (Join-Path $folderVerbPath 'command') -Name '(default)' -Value ('"{0}" "%1"' -f $resolvedExe)

Write-Host "Registered shell integration for NativPlayer at $resolvedExe"
