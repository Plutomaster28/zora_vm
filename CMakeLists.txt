cmake_minimum_required(VERSION 3.20)
project(zora_vm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-only build
if(NOT WIN32)
    message(FATAL_ERROR "This project only supports Windows")
endif()

add_definitions(-D_WIN32_WINNT=0x0601 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Static linking for standalone executable
if(MINGW OR MSYS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Enable Windows resource compilation
enable_language(RC)

# Find Lua
find_path(LUA_INCLUDE_DIR lua.h
    HINTS C:/msys64/ucrt64/include/lua5.4 C:/msys64/ucrt64/include/lua
)
find_library(LUA_LIBRARIES
    NAMES lua5.4 lua54 lua
    HINTS C:/msys64/ucrt64/lib
)

if(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARIES)
    message(FATAL_ERROR "Lua not found! Install with: pacman -S mingw-w64-ucrt-x86_64-lua")
endif()

# Include directories
include_directories(include include/platform include/binary include/merl include/vfs 
                   include/syscall include/virtualization include/network include/lua 
                   include/python include/perl include/meisei include/desktop include/sandbox
                   MERL ${LUA_INCLUDE_DIR})

# Source files - simple and direct
set(SOURCES
    # Main
    src/main.c
    
    # Core VM
    src/cpu/cpu.c
    src/memory/memory.c
    src/devices/device.c
    src/kernel/kernel.c
    
    # Binary execution
    src/binary/binary_executor.c
    src/binary/elf_parser.c
    src/binary/windows/binary_executor_win.c
    src/binary/windows/elf_parser_win.c
    
    # Sandbox (Windows-specific only)
    src/sandbox/windows/sandbox_win.c
    src/sandbox/windows/syscall_filter_win.c
    
    # VM subsystems
    src/merl/merl_vm.c
    src/vfs/vfs.c
    src/syscall/syscall.c
    src/virtualization/virtualization.c
    src/network/network.c
    src/lua/lua_vm.c
    src/python/python_vm.c
    src/perl/perl_vm.c
    src/meisei/virtual_silicon.c
    src/meisei/script_accelerator.c
    src/desktop/desktop.c
    
    # MERL shell
    MERL/color-and-test.c
    MERL/crash.c
    MERL/kernel.c
    MERL/shell.c
    MERL/tetra.c
    MERL/user.c
    MERL/utils.c
    
    # Windows resources (icon and version info)
    resources.rc
)

# Create executable
add_executable(zora_vm ${SOURCES})

# Link libraries
target_link_libraries(zora_vm ${LUA_LIBRARIES} kernel32 user32 ws2_32)

message(STATUS "Windows-only build configured successfully with icon and version info")
