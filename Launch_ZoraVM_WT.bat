@echo off
REM Simple Windows Terminal launcher for ZoraVM
REM Use this if you have Windows Terminal installed and want the best experience

echo ZoraVM - Windows Terminal Launcher
echo.

REM Launch ZoraVM in Windows Terminal
echo Launching ZoraVM in Windows Terminal...
wt -d "%~dp0" "build\zora_vm.exe"

if errorlevel 1 (
    echo.
    echo ERROR: Failed to launch Windows Terminal
    echo.
    echo Please install Windows Terminal:
    echo - Microsoft Store: Search "Windows Terminal"
    echo - Command line: winget install Microsoft.WindowsTerminal
    echo.
    echo Alternatively, use the full launcher: Launch_ZoraVM_Enhanced.bat
    echo Or run directly: build\zora_vm.exe
    pause
    exit /b 1
)

echo ZoraVM launched in new Windows Terminal window.
echo You can close this console window now.
timeout /t 3 /nobreak >nul
