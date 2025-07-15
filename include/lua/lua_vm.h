// Create include/lua/lua_vm.h

#ifndef LUA_VM_H
#define LUA_VM_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Lua VM state
typedef struct {
    lua_State* L;
    int initialized;
} LuaVM;

// Lua VM functions
int lua_vm_init(void);
void lua_vm_cleanup(void);
int lua_vm_execute_file(const char* filename);
int lua_vm_execute_string(const char* code);
int lua_vm_load_script(const char* vm_path);

// Lua API functions exposed to scripts
int lua_vm_print(lua_State* L);
int lua_vm_vfs_read(lua_State* L);
int lua_vm_vfs_write(lua_State* L);
int lua_vm_vfs_list(lua_State* L);
int lua_vm_system_info(lua_State* L);
int lua_vm_safe_getenv(lua_State* L);
int lua_vm_safe_execute(lua_State* L);
int lua_vm_safe_file_open(lua_State* L);

// Get Lua state for advanced operations
lua_State* lua_vm_get_state(void);

#endif // LUA_VM_H