#pragma once

#include <configure/fwd.hpp>
#include <configure/lua/fwd.hpp>
#include <configure/Environ.hpp>

namespace configure { namespace utils {

	int env_get(lua_State* state, Environ& self, int index = 2);

	int env_set(lua_State* state, Environ& self, int index = 2);

	int env_set_default(lua_State* state, Environ& self, int index = 2);

}}

namespace configure { namespace lua {

	template<> struct Converter<Environ::Value>
	{
		typedef Environ::Value extract_type;
		static extract_type extract(lua_State* state, int index);
		static void push(lua_State* state, extract_type const& value);
	};

}}
