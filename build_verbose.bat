@echo off
echo Building ZoraVM in Verbose Mode (Full Debug Output)...
echo.

if not exist build mkdir build
cd build

echo Configuring build with CMake...
cmake .. -G "Ninja" -DZORA_RELEASE_MODE=OFF -DZORA_VERBOSE_BOOT=ON

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building with Ninja...
ninja

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ===== ZoraVM Verbose Build Complete! =====
echo.
echo The executable has been built with:
echo   - Full debug output and verbose messages
echo   - Detailed initialization information
echo   - Development/debugging experience
echo.
echo Run: .\zora_vm.exe
echo.
pause
