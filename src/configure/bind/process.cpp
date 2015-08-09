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
					error::LuaError(
						"Expected node, path or string, got '" +
						std::string(luaL_tolstring(state, -1, nullptr)) + "'"
					)
				);
		}
		Process::Options options;
		options.stdout_ = Process::Stream::PIPE;
		options.stderr_ = Process::Stream::DEVNULL;
		bool ignore_errors = false;
		if (lua_istable(state, 3))
		{
			lua_pushnil(state);
			while (lua_next(state, 3))
			{
				std::string key = lua::Converter<std::string>::extract(state, -2);
				if (key == "stdin")
					options.stdin_ = static_cast<Process::Stream>(lua::Converter<int>::extract(state, -1));
				else if (key == "stdout")
					options.stdout_ = static_cast<Process::Stream>(lua::Converter<int>::extract(state, -1));
				else if (key == "stderr")
					options.stderr_ = static_cast<Process::Stream>(lua::Converter<int>::extract(state, -1));
				else if (key == "ignore_errors")
					ignore_errors = lua::Converter<bool>::extract(state, -1);
				else
					CONFIGURE_THROW(
					    error::InvalidArgument("Unknown argument '" + key + "'")
					);
				lua_pop(state, 1);
			}
			lua_pop(state, 1);
		}
		lua::Converter<std::string>::push(
			state,
			Process::check_output(command, options, ignore_errors)
		);
		return 1;
	}

	void bind_process(lua::State& state)
	{
		/// Process instance.
		// @classmod Process
		lua::Type<Process> type(state, "Process");
		type
			/// Retreive a command output
			// @function Process::check_output
			// @treturn string Command output
			.def("check_output", Process_check_output)
		;

#define ENUM_VALUE(T, key) \
		lua_pushunsigned(\
			state.ptr(), \
			static_cast<unsigned>(Process::T::key)\
		); \
		lua_setfield(state.ptr(), -2, #key); \
/**/
		lua_newtable(state.ptr());
		ENUM_VALUE(Stream, STDIN);
		ENUM_VALUE(Stream, STDOUT);
		ENUM_VALUE(Stream, STDERR);
		ENUM_VALUE(Stream, PIPE);
		ENUM_VALUE(Stream, DEVNULL);
		lua_setfield(state.ptr(), -2, "Stream");
	}

}
