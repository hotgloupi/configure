#pragma once

#include <boost/filesystem/path.hpp>

namespace configure { namespace commands {

	void extract(boost::filesystem::path const& tarball,
	             boost::filesystem::path const& dest_dir);

}}
