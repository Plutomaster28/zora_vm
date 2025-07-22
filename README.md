# ZoraVM

**A tiny virtual machine embedded with Zora — because we needed it, and nobody else was gonna do it.**

---

## What is ZoraVM?

ZoraVM is our minimal, purpose-built virtual machine designed specifically for Zora and its ecosystem. Think of it like a sandboxed environment for executing low-level code, scripts, or even entire apps — without bloating the system or reinventing QEMU.

If Zora had a napkin, this is what we scribbled on it:
```
run bytecode
support Kairos
simulate hardware (eventually)
don't suck
```

---

## Features (So Far)

- Simple instruction set (custom ISA)  
- Kairos bytecode support  
- Built-in debugging mode  
- Super low RAM/disk usage  
- Terminal-friendly (can run headless)  
- Features Meisei Virtual Silicon
- Multi-language scripting (Lua, Python, Perl)
- Virtual networking with security
- Sandboxed execution environment

---

## Status

> **Actively being worked on.**  
It boots, it runs stuff, it throws errors when it should — and sometimes when it shouldn't. Still, it's a VM. A *Zora* VM.

---

## Getting Started

### Option 1: Quick Start with Docker (Easy)
*No dependencies, no compilation, just run it:*

```bash
# Pull and run instantly
docker run -it ghcr.io/plutomaster28/zora-vm:latest

# Or with persistent storage
docker run -it -v $(pwd)/my-zora-data:/home/zora/ZoraPerl ghcr.io/plutomaster28/zora-vm:latest
```

### Option 2: Build from Source (Traditional)
*For developers or if you want to modify the code:*

#### Prerequisites:
- CMake 3.20+
- Ninja or Make
- Lua 5.4 development libraries
- Python 3.x development headers
- Perl development libraries
- C11 compatible compiler

#### Windows (MSYS2):
```bash
# Install dependencies first in MSYS2
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
pacman -S mingw-w64-ucrt-x86_64-lua mingw-w64-ucrt-x86_64-python
pacman -S mingw-w64-ucrt-x86_64-perl

# Then build
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm
mkdir build
cd build
cmake ..
ninja
./zora_vm.exe
```

#### Linux/Mac:
```bash
# Install dependencies (Ubuntu/Debian example)
sudo apt-get install cmake ninja-build liblua5.4-dev python3-dev libperl-dev

# Build
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm
mkdir build
cd build
cmake ..
ninja
./zora_vm
```

### Option 3: Build Docker Image Locally
*Want Docker but with your modifications:*

```bash
git clone https://github.com/Plutomaster28/zora_vm.git
cd zora_vm
./build-docker.sh    # Linux/Mac
# OR
build-docker.bat     # Windows (requires Docker Desktop)
```

---

## Usage Examples

### Basic VM Commands:
```bash
# Once running (any method above):
zora-vm:merl> help                    # Show available commands
zora-vm:merl> lua print("Hello!")     # Run Lua script  
zora-vm:merl> network ping google.com # Test virtual networking
zora-vm:merl> vfs ls                  # List virtual filesystem
zora-vm:merl> binary run myapp.elf    # Execute binary
```

### Docker-Specific Commands:
```bash
# Run specific script from host
docker run -v $(pwd):/scripts ghcr.io/plutomaster28/zora-vm:latest -c "lua /scripts/my_script.lua"

# Development mode with source mounted
docker run -it -v $(pwd):/workspace ghcr.io/plutomaster28/zora-vm:dev

# Network-enabled container
docker run -it --network host ghcr.io/plutomaster28/zora-vm:latest
```

---

## Development

### Project Structure:
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