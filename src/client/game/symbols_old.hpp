#pragma once

#define WEAK __declspec(selectany)

#define REBASE(address) (uintptr_t)((address - 0x140000000) + game::base)
#define OFFSET(address) (uintptr_t)((address - 0x140000000) + (uintptr_t)GetModuleHandle(NULL))

namespace game
{
	WEAK symbol<bool()> Com_IsRunningUILevel{ OFFSET(0x142148350) };

	static lua::lua_State* UI_luaVM = ((lua::lua_State*)((*(INT64*)OFFSET(0x159C76D88))));
}
