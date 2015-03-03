#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>

#include <vector>
#include <string>

namespace configure {

	class Generator
	{
	public:
		typedef boost::filesystem::path path_t;

	protected:
		Build& _build;
		path_t _project_directory;
		path_t _configure_exe;
		std::string const _name;

	public:
		Generator(Build& build,
		          path_t project_directory,
		          path_t configure_exe,
		          std::string name);
		virtual ~Generator();

		std::string const& name() const { return _name; }

	public:
		// Prepare the generator and the build, might alter the build graph.
		virtual void prepare();

		// Generate necessary build files.
		virtual void generate() const = 0;

		// Return a command that triggers the build.
		virtual
		std::vector<std::string>
		build_command(std::string const& target) const = 0;
	};

}

