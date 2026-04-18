@echo off
setlocal

set "ROOT=%~dp0"
set "ASSET_DIR=%ROOT%third_party\libmpv"
set "ASSET_NAME=mpv-dev-lgpl-x86_64-v3-20260417-git-1fea31f.7z"
set "ASSET_PATH=%ASSET_DIR%\%ASSET_NAME%"
set "TEMP_PATH=%ASSET_PATH%.download"
set "ASSET_URL=https://github.com/zhongfly/mpv-winbuild/releases/download/2026-04-17-1fea31f/%ASSET_NAME%"
set "ASSET_SHA256=7304f2b886cec13f3602488e3c84f37c8d1e7acdc448185b2471e501e8f3af3d"

if not exist "%ASSET_DIR%" (
    mkdir "%ASSET_DIR%"
)

del /f /q "%TEMP_PATH%" >nul 2>nul

echo Downloading %ASSET_NAME% ...
where curl.exe >nul 2>nul
if not errorlevel 1 (
    curl.exe -L --fail --silent --show-error -o "%TEMP_PATH%" "%ASSET_URL%"
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
      "$ProgressPreference='SilentlyContinue';" ^
      "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12;" ^
      "$headers=@{ 'User-Agent'='NativPlayer-Setup'; 'Accept'='application/octet-stream' };" ^
      "Invoke-WebRequest -Uri '%ASSET_URL%' -Headers $headers -OutFile '%TEMP_PATH%'"
)
if errorlevel 1 (
    echo Download failed.
    del /f /q "%TEMP_PATH%" >nul 2>nul
    pause
    exit /b 1
)

echo Verifying SHA-256 ...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$hash=(Get-FileHash -LiteralPath '%TEMP_PATH%' -Algorithm SHA256).Hash.ToLowerInvariant();" ^
  "if ($hash -ne '%ASSET_SHA256%') { Write-Error ('SHA-256 mismatch: ' + $hash) }"
if errorlevel 1 (
    echo Verification failed. The downloaded file is not the expected archive.
    del /f /q "%TEMP_PATH%" >nul 2>nul
    pause
    exit /b 1
)

move /y "%TEMP_PATH%" "%ASSET_PATH%" >nul
if errorlevel 1 (
    echo Failed to move the downloaded archive into place.
    del /f /q "%TEMP_PATH%" >nul 2>nul
    pause
    exit /b 1
)

echo Done. Saved to:
echo %ASSET_PATH%
pause
exit /b 0
