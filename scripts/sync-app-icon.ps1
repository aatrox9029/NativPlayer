param(
    [string]$LogoPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'logo.png'),
    [string]$OutputPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'resources\app.ico')
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

function New-ScaledPngBytes {
    param(
        [Parameter(Mandatory = $true)]
        [System.Drawing.Image]$SourceImage,
        [Parameter(Mandatory = $true)]
        [int]$Size
    )

    $bitmap = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $graphics.DrawImage($SourceImage, (New-Object System.Drawing.Rectangle(0, 0, $Size, $Size)))
        } finally {
            $graphics.Dispose()
        }

        $stream = New-Object System.IO.MemoryStream
        try {
            $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
            return ,($stream.ToArray())
        } finally {
            $stream.Dispose()
        }
    } finally {
        $bitmap.Dispose()
    }
}

if (-not (Test-Path $LogoPath)) {
    throw "Logo not found: $LogoPath"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not (Test-Path $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}

$sourceImage = [System.Drawing.Image]::FromFile($LogoPath)
try {
    $sizes = @(16, 24, 32, 48, 64, 128, 256)
    $iconEntries = foreach ($size in $sizes) {
        [byte[]]$pngBytes = New-ScaledPngBytes -SourceImage $sourceImage -Size $size
        [PSCustomObject]@{
            Size = $size
            Bytes = $pngBytes
        }
    }
} finally {
    $sourceImage.Dispose()
}

$fileStream = [System.IO.File]::Open($OutputPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::Read)
try {
    $writer = New-Object System.IO.BinaryWriter($fileStream)
    try {
        $writer.Write([UInt16]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]$iconEntries.Count)

        $imageOffset = 6 + (16 * $iconEntries.Count)
        foreach ($entry in $iconEntries) {
            $dimensionByte = if ($entry.Size -ge 256) { [byte]0 } else { [byte]$entry.Size }
            $writer.Write($dimensionByte)
            $writer.Write($dimensionByte)
            $writer.Write([byte]0)
            $writer.Write([byte]0)
            $writer.Write([UInt16]1)
            $writer.Write([UInt16]32)
            $writer.Write([UInt32]$entry.Bytes.Length)
            $writer.Write([UInt32]$imageOffset)
            $imageOffset += $entry.Bytes.Length
        }

        foreach ($entry in $iconEntries) {
            $writer.Write($entry.Bytes)
        }
    } finally {
        $writer.Dispose()
    }
} finally {
    $fileStream.Dispose()
}

Write-Output $OutputPath
