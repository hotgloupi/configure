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

	static int ShellCommand_working_directory(lua_State* state)
	{
		ShellCommand& cmd = lua::Converter<ShellCommand>::extract(state, 1);
		int count = lua_gettop(state);
		if (count == 2)
			cmd.working_directory(lua::Converter<boost::filesystem::path>::extract(state, 2));

		if (cmd.has_working_directory())
			lua::Converter<boost::filesystem::path>::push(
			  state, cmd.working_directory());
		else
			lua_pushnil(state);
		return 1;
	}

	static int ShellCommand_env(lua_State* state)
	{
		ShellCommand& cmd = lua::Converter<ShellCommand>::extract(state, 1);
		int count = lua_gettop(state);
		if (count == 2)
		{
			if (!lua_istable(state, -1))
				CONFIGURE_THROW(error::InvalidArgument("Expect a table argument"));
			ShellCommand::Environ env;
			std::string key, val;
			lua_pushnil(state);
			// -3  : -2  : -1
			// cmd : env : nil
			while (lua_next(state, -2))
			{
				// -4  : -3  : -2  :   -1
				// cmd : env : key : value
				if (auto ptr = luaL_tolstring(state, -2, nullptr))
					key = ptr;
				else
					CONFIGURE_THROW(error::RuntimeError("Cannot convert table key to string"));
				lua_pop(state, 1);
				if (auto ptr = luaL_tolstring(state, -1, nullptr))
					val = ptr;
				else
					CONFIGURE_THROW(error::RuntimeError("Cannot convert table value to string"));
				lua_pop(state, 1);
				env[key] = val;
				lua_pop(state, 1);
			}
			cmd.env(std::move(env));
		}
		else
			CONFIGURE_THROW(error::RuntimeError("not implemented"));
		return 0;
	}

	void bind_shell_command(lua::State& state)
	{
		/// @classmod ShellCommand
		lua::Type<ShellCommand>(state, "ShellCommand")
			.def("new", &ShellCommand_new)
			.def("__tostring", &ShellCommand_tostring)
			.def("working_directory", &ShellCommand_working_directory)
			.def("env", &ShellCommand_env)
		;
	}

}

