#include <configure/bind.hpp>

#include <configure/Process.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>
#include <configure/error.hpp>
#include <configure/Node.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int Process_check_output(lua_State* state)
	{
		if (!lua_istable(state, 2))
			CONFIGURE_THROW(
				error::LuaError(
					"Expected a table, got '" + std::string(luaL_tolstring(state, 2, nullptr)) + "'"
				)
			);
		Process::Command command;
		for (int i = 1, len = lua_rawlen(state, 2); i <= len; ++i)
		{
			lua_rawgeti(state, 2, i);
			if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, -1))
				command.push_back((*n)->path().string());
			else if (fs::path* ptr = lua::Converter<fs::path>::extract_ptr(state, -1))
				command.push_back(ptr->string());
			else if (lua_isstring(state, -1))
				command.push_back(lua_tostring(state, -1));
			else
				CONFIGURE_THROW(
					error::LuaError("Expected node, path or string array, got '" + std::string(luaL_tolstring(state, -1, nullptr)) + "'")
				);
		}
		lua::Converter<std::string>::push(state, Process::check_output(command));
		return 1;
	}

	void bind_process(lua::State& state)
	{
		lua::Type<Process>(state, "Process")
			/// Retreive a command output
			// @function Process::check_output
			// @treturn string Command output
			.def("check_output", Process_check_output)
		;
	}

}
