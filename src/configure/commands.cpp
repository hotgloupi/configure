#include "commands.hpp"

#include "commands/extract.hpp"
#include "commands/fetch.hpp"
#include "commands/header_dependencies.hpp"
#include "commands/lua_function.hpp"
#include "commands/touch.hpp"

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
		else if (args[0] == "fetch")
			fetch(args.at(1), args.at(2));
		else if (args[0] == "extract")
			extract(args.at(1), args.at(2));
		else if (args[0] == "touch")
			touch(args.at(1));
		else if (args[0] == "lua-function")
			lua_function(
			  args.at(1), args.at(2), {args.begin() + 3, args.end()});
		else
			throw std::runtime_error("Unknown command '" + args[0] + "'");
	}

}}
