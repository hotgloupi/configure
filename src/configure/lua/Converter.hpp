#pragma once

#include "fwd.hpp"
#include "is_trivially_destructible.hpp"
#include <configure/log.hpp>

#include <string>
#include <typeinfo>
#include <stdexcept>

namespace configure { namespace lua {

	template<typename T>
	struct Converter
	{
		typedef T& extract_type;

		static std::string const& type_name()
		{
			static std::string name = std::string("typeid-") + typeid(T).name();
			return name;
		}

		static extract_type extract(lua_State* state, int index)
		{

			T* value = extract_ptr(state, index);
			if (value == nullptr)
			{
				std::string err = "Expected value of type '"
					+ type_name()
					+ "', got '" + luaL_tolstring(state, index, nullptr) + "'";
				throw std::runtime_error(err);
			}
			return *value;
		}

		static T* extract_ptr(lua_State* state, int index)
		{
			void* mem = luaL_testudata(state, index, type_name().c_str());
			return ((T*) mem);
		}

		static bool push_metatable(lua_State* state)
		{
			std::string name = type_name();
			if (!luaL_newmetatable(state, name.c_str()))
				return false;
			if (!is_trivially_destructible<T>::value)
			{
				lua_pushstring(state, "__gc");
				lua_pushcfunction(state, &_call_destructor);
				lua_settable(state, -3);
			}
			return true;
		}
		static bool push_metatable(lua_State* state, std::string const& friendly_name)
		{
			bool is_new = push_metatable(state);
			if (is_new && !friendly_name.empty())
			{
				lua_pushvalue(state, -1);
				lua_setglobal(state, friendly_name.c_str());
			}
			return is_new;
		}

		static int _call_destructor(lua_State* L)
		{
			if (T* object = reinterpret_cast<T*>(lua_touserdata(L, 1)))
				object->~T();
			return 0;
		}

		template<typename... Args>
		static T& push(lua_State* state, Args&&... args)
		{
			void* mem = lua_newuserdata(state, sizeof(T));
			T* data = new (mem) T(std::forward<Args>(args)...);
			push_metatable(state);
			lua_setmetatable(state, -2);
			return *data;
		}
	};

	template<typename T> struct Converter<T*> : Converter<T> {};
	template<typename T> struct Converter<T&> : Converter<T> {};
	template<typename T> struct Converter<T const> : Converter<T> {};

	template<>
	struct Converter<std::string>
	{
		typedef std::string extract_type;
		static extract_type extract(lua_State* state, int index)
		{ return lua_tostring(state, index); }

		static void push(lua_State* state, std::string const& value)
		{ lua_pushstring(state, value.c_str()); }
	};

	template<>
	struct Converter<char const*> : Converter<std::string>
	{
		static void push(lua_State* state, char const* value)
		{ lua_pushstring(state, value); }
	};

	template<unsigned size>
	struct Converter<const char[size]> : Converter<char const*> {};

	template<>
	struct Converter<int>
	{
		typedef int extract_type;
		static extract_type extract(lua_State* state, int index)
		{ return lua_tointeger(state, index); }

		static void push(lua_State* state, int value)
		{ lua_pushinteger(state, value); }
	};

	template<>
	struct Converter<double>
	{
		typedef double extract_type;
		static extract_type extract(lua_State* state, int index)
		{ return lua_tonumber(state, index); }

		static void push(lua_State* state, double value)
		{ lua_pushnumber(state, value); }
	};

	template<>
	struct Converter<bool>
	{
		typedef bool extract_type;
		static extract_type extract(lua_State* state, int index)
		{ return lua_toboolean(state, index); }

		static void push(lua_State* state, bool value)
		{ lua_pushboolean(state, value); }
	};

}}
