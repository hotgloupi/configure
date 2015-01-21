#include <configure/bind.hpp>

#include <configure/ShellCommand.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int ShellCommand_new(lua_State* state)
	{
		int count = lua_gettop(state);
		ShellCommand& cmd = lua::Converter<ShellCommand>::push(state);
		for (int i = 2; i <= count; i++)
		{
			if (char const* arg = lua_tostring(state, i))
				cmd.append(std::string(arg));
			else if (NodePtr* node = lua::Converter<NodePtr>::extract_ptr(state, i))
				cmd.append(*node);
			else if (fs::path* path = lua::Converter<fs::path>::extract_ptr(state, i))
				cmd.append(*path);
			else
				throw std::runtime_error(
				    "ShellCommand:new args must contain only string, path or Node values (got " +
				    std::string(luaL_tolstring(state, i, nullptr)) + " at index " + std::to_string(i - 1) + ")"
				);
		}
		return 1;
	}

	static int ShellCommand_tostring(lua_State* state)
	{
		ShellCommand& cmd = lua::Converter<ShellCommand>::extract(state, 1);
		std::string res = "<ShellCommand";
		for (auto& arg: cmd.dump())
		{
			res.push_back(' ');
			res.append(arg);
		}
		res.push_back('>');
		lua_pushstring(state, res.c_str());
		return 1;
	}

	void bind_shell_command(lua::State& state)
	{
		/// @classmod ShellCommand
		lua::Type<ShellCommand>(state, "ShellCommand")
			.def("new", &ShellCommand_new)
			.def("__tostring", &ShellCommand_tostring)
		;
	}

}

