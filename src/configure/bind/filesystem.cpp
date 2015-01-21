#include <configure/bind.hpp>

#include <configure/Filesystem.hpp>
#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;


namespace configure {

	static int fs_glob(lua_State* state)
	{
		Filesystem& self = lua::Converter<std::reference_wrapper<Filesystem>>::extract(state, 1);
		std::vector<NodePtr> res;
		if (char const* arg = lua_tostring(state, 2))
		{
			res = self.glob(arg);
		}

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
		lua::Type<Filesystem, std::reference_wrapper<Filesystem>>(state)
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

			/// Find an executable path
			// @function Filesystem:which
			// @tparam string|Path name An executable name
			// @treturn Path|nil Absolute path to the executable found or nil
			.def("which", &fs_which)

			/// Generate rule that copy a file.
			// @function Filesystem:copy
			// @treturn Node the target node
			.def("copy", &fs_copy)
		;
	}

}
