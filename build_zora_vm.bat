@echo off
setlocal enabledelayedexpansion

:: Zora VM Build Script for Windows
:: This script launches MSYS2 and runs the build process

title Zora VM - Automated Build

echo ==========================================
echo     Zora VM - Windows Build Launcher
echo ==========================================
echo.

:: Check if MSYS2 is installed
set "MSYS2_PATH=C:\msys64"
if not exist "%MSYS2_PATH%" (
    echo ERROR: MSYS2 not found at %MSYS2_PATH%
    echo.
    echo Please install MSYS2 from: https://www.msys2.org/
    echo.
    echo Installation instructions:
    echo 1. Download MSYS2 installer
    echo 2. Install to C:\msys64
    echo 3. Run this script again
    echo.
    pause
    exit /b 1
)

echo ✓ MSYS2 found at %MSYS2_PATH%

:: Get the current directory (project root)
set "PROJECT_DIR=%~dp0"
set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

echo ✓ Project directory: %PROJECT_DIR%
echo.

:: Convert Windows path to MSYS2 path
set "MSYS2_PROJECT_DIR=%PROJECT_DIR%"
set "MSYS2_PROJECT_DIR=!MSYS2_PROJECT_DIR:C:\=/c/!"
set "MSYS2_PROJECT_DIR=!MSYS2_PROJECT_DIR:\=/!"

echo Starting MSYS2 UCRT64 environment...
echo Running automated build script...
echo.

:: Launch MSYS2 with the build script
"%MSYS2_PATH%\ucrt64.exe" -c "cd '%MSYS2_PROJECT_DIR%' && chmod +x build_zora_vm.sh && ./build_zora_vm.sh"

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ Build failed! Check the output above for errors.
    echo.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo     🎉 Build Completed Successfully!
echo ==========================================
echo.

:: Check if the executable was created
if exist "%PROJECT_DIR%\build\zora_vm.exe" (
    echo ✓ Zora VM executable created: build\zora_vm.exe
    echo.
    
    echo Would you like to launch Zora VM now? (Y/N)
    set /p "LAUNCH_CHOICE="
    
    if /i "!LAUNCH_CHOICE!"=="Y" (
        echo.
        echo 🚀 Launching Zora VM...
        echo Features: MS Mincho font, Campbell colors, Block cursor
        echo Type 'terminal-demo' to see all enhancements!
        echo.
        start "" "%PROJECT_DIR%\build\zora_vm.exe"
    )
) else (
    echo ❌ Executable not found! Build may have failed.
)

echo.
echo Usage Instructions:
echo   Run: %PROJECT_DIR%\build\zora_vm.exe
echo   Or:  %PROJECT_DIR%\launch_zora_vm.sh (in MSYS2)
echo.
pause
