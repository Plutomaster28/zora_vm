cmake_minimum_required(VERSION 3.20)
project(zora_vm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-only build (including MSYS2/MinGW environments)
if(NOT WIN32 AND NOT MSYS AND NOT MINGW)
    message(FATAL_ERROR "This project only supports Windows")
endif()

add_definitions(-D_WIN32_WINNT=0x0601 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Static linking for standalone executable
if(MINGW OR MSYS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Enable Windows resource compilation
enable_language(RC)

# Find Lua with better error handling
find_path(LUA_INCLUDE_DIR lua.h
    HINTS 
        C:/msys64/ucrt64/include/lua5.4 
        C:/msys64/ucrt64/include/lua
        /ucrt64/include/lua5.4
        /ucrt64/include/lua
        ${CMAKE_PREFIX_PATH}/include/lua5.4
        ${CMAKE_PREFIX_PATH}/include/lua
)

find_library(LUA_LIBRARIES
    NAMES lua5.4 lua54 lua
    HINTS 
        C:/msys64/ucrt64/lib
        /ucrt64/lib
        ${CMAKE_PREFIX_PATH}/lib
)

if(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARIES)
    message(STATUS "Lua not found in standard locations")
    message(STATUS "Searched for headers in:")
    message(STATUS "  - C:/msys64/ucrt64/include/lua5.4")
    message(STATUS "  - C:/msys64/ucrt64/include/lua") 
    message(STATUS "  - /ucrt64/include/lua5.4")
    message(STATUS "  - /ucrt64/include/lua")
    message(STATUS "Searched for libraries in:")
    message(STATUS "  - C:/msys64/ucrt64/lib")
    message(STATUS "  - /ucrt64/lib")
    message(STATUS "")
    message(STATUS "To install Lua in MSYS2/UCRT64:")
    message(STATUS "  pacman -S mingw-w64-ucrt-x86_64-lua")
    message(STATUS "")
    message(STATUS "Or run the automated build script:")
    message(STATUS "  ./build_zora_vm.sh")
    message(FATAL_ERROR "Lua development files not found!")
else()
    message(STATUS "Found Lua headers: ${LUA_INCLUDE_DIR}")
    message(STATUS "Found Lua library: ${LUA_LIBRARIES}")
endif()

# Include directories
include_directories(include include/platform include/binary include/merl include/vfs 
                   include/syscall include/virtualization include/network include/lua 
                   include/python include/perl include/meisei include/desktop include/sandbox
                   include/terminal
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
    src/terminal/terminal_style.c
    
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
