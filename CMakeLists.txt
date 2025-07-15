cmake_minimum_required(VERSION 3.20)
project(zora_vm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable security features
if(MSVC)
    add_compile_options(/GS /sdl)
else()
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
endif()

# Find Lua
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUA REQUIRED lua5.4)

# If Lua is not found via pkg-config, try manual search
if(NOT LUA_FOUND)
    find_path(LUA_INCLUDE_DIR lua.h
        HINTS
        /usr/include/lua5.4
        /usr/include/lua
        /usr/local/include/lua5.4
        /usr/local/include/lua
        C:/msys64/ucrt64/include/lua5.4
        C:/msys64/ucrt64/include/lua
    )
    
    find_library(LUA_LIBRARIES
        NAMES lua5.4 lua54 lua
        HINTS
        /usr/lib
        /usr/local/lib
        C:/msys64/ucrt64/lib
    )
endif()

# Simple scripting support (no embedding)
add_definitions(-DPYTHON_SCRIPTING)
add_definitions(-DPERL_SCRIPTING)

# Source files organized by directory
file(GLOB MAIN_SOURCES "src/*.c")
file(GLOB CPU_SOURCES "src/cpu/*.c")
file(GLOB MEMORY_SOURCES "src/memory/*.c")
file(GLOB DEVICE_SOURCES "src/devices/*.c")
file(GLOB KERNEL_SOURCES "src/kernel/*.c")
file(GLOB SANDBOX_SOURCES "src/sandbox/*.c")
file(GLOB ZORAPERL_SOURCES "src/zoraperl/*.c")
file(GLOB MERL_VM_SOURCES "src/merl/*.c")

# Add new virtualization components
file(GLOB VFS_SOURCES "src/vfs/*.c")
file(GLOB SYSCALL_SOURCES "src/syscall/*.c")
file(GLOB VIRTUALIZATION_SOURCES "src/virtualization/*.c")
file(GLOB NETWORK_SOURCES "src/network/*.c")
file(GLOB LUA_SOURCES "src/lua/*.c")  # Add Lua integration sources
file(GLOB PYTHON_SOURCES "src/python/*.c")  # Add Python sources
file(GLOB PERL_SOURCES "src/perl/*.c")      # Add Perl sources

# Include the actual MERL shell sources but exclude merl_main.c
file(GLOB MERL_SHELL_SOURCES "MERL/*.c")
list(REMOVE_ITEM MERL_SHELL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/MERL/merl_main.c")

# All headers
file(GLOB_RECURSE HEADERS "include/*.h")
file(GLOB_RECURSE MERL_HEADERS "MERL/*.h")

# Include directories
include_directories(include)
include_directories(include/zoraperl)
include_directories(include/merl)
include_directories(include/vfs)
include_directories(include/syscall)
include_directories(include/virtualization)
include_directories(include/network)
include_directories(include/lua)  # Add Lua include directory
include_directories(MERL)
include_directories(${LUA_INCLUDE_DIRS})  # Add Lua system includes
include_directories(include/python)
include_directories(include/perl)
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(include/binary)  # Add binary include directory

# Add binary execution sources
file(GLOB BINARY_SOURCES "src/binary/*.c")

# Include binary directory
include_directories(include/binary)

# Add to executable
add_executable(zora_vm 
    ${MAIN_SOURCES}
    ${CPU_SOURCES}
    ${MEMORY_SOURCES}
    ${DEVICE_SOURCES}
    ${KERNEL_SOURCES}
    ${SANDBOX_SOURCES}
    ${ZORAPERL_SOURCES}
    ${MERL_VM_SOURCES}
    ${VFS_SOURCES}
    ${SYSCALL_SOURCES}
    ${VIRTUALIZATION_SOURCES}
    ${NETWORK_SOURCES}
    ${LUA_SOURCES}
    ${PYTHON_SOURCES}
    ${PERL_SOURCES}
    ${BINARY_SOURCES}  # Add this line
    ${MERL_SHELL_SOURCES}
    ${HEADERS}
    ${MERL_HEADERS}
)

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(zora_vm kernel32 user32 advapi32 ${LUA_LIBRARIES})
else()
    target_link_libraries(zora_vm ${LUA_LIBRARIES})
endif()

# Add Perl flags if found
if(PERL_EXECUTABLE)
    target_compile_options(zora_vm PRIVATE ${PERL_CFLAGS_LIST})
    target_link_libraries(zora_vm ${PERL_LDFLAGS_LIST})
endif()

# Debug configuration
set_target_properties(zora_vm PROPERTIES
    DEBUG_POSTFIX "_d"
)

# Add compiler definitions for VM mode
add_definitions(-DZORA_VM_MODE)
add_definitions(-DVIRTUAL_FILESYSTEM)
add_definitions(-DSANDBOXED_SYSCALLS)
add_definitions(-DLUA_SCRIPTING)  # Add Lua scripting support