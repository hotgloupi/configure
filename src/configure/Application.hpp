#pragma once

#include "fwd.hpp"

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

namespace configure {

	// Parse arguments and generate builds.
	class Application
	{
	private:
		typedef boost::filesystem::path path_t;
	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		Application(int ac, char** av);
		explicit Application(std::vector<std::string> args);
		virtual ~Application();

	public:
		void run();

	public:
		virtual void print_help();
		virtual void exit();

	public:
		path_t const& program_name() const;

		path_t const& project_directory() const;

		std::vector<path_t> const& build_directories() const;

	private:
		std::unique_ptr<Generator> _generator(Build& build) const;
		void _parse_args();
	};

}
