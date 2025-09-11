#!/bin/bash

# Zora VM Build Script for MSYS2/UCRT64
# This script automatically installs dependencies and builds the project

echo "=========================================="
echo "    Zora VM - Automated Build Script"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in MSYS2/UCRT64 environment
check_environment() {
    print_status "Checking environment..."
    
    if [[ "$MSYSTEM" != "UCRT64" ]]; then
        print_warning "Not in UCRT64 environment. Current: $MSYSTEM"
        print_status "Please run this script in MSYS2 UCRT64 terminal"
        print_status "You can launch it from: C:\\msys64\\ucrt64.exe"
        exit 1
    fi
    
    print_success "Running in UCRT64 environment"
}

# Function to check if a package is installed
is_package_installed() {
    pacman -Q "$1" &> /dev/null
    return $?
}

# Function to install a package if not already installed
install_package() {
    local package=$1
    local description=$2
    
    if is_package_installed "$package"; then
        print_success "$description is already installed"
    else
        print_status "Installing $description ($package)..."
        if pacman -S --noconfirm "$package"; then
            print_success "$description installed successfully"
        else
            print_warning "Standard installation failed, trying with dependency resolution..."
            if pacman -S --noconfirm --needed "$package"; then
                print_success "$description installed successfully (with dependency resolution)"
            else
                print_error "Failed to install $description"
                print_status "You may need to manually run: pacman -Syu"
                print_status "Then try the build script again"
                exit 1
            fi
        fi
    fi
}

# Install required dependencies
install_dependencies() {
    print_status "Checking and installing dependencies..."
    
    # Update package database and upgrade existing packages to resolve conflicts
    print_status "Updating package database and upgrading existing packages..."
    print_warning "This may take a few minutes and will resolve dependency conflicts..."
    
    # First, update the package database
    pacman -Sy --noconfirm
    
    # Upgrade all existing packages to resolve version conflicts
    print_status "Upgrading existing packages to resolve version conflicts..."
    if ! pacman -Su --noconfirm; then
        print_warning "Some packages couldn't be upgraded automatically"
        print_status "Trying to force upgrade core GCC components..."
        pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gcc-libs
    fi
    
    # Core build tools - install as a group to resolve dependencies
    print_status "Installing core build tools as a group..."
    local core_packages=(
        "mingw-w64-ucrt-x86_64-gcc"
        "mingw-w64-ucrt-x86_64-cmake" 
        "mingw-w64-ucrt-x86_64-make"
        "mingw-w64-ucrt-x86_64-ninja"
    )
    
    if ! pacman -S --noconfirm "${core_packages[@]}"; then
        print_error "Failed to install core build tools as group"
        print_status "Trying individual installation..."
        for package in "${core_packages[@]}"; do
            local description=""
            case $package in
                *gcc*) description="GCC Compiler" ;;
                *cmake*) description="CMake" ;;
                *make*) description="Make" ;;
                *ninja*) description="Ninja Build System" ;;
            esac
            install_package "$package" "$description"
        done
    else
        print_success "Core build tools installed successfully"
    fi
    
    # Required libraries
    install_package "mingw-w64-ucrt-x86_64-lua" "Lua Runtime"
    install_package "mingw-w64-ucrt-x86_64-zlib" "Zlib Compression Library"
    
    # GNU toolchain components
    install_package "mingw-w64-ucrt-x86_64-gcc-fortran" "GNU Fortran Compiler"
    install_package "mingw-w64-ucrt-x86_64-binutils" "GNU Binutils"
    install_package "mingw-w64-ucrt-x86_64-nasm" "NASM x86 Assembler"
    
    # Optional but recommended tools
    install_package "mingw-w64-ucrt-x86_64-gdb" "GDB Debugger"
    install_package "mingw-w64-ucrt-x86_64-pkg-config" "pkg-config"
    install_package "mingw-w64-ucrt-x86_64-python3" "Python 3 (for scripting)"
    install_package "mingw-w64-ucrt-x86_64-perl" "Perl (for scripting)"
    
    print_success "All dependencies installed successfully!"
}

# Create and configure build directory
setup_build_directory() {
    print_status "Setting up build directory..."
    
    # Navigate to project root
    cd "$(dirname "$0")" || exit 1
    
    # Create build directory if it doesn't exist
    if [[ ! -d "build" ]]; then
        mkdir build
        print_status "Created build directory"
    fi
    
    cd build || exit 1
    print_success "Build directory ready"
}

