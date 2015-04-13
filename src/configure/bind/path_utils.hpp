#pragma once

#include <configure/lua/fwd.hpp>

#include <boost/filesystem/path.hpp>

namespace configure { namespace utils {

	boost::filesystem::path extract_path(lua_State* state, int index = -1);

}}
