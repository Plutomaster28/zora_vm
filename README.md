# Zora VM - **Complete Unix/Linux Shell** - 80+ commands working perfectly  
 **Beautiful User Interface** - Categorized help system with Unicode styling  
 **Pipeline Operations** - Full command chaining and redirection  
 **Lua Scripting** - Full Lua 5.4.6 VM with sandboxed APIs  
 **Virtual File System** - Persistent storage with Unix permissions  
 **Network Simulation** - Complete virtual networking stack  
 **Sandbox Security** - Isolated execution environment  
 **Command Help System** - Every command has `--help` documentation  
 **Professional Experience** - Clean startup modes and polished interface  
 **Build Configurations** - Release mode for clean startup, verbose mode for debugging Virtual Machine with MERL Shell

<div align="center">

![Zora VM Logo](proto_meisei_font.png)

**A complete, production-ready Windows virtual machine environment with Unix/Linux compatibility**

[![Build Status](https://img.shields.io/badge/build-stable-brightgreen)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()
[![Status](https://img.shields.io/badge/status-fully_functional-success)]()

</div>

##  ZoraVM is Now Fully Functional!

**ZoraVM has reached full production maturity** with enterprise-grade functionality:

 **Complete Unix/Linux Shell** - 80+ commands working perfectly  
 **Beautiful User Interface** - Categorized help system with Unicode styling  
 **Pipeline Operations** - Full command chaining and redirection  
 **Lua Scripting** - Full Lua 5.4.6 VM with sandboxed APIs  
 **Virtual File System** - Persistent storage with Unix permissions  
 **Network Simulation** - Complete virtual networking stack  
 **Sandbox Security** - Isolated execution environment  
 **Command Help System** - Every command has `--help` documentation  
 **Professional Experience** - Clean startup modes and polished interface  

## Quick Start

### Prerequisites
- **Windows**: MinGW-w64 (MSYS2/UCRT64 recommended)
- **CMake**: 3.20 or higher
- **Ninja**: Build system (install via MSYS2: `pacman -S ninja`)

### Installation

#### Option 1: Release Mode (Clean Startup - Recommended)
```bash
# Clone the repository
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm

# Build in clean release mode
build_release.bat
```

#### Option 2: Verbose Mode (Full Debug Output)
```bash
# Build with full debug output for development
build_verbose.bat
```

#### Option 3: Manual Build
```bash
mkdir build && cd build
cmake .. -G "Ninja" -DZORA_RELEASE_MODE=ON -DZORA_VERBOSE_BOOT=OFF
ninja
```

### First Run
```bash
cd build
.\zora_vm.exe
```

## What is Zora VM?

**Zora VM is a complete virtual machine environment** that provides a fully functional Unix-like operating system running natively on Windows. It's not an emulator or compatibility layer - it's a sophisticated virtual machine with its own kernel, memory management, and complete shell environment.

Zora VM is a **standalone virtual machine environment** that provides:

- **Complete Unix/Linux command compatibility** on Windows
- **Isolated sandbox environment** for safe code execution
- **Lua 5.4.6 scripting** with sandboxed VM APIs
- **Virtual file system** with persistent storage
- **Network simulation** with security features
- **Desktop environment** with theming support

## Features

###  Production-Ready Core Features
- **Complete Unix Shell Experience**: 80+ working commands including `ls`, `cd`, `grep`, `tar`, `ssh`, `top`, `find`, `sort`, `uniq`, `wc`, `awk`, and many more
- **Beautiful Interface**: Unicode-enhanced help system with categorized commands and professional styling
- **Pipeline Operations**: Full support for command chaining (`|`), redirection (`>`, `<`, `>>`), and logical operators
- **Command Documentation**: Every command includes comprehensive `--help` documentation with examples
- **Windows-Native**: Runs natively on Windows with no external dependencies after build
- **Multi-language Scripting**: Full Lua 5.4.6 interpreter with sandboxed VFS, VM, and system APIs
- **Virtual File System**: Complete filesystem with Unix-style permissions and persistent storage
- **Network Stack**: Virtual networking with NAT, DNS, firewall simulation, and security features
- **Process Management**: Background jobs, process monitoring, signals, and complete process control

###  User Experience
- **Clean Startup Modes**: Choose between minimal release mode or verbose debug mode
- **Campbell Color Scheme**: Professional terminal styling inspired by modern development environments
- **Command History**: Full command history with search and recall functionality
- **Tab Completion**: Intelligent auto-completion for commands and file paths
- **Exit Command**: Graceful VM shutdown with proper cleanup
- **Error Handling**: Comprehensive error messages and recovery mechanisms

###  Security & Isolation
- **Complete Sandbox**: All code execution is fully sandboxed and isolated from the host system
- **Memory Limits**: Configurable memory limits with out-of-memory protection
- **Network Security**: Virtual firewall with configurable rules and safe mode
- **File Permissions**: Complete Unix-style permission system
- **Resource Monitoring**: CPU and memory usage monitoring with limits

##  Getting Started - Your First Commands

Once ZoraVM is running, try these commands to explore:

```bash
# Get beautiful categorized help
help

# See detailed help for any command
ls --help
grep --help
sort --help

# Explore the file system
ls -la
tree
pwd

# Try some pipelines
help | grep file
ls -la | grep "txt"
cat /etc/passwd | sort | uniq

# File operations
touch myfile.txt
echo "Hello ZoraVM!" > myfile.txt
cat myfile.txt

# Process management
ps aux
top

# Exit the VM cleanly
exit
```

##  Build Configuration Options

ZoraVM supports two build modes for different use cases:

### Release Mode (Default - Recommended)
- **Clean startup experience** with minimal output
- **MERL-inspired boot sequence** with spinner animations  
- **Professional user interface** perfect for daily use
- **Build command**: `build_release.bat` or cmake with `-DZORA_RELEASE_MODE=ON`

### Verbose Mode (Development)
- **Full debug output** showing all initialization steps
- **Detailed logging** for troubleshooting and development
- **Complete system information** during startup
- **Build command**: `build_verbose.bat` or cmake with `-DZORA_VERBOSE_BOOT=ON`

## Command Reference

### Essential File System Commands
```bash
ls [options] [path]    # List directory contents (supports -l, -a, -h, etc.)
cd <path>              # Change directory  
pwd                    # Print working directory
mkdir <dir>            # Create directory
touch <file>           # Create empty file or update timestamp
cp <src> <dst>         # Copy files and directories
mv <src> <dst>         # Move/rename files
rm [options] <file>    # Remove files (-r for recursive, -f for force)
find <path> <pattern>  # Search for files and directories
tree [path]            # Display directory tree structure
ln <target> <link>     # Create symbolic links
chmod <mode> <file>    # Change file permissions
```

### Text Processing & Search
```bash
cat <file>             # Display file contents
grep <pattern> <file>  # Search text patterns (supports regex)
sort <file>            # Sort lines in files
uniq <file>            # Remove duplicate lines  
wc <file>              # Count lines, words, characters
head <file>            # Show first lines of file
tail <file>            # Show last lines of file
awk <pattern> <file>   # Advanced text processing
sed 's/old/new/' <file> # Stream editor for text substitution
```

### Process & System Management
```bash
ps [aux]               # List running processes
top                    # Display running processes (interactive)
kill <pid>             # Terminate process by ID
killall <name>         # Terminate processes by name
jobs                   # List background jobs
bg                     # Put job in background
fg                     # Bring job to foreground
nohup <command> &      # Run command immune to hangups
```

### Text Processing
```bash
cat <file>      # Display file contents
head <file>     # Show first lines of file
tail <file>     # Show last lines of file
more/less <file> # Page through file
grep <pattern> <file> # Search within files
```

### System Information
```bash
uname -a        # System information
date            # Current date/time
df -h           # Disk space usage
du <path>       # Directory sizes
top/htop        # Process monitor
ps              # List processes
whoami          # Current user
hostname        # System hostname
```

### Network Commands
```bash
ping <host>     # Test connectivity
ifconfig        # Network interfaces
netstat         # Network connections
nslookup <host> # DNS lookup
wget <url>      # Download files
ssh <host>      # Secure shell
```

### Archive & Compression
```bash
tar -czf archive.tar.gz files/  # Create compressed archive
tar -xzf archive.tar.gz         # Extract archive
gzip <file>                     # Compress file
gunzip <file.gz>               # Decompress file
zip archive.zip files/          # Create zip archive
unzip archive.zip              # Extract zip archive
```

### User Management
```bash
login <user> <password>    # Log in as user
logout                     # Log out current user
useradd <user> <password>  # Add new user
passwd <newpassword>       # Change password
hostname <new_hostname>    # Set system hostname
```

### Virtual Machine Control
```bash
vmstat          # VM status and performance
reboot          # Restart the VM
shutdown        # Shutdown the VM
```

## Getting Started Tutorial

### 1. Basic Navigation
```bash
# Start the VM
zora_vm

# Check where you are
pwd

# List files in current directory
ls

# Navigate to home directory
cd /home

# Create a new directory
mkdir my_projects
cd my_projects
```

### 2. Working with Files
```bash
# Create a new file
touch hello.txt

# Edit the file (opens simple editor)
edit hello.txt

# View file contents
cat hello.txt

# Search for text
grep "hello" hello.txt
```

### 3. Running Scripts
```bash
# Run a Lua script
lua /scripts/example.lua

# Execute Lua code directly
luacode "print('Hello from Lua!')"

# List available scripts
ls /scripts
```

### 4. Network Operations
```bash
# Test connectivity
ping google.com

# Check network interfaces
ifconfig

# Download a file (simulated)
wget https://example.com/file.txt
```

### 5. User Management
```bash
# Create a new user
useradd myuser mypassword

# Log in as the new user
login myuser mypassword

# Check current user
whoami

# Change hostname
hostname my-computer
```

## Configuration

### Environment Variables
- `ZORA_HOME`: Set custom home directory
- `ZORA_THEME`: Default desktop theme
- `ZORA_SAFE_MODE`: Enable restricted execution mode

### Configuration Files
- `/etc/zora.conf`: Main configuration
- `/etc/themes/`: Desktop themes
- `/home/.zora-desktop/`: User settings

## Troubleshooting

### Common Issues

**Q: VM won't start on Windows**
A: Ensure you have Windows 10 version 1903+ or run `system_check.bat` to verify compatibility.

**Q: Commands are slow**
A: Check available RAM. VM requires minimum 2GB free memory for optimal performance.

**Q: Network commands don't work**
A: Network features are simulated for security. They demonstrate functionality without actual network access.

**Q: Colors don't show properly**
A: Ensure your terminal supports ANSI escape sequences. On Windows 10+, this should work automatically.

### Debug Mode
```bash
# Enable debug output
export ZORA_DEBUG=1
./zora_vm

# Check VM status
vmstat

# View system logs
cat /var/log/zora_vm.log
```

## Architecture

```
┌─────────────────────────────────────┐
│             MERL Shell              │
├─────────────────────────────────────┤
│    Command Parser & Dispatcher     │
├─────────────────────────────────────┤
│   Lua VM  │ Binary  │ Compilers    │
│  (5.4.6)  │Executor │ (C/ASM/F77)  │
├─────────────────────────────────────┤
│         Virtual File System        │
├─────────────────────────────────────┤
│      Network Simulation Layer      │
├─────────────────────────────────────┤
│         Sandbox & Security         │
├─────────────────────────────────────┤
│       Host OS Abstraction         │
└─────────────────────────────────────┘
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Setup
```bash
# Clone for development
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm

# Build in debug mode
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Run tests
ctest
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **MERL Shell**: Inspired by Unix shell design principles
- **Tetra Package Manager**: Custom lightweight package management
- **Virtual Silicon**: High-performance execution engine
- **Community**: Thanks to all contributors and testers

## Support

- **Documentation**: [Wiki](https://github.com/Plutomaster28/zora_vm/wiki)
- **Issues**: [GitHub Issues](https://github.com/Plutomaster28/zora_vm/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Plutomaster28/zora_vm/discussions)

---

<div align="center">

**Made with ❤️ by Tomoko**

[Website](https://zoravirus.dev) • [Documentation](https://docs.zoravirus.dev) • [Community](https://discord.gg/zoravirus)

</div>
```
zora_vm/
├── src/               # Core VM implementation
│   ├── cpu/          # CPU emulation
│   ├── memory/       # Memory management
│   ├── meisei/       # Meisei Virtual Silicon (script acceleration)
│   ├── network/      # Virtual networking
│   └── ...
├── include/          # Header files
├── MERL/            # MERL shell implementation
└── docs/            # Documentation
```

### Contributing:
1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Make changes and test
4. Submit pull request

### Testing:
```bash
# Native testing
ninja test
```

---

## Distribution

- **Source**: `https://github.com/Plutomaster28/zora_vm`
- **Releases**: Check GitHub releases for precompiled Windows binaries

---

## Community

- Report bugs via GitHub Issues
- Discuss features in GitHub Discussions

---

> Works best on systems that don't ask too many questions.