#pragma once

#include <boost/filesystem/path.hpp>

#include <iosfwd>
#include <vector>

namespace configure { namespace commands {

	void header_dependencies(
	    std::ostream& out,
	    boost::filesystem::path const& source,
	    std::vector<boost::filesystem::path> const& targets,
	    std::vector<boost::filesystem::path> const& include_directories);

}}
