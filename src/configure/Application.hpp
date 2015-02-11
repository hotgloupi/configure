#pragma once

#include "fwd.hpp"

#include <boost/filesystem.hpp>

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
		std::string const& program_name() const;

		path_t const& project_directory() const;

		std::vector<path_t> const& build_directories() const;

	private:
		void _generate(Build& build);
		void _parse_args();
	};

}
