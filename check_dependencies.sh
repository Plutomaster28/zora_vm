#!/bin/bash

# Zora VM Dependency Check Script
# Checks if all required dependencies are installed

echo "=========================================="
echo "    Zora VM - Dependency Check"
echo "=========================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[CHECK]${NC} $1"; }
print_success() { echo -e "${GREEN}[OK]${NC} $1"; }
print_error() { echo -e "${RED}[MISSING]${NC} $1"; }

MISSING_PACKAGES=()

# Check if running in MSYS2/UCRT64
print_status "Checking environment..."
if [[ "$MSYSTEM" == "UCRT64" ]]; then
    print_success "Running in UCRT64 environment"
else
    print_error "Not in UCRT64 environment (current: $MSYSTEM)"
    echo "Please run in MSYS2 UCRT64 terminal"
    exit 1
fi

# Function to check package
check_package() {
    local package=$1
    local description=$2
    
    print_status "Checking $description..."
    if pacman -Q "$package" &> /dev/null; then
        local version=$(pacman -Q "$package" | awk '{print $2}')
        print_success "$description ($version)"
    else
        print_error "$description - install with: pacman -S $package"
        MISSING_PACKAGES+=("$package")
    fi
}

echo ""
echo "Checking build tools..."
check_package "mingw-w64-ucrt-x86_64-gcc" "GCC Compiler"
check_package "mingw-w64-ucrt-x86_64-cmake" "CMake"
check_package "mingw-w64-ucrt-x86_64-make" "Make"

echo ""
echo "Checking GNU toolchain..."
check_package "mingw-w64-ucrt-x86_64-gcc-fortran" "GNU Fortran Compiler"
check_package "mingw-w64-ucrt-x86_64-binutils" "GNU Binutils"
check_package "mingw-w64-ucrt-x86_64-gdb" "GNU Debugger (GDB)"

echo ""
echo "Checking assemblers..."
check_package "mingw-w64-ucrt-x86_64-nasm" "NASM x86 Assembler"

echo ""
echo "Checking libraries..."
check_package "mingw-w64-ucrt-x86_64-lua" "Lua Runtime"
check_package "mingw-w64-ucrt-x86_64-zlib" "Zlib Compression Library"

echo ""
echo "Checking optional development tools..."
check_package "mingw-w64-ucrt-x86_64-ninja" "Ninja Build System"
check_package "mingw-w64-ucrt-x86_64-pkg-config" "pkg-config"
check_package "mingw-w64-ucrt-x86_64-python3" "Python 3 (for scripting)"
check_package "mingw-w64-ucrt-x86_64-perl" "Perl (for scripting)"

echo ""
echo "=========================================="

if [[ ${#MISSING_PACKAGES[@]} -eq 0 ]]; then
    print_success "All dependencies are installed!"
    echo ""
    echo "You can now build Zora VM with:"
    echo "  ./build_zora_vm.sh"
    echo ""
    echo "Or manually:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  make -j\$(nproc)"
else
    print_error "Missing ${#MISSING_PACKAGES[@]} package(s)"
    echo ""
    echo "To install all missing packages:"
    echo "  pacman -S ${MISSING_PACKAGES[*]}"
    echo ""
    echo "Or run the automated build script:"
    echo "  ./build_zora_vm.sh"
    echo ""
    exit 1
fi
