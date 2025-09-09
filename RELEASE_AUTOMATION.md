# ZoraVM Windows Release Automation

## Fixed GitHub Actions Workflow

✅ **Removed Docker Build**: Eliminated the broken Docker workflow (`docker.yml`)
✅ **Windows-Only Focus**: Streamlined to focus on Windows native binary releases
✅ **Correct Build Options**: Updated to use actual CMake options from our CMakeLists.txt
✅ **Sakemono Tag Support**: Configured to trigger releases on "Sakemono*" tags
✅ **Enhanced Packaging**: Includes both launcher scripts in release packages

## Release Workflow Features

### Trigger Conditions
- **Push to main**: Builds for testing
- **Pull requests**: Builds for validation
- **Sakemono tags**: Creates GitHub releases (e.g., `Sakemono-v1.0`, `Sakemono-beta`)

### Build Process
1. **MSYS2 Setup**: Uses UCRT64 toolchain for modern Windows compatibility
2. **Static Linking**: Creates fully standalone executable with no dependencies
3. **Release Mode**: Clean startup experience without debug noise
4. **Comprehensive Testing**: Validates binary creation and file properties

### Package Contents
Each release ZIP includes:
- `zora_vm.exe` - Main executable
- `Launch_ZoraVM_Enhanced.bat` - Auto-detects Windows Terminal
- `Launch_ZoraVM_WT.bat` - Direct Windows Terminal launcher
- `zora-vm.bat` - Simple compatibility launcher
- `README.md` and `LICENSE` (if present)

### Release Notes Template
Automatically generates detailed release notes highlighting:
- Terminal compatibility features
- Unix-like OS experience
- System monitoring capabilities
- Security features
- Usage instructions for different launcher options

## How to Create a Release

1. **Tag your commit**:
   ```bash
   git tag Sakemono-v2.1.0
   git push origin Sakemono-v2.1.0
   ```

2. **GitHub Actions will**:
   - Build the Windows binary using MSYS2/MinGW
   - Package it with all launcher scripts
   - Create a GitHub release with the ZIP file
   - Generate comprehensive release notes

3. **Users can then**:
   - Download the ZIP from GitHub Releases
   - Extract and use any of the launcher scripts
   - Enjoy the full ZoraVM experience with terminal auto-detection

## Build Configuration

The workflow uses these CMake options to match our current setup:
```cmake
-DCMAKE_BUILD_TYPE=Release
-DZORA_RELEASE_MODE=ON
-DZORA_VERBOSE_BOOT=OFF
```

This ensures a clean, professional release build with:
- Static linking for portability
- Release mode optimizations
- Clean startup without debug verbosity
- Full Windows sandbox and terminal compatibility features
