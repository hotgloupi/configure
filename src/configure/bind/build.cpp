#include <configure/bind.hpp>
#include <configure/bind/path_utils.hpp>

#include <configure/Build.hpp>
#include <configure/Rule.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>
#include <configure/Filesystem.hpp>
#include <configure/Platform.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

namespace configure {

	template<typename T>
	static int Build_option(lua_State* state)
	{
		Build& self = lua::Converter<std::reference_wrapper<Build>>::extract(state, 1);
		auto name = lua::Converter<std::string>::extract(state, 2);
		auto descr = lua::Converter<std::string>::extract(state, 3);
		if (lua_gettop(state) == 4 && !lua_isnil(state, 4))
		{
			lua::Converter<T>::push(
			    state,
				self.option<T>(
					std::move(name),
					std::move(descr),
					lua::Converter<T>::extract(state, 4)
				)
			);
		}
		else
		{
			auto res = self.option<T>(std::move(name), std::move(descr));
			if (res)
				lua::Converter<T>::push(state, *res);
			else
				lua_pushnil(state);
		}
		return 1;
	}

	template<typename T>
	static int Build_lazy_option(lua_State* state)
	{
		Build& self = lua::Converter<std::reference_wrapper<Build>>::extract(state, 1);
		auto key = lua::Converter<std::string>::extract(state, 2);
		auto descr = lua::Converter<std::string>::extract(state, 3);
		if (!lua_isfunction(state, -1))
			throw std::runtime_error("Expected a function as a second argument");
		auto res = self.lazy_option<T>(
			key,
			descr,
			[&]() -> T {
				lua::State::check_status(state, lua_pcall(state, 0, 1, 0));
				try { return lua::Converter<T>::extract(state, -1); }
				catch (...) {
				    CONFIGURE_THROW(
				      error::BuildError("The lua callback for option '" + key +
				                        "' didn't return a valid value")
				      << error::nested(std::current_exception()));
			    }
			});
		lua::Converter<T>::push(state, std::move(res));
		return 1;
	}

	static int Build_visit_targets(lua_State* state)
	{
		Build& self = lua::Converter<std::reference_wrapper<Build>>::extract(state, 1);
		if (!lua_isfunction(state, -1))
			throw std::runtime_error("Expected a function as a second argument");
		self.visit_targets(
			[&](NodePtr& node) {
				lua_pushvalue(state, -1); // Save the callback
				lua::Converter<NodePtr>::push(state, node);
				self.lua_state().call(1, 0);
			}
		);
		return 0;
	}

	template<log::Level level>
	static int Build_log(lua_State* state)
	{
		//Build& self = lua::Converter<std::reference_wrapper<Build>>::extract(state, 1);
		std::string msg;
		for (int i = 2, len = lua_gettop(state); i <= len; ++i)
		{
			if (!msg.empty()) msg.push_back(' ');
			msg += luaL_tolstring(state, i, nullptr);
		}
		if (level == log::Level::error)
			CONFIGURE_THROW(error::BuildError(msg));
		log::log<level>(msg);
		return 0;
	}

	static int Build_configure(lua_State* state)
	{
		Build& self = lua::Converter<std::reference_wrapper<Build>>::extract(state, 1);
		luaL_checktype(state, 2, LUA_TTABLE);


		fs::path dir;
		fs::path build_dir = ".";
		bool has_args = false;
		lua_pushnil(state);
		while (lua_next(state, 2))
		{
			std::string key = lua::Converter<std::string>::extract(state, -2);
			if (key == "directory")
				dir = utils::extract_path(state, -1);
			else if (key == "build_directory")
				build_dir = utils::extract_path(state, -1);
			else if (key == "args")
				has_args = true;
			else
				CONFIGURE_THROW(
					error::InvalidArgument("Invalid key '" + key + "'")
				);
			lua_pop(state, 1);
		}
		if (dir.empty())
			CONFIGURE_THROW(
				error::InvalidArgument("The 'directory' argument is mandatory'")
			);

		if (!dir.is_absolute())
			dir = self.project_directory() / dir;
		if (has_args)
		{
			luaL_checktype(state, 2, LUA_TTABLE);
			lua_pushstring(state, "args");
			lua_gettable(state, 2);
			luaL_checktype(state, 3, LUA_TTABLE);
		}
		self.configure(dir, build_dir, has_args);
		return 1;
	}

