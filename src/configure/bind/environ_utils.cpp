#include "environ_utils.hpp"

#include <configure/lua/State.hpp>
#include <configure/Environ.hpp>
#include <configure/Node.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure { namespace lua {

	template<>
	struct Converter<std::vector<Environ::Value>>
	{
		typedef std::vector<Environ::Value> extract_type;
		static extract_type extract(lua_State* state, int index)
		{
			extract_type res;
			int len = lua_rawlen(state, index);
			if (len <= 0)
				return res;
			res.reserve(len);
			for (int i = 1; i <= len; ++i)
			{
				lua_rawgeti(state, index, i);
				res.push_back(lua::Converter<Environ::Value>::extract(state, -1));
				lua_pop(state, 1);
			}
			return res;
		}

		static void push(lua_State* state, extract_type const& value)
		{
			lua_createtable(state, value.size(), 0);
			for (size_t i = 0; i < value.size(); ++i)
			{
				Converter<Environ::Value>::push(state, value[i]);
				lua_rawseti(state, -2, i);
			}
		}
	};

	void
	Converter<Environ::Value>::push(lua_State* state, extract_type const& value)
	{
		switch (value.which())
		{
			case 0:
				Converter<std::string>::push(
					state,
					boost::get<std::string>(value)
				);
				break;
			case 1:
				Converter<boost::filesystem::path>::push(
					state,
					boost::get<boost::filesystem::path>(value)
				);
				break;
			case 2:
				Converter<bool>::push(
					state,
					boost::get<bool>(value)
				);
				break;
			case 3:
				Converter<int64_t>::push(
					state,
					boost::get<int64_t>(value)
				);
				break;
			case 4:
				Converter<std::vector<Environ::Value>>::push(
					state,
					boost::get<std::vector<Environ::Value>>(value)
				);
				break;
			case 5:
				lua_pushnil(state);
				break;
			default: std::abort();
		}
	}

	Converter<Environ::Value>::extract_type
	Converter<Environ::Value>::extract(lua_State* state, int index)
	{
		typedef Environ::Value Value;
		if (auto ptr = lua::Converter<fs::path>::extract_ptr(state, index))
			return Value(fs::path(*ptr));
		else if (auto ptr = lua::Converter<NodePtr>::extract_ptr(state, index))
			return Value((*ptr)->path());
		else if (lua_isboolean(state, index))
			return Value(lua_toboolean(state, index) != 0);
		else if (lua_isnumber(state, index))
			return Value(static_cast<int64_t>(lua_tointeger(state, index)));
		else if (lua_isstring(state, index))
			return Value(std::string(lua_tostring(state, index)));
		else if (lua_istable(state, index))
			return lua::Converter<std::vector<Environ::Value>>::extract(state, index);
		else
			throw std::runtime_error(
			    "Cannot extract value " + std::string(luaL_tolstring(state, index, nullptr))
			);
	}

}}

namespace configure { namespace utils {

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
			lua::Converter<int64_t>::push(state, self.get<int64_t>(key));
			break;
		case Environ::Kind::list:
		{
			lua::Converter<std::vector<Environ::Value>>::push(
			    state,
			    self.get<std::vector<Environ::Value>>(key)
			);
			break;
		}
		case Environ::Kind::none:
			lua_pushnil(state);
			break;
		}
		return 1;
	}

	int env_set(lua_State* state, Environ& self, int index)
	{
		self.set<Environ::Value>(
			lua::Converter<std::string>::extract(state, index),
		    lua::Converter<Environ::Value>::extract(state, index + 1)
		);
		return 1; // returns last argument
	}

	int env_set_default(lua_State* state, Environ& self, int index)
	{
		auto& value = self.set_default<Environ::Value>(
			lua::Converter<std::string>::extract(state, index),
		    lua::Converter<Environ::Value>::extract(state, index + 1)
		);
		lua::Converter<Environ::Value>::push(state, value);
		return 1;
	}
}}
