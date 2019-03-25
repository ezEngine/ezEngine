// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT

