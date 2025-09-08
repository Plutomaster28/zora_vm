// Create src/lua/lua_vm.c

#include "lua_vm.h"
#include "vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LuaVM lua_vm = {0};

// Lua API: print function
int lua_vm_print(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        if (i > 1) printf("\t");
        printf("%s", lua_tostring(L, i));
    }
    printf("\n");
    return 0;
}

// Lua API: VFS read function
int lua_vm_vfs_read(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    
    // Only allow reading from persistent storage
    if (strstr(path, "/persistent/") != path) {
        printf("Access denied: %s (outside persistent storage)\n", path);
        lua_pushnil(L);
        return 1;
    }
    
    // Use VFS to read file
    VNode* node = vfs_find_node(path);
    if (node && !node->is_directory) {
        // Load file content if not already loaded
        if (!node->data && node->host_path) {
            // Only load if it's within the ZoraPerl directory
            if (strstr(node->host_path, "../ZoraPerl/") != node->host_path) {
                printf("Access denied: Host path outside ZoraPerl\n");
                lua_pushnil(L);
                return 1;
            }
            
            FILE* f = fopen(node->host_path, "r");
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                
                // Limit file size to prevent memory exhaustion
                if (size > 1024 * 1024) {  // 1MB limit
                    printf("File too large: %s\n", path);
                    fclose(f);
                    lua_pushnil(L);
                    return 1;
                }
                
                node->data = malloc(size + 1);
                if (node->data) {
                    fread(node->data, 1, size, f);
                    ((char*)node->data)[size] = '\0';
                    node->size = size;
                }
                fclose(f);
            }
        }
        
        if (node->data) {
            lua_pushstring(L, (char*)node->data);
            return 1;
        }
    }
    
    lua_pushnil(L);
    return 1;
}

// Lua API: VFS write function
int lua_vm_vfs_write(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* content = luaL_checkstring(L, 2);
    
    // Only allow writing to persistent storage
    if (strstr(path, "/persistent/") != path) {
        printf("Write access denied: %s (outside persistent storage)\n", path);
        lua_pushboolean(L, 0);
        return 1;
    }
    
    // Limit content size
    if (strlen(content) > 1024 * 1024) {  // 1MB limit
        printf("Content too large for: %s\n", path);
        lua_pushboolean(L, 0);
        return 1;
    }
    
    // Use VFS to write (this keeps it contained)
    int result = vfs_create_file(path);
    if (result == 0) {
        VNode* node = vfs_find_node(path);
        if (node) {
            if (node->data) {
                free(node->data);
            }
            
            node->data = malloc(strlen(content) + 1);
            if (node->data) {
                strcpy((char*)node->data, content);
                node->size = strlen(content);
                lua_pushboolean(L, 1);
                return 1;
            }
        }
    }
    
    lua_pushboolean(L, 0);
    return 1;
}

