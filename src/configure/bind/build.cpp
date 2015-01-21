#include <configure/bind.hpp>

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
		if (lua_gettop(state) == 3)
		{
			auto res = self.option<T>(name, descr);
			if (res)
				lua::Converter<T>::push(state, *res);
			else
				lua_pushnil(state);
		}
		else
		{
			lua::Converter<T>::push(
			    state,
				self.option<T>(
					name,
					descr,
					lua::Converter<T>::extract(state, 4)
				)
			);
		}
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

			/// Current build directory @{Path}.
			// @function Build:directory
			.def("directory", &Build::directory)

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

			/// Declare an option of type @{string} and return it's value.
			// @string name
			// @string description
			// @tparam[opt] string default_value
			// @treturn string|nil Associated value
			// @function Build:string_option
			.def("string_option", &Build_option<std::string>)

			/// Declare an option of type int and return it's value.
			// @string name
			// @string description
			// @tparam[opt] int default_value
			// @treturn int|nil Associated value
			// @function Build:int_option
			.def("int_option", &Build_option<int>)

			/// Declare an option of type bool and return it's value.
			// @string name
			// @string description
			// @tparam[opt] bool default_value
			// @treturn bool|nil Associated value
			// @function Build:bool_option
			.def("bool_option", &Build_option<bool>)

			/// Declare an option of type @{Path} and return it's value.
			// @string name
			// @string description
			// @tparam[opt] Path default_value
			// @treturn Path|nil Associated value
			// @function Build:path_option
			.def("path_option", &Build_option<fs::path>)

			/// The host platform.
			// @treturn Platform
			// @function Build:host_platform
			.def<lua::return_policy::ref>("host", &Build::host)

			/// The target platform.
			// @treturn Platform
			// @function Build:target_platform
			.def<lua::return_policy::ref>("target", &Build::target)
		;

	}

}
