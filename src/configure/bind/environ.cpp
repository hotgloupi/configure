#include "environ_utils.hpp"

#include <configure/bind.hpp>
#include <configure/Environ.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

namespace configure {

	static int env_get(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		return utils::env_get(state, self);
	}

	static int env_set(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		return utils::env_set(state, self);
	}

	static int env_set_default(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		return utils::env_set_default(state, self);
	}

	void bind_environ(lua::State& state)
	{
		/// Store the build environment.
		// @classmod Environ
		lua::Type<Environ, std::reference_wrapper<Environ>>(state)
			/// Retreive a value.
			/// @string key
			/// @treturn string|Path|integer|bool|nil
			/// @function Environ:get
			.def("get", &env_get)

			/// Set a key/value pair.
			/// @string key
			/// @tparam string|Path|integer|bool|nil value
			/// @treturn string|Path|integer|bool|nil value just set
			/// @function Environ:set
			.def("set", &env_set)

			/// Set a default value if not present
			/// @string key
			/// @tparam string|Path|integer|bool|nil default
			/// @treturn string|Path|integer|bool|nil default value or value already set
			/// @function Environ:set_default
			.def("set_default", &env_set_default)
		;
	}

}
