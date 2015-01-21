#pragma once

#include "fwd.hpp"
#include "traceback.hpp"
#include "Converter.hpp"

#include <configure/error.hpp>

namespace configure { namespace lua {


	inline std::string empty_function_name() { return ""; }

	template<typename Ret, typename Fn, typename FnName = std::string(*)()>
	Ret store_exception(lua_State* state, Fn&& fn, FnName function_name = &empty_function_name)
	{
		try {
			try { return fn(); }
			catch (error::LuaError& e) {
				if (boost::get_error_info<error::lua_function>(e) == nullptr)
				{
					std::string name = function_name();
					if (!name.empty())
						e << error::lua_function(std::move(name));
				}
				if (boost::get_error_info<error::lua_traceback>(e) == nullptr)
					e << error::lua_traceback(traceback(state));
				throw;
			}
			catch (...) {
				std::string name = function_name();
				if (!name.empty())
					CONFIGURE_THROW(
						error::LuaError("While Calling C++")
							<< error::lua_function(std::move(name))
							<< error::lua_traceback(traceback(state))
							<< error::nested(std::current_exception())
					);
				else
					CONFIGURE_THROW(
						error::LuaError("While Calling C++")
							<< error::lua_traceback(traceback(state))
							<< error::nested(std::current_exception())
					);
			}
		}
		catch (...) {
			Converter<std::exception_ptr>::push(state, std::current_exception());
			lua_error(state);
		}
		std::abort();
	}

}}
