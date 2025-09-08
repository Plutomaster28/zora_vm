@echo off
echo Building ZoraVM in Release Mode (Clean Startup)...
echo.

if not exist build mkdir build
cd build

echo Configuring build with CMake...
cmake .. -G "Ninja" -DZORA_RELEASE_MODE=ON -DZORA_VERBOSE_BOOT=OFF

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
echo ===== ZoraVM Release Build Complete! =====
echo.
echo The executable has been built with:
echo   - Clean, minimal startup messages
echo   - MERL-inspired boot sequence
echo   - Professional user experience
echo.
echo Run: .\zora_vm.exe
echo.
pause
