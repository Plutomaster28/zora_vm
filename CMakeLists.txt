cmake_minimum_required(VERSION 3.20)
project(zora_vm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build configuration options
option(ZORA_VERBOSE_BOOT "Enable verbose boot messages and debug output" OFF)
option(ZORA_RELEASE_MODE "Enable clean release mode startup (minimal messages)" ON)
option(ENABLE_LUA_SCRIPTING "Enable Lua scripting support" ON)
option(ENABLE_PYTHON_SCRIPTING "Enable Python scripting support" ON)
option(ENABLE_PERL_SCRIPTING "Enable Perl scripting support" ON)

# Set preprocessor definitions based on build options
if(ZORA_VERBOSE_BOOT)
    add_definitions(-DZORA_VERBOSE_BOOT=1)
    message(STATUS "Verbose boot mode enabled - full debug output")
else()
    add_definitions(-DZORA_VERBOSE_BOOT=0)
endif()

if(ZORA_RELEASE_MODE)
    add_definitions(-DZORA_RELEASE_MODE=1)
    message(STATUS "Release mode enabled - clean startup experience")
else()
    add_definitions(-DZORA_RELEASE_MODE=0)
endif()

# Scripting engine definitions
if(ENABLE_LUA_SCRIPTING)
    add_definitions(-DLUA_SCRIPTING=1)
    message(STATUS "Lua scripting enabled")
else()
    message(STATUS "Lua scripting disabled")
endif()

if(ENABLE_PYTHON_SCRIPTING)
    add_definitions(-DPYTHON_SCRIPTING=1)
    message(STATUS "Python scripting enabled")
else()
    message(STATUS "Python scripting disabled")
endif()

if(ENABLE_PERL_SCRIPTING)
    add_definitions(-DPERL_SCRIPTING=1)
    message(STATUS "Perl scripting enabled")
else()
    message(STATUS "Perl scripting disabled")
endif()

# Windows-only build (including MSYS2/MinGW environments)
if(NOT WIN32 AND NOT MSYS AND NOT MINGW)
    message(FATAL_ERROR "This project only supports Windows")
endif()

add_definitions(-D_WIN32_WINNT=0x0601 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Enhanced static linking for standalone executable
if(MINGW OR MSYS)
    # Comprehensive static linking flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
    
    # Prefer static libraries over dynamic ones
    set_property(GLOBAL PROPERTY LINK_SEARCH_START_STATIC ON)
    set_property(GLOBAL PROPERTY LINK_SEARCH_END_STATIC ON)
    
    message(STATUS "Configured for comprehensive static linking on Windows (MinGW/MSYS2)")
    message(STATUS "This will create a fully standalone executable with no external dependencies")
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

# Static linking preference for Lua on Windows (from backup)
if(WIN32 AND NOT MSVC)
    # Try to find static Lua library if available for better bundling
    find_library(LUA_STATIC_LIB 
        NAMES liblua.a lua54.a lua5.4.a liblua5.4.a
        HINTS 
            C:/msys64/ucrt64/lib
            /ucrt64/lib
            ${CMAKE_PREFIX_PATH}/lib
        PATH_SUFFIXES lib
    )
    
    if(LUA_STATIC_LIB)
        set(LUA_LIBRARIES ${LUA_STATIC_LIB})
        message(STATUS "Using static Lua library for bundling: ${LUA_STATIC_LIB}")
    else()
        message(STATUS "Static Lua library not found, using: ${LUA_LIBRARIES}")
        message(STATUS "Note: This may result in dynamic linking to Lua")
    endif()
endif()

# Include directories
include_directories(include include/platform include/binary include/merl include/vfs 
                   include/syscall include/virtualization include/network include/lua 
                   include/python include/perl include/meisei include/sandbox
                   include/terminal include/unix_core
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
    src/kernel/system_monitor.c
    src/kernel/init.c
    src/kernel/java_detector.c
    
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
    src/meisei/virtual_silicon.c
    src/meisei/script_accelerator.c
    src/terminal/terminal_style.c
    src/terminal/terminal_detector.c
    
    # Research UNIX Core Subsystem
    src/unix_core/unix_core.c
    src/unix_core/unix_directories.c
    src/unix_core/unix_compiler.c
    src/unix_core/unix_embedded_compiler.c
    src/unix_core/unix_ipc.c
    src/unix_core/unix_textproc.c
    src/unix_core/unix_games.c
    src/unix_core/unix_games_snake.c
    src/unix_core/unix_games_hangman.c
    
    # MERL shell
    MERL/color-and-test.c
    MERL/crash.c
    MERL/kernel.c
    MERL/shell.c
    # MERL/shell_script.c  # Temporarily disabled
    MERL/tetra.c
    MERL/user.c
    MERL/utils.c
    
    # Windows resources (icon and version info)
    resources.rc
)

# Conditionally add Python support
if(ENABLE_PYTHON_SCRIPTING)
    list(APPEND SOURCES src/python/python_vm.c)
    message(STATUS "Python VM source added to build")
endif()

# Conditionally add Perl support  
if(ENABLE_PERL_SCRIPTING)
    list(APPEND SOURCES src/perl/perl_vm.c)
    message(STATUS "Perl VM source added to build")
endif()

# Create executable
add_executable(zora_vm ${SOURCES})

# Enhanced static linking setup for Windows (from backup)
if(WIN32)
    if(NOT MSVC)
        # For MinGW/MSYS2: Enhanced static linking for truly standalone executable
        set_target_properties(zora_vm PROPERTIES 
            LINK_FLAGS "-static-libgcc -static-libstdc++ -static -Wl,--allow-multiple-definition"
        )
        
        # Force static library preference for this target
        set_property(TARGET zora_vm PROPERTY LINK_SEARCH_START_STATIC ON)
        set_property(TARGET zora_vm PROPERTY LINK_SEARCH_END_STATIC ON)
        
        message(STATUS "Target configured for maximum static linking")
    endif()
endif()

# Link libraries with static preference
if(WIN32)
    target_link_libraries(zora_vm 
        ${LUA_LIBRARIES}
        # Core Windows libraries (these are always static on Windows)
        kernel32 
        user32 
        ws2_32      # Windows sockets
        iphlpapi    # IP Helper API
        wininet     # Windows Internet API  
        psapi       # Process Status API
        advapi32    # Advanced Windows API (registry, security)
        winmm       # Windows multimedia (timers)
        dbghelp     # Debug help library
        # Try to link zlib statically if available
        z
    )
    
    # Try to find and link additional static libraries
    find_library(ZLIB_STATIC_LIB 
        NAMES libz.a libzlib.a zlibstatic.a
        HINTS 
            C:/msys64/ucrt64/lib
            /ucrt64/lib
            ${CMAKE_PREFIX_PATH}/lib
    )
    
    if(ZLIB_STATIC_LIB)
        target_link_libraries(zora_vm ${ZLIB_STATIC_LIB})
        message(STATUS "Linking static zlib: ${ZLIB_STATIC_LIB}")
    else()
        message(STATUS "Static zlib not found, using system zlib")
    endif()
else()
    # Non-Windows linking (though we don't support it currently)
    target_link_libraries(zora_vm ${LUA_LIBRARIES})
endif()

message(STATUS "Windows-only build configured successfully with icon and version info")

# === POST-BUILD ACTIONS (from backup) ===
if(WIN32)
    # Windows-specific post-build validation
    add_custom_command(TARGET zora_vm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Windows static build completed successfully"
        COMMAND ${CMAKE_COMMAND} -E echo "Executable location: $<TARGET_FILE:zora_vm>"
        COMMENT "Post-build validation"
    )
    
    # Add dependency check if objdump is available (shows if truly static)
    find_program(OBJDUMP_PROGRAM objdump)
    if(OBJDUMP_PROGRAM AND NOT MSVC)
        add_custom_command(TARGET zora_vm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Checking dynamic dependencies..."
            COMMAND ${OBJDUMP_PROGRAM} -p $<TARGET_FILE:zora_vm> | findstr "DLL Name" || echo "SUCCESS: No external DLL dependencies found (fully static build)"
            COMMENT "Static linking verification"
            VERBATIM
        )
    endif()
endif()

# === BUILD SUMMARY (from backup) ===
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
    message(STATUS "Distribution: Single executable file")
else()
    message(STATUS "Linking: Dynamic (MSVC or non-Windows)")
endif()

message(STATUS "Features: Enhanced terminal, MS Mincho font, Campbell colors")
message(STATUS "Terminal: Syntax highlighting, retro mode, block cursor")
message(STATUS "=== BUILD READY ===")
message(STATUS "")
