$ErrorActionPreference = 'Stop'

$appProgId = 'NativPlayer.Media'
$supportedExtensions = @('.mp4', '.mkv', '.avi', '.mov', '.wmv', '.flv', '.webm', '.m4v', '.mpg', '.mpeg')
$basePath = 'HKCU:\Software\Classes'

function Remove-KeyIfExists {
    param([string]$Path)

    if (Test-Path $Path) {
        Remove-Item -Path $Path -Recurse -Force
    }
}

Remove-KeyIfExists -Path (Join-Path $basePath $appProgId)

foreach ($extension in $supportedExtensions) {
    Remove-KeyIfExists -Path (Join-Path $basePath "$extension\shell\NativPlayer")
}

Remove-KeyIfExists -Path (Join-Path $basePath 'Directory\shell\NativPlayer')

Write-Host 'Removed NativPlayer shell integration keys from HKCU\Software\Classes'
