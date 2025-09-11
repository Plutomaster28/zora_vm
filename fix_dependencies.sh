#!/bin/bash

# Quick fix for MSYS2 package conflicts
# This script resolves common dependency issues

echo "=========================================="
echo "    MSYS2 Package Conflict Resolver"
echo "=========================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check environment
if [[ "$MSYSTEM" != "UCRT64" ]]; then
    print_error "Please run this in MSYS2 UCRT64 terminal"
    exit 1
fi

print_status "Resolving MSYS2 package conflicts..."

# Step 1: Update package database
print_status "Updating package database..."
pacman -Sy

# Step 2: Force upgrade core GCC components
print_status "Upgrading GCC components to resolve version conflicts..."
print_warning "This will upgrade GCC and related libraries"

if pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gcc-libs; then
    print_success "GCC components upgraded successfully"
else
    print_warning "GCC upgrade failed, trying full system upgrade..."
    
    print_status "Performing full system upgrade..."
    print_warning "This may take several minutes..."
    
    if pacman -Syu --noconfirm; then
        print_success "System upgrade completed"
    else
        print_error "System upgrade failed"
        echo ""
        echo "Manual resolution required:"
        echo "1. Run: pacman -Syu"
        echo "2. Resolve any conflicts manually"
        echo "3. Run this script again"
        exit 1
    fi
fi

# Step 3: Install missing development tools
print_status "Installing development tools..."

packages=(
    "mingw-w64-ucrt-x86_64-cmake"
    "mingw-w64-ucrt-x86_64-make" 
    "mingw-w64-ucrt-x86_64-ninja"
    "mingw-w64-ucrt-x86_64-lua"
    "mingw-w64-ucrt-x86_64-zlib"
    "mingw-w64-ucrt-x86_64-gcc-fortran"
    "mingw-w64-ucrt-x86_64-binutils"
    "mingw-w64-ucrt-x86_64-gdb"
    "mingw-w64-ucrt-x86_64-nasm"
    "mingw-w64-ucrt-x86_64-pkg-config"
    "mingw-w64-ucrt-x86_64-python3"
    "mingw-w64-ucrt-x86_64-perl"
)

for package in "${packages[@]}"; do
    print_status "Installing $package..."
    if pacman -S --noconfirm --needed "$package"; then
        print_success "$package installed"
    else
        print_warning "$package installation failed, but continuing..."
    fi
done

# Step 4: Verify installation
print_status "Verifying installation..."

echo ""
echo "Checking installed packages:"
for package in "${packages[@]}"; do
    if pacman -Q "$package" &> /dev/null; then
        version=$(pacman -Q "$package" | awk '{print $2}')
        print_success "$package ($version)"
    else
        print_error "$package - not installed"
    fi
done

echo ""
print_success "Package conflict resolution completed!"
echo ""
echo "You can now run the build script:"
echo "  ./build_zora_vm.sh"
echo ""
echo "Or check dependencies:"
echo "  ./check_dependencies.sh"
