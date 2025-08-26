# Zora VM - Virtual Machine with MERL Shell

<div align="center">

![Zora VM Logo](proto_meisei_font.png)

**A lightweight, cross-platform virtual machine environment with Unix/Linux command compatibility**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

</div>

## Quick Start

### Prerequisites
- **Windows**: MinGW-w64 or Visual Studio 2019+
- **Linux**: GCC 9.0+ 
- **CMake**: 3.20 or higher
- **Git**: For cloning the repository

### Installation

#### Option 1: Download Pre-built Executable (Recommended)
1. Download the latest `ZoraVM_Distribution.zip` from the releases
2. Extract to your desired location
3. Run `Launch_ZoraVM.bat` (Windows) or `./zora_vm` (Linux)

#### Option 2: Build from Source
```bash
# Clone the repository
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm

# Build on Windows
build-docker.bat

# Build on Linux
./build-docker.sh

# Or use CMake directly
mkdir build && cd build
cmake ..
cmake --build .
```

### First Run
```bash
# Windows
.\Launch_ZoraVM.bat

# Linux
./zora_vm
```

## What is Zora VM?

Zora VM is a **standalone virtual machine environment** that provides:

- **Complete Unix/Linux command compatibility** on any platform
- **Isolated sandbox environment** for safe code execution
- **Multi-language scripting support** (Python, Lua, Perl)
- **Virtual file system** with persistent storage
- **Network simulation** with security features
- **Desktop environment** with theming support

## Features

### Core Features
- **Full Unix Shell Experience**: 80+ commands including `ls`, `cd`, `grep`, `tar`, `ssh`, `top`, etc.
- **Multi-Platform**: Runs natively on Windows, Linux, and macOS
- **Scripting Languages**: Python, Lua, and Perl interpreters built-in
- **Virtual File System**: Persistent storage with Unix-style permissions
- **Network Stack**: Virtual networking with NAT, DNS, and firewall simulation
- **Process Management**: Background jobs, process monitoring, and control
- **Package Management**: Tetra package system for easy software installation

### User Interface
- **Colored Terminal**: Kali Linux-inspired color scheme
- **User System**: Multi-user support with authentication
- **Command History**: Full command history with search
- **Tab Completion**: Auto-completion for commands and paths
- **Desktop Environment**: GTK-based GUI with CDE theming

### Security
- **Sandbox Isolation**: All code execution is sandboxed
- **Permission System**: Unix-style file permissions
- **Network Security**: Configurable firewall rules
- **Safe Mode**: Restricted execution environment

## Command Reference

### File System Commands
```bash
ls              # List directory contents
cd <path>       # Change directory
pwd             # Print working directory
mkdir <dir>     # Create directory
touch <file>    # Create empty file
cp <src> <dst>  # Copy files
mv <src> <dst>  # Move/rename files
rm <file>       # Remove files
find <pattern>  # Search for files
tree            # Display directory tree
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
# Run a Python script
python /scripts/example.py

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

**Q: Python/Lua scripts fail**
A: Ensure scripts are placed in `/scripts/` directory and have proper syntax.

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

## 🏗️ Architecture

```
┌─────────────────────────────────────┐
│             MERL Shell              │
├─────────────────────────────────────┤
│    Command Parser & Dispatcher     │
├─────────────────────────────────────┤
│  Python │  Lua  │  Perl │ Binary   │
│   VM    │  VM   │  VM   │ Executor │
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
3. Make changes and test (both native and Docker if possible)
4. Submit pull request

### Testing:
```bash
# Native testing
ninja test

# Docker testing  
docker run --rm ghcr.io/plutomaster28/zora-vm:latest -c "echo 'test' | zora_vm --batch-mode"
```

---

## Distribution

- **Docker Hub**: `ghcr.io/plutomaster28/zora-vm:latest`
- **Source**: `https://github.com/Plutomaster28/zora_vm`
- **Releases**: Check GitHub releases for precompiled binaries

---

## Community

- Report bugs via GitHub Issues
- Discuss features in GitHub Discussions  
- Docker-related issues: tag with `docker` label

---

> Works best on systems that don't ask too many questions.