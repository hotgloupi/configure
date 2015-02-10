#include <configure/bind.hpp>

#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace configure {

	static int Path_new(lua_State* state)
	{
		if (lua_isnil(state, -1))
			lua::Converter<fs::path>::push(state);
		else if (char const* path = lua_tostring(state, -1))
			lua::Converter<fs::path>::push(state, path);
		else if (lua::Converter<fs::path>::extract_ptr(state, -1) != nullptr)
		{ /* Already a path instance */ }
		else
			throw std::runtime_error("Expected string to construct a path");
		return 1;
	}

	static int Path_concat(lua_State* state)
	{
		fs::path p1, p2;
		if (lua_isstring(state, -2))
			p1 = lua_tostring(state, -2);
		else
			p1 = lua::Converter<fs::path>::extract(state, -2);

		if (lua_isstring(state, -1))
			p2 = lua_tostring(state, -1);
		else
			p2 = lua::Converter<fs::path>::extract(state, -1);

		lua::Converter<fs::path>::push(state, p1 / p2);
		return 1;
	}

	static int Path_add(lua_State* state)
	{
		auto& self = lua::Converter<fs::path>::extract(state, 1);
		auto ext = lua::Converter<std::string>::extract(state, 2);
		lua::Converter<fs::path>::push(state, self.string() + ext);
		return 1;
	}

	static int Path_is_directory(lua_State* state)
	{
		auto& self = lua::Converter<fs::path>::extract(state, 1);
		lua_pushboolean(state, fs::is_directory(self));
		return 1;
	}

	static int Path_tostring(lua_State* state)
	{
		auto& self = lua::Converter<fs::path>::extract(state, 1);
		lua_pushstring(state, self.string().c_str());
		return 1;
	}

	void bind_path(lua::State& state)
	{
		/// Store a filesystem path.
		// @classmod Path
		lua::Type<fs::path>(state, "Path")
			/// Create a new @{Path} instance
			// @function Path:new
			//
			// @tparam string|Path|nil value Path to store
			// @treturn Path New instance.
			// @usage
			// local p = Path:new() -- equivalent to "."
			// local p = Paht:new('a')
			// local p = Path:new(Path:new('b'))
			.def("new", &Path_new)

			/// Convert to a string
			// @function Path:__tostring
			.def("__tostring", &Path_tostring)

			/// Concatenate paths.
			// @function Path:__div
			// @tparam Path|string component Path to append
			// @treturn Path Concatenated paths
			// @usage
			// -- Concatenate a Path and a string
			// local p = Path:new("a") / "b" -- p == "a/b"
			// -- Or a string and a Path
			// local p = "a" / Path:new("b") -- p == "a/b"
			// -- Or two Paths
			// local p = Path:new("a") / Path:new("b") -- p == "a/b"
			//
			.def("__div", &Path_concat)

			/// Add string to the last component.
			// @function Path:__add
			// @string suffix String to append
			// @treturn Path Path with the new suffix
			// @usage
			// local p = Path:new("path/to/file") + '.extension'
			// -- p == "path/to/file.extension"
			//
			.def("__add", &Path_add)

			/// Last component of a path.
			// @function Path:filename
			// @usage local f = Path:new("a/b/c"):filename()
			// -- f == "c"
			//
			.def("filename", &fs::path::filename)

			/// Parent path
			// @function Path:parent_path
			// @usage local f = Path:new("a/b/c"):parent_path()
			// -- f == "a/b"
			.def("parent_path", &fs::path::parent_path)

			/// Check if a path is absolute.
			/// @function Path:is_absolute
			/// @treturn bool True when the path is absolute
			.def("is_absolute", &fs::path::is_absolute)

			/// True when a path refers to a directory
			// @function Path:is_directory
			// @treturn bool
			.def("is_directory", &Path_is_directory)

			/// Filename without the extension (if any)
			// @function Path:stem
			// @treturn Path
			.def("stem", &fs::path::stem)

			/// Extension of the filename
			// @function Path:extension
			// @treturn Path
			.def("extension", &fs::path::extension)

			/// Extension of the filename
			// @function Path:ext
			// @treturn Path
			.def("ext", &fs::path::extension)

			/// True when the path is empty
			// @function Path:empty
			// @treturn bool
			.def("empty", &fs::path::empty)
		;
	}

}

