#include "NMakefile.hpp"

#include <configure/Build.hpp>
#include <configure/Filesystem.hpp>
#include <configure/quote.hpp>
#include <configure/Node.hpp>

namespace configure { namespace generators {

	std::string NMakefile::name() const
	{ return "NMakefile"; }

	bool NMakefile::is_available(Build& build) const
	{
		return build.fs().which("nmake") != boost::none;
	}

	std::string NMakefile::dump_command(std::vector<std::string> const& cmd) const
	{ return quote<CommandParser::nmake>(cmd); }

	std::vector<std::string>
	NMakefile::build_command(Build& build, std::string const& target) const
	{
		return {
			"nmake", "-nologo", "-f", (build.directory() / "Makefile").string(),
			target.empty() ? "all" : target
		};
	}

	bool NMakefile::use_relative_path() const { return false; }

}}
