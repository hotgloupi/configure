#include "path_utils.hpp"

#include <configure/error.hpp>
#include <configure/lua/Converter.hpp>
#include <configure/Node.hpp>

namespace fs = boost::filesystem;

namespace configure { namespace utils {

	fs::path extract_path(lua_State* state, int index)
	{
		if (NodePtr* n = lua::Converter<NodePtr>::extract_ptr(state, index))
			return (*n)->path().string();
		else if (fs::path* ptr = lua::Converter<fs::path>::extract_ptr(state, index))
			return *ptr;
		else if (lua_isstring(state, index))
			return fs::path(lua_tostring(state, index));
		else
			CONFIGURE_THROW(
				error::LuaError(
					"Expected node, path or string, got '" +
					std::string(luaL_tolstring(state, -1, nullptr)) + "'"
				)
			);
	}

}}

