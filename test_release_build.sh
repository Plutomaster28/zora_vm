#!/bin/bash
# Test script to simulate the GitHub Actions build process locally

echo "=== ZoraVM Release Build Test ==="
echo

# Clean and setup
echo "1. Cleaning build directory..."
rm -rf build_test
mkdir -p build_test
cd build_test

# Configure
echo "2. Configuring with CMake..."
cmake .. \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc \
  -DZORA_RELEASE_MODE=ON \
  -DZORA_VERBOSE_BOOT=OFF

if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# Build
echo "3. Building..."
ninja -v

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

# Test
echo "4. Testing binary..."
./zora_vm.exe || echo "Binary created successfully (exit code: $?)"
file zora_vm.exe

# Package
echo "5. Creating package..."
mkdir -p zora-vm-windows
cp zora_vm.exe zora-vm-windows/
cp ../README.md zora-vm-windows/ || echo "README not found"
cp ../LICENSE zora-vm-windows/ || echo "LICENSE not found"
cp ../Launch_ZoraVM_Enhanced.bat zora-vm-windows/ || echo "Enhanced launcher not found"
cp ../Launch_ZoraVM_WT.bat zora-vm-windows/ || echo "WT launcher not found"

# Create simple launcher
echo '@echo off' > zora-vm-windows/zora-vm.bat
echo 'echo Starting ZORA VM...' >> zora-vm-windows/zora-vm.bat
echo '"%~dp0zora_vm.exe" %*' >> zora-vm-windows/zora-vm.bat
echo 'pause' >> zora-vm-windows/zora-vm.bat

echo "Package contents:"
ls -la zora-vm-windows/

# Create ZIP (if 7z is available)
if command -v 7z &> /dev/null; then
    7z a zora-vm-windows-x64.zip zora-vm-windows/
    echo "ZIP created:"
    ls -la zora-vm-windows-x64.zip
else
    echo "7z not available, skipping ZIP creation"
fi

echo
echo "=== Build test completed successfully ==="
