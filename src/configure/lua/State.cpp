#include "State.hpp"
#include "traceback.hpp"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <map>

namespace configure { namespace lua {

	State::~State()
	{
		if (_owner)
		{
			//if (_error_handler != 0)
			//	luaL_unref(_state, LUA_REGISTRYINDEX, _error_handler_ref);
			lua_close(_state);
		}
	}

	void State::check_status(lua_State* L, int status)
	{
		static std::map<int, std::string> error_strings{
			{LUA_ERRRUN, "Runtime error"},
			{LUA_ERRSYNTAX, "Syntax error"},
			{LUA_ERRMEM, "Memory error"},
			{LUA_ERRGCMM, "GC error"},
			{LUA_ERRERR, "Error handling error"},
		};
		switch (status)
		{
		case LUA_OK:
		case LUA_YIELD:
			return;
		case LUA_ERRRUN:
		case LUA_ERRSYNTAX:
		case LUA_ERRMEM:
		case LUA_ERRGCMM:
		case LUA_ERRERR:
			break;
		default:
			throw std::runtime_error("Unknown lua status: " + std::to_string(status));
		}
		if (auto e = Converter<std::exception_ptr>::extract_ptr(L, -1))
		{
			std::exception_ptr cpy = *e;
			lua_remove(L, -1);
			std::rethrow_exception(cpy);
		}
		std::string msg = error_strings[status] + ": ";
		if (char const* str = lua_tostring(L, -1))
		{
			msg.append(str);
			lua_remove(L, -1);
		}
		else
			msg.append("Unknown error");
		CONFIGURE_THROW(
			error::LuaError(msg) << error::lua_traceback(traceback(L))
		);
	}

	namespace {

		int string_starts_with(lua_State* state)
		{
			char const* s1 = lua_tostring(state, 1);
			char const* s2 = lua_tostring(state, 2);
			if (s1 == nullptr || s2 == nullptr)
			{
				lua_pushstring(state, "string:starts_with() expect one argument");
				lua_error(state);
			}
			if (boost::algorithm::starts_with(s1, s2))
				lua_pushboolean(state, 1);
			else
				lua_pushboolean(state, 0);
			return 1;
		}

		int string_ends_with(lua_State* state)
		{
			char const* s1 = lua_tostring(state, 1);
			char const* s2 = lua_tostring(state, 2);
			if (s1 == nullptr || s2 == nullptr)
			{
				lua_pushstring(state, "string:starts_with() expect one argument");
				lua_error(state);
			}
			if (boost::algorithm::ends_with(s1, s2))
				lua_pushboolean(state, 1);
			else
				lua_pushboolean(state, 0);
			return 1;
		}

		int table_append(lua_State* state)
		{
			// table : value
			int len = lua_rawlen(state, 1);
			lua_rawseti(state, 1, len + 1);
			// table
			return 1;
		}

		int table_extend(lua_State* state)
		{
			if ((lua_gettop(state) != 2) || !lua_istable(state, 1) || !lua_istable(state, 2))
			{
				lua_pushstring(state, "table.extend(): Two arguments of type table expected");
				lua_error(state);
			}
			// table1 : table2
			int len1 = lua_rawlen(state, 1);
			int len2 = lua_rawlen(state, 2);
			for (int i = 1; i <= len2; ++i)
			{
				lua_rawgeti(state, 2, i);
				// t1 : t2 : t2[i]
				lua_rawseti(state, 1, len1 + i);

			}
			lua_remove(state, -1);
			return 1;
		}

		int table_update(lua_State* state)
		{
			if ((lua_gettop(state) != 2) || !lua_istable(state, 1) || !lua_istable(state, 2))
			{
				lua_pushstring(state, "table.update(): Two arguments of type table expected");
				lua_error(state);
			}
			lua_pushnil(state);
			// -3 : -2 : -1
			// t1 : t2 : nil
			while (lua_next(state, -2)) // iterate through t2
			{
				// -4 : -3 : -2  : -1
				// t1 : t2 : key : value

				lua_pushvalue(state, -2);
				// -5 : -4 : -3  : -2    : -1
				// t1 : t2 : key : value : key

				lua_insert(state, -2);
				// -5 : -4 : -3  : -2  : -1
				// t1 : t2 : key : key : value

				// t1[key] = value
				lua_settable(state, -5);
				// -3 : -2  : -1
				// t1 : t2 : key
			}
			// t1 : t2
			lua_pop(state, 1);
			return 1;
		}

		int error_handler(lua_State* state)
		{
			if (char const* str = lua_tostring(state, -1))
			{
				Converter<std::exception_ptr>::push(
					state,
					std::make_exception_ptr(
						error::LuaError(str)
							<< error::lua_traceback(traceback(state))
					)
				);
			}
			else if (auto ptr = Converter<std::exception_ptr>::extract_ptr(state, -1))
			{
				try { std::rethrow_exception(*ptr); }
				catch (error::LuaError&) {
					/* Right type already */
				}
				catch (...) {
					Converter<std::exception_ptr>::push(
						state,
						std::make_exception_ptr(
							error::LuaError("Runtime error")
								<< error::lua_traceback(traceback(state))
								<< error::nested(*ptr)
						)
					);
				}
			}
			return 1;
		}

	}


	void State::_register_extensions()
	{
#define SET_METHOD(method, ptr) \
		lua_pushstring(_state, method); \
		lua_pushcfunction(_state, ptr); \
		lua_rawset(_state, -3); \
/**/
		lua_getglobal(_state, "string");
		SET_METHOD("starts_with", &string_starts_with);
		SET_METHOD("ends_with", &string_ends_with);

		lua_getglobal(_state, "table");
		SET_METHOD("append", &table_append);
		SET_METHOD("extend", &table_extend);
		SET_METHOD("update", &table_update);

		lua_settop(_state, 0);
		lua_pushcfunction(_state, &error_handler);
		_error_handler = lua_gettop(_state); // save error handler index

		//_error_handler_ref = luaL_ref(_state, LUA_REGISTRYINDEX); // add ref to avoid gc
		//lua_rawgeti(_state, LUA_REGISTRYINDEX, _error_handler_ref); // push it again on top
#undef SET_METHOD

	}

	void State::load(boost::filesystem::path const& p)
	{
		check_status(_state, luaL_loadfile(_state, p.string().c_str()));
		check_status(_state, lua_pcall(_state, 0, 0, _error_handler));
	}

	void State::load(std::string const& buffer)
	{ this->load(buffer.c_str()); }

	void State::load(char const* buffer)
	{
		check_status(_state, luaL_loadstring(_state, buffer));
		check_status(_state, lua_pcall(_state, 0, 0, _error_handler));
	}

	int State::_print_override(lua_State* L)
	{
		int count = lua_gettop(L);
		if (count >= 1)
			std::cout << luaL_tolstring(L, 1, nullptr);
		for (int i = 2; i <= count; ++i)
			std::cout << ' ' << luaL_tolstring(L, i, nullptr);
		std::cout << std::endl;
		return LUA_OK;
	}

}}
