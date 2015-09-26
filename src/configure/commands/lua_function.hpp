#pragma once

#include <boost/filesystem/path.hpp>

#include <vector>
#include <string>

namespace configure { namespace commands {
	void lua_function(boost::filesystem::path const& script,
	                  std::string const& function,
	                  std::vector<std::string> const& args);
}}
