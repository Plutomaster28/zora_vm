#remember to make it so that dependencies are baked in or bundled in
cmake_minimum_required(VERSION 3.20)
project(zora_vm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection and compatibility
if(WIN32)
    set(PLATFORM_NAME "Windows")
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    message(STATUS "Building for Windows")
else()
    message(FATAL_ERROR "This project now only supports Windows. Use Windows with MinGW, MSYS2, or Visual Studio.")
endif()

# Security features
if(MSVC)
    add_compile_options(/GS /sdl)
else()
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
    
    # Static linking for standalone executables on MinGW/MSYS2
    if(WIN32)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
        
        # Try to link CRT statically too (may not work on all systems)
        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic -lmsvcrt -Wl,-Bdynamic")
        
        message(STATUS "Configured for static linking on Windows (MinGW)")
    endif()
endif()

# === LUA DETECTION (Unified Logic) ===
find_package(PkgConfig QUIET)
set(LUA_FOUND FALSE)

# Windows-specific Lua detection
find_path(LUA_INCLUDE_DIR lua.h
    HINTS
    C:/msys64/ucrt64/include/lua5.4
    C:/msys64/ucrt64/include/lua
    C:/lua/include
    C:/Program Files/Lua/include
)

find_library(LUA_LIBRARIES
    NAMES lua5.4 lua54 lua
    HINTS
        C:/msys64/ucrt64/lib
        C:/lua/lib
        C:/Program Files/Lua/lib
)

if(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
    set(LUA_FOUND TRUE)
    set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
    message(STATUS "Found Lua manually: ${LUA_LIBRARIES}")
else()
    message(FATAL_ERROR "Lua not found! Please install Lua 5.4 via MSYS2: pacman -S mingw-w64-ucrt-x86_64-lua")
endif()

# Static linking preference for Lua on Windows
if(WIN32 AND NOT MSVC)
    # Try to find static Lua library if available
    find_library(LUA_STATIC_LIB 
        NAMES liblua.a lua54.a lua5.4.a
        HINTS ${LUA_LIBRARY_DIRS}
        PATH_SUFFIXES lib
    )
    
    if(LUA_STATIC_LIB)
        set(LUA_LIBRARIES ${LUA_STATIC_LIB})
        message(STATUS "Using static Lua library: ${LUA_STATIC_LIB}")
    else()
        message(STATUS "Static Lua library not found, using: ${LUA_LIBRARIES}")
    endif()
endif()

# === INCLUDE DIRECTORIES ===
include_directories(include)
include_directories(include/platform)     # Critical for platform.h
include_directories(include/sandbox)      # NEW: Sandbox headers
include_directories(include/binary)       # Binary execution headers
include_directories(include/merl)
include_directories(include/vfs)
include_directories(include/syscall)
include_directories(include/virtualization)
include_directories(include/network)
include_directories(include/lua)
include_directories(include/python)
include_directories(include/perl)
include_directories(include/meisei)
include_directories(include/desktop)
include_directories(MERL)
include_directories(${LUA_INCLUDE_DIRS})

# === SOURCE FILES ===
file(GLOB MAIN_SOURCES "src/*.c")
file(GLOB CPU_SOURCES "src/cpu/*.c")
file(GLOB MEMORY_SOURCES "src/memory/*.c")
file(GLOB DEVICE_SOURCES "src/devices/*.c")
file(GLOB KERNEL_SOURCES "src/kernel/*.c")

# Windows-only ELF Parser sources
set(ELF_SOURCES "src/binary/elf_parser.c")
set(BINARY_PLATFORM_SOURCES "src/binary/windows/binary_executor_win.c")
set(ELF_PLATFORM_SOURCES "src/binary/windows/elf_parser_win.c")
message(STATUS "Added Windows-specific files: ${BINARY_PLATFORM_SOURCES}, ${ELF_PLATFORM_SOURCES}")

# Update binary sources - Include both wrapper and platform-specific files
set(BINARY_SOURCES 
    "src/binary/binary_executor.c"
    ${BINARY_PLATFORM_SOURCES}
)

set(ELF_SOURCES 
    "src/binary/elf_parser.c"
    ${ELF_PLATFORM_SOURCES}
)

# CRITICAL: Update the platform sources to include the binary executor files
set(ALL_PLATFORM_SOURCES 
    ${PLATFORM_SANDBOX_SOURCES}
    ${BINARY_SOURCES}           # Changed from PLATFORM_BINARY_SOURCES
    ${ELF_SOURCES}              # Include ELF sources
)

# OTHER SOURCES
file(GLOB MERL_VM_SOURCES "src/merl/*.c")
file(GLOB VFS_SOURCES "src/vfs/*.c")
file(GLOB SYSCALL_SOURCES "src/syscall/*.c")
file(GLOB VIRTUALIZATION_SOURCES "src/virtualization/*.c")
file(GLOB NETWORK_SOURCES "src/network/*.c")
file(GLOB LUA_SOURCES "src/lua/*.c")
file(GLOB PYTHON_SOURCES "src/python/*.c")
file(GLOB PERL_SOURCES "src/perl/*.c")
file(GLOB MEISEI_SOURCES "src/meisei/*.c")
file(GLOB DESKTOP_SOURCES "src/desktop/*.c")

# MERL shell sources (excluding main)
file(GLOB MERL_SHELL_SOURCES "MERL/*.c")
list(REMOVE_ITEM MERL_SHELL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/MERL/merl_main.c")

# Headers - explicit list to avoid GLOB_RECURSE causing reconfiguration loops
set(HEADERS 
    include/cpu.h
    include/device.h  
    include/kernel.h
    include/memory.h
    include/merl.h
    include/sandbox.h
    include/vm.h
    include/binary/binary_executor.h
    include/binary/elf_parser.h
    include/desktop/desktop.h
    include/lua/lua_vm.h
    include/meisei/virtual_silicon.h
    include/network/network.h
    include/perl/perl_vm.h
    include/platform/platform.h
    include/python/python_vm.h
    include/syscall/syscall.h
    include/vfs/vfs.h
    include/virtualization/virtualization.h
)
set(MERL_HEADERS
    MERL/color-and-test.h
    MERL/config.h
    MERL/crash.h
    MERL/kernel.h
    MERL/shell.h
    MERL/tetra.h
    MERL/user.h
    MERL/utils.h
)

# === WINDOWS-ONLY SOURCE FILES ===
message(STATUS "Including Windows-specific sources via wrappers")

# Only include the WRAPPER files for Windows
set(PLATFORM_SANDBOX_SOURCES 
    "src/sandbox/sandbox.c"
    "src/sandbox/syscall_filter.c"
)
set(PLATFORM_BINARY_SOURCES 
    "src/binary/binary_executor.c"
)

# Windows-specific definitions
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

# Remove any direct inclusion of platform-specific files from the main sources
# Make sure these lines are NOT in your main sources:
# "src/sandbox/windows/*.c"
# "src/binary/windows/*.c"

# The wrapper files will #include the platform-specific implementations

# Combine all platform sources
set(ALL_PLATFORM_SOURCES 
    ${PLATFORM_SANDBOX_SOURCES}
    ${BINARY_SOURCES}               # Include wrapper and platform-specific binary files
    ${ELF_SOURCES}                  # Include wrapper and platform-specific ELF files
)

# Debug: Print what sources we found
message(STATUS "Platform sandbox sources: ${PLATFORM_SANDBOX_SOURCES}")
message(STATUS "Platform binary sources: ${PLATFORM_BINARY_SOURCES}")
message(STATUS "Wrapper sources: ${SANDBOX_WRAPPER_SOURCES} ${BINARY_WRAPPER_SOURCES}")
message(STATUS "ELF sources: ${ELF_SOURCES}")

# === VERIFY CRITICAL FILES EXIST ===
foreach(WRAPPER_FILE ${SANDBOX_WRAPPER_SOURCES} ${BINARY_WRAPPER_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${WRAPPER_FILE}")
        message(FATAL_ERROR "Critical wrapper file missing: ${WRAPPER_FILE}")
    endif()
endforeach()

# === CREATE EXECUTABLE ===
# Add Windows resource file if on Windows
if(WIN32)
    set(RESOURCE_FILES resources.rc)
else()
    set(RESOURCE_FILES "")
endif()

add_executable(zora_vm 
    ${MAIN_SOURCES}
    ${CPU_SOURCES}
    ${MEMORY_SOURCES}
    ${DEVICE_SOURCES}
    ${KERNEL_SOURCES}
    ${ALL_PLATFORM_SOURCES}  # Use combined platform sources
    ${MERL_VM_SOURCES}
    ${VFS_SOURCES}
    ${SYSCALL_SOURCES}
    ${VIRTUALIZATION_SOURCES}
    ${NETWORK_SOURCES}
    ${LUA_SOURCES}
    ${PYTHON_SOURCES}
    ${PERL_SOURCES}
    ${MEISEI_SOURCES}
    ${DESKTOP_SOURCES}
    ${MERL_SHELL_SOURCES}
    ${HEADERS}
    ${MERL_HEADERS}
    ${RESOURCE_FILES}
)

# === LINK LIBRARIES ===
if(WIN32)
    # Static linking setup for Windows
    if(NOT MSVC)
        # For MinGW/MSYS2: Link libraries statically when possible
        set_target_properties(zora_vm PROPERTIES 
            LINK_FLAGS "-static-libgcc -static-libstdc++ -static"
        )
        
        # Prefer static libraries
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
    
    target_link_libraries(zora_vm 
        ${LUA_LIBRARIES}
        ws2_32      # Windows sockets
        advapi32    # Windows API (registry, security)
        kernel32    # Windows kernel API
        user32      # Windows user API
        winmm       # Windows multimedia (timers)
        psapi       # Process status API
        dbghelp     # Debug help library
    )
endif()

# === COMPILER DEFINITIONS ===
add_definitions(-DZORA_VM_MODE)
add_definitions(-DVIRTUAL_FILESYSTEM)
add_definitions(-DSANDBOXED_SYSCALLS)
add_definitions(-DLUA_SCRIPTING)
add_definitions(-DPYTHON_SCRIPTING)

# NEW: Enhanced sandbox definitions
add_definitions(-DADVANCED_SANDBOX)
add_definitions(-DCROSS_PLATFORM_SANDBOX)
add_definitions(-DELF_BINARY_SUPPORT)
add_definitions(-DSYSCALL_FILTERING)

# Meisei Virtual Silicon
add_definitions(-DMEISEI_VIRTUAL_SILICON)
add_definitions(-DMEISEI_JIT_ENABLED)
add_definitions(-DMEISEI_PARALLEL_EXECUTION)
add_definitions(-DMEISEI_AUTO_ACCELERATION)
add_definitions(-DMEISEI_UNIVERSAL_LAYER)

# === OPTIMIZATION ===
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /Ot)
    else()
        add_compile_options(-O3 -march=native)
    endif()
    add_definitions(-DNDEBUG)
    message(STATUS "Release build optimizations enabled")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-O0 -g3 -ggdb)
    endif()
    add_definitions(-DDEBUG)
    add_definitions(-DSANDBOX_DEBUG)
    message(STATUS "Debug build with sandbox debugging enabled")
endif()

# Advanced CPU features (non-MSVC only)
if(NOT MSVC)
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    
    if(COMPILER_SUPPORTS_AVX2)
        add_compile_options(-mavx2)
        add_definitions(-DMEISEI_AVX2_SUPPORT)
        message(STATUS "AVX2 acceleration enabled")
    endif()
    
    # Additional security hardening for non-MSVC
    add_compile_options(-fstack-clash-protection)
    add_compile_options(-fcf-protection)
endif()

# === POST-BUILD ACTIONS ===
if(WIN32)
    # Windows-specific post-build
    add_custom_command(TARGET zora_vm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Windows build completed successfully"
        COMMAND ${CMAKE_COMMAND} -E echo "Executable location: $<TARGET_FILE:zora_vm>"
        COMMENT "Post-build validation"
    )
    
    # Add optional dependency check if objdump is available
    find_program(OBJDUMP_PROGRAM objdump)
    if(OBJDUMP_PROGRAM AND NOT MSVC)
        add_custom_command(TARGET zora_vm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Checking dynamic dependencies..."
            COMMAND ${OBJDUMP_PROGRAM} -p $<TARGET_FILE:zora_vm> | findstr "DLL Name" || echo "No external DLL dependencies found (static build)"
            COMMENT "Dependency analysis"
            VERBATIM
        )
    endif()
else()
    # Linux-specific post-build
    add_custom_command(TARGET zora_vm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Linux build completed successfully"
        COMMAND file $<TARGET_FILE:zora_vm>
        COMMENT "Post-build validation and file info"
    )
endif()

# === BUILD SUMMARY ===
message(STATUS "")
message(STATUS "=== ZORA VM BUILD CONFIGURATION ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Lua: ${LUA_LIBRARIES}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Show linking strategy
if(WIN32 AND NOT MSVC)
    message(STATUS "Linking: Static (standalone executable)")
    message(STATUS "Runtime dependencies: None (fully self-contained)")
else()
    message(STATUS "Linking: Dynamic")
endif()

message(STATUS "Sandbox: Advanced cross-platform")
message(STATUS "Binary support: ELF parsing enabled")
if(WIN32)
    message(STATUS "Windows features: Enhanced sandbox, syscall filtering")
else()
    if(SECCOMP_FOUND)
        message(STATUS "Linux sandbox: Enhanced (seccomp)")
    else()
        message(STATUS "Linux sandbox: Basic")
    endif()
endif()
if(DEFINED ENV{DOCKER_BUILD})
    message(STATUS "Docker build: $ENV{DOCKER_BUILD}")
else()
    message(STATUS "Docker build: No")
endif()
message(STATUS "Total source files: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "===================================")