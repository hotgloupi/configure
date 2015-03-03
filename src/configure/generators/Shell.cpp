#include "Shell.hpp"

#include <configure/BuildGraph.hpp>
#include <configure/Build.hpp>
#include <configure/Command.hpp>
#include <configure/Filesystem.hpp>
#include <configure/Graph.hpp>
#include <configure/log.hpp>
#include <configure/quote.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/graph/topological_sort.hpp>

#include <fstream>
#include <unordered_set>

namespace configure { namespace generators {

	void Shell::generate() const
	{
		std::ofstream out((_build.directory() / "build.sh").string());
		out << "#!/bin/sh" << std::endl;
		out << "set -x" << std::endl;
		out << "set -e" << std::endl;
		BuildGraph const& bg = _build.build_graph();
		Graph const& g = bg.graph();
		std::list<Vertex> ordered;
		boost::topological_sort(g, std::front_inserter(ordered));
		for (auto& v: ordered)
		{
			auto node = bg.node(v);
			if (node->is_virtual())
				out << "# virtual target: " << node->name() << std::endl;
			else
				out << "# file node: " << node->path() << std::endl;
			GraphTraits::in_edge_iterator it, end;
			boost::tie(it, end) = boost::in_edges(v, g);
			if (it == end)
				out << "# -> Nothing to do" << std::endl;
			std::unordered_set<Command const*> seen_commands;
			for (; it != end; ++it)
			{
				auto& link = bg.link(*it);
				Command const* cmd_ptr = &link.command();
				if (seen_commands.count(cmd_ptr) != 0)
					continue;
				seen_commands.insert(cmd_ptr);
				for (auto const& shell_command: cmd_ptr->shell_commands())
				{
					out <<  quote<CommandParser::unix_shell>(shell_command.string(_build, link)) << std::endl;
				}
			}
		}
	}

	bool Shell::is_available(Build& build)
	{ return build.fs().which("sh") != boost::none; }

	std::vector<std::string>
	Shell::build_command(std::string const& target) const
	{
		if (!target.empty())
			log::warning("Shell generator does not support targets");
		return {
			"sh", (_build.directory() / "build.sh").string(),
		};
	}

}}
