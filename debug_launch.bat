@echo off
echo Testing Zora VM on target system...
echo.
echo Current directory: %CD%
echo.
echo Checking if executable exists...
if not exist "zora_vm.exe" (
    echo ERROR: zora_vm.exe not found in current directory
    echo Looking for it in build directory...
    if exist "build\zora_vm.exe" (
        echo Found in build directory, copying...
        copy "build\zora_vm.exe" .
    ) else (
        echo ERROR: zora_vm.exe not found anywhere
        pause
        exit /b 1
    )
)

echo.
echo Launching Zora VM...
echo If it crashes immediately, you'll see the error below:
echo.

REM Launch and capture any error output
zora_vm.exe 2>&1
set ERRORLEVEL_BACKUP=%ERRORLEVEL%

echo.
echo Program exited with code: %ERRORLEVEL_BACKUP%
echo.
echo Press any key to continue...
pause > nul
