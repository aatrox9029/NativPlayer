@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD_SCRIPT=%ROOT%scripts\build-clang.ps1"

if not exist "%BUILD_SCRIPT%" (
    echo Build script not found:
    echo %BUILD_SCRIPT%
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%BUILD_SCRIPT%"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" (
    echo Build failed with exit code %EXIT_CODE%.
    pause
    exit /b %EXIT_CODE%
)

echo Build completed successfully.
pause
exit /b 0
