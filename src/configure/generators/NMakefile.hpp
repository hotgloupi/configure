#pragma once

#include "Makefile.hpp"

namespace configure { namespace generators {

	class NMakefile
		: public Makefile
	{
	public:
		NMakefile(Build& build, path_t project_directory, path_t configure_exe)
			: Makefile(
				build,
				std::move(project_directory),
				std::move(configure_exe),
				name()
			)
		{}

		std::vector<std::string>
		build_command(std::string const& target) const override;

	public:
		static char const* name() { return "NMakefile"; }
		static bool is_available(Build& build);

	protected:
		std::string dump_command(ShellCommand const& cmd,
		                         DependencyLink const& link,
		                         ShellFormatter const& formatter) const override;
		bool use_relative_path() const override;
		void include_dependencies(std::ostream& out, bool relative) const override;
		void prepare();
	};

}}
