#include "NMakefile.hpp"

#include <configure/Build.hpp>
#include <configure/Filesystem.hpp>
#include <configure/quote.hpp>

namespace configure { namespace generators {

	std::string NMakefile::name() const
	{ return "NMakefile"; }

	bool NMakefile::is_available(Build& build) const
	{
		return build.fs().which("nmake") != boost::none;
	}

	void NMakefile::dump_command(
		    std::ostream& out,
		    std::vector<std::string> const& cmd) const
	{
		out << quote<CommandParser::nmake>(cmd);
	}

	std::vector<std::string>
	NMakefile::build_command(Build& build, std::string const& target) const
	{
		return {
			"nmake", "-f", (build.directory() / "Makefile").string(),
			target.empty() ? "all" : target
		};
	}

}}
