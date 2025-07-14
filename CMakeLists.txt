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
include_directories(MERL)  # Add MERL directory

# Create executable with all sources
add_executable(zora_vm 
    ${MAIN_SOURCES}
    ${CPU_SOURCES}
    ${MEMORY_SOURCES}
    ${DEVICE_SOURCES}
    ${KERNEL_SOURCES}
    ${SANDBOX_SOURCES}
    ${ZORAPERL_SOURCES}
    ${MERL_VM_SOURCES}
    ${VFS_SOURCES}              # Add VFS sources
    ${SYSCALL_SOURCES}          # Add syscall interception
    ${VIRTUALIZATION_SOURCES}   # Add virtualization layer
    ${MERL_SHELL_SOURCES}       # Add MERL shell sources (excluding merl_main.c)
    ${HEADERS}
    ${MERL_HEADERS}
)

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(zora_vm kernel32 user32 advapi32)
endif()

# Debug configuration
set_target_properties(zora_vm PROPERTIES
    DEBUG_POSTFIX "_d"
)

# Add compiler definitions for VM mode
add_definitions(-DZORA_VM_MODE)
add_definitions(-DVIRTUAL_FILESYSTEM)
add_definitions(-DSANDBOXED_SYSCALLS)