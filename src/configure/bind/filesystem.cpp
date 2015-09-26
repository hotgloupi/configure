#include <configure/bind.hpp>
#include <configure/bind/path_utils.hpp>

#include <configure/Filesystem.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>
#include <configure/Node.hpp>
#include <configure/bind/path_utils.hpp>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


namespace configure {

	static int fs_glob(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		std::vector<NodePtr> res;
		if (lua_gettop(state) == 3)
		{
			res = self.glob(
			    utils::extract_path(state, 2),
			    lua::Converter<std::string>::extract(state, 3)
			);
		}
		else if (char const* arg = lua_tostring(state, 2))
		{
			res = self.glob(arg);
		}
		else
			CONFIGURE_THROW(
				error::InvalidArgument("Expected a glob pattern")
			);


		lua_createtable(state, res.size(), 0);
		for (int i = 0, len = res.size(); i < len; ++i)
		{
			lua::Converter<NodePtr>::push(state, res[i]);
			lua_rawseti(state, -2, i + 1);
		}
		return 1;
	}

	static int fs_rglob(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		std::vector<NodePtr> res;
		fs::path dir;
		if (char const* arg = lua_tostring(state, 2))
			dir = arg;
		else
			dir = lua::Converter<fs::path>::extract(state, 2);
		if (char const* arg = lua_tostring(state, 3))
		{
			res = self.rglob(dir, arg);
		}

		lua_createtable(state, res.size(), 0);
		for (int i = 0, len = res.size(); i < len; ++i)
		{
			lua::Converter<NodePtr>::push(state, res[i]);
			lua_rawseti(state, -2, i + 1);
		}
		return 1;
	}

	static int fs_list_directory(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		std::vector<NodePtr> res;
		if (char const* arg = lua_tostring(state, 2))
			res = self.list_directory(arg);
		else
			res = self.list_directory(
				lua::Converter<fs::path>::extract(state, 2)
			);

		lua_createtable(state, res.size(), 0);
		for (int i = 0, len = res.size(); i < len; ++i)
		{
			lua::Converter<NodePtr>::push(state, res[i]);
			lua_rawseti(state, -2, i + 1);
		}
		return 1;
	}

	static int fs_find_file(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		if (!lua_istable(state, 2))
			CONFIGURE_THROW(
				error::LuaError(
					"Expected a table, got '" + std::string(luaL_tolstring(state, 2, nullptr)) + "'"
				)
			);
		std::vector<fs::path> directories;
		for (int i = 1, len = lua_rawlen(state, 2); i <= len; ++i)
		{
			lua_rawgeti(state, 2, i);
			directories.push_back(utils::extract_path(state, -1));
		}
		lua::Converter<NodePtr>::push(
		    state,
			self.find_file(directories, utils::extract_path(state, 3))
		);
		return 1;
	}

	static int fs_which(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);

		std::string arg;
		if (fs::path* ptr = lua::Converter<fs::path>::extract_ptr(state, 2))
			arg = ptr->string();
		else if (char const* ptr = lua_tostring(state, 2))
			arg = ptr;

		if (!arg.empty())
		{
			auto res = self.which(arg);
			if (res)
				lua::Converter<fs::path>::push(state, *res);
			else
				lua_pushnil(state);
		}
		else
		{
			throw std::runtime_error(
			    "Filesystem.which(): Expected program name, got '" + std::string(luaL_tolstring(state, 2, 0)) + "'");
		}
		return 1;
	}

	static int fs_cwd(lua_State* state)
	{
		lua::Converter<fs::path>::push(state, fs::current_path());
		return 1;
	}

	static int fs_current_script(lua_State* state)
	{
		lua_Debug ar;
		if (lua_getstack(state, 1, &ar) != 1)
			CONFIGURE_THROW(error::LuaError("Couldn't get the stack"));
		if (lua_getinfo(state, "S", &ar) == 0)
			CONFIGURE_THROW(error::LuaError("Couldn't get stack info"));
		if (ar.source == nullptr)
			CONFIGURE_THROW(error::LuaError("Invalid source file"));
		fs::path src(ar.source[0] == '@' ? &ar.source[1] : ar.source);
		if (!src.is_absolute())
			src = fs::current_path() / src;

		if (!fs::exists(src))
			CONFIGURE_THROW(error::LuaError("Couldn't find the script path")
			                << error::path(src));
		lua::Converter<fs::path>::push(state, src);
		return 1;
	}

	static int fs_copy(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		NodePtr res;
		fs::path dst;
		if (char const* arg = lua_tostring(state, 3))
			dst = arg;
		else if (fs::path* arg = lua::Converter<fs::path>::extract_ptr(state, 3))
			dst = *arg;
		else
			CONFIGURE_THROW(
				error::LuaError("Expected string or path for dest argument")
					<< error::lua_function("Filesystem::copy")
			);
		if (char const* arg = lua_tostring(state, 2))
			res = self.copy(arg, dst);
		else if (fs::path* arg = lua::Converter<fs::path>::extract_ptr(state, 2))
			res = self.copy(*arg, dst);
		else if (NodePtr* arg = lua::Converter<NodePtr>::extract_ptr(state, 2))
			res = self.copy(*arg, dst);
		else
			CONFIGURE_THROW(
				error::LuaError("Expected string, path or Node for src argument")
					<< error::lua_function("Filesystem::copy")
			);
		lua::Converter<NodePtr>::push(state, std::move(res));
		return 1;
	}

	void bind_filesystem(lua::State& state)
	{
		/// Filesystem operations.
		// @classmod Filesystem
		lua::Type<Filesystem, std::reference_wrapper<Filesystem>>(state, "Filesystem")
			/// Find files according to a glob pattern
			// @function Filesystem:glob
			// @string pattern A glob pattern
			// @return A list of @{Node}s
			.def("glob", &fs_glob)

			/// Find files recursively according to a glob pattern
			// @function Filesystem:rglob
			// @tparam string|Path dir The base directory
			// @string pattern A glob pattern
			// @return A list of @{Node}s
			.def("rglob", &fs_rglob)

			/// List a directory
			// @function Filesystem:list_directory
			// @tparam string|Path dir Directory to list
			// @return A list of @{Node}s
			.def("list_directory", &fs_list_directory)

			/// Find a file
			// @function Filesystem:find_file
			// @tparam table directories A list of directories to inspect
			// @tparam string|Path file The file to search for
			// @return A @{Node}
			.def("find_file", &fs_find_file)

			/// Find an executable path
			// @function Filesystem:which
			// @tparam string|Path name An executable name
			// @treturn Path|nil Absolute path to the executable found or nil
			.def("which", &fs_which)

			/// Generate rule that copy a file.
			// @function Filesystem:copy
			// @treturn Node the target node
			.def("copy", &fs_copy)

			/// Return the current working directory
			// @function Filesystem:cwd
			// @treturn Path current directory
			.def("cwd", &fs_cwd)

			/// Return the current script path
			// @function Filesystem.current_script
			.def("current_script", &fs_current_script)
		;
	}

}
