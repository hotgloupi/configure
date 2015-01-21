#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

namespace configure { namespace lua {

	struct Frame
	{
		std::string name;
		std::string kind;
		boost::filesystem::path source;
		int current_line;
		int first_function_line;
		bool builtin;
	};

	std::vector<Frame> traceback(lua_State *L);

}}
