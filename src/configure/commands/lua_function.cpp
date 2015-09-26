#include "lua_function.hpp"

#include <configure/lua/State.hpp>
#include <configure/bind.hpp>

namespace configure { namespace commands {

	void lua_function(boost::filesystem::path const& script,
	                  std::string const& function,
	                  std::vector<std::string> const& args)
	{
		lua::State lua;
		bind(lua);
		lua.load(script);
		lua.getglobal(function.c_str());
		for (auto& arg : args) lua.push(arg);
		lua.call(args.size());
	}

}}