// Lua API: VFS list directory
int lua_vm_vfs_list(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    
    VNode* dir = vfs_find_node(path);
    if (dir && dir->is_directory) {
        lua_newtable(L);
        int index = 1;
        
        VNode* child = dir->children;
        while (child) {
            lua_pushnumber(L, index++);
            lua_pushstring(L, child->name);
            lua_settable(L, -3);
            child = child->next;
        }
        
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// Lua API: System info
int lua_vm_vfs_system_info(lua_State* L) {
    lua_newtable(L);
    
    lua_pushstring(L, "vm_name");
    lua_pushstring(L, "Zora VM");
    lua_settable(L, -3);
    
    lua_pushstring(L, "version");
    lua_pushstring(L, "1.0");
    lua_settable(L, -3);
    
    lua_pushstring(L, "lua_version");
    lua_pushstring(L, LUA_VERSION);
    lua_settable(L, -3);
    
    return 1;
}

// Safe getenv that only allows specific variables
int lua_vm_safe_getenv(lua_State* L) {
    const char* var = luaL_checkstring(L, 1);
    
    // Only allow safe environment variables
    if (strcmp(var, "PATH") == 0 || 
        strcmp(var, "USER") == 0 || 
        strcmp(var, "HOME") == 0 ||
        strcmp(var, "USERPROFILE") == 0) {
        
        // Return fake/safe values instead of real ones
        if (strcmp(var, "HOME") == 0 || strcmp(var, "USERPROFILE") == 0) {
            lua_pushstring(L, "/vm/home");  // Fake home directory
        } else if (strcmp(var, "USER") == 0) {
            lua_pushstring(L, "vmuser");    // Fake username
        } else {
            lua_pushstring(L, "/vm/safe");  // Safe default
        }
    } else {
        lua_pushnil(L);
    }
    
    return 1;
}

// Safe command execution - only allow whitelisted commands
int lua_vm_safe_execute(lua_State* L) {
    const char* cmd = luaL_checkstring(L, 1);
    
    // Whitelist of allowed commands (all internal to VM)
    const char* allowed_commands[] = {
        "vm_status",
        "vm_info",
        "help",
        NULL
    };
    
    // Check if command is in whitelist
    for (int i = 0; allowed_commands[i]; i++) {
        if (strstr(cmd, allowed_commands[i]) == cmd) {
            // Execute only VM-internal commands
            printf("Executing safe command: %s\n", cmd);
            lua_pushboolean(L, 1);  // Pretend it succeeded
            return 1;
        }
    }
    
    printf("Command blocked by security policy: %s\n", cmd);
    lua_pushboolean(L, 0);  // Command blocked
    return 1;
}

// Safe file operations - only work within VFS
int lua_vm_safe_file_open(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* mode = luaL_optstring(L, 2, "r");
    
    // Only allow access to VFS paths
    if (strstr(path, "/persistent/") != path) {
        printf("File access denied: %s (outside VFS)\n", path);
        lua_pushnil(L);
        return 1;
    }
    
    // Use VFS functions instead of direct file access
    if (strcmp(mode, "r") == 0) {
        VNode* node = vfs_find_node(path);
        if (node && node->data) {
            lua_pushstring(L, (char*)node->data);
        } else {
            lua_pushnil(L);
        }
    } else {
        printf("Write mode not supported in safe mode\n");
        lua_pushnil(L);
    }
    
    return 1;
}

// Initialize Lua VM
int lua_vm_init(void) {
    if (lua_vm.initialized) {
        return 0;
    }
    
    lua_vm.L = luaL_newstate();
    if (!lua_vm.L) {
        return -1;
    }
    
    // Load standard libraries
    luaL_openlibs(lua_vm.L);
    
    // **REMOVE DANGEROUS FUNCTIONS**
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "os");           // Remove entire os module
    
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "io");           // Remove entire io module
    
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "package");      // Remove package loading
    
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "require");      // Remove require function
    
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "dofile");       // Remove dofile
    
    lua_pushnil(lua_vm.L);
    lua_setglobal(lua_vm.L, "loadfile");     // Remove loadfile
    
    // Create a safe, limited os table
    lua_newtable(lua_vm.L);
    lua_pushcfunction(lua_vm.L, lua_vm_safe_getenv);
    lua_setfield(lua_vm.L, -2, "getenv");
    lua_setglobal(lua_vm.L, "os");
    
    // Register safe custom functions
    lua_register(lua_vm.L, "vm_print", lua_vm_print);
    lua_register(lua_vm.L, "vfs_read", lua_vm_vfs_read);
    lua_register(lua_vm.L, "vfs_write", lua_vm_vfs_write);
    lua_register(lua_vm.L, "vfs_list", lua_vm_vfs_list);
    lua_register(lua_vm.L, "system_info", lua_vm_vfs_system_info);
    lua_register(lua_vm.L, "vm_execute", lua_vm_safe_execute);  // Safe command execution
    
    lua_vm.initialized = 1;
#if ZORA_VERBOSE_BOOT
    printf("Lua VM initialized successfully with security restrictions\n");
#endif
    return 0;
}

// Cleanup Lua VM
void lua_vm_cleanup(void) {
    if (lua_vm.initialized && lua_vm.L) {
        lua_close(lua_vm.L);
        lua_vm.L = NULL;
        lua_vm.initialized = 0;
    }
}

// Execute Lua file
int lua_vm_execute_file(const char* filename) {
    if (!lua_vm.initialized) {
        return -1;
    }
    
    if (luaL_dofile(lua_vm.L, filename) != LUA_OK) {
        printf("Lua error: %s\n", lua_tostring(lua_vm.L, -1));
        return -1;
    }
    
    return 0;
}

// Execute Lua string
int lua_vm_execute_string(const char* code) {
    if (!lua_vm.initialized) {
        return -1;
    }
    
    if (luaL_dostring(lua_vm.L, code) != LUA_OK) {
        printf("Lua error: %s\n", lua_tostring(lua_vm.L, -1));
        return -1;
    }
    
    return 0;
}

// Load and execute script from VFS
int lua_vm_load_script(const char* vm_path) {
    if (!lua_vm.initialized) {
        return -1;
    }
    
    VNode* node = vfs_find_node(vm_path);
    if (!node || node->is_directory) {
        printf("Script not found: %s\n", vm_path);
        return -1;
    }
    
    // Load file content if needed
    if (!node->data && node->host_path) {
        FILE* f = fopen(node->host_path, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            node->data = malloc(size + 1);
            if (node->data) {
                fread(node->data, 1, size, f);
                ((char*)node->data)[size] = '\0';
                node->size = size;
            }
            fclose(f);
        }
    }
    
    if (node->data) {
        return lua_vm_execute_string((char*)node->data);
    }
    
    return -1;
}

// Get Lua state
lua_State* lua_vm_get_state(void) {
    return lua_vm.L;
}