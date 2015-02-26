#pragma once

#include <configure/fwd.hpp>
#include <configure/lua/fwd.hpp>

namespace configure { namespace utils {

	int env_get(lua_State* state, Environ& self, int index = 2);

	int env_set(lua_State* state, Environ& self, int index = 2);

	int env_set_default(lua_State* state, Environ& self, int index = 2);

}}
