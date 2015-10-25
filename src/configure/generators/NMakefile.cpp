#include "NMakefile.hpp"

#include <configure/Build.hpp>
#include <configure/Filesystem.hpp>
#include <configure/quote.hpp>
#include <configure/Node.hpp>
#include <configure/ShellCommand.hpp>

#include <iostream>

namespace configure { namespace generators {

	void NMakefile::prepare()
	{
		Makefile::prepare();
		for (auto& node: _includes)
			_final_targets.push_back(node);
	}

	bool NMakefile::is_available(Build& build)
	{
		return build.fs().which("nmake") != boost::none;
	}

	std::string NMakefile::dump_command(ShellCommand const& cmd,
	                                    DependencyLink const& link,
	                                    ShellFormatter const& formatter) const
	{
		std::string res;

		if (cmd.has_working_directory())
		{
			ShellCommand chdir;
			chdir.append("cd", cmd.working_directory());
			res += quote<CommandParser::nmake>(chdir.string(_build, link, formatter)) + " && ";
		}
		if (cmd.has_env())
		{
			// Note: It's important that there is no space between the value
			// and the separator.
			//      Bad: > SET key=value && cmd ...
			// will set `key` to "value " (with a trailing white space), while
			//      Good: > SET key=value&& cmd ...
			// will correctly assign "value" to `key`.
			// Hours spent fighting with weird windows shell parsing rules: 2
			for (auto& pair: cmd.env())
				res += "SET " + pair.first + "=" + pair.second + "&& ";
		}
		return res + quote<CommandParser::nmake>(cmd.string(_build, link, formatter));
	}

	std::vector<std::string>
	NMakefile::build_command(std::string const& target) const
	{
		return {
			"nmake", "-nologo", "-f", (_build.directory() / "Makefile").string(),
			target.empty() ? "all" : target
		};
	}

	bool NMakefile::use_relative_path() const { return false; }

	void NMakefile::include_dependencies(std::ostream& out, bool /* XXX relative */) const
	{
		for (auto& node: _includes)
			out
				<< "!IF EXISTS(" << node->path().string() << ")" << std::endl
				<< "!INCLUDE " << node->path().string() << std::endl
				<< "!ENDIF" << std::endl;
	}

}}
