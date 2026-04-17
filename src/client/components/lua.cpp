#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/dvars.hpp"
#if defined(GAME_VERSION_FEB2026)
	#include "havok/hks_api.hpp"
#elif defined(GAME_VERSION_OLD)
	#include "havok/hks_api_old.hpp"
#endif
#include "havok/lua_api.hpp"

namespace lua
{
	utils::hook::detour Lua_CoD_LuaStateManager_Interface_ErrorPrint_hook;

	class component final : public component_interface
	{
	public:
		void lua_start() override
		{
			const lua::luaL_Reg UIErrorHashLibrary[] =
			{
				{nullptr, nullptr},
			};
			hks::hksI_openlib(game::UI_luaVM, "UIErrorHash", UIErrorHashLibrary, 0, 1);
		}

		void start_hooks() override
		{
		}

		void destroy_hooks() override
		{
		}
	};
}

//REGISTER_COMPONENT(lua::component)