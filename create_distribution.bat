@echo off
echo Creating Zora VM Distribution Package...
echo.

REM Create distribution directory
if exist "ZoraVM_Distribution" rmdir /s /q "ZoraVM_Distribution"
mkdir "ZoraVM_Distribution"

REM Copy executable
echo Copying executable...
copy "build\zora_vm.exe" "ZoraVM_Distribution\"

REM Copy entire ZoraPerl directory structure
echo Copying ZoraPerl directory structure...
xcopy "ZoraPerl" "ZoraVM_Distribution\ZoraPerl\" /E /I /Y

REM Create launch script for end users
echo Creating user launch script...
echo @echo off > "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo Starting Zora VM... >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo. >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo If the program exits immediately, check that: >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo 1. ZoraPerl directory exists in the same folder as this script >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo 2. You have proper permissions to run executables >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo 3. Your antivirus is not blocking the executable >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo. >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo zora_vm.exe >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo. >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo echo VM exited. Press any key to close... >> "ZoraVM_Distribution\Launch_ZoraVM.bat"
echo pause ^> nul >> "ZoraVM_Distribution\Launch_ZoraVM.bat"

REM Copy documentation
echo Copying documentation...
copy "BUILD_SUMMARY.md" "ZoraVM_Distribution\README.md"
copy "system_check.bat" "ZoraVM_Distribution\"

REM Create system requirements file
echo Creating system requirements...
echo Zora VM System Requirements > "ZoraVM_Distribution\REQUIREMENTS.txt"
echo. >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo Minimum Requirements: >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - Windows 10 or later (64-bit) >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - 4 MB free disk space >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - No additional runtime libraries required >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo. >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo Troubleshooting: >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - If the program exits immediately, run system_check.bat >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - Ensure ZoraPerl directory is in the same folder as zora_vm.exe >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - Try running as Administrator if you encounter permission issues >> "ZoraVM_Distribution\REQUIREMENTS.txt"
echo - Check Windows Event Viewer for crash details if problems persist >> "ZoraVM_Distribution\REQUIREMENTS.txt"

echo.
echo Distribution package created in ZoraVM_Distribution\
echo.
dir /B "ZoraVM_Distribution"
echo.
echo You can now copy the entire ZoraVM_Distribution folder to any Windows machine.
pause
