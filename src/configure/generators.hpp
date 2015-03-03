#pragma once

#include "Generator.hpp"

#include <boost/filesystem/path.hpp>

#include <memory>
#include <string>

namespace configure { namespace generators {

	std::unique_ptr<Generator> from_name(std::string const& name,
	                                     Build& build,
	                                     boost::filesystem::path project_directory,
	                                     boost::filesystem::path configure_exe);

	std::string first_available(Build& build);

}}
