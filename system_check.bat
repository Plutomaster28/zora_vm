@echo off
echo =================================================
echo Zora VM Dependency and Environment Checker
echo =================================================
echo.

REM Check if executable exists
echo [1/6] Checking executable...
if not exist "zora_vm.exe" (
    if exist "build\zora_vm.exe" (
        echo   Found in build\ directory, copying to current directory...
        copy "build\zora_vm.exe" . > nul
    ) else (
        echo   ERROR: zora_vm.exe not found
        goto :error
    )
)
echo   ✓ zora_vm.exe found

REM Check file size and basic info
echo.
echo [2/6] Checking executable info...
for %%F in (zora_vm.exe) do (
    echo   File size: %%~zF bytes
    echo   Modified: %%~tF
)

REM Check if ZoraPerl directory exists
echo.
echo [3/6] Checking ZoraPerl directory structure...
if not exist "ZoraPerl" (
    echo   WARNING: ZoraPerl directory not found
    echo   This might cause the VM to fail during initialization
    echo   The VM expects to find a ZoraPerl directory with subdirectories
) else (
    echo   ✓ ZoraPerl directory found
    dir /B ZoraPerl 2>nul
)

REM Check Windows version and architecture
echo.
echo [4/6] Checking system compatibility...
echo   Windows version: %OS%
systeminfo | findstr /B /C:"OS Name" /C:"System Type"

REM Check for Visual C++ Redistributables
echo.
echo [5/6] Checking runtime dependencies...
echo   Checking for Visual C++ Redistributables...
reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" >nul 2>&1
if %errorlevel% equ 0 (
    echo   ✓ Visual C++ 2015-2022 x64 found
) else (
    echo   WARNING: Visual C++ 2015-2022 x64 redistributable not detected
)

REM Try to run the executable with error capture
echo.
echo [6/6] Attempting to launch VM...
echo   If the program exits immediately, error details will appear below:
echo   ----------------------------------------
echo.

REM Set up error logging
set LOGFILE=zora_vm_debug.log
del %LOGFILE% 2>nul

REM Try to run with timeout and error capture
timeout /t 1 /nobreak >nul
echo Starting VM... > %LOGFILE%
start /wait cmd /c "zora_vm.exe 2>&1 && echo VM_SUCCESS || echo VM_FAILED_CODE_%ERRORLEVEL%" >> %LOGFILE%

REM Wait a moment then check results
timeout /t 2 /nobreak >nul

echo   Results:
type %LOGFILE% 2>nul

echo.
echo ========================================
echo Diagnostic complete. 
echo.
echo If the VM still exits immediately:
echo 1. Try running as Administrator
echo 2. Check Windows Event Viewer for crash details
echo 3. Ensure antivirus isn't blocking the executable
echo 4. Try copying the entire directory structure to the target machine
echo.
pause
goto :end

:error
echo.
echo ERROR: Cannot proceed without zora_vm.exe
pause

:end
