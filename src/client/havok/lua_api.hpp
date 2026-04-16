#pragma once
#include "game/game.hpp"
#define WEAK __declspec(selectany)

#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define OFFSET(address) (uintptr_t)((address - 0x140000000) + (uintptr_t)GetModuleHandle(NULL))
namespace lua
{
	WEAK game::symbol<const char* (lua_State* s, const char* str)> lua_pushstring{ OFFSET(0x140A186B0) }; //updated

	//guessing this is not needed since we never use it
	//WEAK game::symbol<__int64 (const char* key, const char* value, lua_State* luaVM)> Lua_SetTableString{ (uintptr_t)GetModuleHandle(NULL) + 0x32534688 }; 

	void luaL_register(lua_State* s, const char* libname, const luaL_Reg* l);
	void lua_pop(lua_State* s, int n);
	HksNumber lua_tonumber(lua_State* s, int index);
	const char* lua_tostring(lua_State* s, int index);
	void lua_pushnumber(lua_State* s, HksNumber n);
	void lua_pushinteger(lua_State* s, int n);
	void lua_pushnil(lua_State* s);
	void lua_pushboolean(lua_State* s, int b);
	void lua_pushvalue(lua_State* s, int index);
	void lua_getfield(lua_State* s, int index, const char* k);
	void lua_getglobal(lua_State* s, const char* k);
}


