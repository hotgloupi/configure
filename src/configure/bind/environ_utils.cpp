#include "environ_utils.hpp"

#include <configure/lua/State.hpp>
#include <configure/Environ.hpp>

#include <boost/filesystem/path.hpp>

namespace configure { namespace utils {

	namespace fs = boost::filesystem;

	int env_get(lua_State* state, Environ& self, int index)
	{
		auto key = lua::Converter<std::string>::extract(state, index);
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

	int env_set(lua_State* state, Environ& self, int index)
	{
		auto key = lua::Converter<std::string>::extract(state, index);
		if (auto ptr = lua::Converter<fs::path>::extract_ptr(state, index + 1))
			lua::Converter<fs::path>::push(
				state,
				self.set<fs::path>(key, *ptr)
			);
		else if (auto ptr = lua_tostring(state, index + 1))
			lua::Converter<std::string>::push(
				state,
				self.set<std::string>(key, ptr)
			);
		else if (lua_isboolean(state, index + 1))
			lua::Converter<bool>::push(
				state,
				self.set<bool>(key, lua_toboolean(state, index + 1))
			);
		else if (lua_isnumber(state, index + 1))
			lua::Converter<int>::push(
				state,
				self.set<int>(key, lua_tointeger(state, index + 1))
			);
		else
			throw std::runtime_error("Unknown type");
		return 1;
	}

	int env_set_default(lua_State* state, Environ& self, int index)
	{
		auto key = lua::Converter<std::string>::extract(state, index);
		int value_index = index + 1;
		if (auto ptr = lua::Converter<fs::path>::extract_ptr(state, value_index))
			lua::Converter<fs::path>::push(
				state,
				self.set_default<fs::path>(key, *ptr)
			);
		else if (auto ptr = lua_tostring(state, value_index))
			lua::Converter<std::string>::push(
				state,
				self.set_default<std::string>(key, ptr)
			);
		else if (lua_isboolean(state, value_index))
			lua::Converter<bool>::push(
				state,
				self.set_default<bool>(key, lua_toboolean(state, value_index))
			);
		else if (lua_isnumber(state, value_index))
			lua::Converter<int>::push(
				state,
				self.set_default<int>(key, lua_tointeger(state, value_index))
			);
		else
			throw std::runtime_error("Unknown type");
		return 1;
	}
}}
