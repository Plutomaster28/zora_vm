# Zora VM - Virtual Machine with MERL Shell

## Overview
Zora VM is a sophisticated virtual machine that creates a completely sandboxed environment, featuring the MERL (Micro Extensible Runtime Language) shell as its primary interface. The VM implements a full virtualization layer with its own filesystem, system call interception, and ZoraPerl runtime integration, providing a secure and isolated computing environment that cannot access the host system.

## Key Features
- **Complete Sandboxing** - Isolated from host filesystem and system calls
- **Virtual Filesystem** - In-memory filesystem with Unix-like directory structure
- **MERL Shell** - Feature-rich shell with 20+ built-in commands
- **ZoraPerl Integration** - Custom Perl-like scripting language support
- **System Call Interception** - Redirects system calls to virtual implementations
- **Memory Management** - Configurable memory allocation (64MB default)
- **CPU Emulation** - Virtual CPU with process management
- **Virtual Devices** - Emulated hardware devices

## Architecture

### Project Structure
```
zora_vm/
├── src/
│   ├── main.c                    # VM entry point and initialization
│   ├── vm.c                      # Core VM lifecycle management
│   ├── cpu/
│   │   └── cpu.c                 # Virtual CPU implementation
│   ├── memory/
│   │   └── memory.c              # Memory management and allocation
│   ├── devices/
│   │   └── device.c              # Virtual device emulation
│   ├── kernel/
│   │   └── kernel.c              # VM kernel operations
│   ├── sandbox/
│   │   └── sandbox.c             # Sandboxing and security layer
│   ├── vfs/
│   │   └── vfs.c                 # Virtual filesystem implementation
│   ├── syscall/
│   │   └── syscall.c             # System call interception
│   ├── virtualization/
│   │   └── virtualization.c      # Virtualization layer management
│   ├── zoraperl/
│   │   ├── zoraperl.c            # ZoraPerl runtime
│   │   └── vfs.c                 # ZoraPerl VFS integration
│   └── merl/
│       └── merl_vm.c             # MERL shell VM integration
├── include/
│   ├── cpu.h                     # CPU emulation headers
│   ├── memory.h                  # Memory management headers
│   ├── device.h                  # Device emulation headers
│   ├── kernel.h                  # Kernel operation headers
│   ├── sandbox.h                 # Sandboxing headers
│   ├── vm.h                      # Core VM headers
│   ├── vfs/
│   │   └── vfs.h                 # Virtual filesystem headers
│   ├── syscall/
│   │   └── syscall.h             # System call interception headers
│   ├── virtualization/
│   │   └── virtualization.h      # Virtualization layer headers
│   ├── zoraperl/
│   │   └── zoraperl.h            # ZoraPerl runtime headers
│   └── merl/
│       └── merl.h                # MERL shell headers
├── MERL/                         # MERL shell implementation
│   ├── shell.c                   # Main shell logic
│   ├── shell.h                   # Shell command definitions
│   ├── kernel.c                  # MERL kernel commands
│   ├── user.c                    # User management
│   ├── utils.c                   # Utility functions
│   └── [other MERL components]
├── build/                        # Build directory (generated)
├── CMakeLists.txt               # CMake build configuration
└── README.md                    # This file
```

### Virtual Filesystem Structure
The VM creates an in-memory Unix-like filesystem:
```
/ (root)
├── bin/          # Virtual binaries
├── home/         # User home directory
│   └── readme.txt
├── tmp/          # Temporary files
├── etc/          # Configuration files
│   ├── hosts
│   └── passwd
├── usr/          # User programs
└── var/          # Variable data
```

## Setup Instructions

### Prerequisites
- CMake 3.20 or higher
- C compiler (GCC/Clang/MSVC)
- Windows (current target platform)

