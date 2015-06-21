#pragma once

#include <string>
#include <boost/filesystem/path.hpp>

namespace configure { namespace commands {

	void fetch(std::string const& uri, boost::filesystem::path const& dest);

}}
