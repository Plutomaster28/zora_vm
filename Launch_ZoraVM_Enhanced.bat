@echo off
REM ZoraVM Launcher with Windows Terminal Support
REM This script tries to launch ZoraVM in Windows Terminal for best experience

echo ZoraVM Launcher v2.1.0

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "ZORA_EXE=%SCRIPT_DIR%zora_vm.exe"

REM Check if zora_vm.exe exists
if not exist "%ZORA_EXE%" (
    echo ERROR: zora_vm.exe not found in %SCRIPT_DIR%
    echo Please make sure zora_vm.exe is in the same directory as this launcher.
    pause
    exit /b 1
)

echo Detecting terminal environment...

REM Method 1: Try Windows Terminal with wt command
where wt >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Windows Terminal detected! Launching with enhanced experience...
    wt -d "%SCRIPT_DIR%" "build\zora_vm.exe"
    echo ZoraVM launched in Windows Terminal.
    echo If the new window didn't open, Windows Terminal may not be properly installed.
    goto :show_install_info
)

REM Method 2: Try to find Windows Terminal in common locations
set "WT_PATHS="
set "WT_PATHS=%WT_PATHS%;%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe"
set "WT_PATHS=%WT_PATHS%;%PROGRAMFILES%\WindowsApps\Microsoft.WindowsTerminal*\wt.exe"

for %%P in (%WT_PATHS%) do (
    if exist "%%P" (
        echo Found Windows Terminal at %%P
        echo Launching ZoraVM in Windows Terminal...
        "%%P" -d "%SCRIPT_DIR%" "build\zora_vm.exe"
        goto :show_install_info
    )
)

REM Method 3: Fallback to console host with warning
echo.
echo WARNING: Windows Terminal not found!
echo ZoraVM will run in legacy Console Host with limited visual features.
echo.
echo For the best experience, install Windows Terminal:
echo   * Microsoft Store: Search "Windows Terminal"
echo   * Command line: winget install Microsoft.WindowsTerminal
echo   * GitHub: https://github.com/microsoft/terminal
echo.
echo Starting ZoraVM in legacy mode...
echo.
"%ZORA_EXE%"
goto :end

:show_install_info
echo.
echo === ZoraVM Terminal Setup Complete ===
echo.
echo If you're still seeing this console window, you can:
echo 1. Close this window and use the new Windows Terminal window
echo 2. Or wait a moment for ZoraVM to start in the new terminal
echo.
echo For future launches, you can:
echo - Use this launcher script for automatic Windows Terminal detection
echo - Or directly run 'wt zora_vm.exe' if Windows Terminal is installed
echo - Or double-click zora_vm.exe for basic console mode
echo.
timeout /t 5 /nobreak >nul
exit /b 0

:end
echo.
echo ZoraVM session ended.
pause
