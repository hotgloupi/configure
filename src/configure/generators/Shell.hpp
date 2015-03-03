#pragma once

#include <configure/Generator.hpp>

namespace configure { namespace generators {

	class Shell
		: public Generator
	{
	public:
		Shell(Build& build, path_t project_directory, path_t configure_exe)
			: Generator(
			    build,
			    std::move(project_directory),
			    std::move(configure_exe),
			    name()
			)
		{}
		void generate() const override;
		std::vector<std::string>
		build_command(std::string const& target) const override;

	public:
		static char const* name() { return "Shell"; }
		static bool is_available(Build& build);
	};

}}