### Building
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd zora_vm
   ```

2. Create build directory and configure:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```bash
   ninja
   # or
   make
   ```

4. Run the virtual machine:
   ```bash
   ./zora_vm.exe
   ```

## Usage

### Starting the VM
When you run Zora VM, it will:
1. Initialize the sandbox environment
2. Set up the virtual filesystem
3. Configure system call interception
4. Allocate virtual memory (64MB default)
5. Start the MERL shell

### MERL Shell Commands
The MERL shell provides numerous built-in commands:

**File Operations:**
- `ls` - List directory contents
- `pwd` - Print current directory
- `cd <dir>` - Change directory
- `mkdir <dir>` - Create directory
- `rmdir <dir>` - Remove directory
- `touch <file>` - Create empty file
- `rm <file>` - Remove file
- `cp <src> <dst>` - Copy file
- `mv <src> <dst>` - Move/rename file

**System Information:**
- `sysinfo` - Display system information
- `ps` - List processes
- `help` - Show available commands
- `man <command>` - Show command manual

**Utilities:**
- `clear` - Clear screen
- `echo <text>` - Print text
- `cat <file>` - Display file contents
- `calendar` - Show calendar
- `clock` - Display current time

**VM-Specific:**
- `vmstat` - Show VM status
- `cpuinfo` - Display CPU information
- `meminfo` - Show memory usage
- `devices` - List virtual devices

### Security Features
- **Complete Isolation**: VM cannot access host filesystem
- **Memory Sandboxing**: Limited to allocated VM memory
- **System Call Filtering**: All system calls redirected to virtual implementations
- **Process Containment**: All processes run within VM context

## Configuration

### Memory Configuration
Modify `include/memory.h` to change memory allocation:
```c
#define MEMORY_SIZE 0x4000000 // 64 MB (current)
#define MEMORY_SIZE 0x8000000 // 128 MB (example)
```

### Build Options
CMake definitions available:
- `ZORA_VM_MODE` - Enable VM mode
- `VIRTUAL_FILESYSTEM` - Enable virtual filesystem
- `SANDBOXED_SYSCALLS` - Enable system call sandboxing

## Development

### Adding New Commands
1. Add command function to `MERL/shell.c`
2. Add command entry to `command_table[]`
3. Update `MERL/shell.h` with function declaration

### Extending VFS
1. Add new VFS functions to `src/vfs/vfs.c`
2. Update `include/vfs/vfs.h` with declarations
3. Integrate with MERL commands as needed

### Virtual Device Creation
1. Implement device in `src/devices/device.c`
2. Add device headers to `include/device.h`
3. Register device in VM initialization

## Technical Details

### Virtualization Layer
- **VFS**: Complete in-memory filesystem
- **Syscall Interception**: Redirects system calls to VM implementations
- **Memory Management**: Virtual memory allocation and protection
- **Process Management**: Virtual process table and scheduling

### Performance
- **Memory Usage**: Configurable (64MB default)
- **CPU Overhead**: Minimal virtualization overhead
- **Startup Time**: Fast initialization (~1 second)

## Troubleshooting

### Common Issues
1. **Memory Initialization Failed**: Reduce `MEMORY_SIZE` in `memory.h`
2. **Build Errors**: Ensure CMake 3.20+ and proper compiler
3. **Missing Commands**: Check MERL shell implementation

### Debug Mode
Build with debug symbols:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Contributing
Contributions are welcome! Areas for improvement:
- Persistent filesystem support (added)
- Network virtualization
- GUI integration
- Performance optimizations
- Additional shell commands

## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments
- MERL shell implementation
- Virtual filesystem design
- Sandboxing architecture
- ZoraPerl integration

## Persistent Storage

### Overview
Zora VM supports persistent storage through host directory mapping. The `/persistent` directory in the VM is mapped to the `ZoraPerl/` directory on the host system, allowing data to survive VM restarts.

### Directory Structure
```
ZoraPerl/                 # Host directory
├── documents/            # -> /persistent/documents in VM
├── scripts/              # -> /persistent/scripts in VM  
├── data/                 # -> /persistent/data in VM
└── projects/             # -> /persistent/projects in VM
```

### Commands
- `save <file>` - Save file to persistent storage
- `load <dir>` - Reload directory from persistent storage
- `mount <vm_path> <host_path>` - Mount host directory
- `sync` - Sync all persistent storage
- `pls` - List persistent storage mappings

### Example Usage
```bash
# Work with persistent files
cd /persistent/documents
echo "Hello World" > myfile.txt
save myfile.txt

# File is now saved to ZoraPerl/documents/myfile.txt on host
```

## Network Virtualization

### Overview
Zora VM includes a complete virtual network stack that simulates network operations without affecting the host system. All network commands operate within the VM's isolated environment.

### Virtual Network Features
- **Virtual Network Interfaces** - Configurable network adapters
- **Simulated Network Protocols** - TCP/UDP, ICMP, DNS
- **Virtual Routing** - Routing table management
- **Network Statistics** - Traffic monitoring
- **Firewall Simulation** - Virtual iptables rules

### Network Commands
- `ifconfig` - Configure network interfaces
- `ping <host>` - Test network connectivity
- `netstat` - Display network connections and statistics
- `nslookup <hostname>` - DNS lookup simulation
- `wget <url>` - Download files (simulated)
- `curl <url>` - HTTP client requests
- `telnet <host> <port>` - Remote connection
- `ssh <user@host>` - Secure shell simulation
- `iptables` - Firewall configuration

### Example Usage
```bash
# Check network configuration
ifconfig

# Test connectivity
ping google.com

# Download a file
wget https://example.com/page.html

# Check active connections
netstat
```

### Security Features
- **Isolated Network Stack** - No actual network access
- **Simulated Responses** - Safe testing environment
- **No Host Impact** - Cannot affect real network settings
- **Educational Purpose** - Learn networking concepts safely