#include "game/game.hpp"
#define WEAK __declspec(selectany)

#define OFFSET(address) (uintptr_t)((address - 0x140000000) + (uintptr_t)GetModuleHandle(NULL))

namespace hks
{
	WEAK game::symbol<void (lua::lua_State* s, const char* libname, const lua::luaL_Reg l[], int nup, int isHksFunc)> hksI_openlib { OFFSET(0x141D3D070) };
	WEAK game::symbol<int (lua::lua_State* s, lua::HksCompilerSettings* options, char const* buff, size_t sz, char const* name)> hksi_hksL_loadbuffer{ OFFSET(0x141D3F9B0) };
	WEAK game::symbol<const char* (lua::lua_State* s, lua::HksObject* obj, size_t* len)> hks_obj_tolstring{ OFFSET(0x141D3F2F0 ) };
	WEAK game::symbol<int(lua::lua_State* s, int nargs, int nresults, int errfunc)> hksi_lua_pcall{ OFFSET(0x141D41FC0) };

	static const char** TypeName = ((const char**)((*(INT64*)(((uintptr_t)GetModuleHandle(NULL) + 0x32FC4C8)))));

	int hks_obj_type(const lua::HksObject* obj);
	int hksi_lua_type(lua::lua_State* s, int index);
	int hks_obj_isstring(const lua::HksObject* x);
	int hksi_lua_isstring(lua::lua_State* s, int index);
	int hksi_lua_gettop(lua::lua_State* s);
	int execute_raw_lua(std::string source, const char* chunkName);
}