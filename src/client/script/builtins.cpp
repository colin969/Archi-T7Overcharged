#include <std_include.hpp>

#include "loader/component_loader.hpp"
#include "game/game.hpp"
#include "utils/string.hpp"

namespace builtins
{
    static std::unordered_map<int, void(__fastcall*)(game::scriptInstance_t)> custom_functions;

    void register_function(const char* name, void(*funcPtr)(game::scriptInstance_t inst))
    {
        custom_functions[fnv1a(name)] = funcPtr;
    }

    class component final : public component_interface
    {
    public:
        void post_start() override
        {

        }
    };
}

REGISTER_COMPONENT(builtins::component)