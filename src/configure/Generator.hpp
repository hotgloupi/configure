#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>

#include <vector>
#include <string>

namespace configure {

	class Generator
	{
	public:
		Generator();
		virtual ~Generator();

	public:
		virtual bool is_available(Build& build) const = 0;
		virtual
		void generate(Build& build,
		              boost::filesystem::path const& root_project_directory) const = 0;
		virtual std::string name() const = 0;
		virtual std::vector<std::string>
		build_command(Build& build, std::string const& target) const = 0;
	};

}

