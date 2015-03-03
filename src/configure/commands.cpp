#include "commands.hpp"
#include "commands/header_dependencies.hpp"

#include <fstream>

namespace configure { namespace commands {

	void execute(std::vector<std::string> const& args)
	{
		if (args[0] == "c-header-dependencies")
		{
			std::ofstream out(args[1]);
			boost::filesystem::path source = args[2];
			std::vector<boost::filesystem::path> targets;
			targets.push_back(args[1]); // the .mk should be re-generated
			size_t i = 3;
			for (; i < args.size() && args[i] != "--"; ++i)
				targets.push_back(args[i]);
			i += 1;
			std::vector<boost::filesystem::path> include_directories;
			for (; i < args.size(); ++i)
				include_directories.push_back(args[i]);
			header_dependencies(out, source, targets, include_directories);
		}
	}

}}
