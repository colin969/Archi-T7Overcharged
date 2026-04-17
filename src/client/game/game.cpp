#include <std_include.hpp>
#include "game.hpp"
#include "dvars.hpp"
#if defined(GAME_VERSION_FEB2026)
	#include "havok/hks_api.hpp"
#elif defined(GAME_VERSION_OLD)
	#include "havok/hks_api_old.hpp"
#endif
#include "havok/lua_api.hpp"
#include "utils/string.hpp"
#include "utils/io.hpp"
#include "utils/hook.hpp"
#include "utils/thread.hpp"

#include <string>
#include <map>

namespace game
{
	uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
	MinLog minlog = MinLog();

}