	void bind_build(lua::State& state)
	{
		/// Represent a build.
		// @classmod Build
		lua::Type<Build, std::reference_wrapper<Build>>(state)
		  /// Current project directory @{Path}.
		  // @function Build:project_directory
		  .def("project_directory", &Build::project_directory)

		  /// Configure a sub-project
		  // @function Build:include
		  // @tparam table args
		  // @tparam Path args.directory relative path to the sub-project directory
		  // @tparam[opt] args table Arguments for the configure function
		  // @returns The configuration function return value
		  .def("include", &Build_configure)

		  /// Current build directory @{Path}.
		  // @function Build:directory
		  .def("directory", &Build::directory)

		  /// Build root node
		  // @function Build:root_node
		  .def("root_node", &Build::root_node)

		  /// Source @{Node} associated to a @{Path}.
		  // @tparam Path path relative path to a source file
		  // @function Build:source_node
		  .def("source_node", &Build::source_node)

		  /// Target @{Node} associated to a @{Path}.
		  // @tparam Path path relative path to a built file
		  // @function Build:target_node
		  .def("target_node", &Build::target_node)

		  /// Directory @{Node} associated to a @{Path}.
		  // @tparam Path path Absolute path to a directory
		  // @function Build:directory_node
		  .def("directory_node", &Build::directory_node)

		  /// File @{Node} associated to a @{Path}.
		  // @tparam Path path Absolute path to a file
		  // @function Build:file_node
		  .def("file_node", &Build::file_node)

		  /// Virtual @{Node}.
		  // @tparam string name A non-empty string
		  // @function Build:virtual_node
		  .def("virtual_node", &Build::virtual_node)

		  /// Visit all generated nodes.
		  // @param callback a function that accept a node
		  // @function Build:visit_targets
		  .def("visit_targets", &Build_visit_targets)

		  /// Add a rule to the build.
		  // @tparam Rule rule Rule to add
		  // @function Build:add_rule
		  .def("add_rule", &Build::add_rule)

		  /// Associated @{Filesystem} instance.
		  // @function Build:fs
		  .def<lua::return_policy::ref>("fs", &Build::fs)

		  /// Associated @{Environ} instance.
		  // @function Build:env
		  .def<lua::return_policy::ref>("env", &Build::env)

		  /// Path to the configure executable
		  // @function Build:configure_program
		  .def<lua::return_policy::copy>(
		     "configure_program", &Build::configure_program)

		  /// Declare an option of type @{string} and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] string default_value
		  // @treturn string|nil Associated value
		  // @function Build:string_option
		  .def( "string_option", &Build_option<std::string> )

		  /// Declare an option of type int and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] int default_value
		  // @treturn int|nil Associated value
		  // @function Build:int_option
		  .def( "int_option", &Build_option<int64_t> )

		  /// Declare an option of type bool and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] bool default_value
		  // @treturn bool|nil Associated value
		  // @function Build:bool_option
		  .def( "bool_option", &Build_option<bool> )

		  /// Declare an option of type @{Path} and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] Path default_value
		  // @treturn Path|nil Associated value
		  // @function Build:path_option
		  .def( "path_option", &Build_option<fs::path> )

		  /// Declare a lazy option of type @{string} and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] string default_value
		  // @treturn string|nil Associated value
		  // @function Build:string_option
		  .def( "lazy_string_option", &Build_lazy_option<std::string> )

		  /// Declare a lazy option of type int and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] int default_value
		  // @treturn int|nil Associated value
		  // @function Build:int_option
		  .def( "lazy_int_option", &Build_lazy_option<int64_t> )

		  /// Declare a lazy option of type bool and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] bool default_value
		  // @treturn bool|nil Associated value
		  // @function Build:bool_option
		  .def( "lazy_bool_option", &Build_lazy_option<bool> )

		  /// Declare a lazy option of type @{Path} and return it's value.
		  // @string name
		  // @string description
		  // @tparam[opt] Path default_value
		  // @treturn Path|nil Associated value
		  // @function Build:path_option
		  .def( "lazy_path_option", &Build_lazy_option<fs::path> )

		  /// The host platform.
		  // @treturn Platform
		  // @function Build:host_platform
		  .def<lua::return_policy::ref>("host", &Build::host)

		  /// The target platform.
		  // @treturn Platform
		  // @function Build:target_platform
		  .def<lua::return_policy::ref>("target", &Build::target)

		  /// Log a debug message.
		  // @param args...
		  // @function Build:debug
		  .def( "debug", &Build_log<log::Level::verbose> )

		  /// Log an informational message.
		  // @param args...
		  // @function Build:status
		  .def( "status", &Build_log<log::Level::status> )

		  /// Log a warning message.
		  // @param args...
		  // @function Build:warning
		  .def( "warning", &Build_log<log::Level::warning> )

		  /// Stop the configuration with an error message.
		  // @param args...
		  // @function Build:error
		  .def("error", &Build_log<log::Level::error>);
	}

}
