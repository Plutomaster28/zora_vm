@echo off
REM Test script to validate standalone operation
echo Testing Zora VM standalone operation...
echo.

REM Check if we can run the executable
echo Checking if zora_vm.exe exists...
if exist "build\zora_vm.exe" (
    echo ✓ zora_vm.exe found
) else (
    echo ✗ zora_vm.exe not found
    exit /b 1
)

echo.
echo Testing executable launch (this should work without MSYS2)...
echo Note: VM will start and should show the MERL shell prompt
echo Type 'exit' in the VM to properly close it.
echo.

REM Launch the VM
cd build
zora_vm.exe
cd ..

echo.
echo Test completed.