# Run CMake configuration
configure_cmake() {
    print_status "Configuring project with CMake..."
    
    # Clean previous configuration if it failed
    if [[ -f "CMakeCache.txt" ]]; then
        print_status "Cleaning previous CMake configuration..."
        rm -f CMakeCache.txt
    fi
    
    # Run CMake with proper generator
    if cmake -G "MinGW Makefiles" ..; then
        print_success "CMake configuration completed successfully"
    else
        print_error "CMake configuration failed"
        print_status "Trying with Ninja generator..."
        if cmake -G "Ninja" ..; then
            print_success "CMake configuration with Ninja completed successfully"
        else
            print_error "CMake configuration failed with both generators"
            exit 1
        fi
    fi
}

# Build the project
build_project() {
    print_status "Building Zora VM..."
    
    # Determine number of CPU cores for parallel build
    local cores=$(nproc)
    print_status "Using $cores CPU cores for parallel build"
    
    # Build with make or ninja
    if [[ -f "build.ninja" ]]; then
        # Using Ninja
        if ninja; then
            print_success "Build completed successfully with Ninja!"
        else
            print_error "Build failed with Ninja"
            exit 1
        fi
    else
        # Using Make
        if make -j"$cores"; then
            print_success "Build completed successfully with Make!"
        else
            print_error "Build failed with Make"
            exit 1
        fi
    fi
}

# Verify the build
verify_build() {
    print_status "Verifying build output..."
    
    if [[ -f "zora_vm.exe" ]]; then
        print_success "Zora VM executable created successfully!"
        
        # Get file size
        local size=$(stat -c%s "zora_vm.exe" 2>/dev/null || echo "unknown")
        print_status "Executable size: $size bytes"
        
        # Check if it's executable
        if [[ -x "zora_vm.exe" ]]; then
            print_success "Executable has proper permissions"
        else
            print_warning "Executable may not have proper permissions"
        fi
        
    else
        print_error "zora_vm.exe not found in build directory"
        print_status "Available files:"
        ls -la
        exit 1
    fi
}

# Create launch script
create_launch_script() {
    print_status "Creating launch script..."
    
    cat > "../launch_zora_vm.sh" << 'EOF'
#!/bin/bash
# Zora VM Launcher Script

echo "=========================================="
echo "    🚀 Launching Zora VM"
echo "=========================================="

# Navigate to project directory
cd "$(dirname "$0")"

# Check if executable exists
if [[ ! -f "build/zora_vm.exe" ]]; then
    echo "❌ Zora VM executable not found!"
    echo "Please run build_zora_vm.sh first"
    exit 1
fi

# Launch Zora VM
echo "🎮 Starting Zora VM with enhanced terminal..."
echo "Features: MS Mincho font, Campbell colors, Block cursor"
echo "Type 'terminal-demo' to see all enhancements!"
echo ""

./build/zora_vm.exe

echo ""
echo "👋 Zora VM session ended"
read -p "Press Enter to close..." -r
EOF

    chmod +x "../launch_zora_vm.sh"
    print_success "Launch script created: launch_zora_vm.sh"
}

# Display usage instructions
show_usage_instructions() {
    print_success "🎉 Build completed successfully!"
    echo ""
    echo "=========================================="
    echo "    📖 Usage Instructions"
    echo "=========================================="
    echo ""
    echo "To run Zora VM:"
    echo "  ./launch_zora_vm.sh"
    echo ""
    echo "Or directly:"
    echo "  ./build/zora_vm.exe"
    echo ""
    echo "Enhanced Terminal Features:"
    echo "  • MS Mincho font support"
    echo "  • Campbell PowerShell color scheme"
    echo "  • Retro block cursor"
    echo "  • Syntax highlighting"
    echo "  • Retro terminal mode"
    echo ""
    echo "Terminal Commands:"
    echo "  style init       - Initialize enhanced terminal"
    echo "  terminal-demo    - Show all features"
    echo "  font 'MS Mincho' - Set retro font"
    echo "  cursor block     - Set block cursor"
    echo "  colors campbell  - Apply PowerShell colors"
    echo "  retro on         - Enable retro mode"
    echo ""
    echo "=========================================="
}

# Main execution
main() {
    echo ""
    print_status "Starting automated build process..."
    echo ""
    
    check_environment
    install_dependencies
    setup_build_directory
    configure_cmake
    build_project
    verify_build
    create_launch_script
    show_usage_instructions
    
    print_success "🚀 Zora VM is ready to use!"
    echo ""
}

# Run main function
main "$@"
