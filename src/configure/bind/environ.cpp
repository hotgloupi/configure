#include <configure/bind.hpp>

#include <configure/Environ.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int env_get(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		auto key = lua::Converter<std::string>::extract(state, 2);
		switch (self.kind(key))
		{
		case Environ::Kind::string:
			lua::Converter<std::string>::push(state, self.get<std::string>(key));
			break;
		case Environ::Kind::path:
			lua::Converter<fs::path>::push(state, self.get<fs::path>(key));
			break;
		case Environ::Kind::boolean:
			lua::Converter<bool>::push(state, self.get<bool>(key));
			break;
		case Environ::Kind::integer:
			lua::Converter<int>::push(state, self.get<int>(key));
			break;
		case Environ::Kind::none:
			lua_pushnil(state);
			break;
		}
		return 1;
	}

	static int env_set(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		auto key = lua::Converter<std::string>::extract(state, 2);
		if (auto ptr = lua::Converter<fs::path>::extract_ptr(state, 3))
			lua::Converter<fs::path>::push(
				state,
				self.set<fs::path>(key, *ptr)
			);
		else if (auto ptr = lua_tostring(state, 3))
			lua::Converter<std::string>::push(
				state,
				self.set<std::string>(key, ptr)
			);
		else if (lua_isboolean(state, 3))
			lua::Converter<bool>::push(
				state,
				self.set<bool>(key, lua_toboolean(state, 3))
			);
		else if (lua_isnumber(state, 3))
			lua::Converter<int>::push(
				state,
				self.set<int>(key, lua_tointeger(state, 3))
			);
		else
			throw std::runtime_error("Unknown type");
		return 1;
	}

	static int env_set_default(lua_State* state)
	{
		Environ& self = lua::Converter<std::reference_wrapper<Environ>>::extract(state, 1);
		auto key = lua::Converter<std::string>::extract(state, 2);
		if (auto ptr = lua::Converter<fs::path>::extract_ptr(state, 3))
			lua::Converter<fs::path>::push(
				state,
				self.set_default<fs::path>(key, *ptr)
			);
		else if (auto ptr = lua_tostring(state, 3))
			lua::Converter<std::string>::push(
				state,
				self.set_default<std::string>(key, ptr)
			);
		else if (lua_isboolean(state, 3))
			lua::Converter<bool>::push(
				state,
				self.set_default<bool>(key, lua_toboolean(state, 3))
			);
		else if (lua_isnumber(state, 3))
			lua::Converter<int>::push(
				state,
				self.set_default<int>(key, lua_tointeger(state, 3))
			);
		else
			throw std::runtime_error("Unknown type");
		return 1;
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
