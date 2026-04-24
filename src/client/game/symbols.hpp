#pragma once

#if defined(GAME_VERSION_FEB2026)
	#define WEAK __declspec(selectany)

	namespace game
	{
		#define REBASE(address) (uintptr_t)((address - 0x140000000) + game::base)
		#define OFFSET(address) (uintptr_t)((address - 0x140000000) + (uintptr_t)GetModuleHandle(NULL))
		WEAK symbol<bool()> Com_IsRunningUILevel{ OFFSET(0x1420EFFB0) };

		static lua::lua_State* UI_luaVM = ((lua::lua_State*)((*(INT64*)OFFSET(0x159BF7E08))));
	}
#elif defined(GAME_VERSION_OLD)
	#define WEAK __declspec(selectany)

	namespace game
	{
		#define REBASE(address) (uintptr_t)((address - 0x140000000) + game::base)
		#define OFFSET(address) (uintptr_t)((address - 0x140000000) + (uintptr_t)GetModuleHandle(NULL))
		WEAK symbol<bool()> Com_IsRunningUILevel{ OFFSET(0x142148350) };

		static lua::lua_State* UI_luaVM = ((lua::lua_State*)((*(INT64*)OFFSET(0x159C76D88))));
	}
#endif
