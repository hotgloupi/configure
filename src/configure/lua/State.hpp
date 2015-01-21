#pragma once

#include "fwd.hpp"
#include "Converter.hpp"
#include "Signature.hpp"
#include "Caller.hpp"
#include "is_trivially_destructible.hpp"
#include "store_exception.hpp"

#include <configure/error.hpp>

#include <boost/filesystem/path.hpp>

namespace configure { namespace lua {

	class State
	{
	private:
		lua_State* _state;
		bool       _owner;
		int        _error_handler;
		//int        _error_handler_ref;

	public:
		inline lua_State* ptr() const { return _state; }

	public:
		explicit State(bool with_libs = true)
			: _state(luaL_newstate())
			, _owner(true)
			, _error_handler(0)
		{
			if (with_libs)
			{
				luaL_openlibs(_state);
				lua_register(_state, "print", &_print_override);
				_register_extensions();
			}
		}

		//explicit State(lua_State* other)
		//	: _state(other)
		//	, _owner(false)
		//	, _error_handler(0)
		//{}

		~State();

	private:
		void _register_extensions();

	public:
		// Load and run a lua file.
		void load(boost::filesystem::path const& p);

		// Load and run lua code.
		void load(std::string const& buffer);
		void load(char const* buffer);

	public:
		// Convert a value on the stack.
		template<typename T>
		typename Converter<T>::extract_type to(int index = -1)
		{ return Converter<T>::extract(_state, index); }

		// Push an arbitrary value on the stack.
		template<typename T>
		void push(T&& value)
		{ Converter<T>::push(_state, value); }

		// Push a callable on the stack.
		template<typename Fn>
		void push_callable(Fn&& fn)
		{
			this->construct<Fn>(std::forward<Fn>(fn));
			lua_pushcclosure(_state, &_call_fn<Fn>, 1);
		}

		// Push a type metatable on the stack
		// If the metatable is created, the destructor is registered and the
		// function return true.
		template<typename T>
		bool push_metatable()
		{ return Converter<T>::push_metatable(_state); }

		template<typename T>
		bool push_metatable(std::string const& name)
		{ return Converter<T>::push_metatable(_state, name); }

		// Construct a new type on the stack as a userdata
		template<typename T, typename... Args>
		T& construct(Args&&... args)
		{ return Converter<T>::push(_state, std::forward<Args>(args)...); }

		// Return a global as type T
		template<typename T> T global(std::string const& name)
		{
			getglobal(name.c_str());
			T res(to<T>());
			pop();
			return std::move(res);
		}

		// Set a global of type T
		template<typename T> void global(std::string const& name, T&& value)
		{ push(std::forward<T>(value)); setglobal(name.c_str()); }


	// Lua API forwards
	public:
		int gettop() { return lua_gettop(_state); }
		void getglobal(char const* name) { lua_getglobal(_state, name); }
		void setglobal(char const* name) { lua_setglobal(_state, name); }
		void call(int nargs, int nresults = -1)
		{
			if (nresults == -1)
				nresults = LUA_MULTRET;
			this->check_status(_state, lua_pcall(_state, nargs, nresults, _error_handler));
		}
		void pop(int count = 1) { lua_pop(_state, count); }
		void settable(int index) { lua_settable(_state, index); }
		void pushvalue(int index) { lua_pushvalue(_state, index); }

	private:
		static int _print_override(lua_State* L);

		template<typename Fn>
		static int _call_fn(lua_State* state)
		{
			typedef typename Signature<Fn>::type signature_type;
			void* fn_ptr = lua_touserdata(state, lua_upvalueindex(1));
			if (fn_ptr != nullptr)
			{
				Fn* fn = reinterpret_cast<Fn*>(fn_ptr);
				return store_exception<int>(
					state,
					[state, fn] () {
						return Caller<signature_type>::call(state, *fn);
					}
				);
			}
			return 0;
		}

	public:
		static void check_status(lua_State* L, int status);
	};

}}
