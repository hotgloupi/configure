#include "Makefile.hpp"

#include <configure/BuildGraph.hpp>
#include <configure/Build.hpp>
#include <configure/Command.hpp>
#include <configure/Filesystem.hpp>
#include <configure/Graph.hpp>
#include <configure/quote.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/graph/topological_sort.hpp>

#include <fstream>
#include <unordered_set>

namespace configure { namespace generators {

	std::string Makefile::name() const
	{ return "makefile"; }

	void Makefile::generate(Build& build)
	{
		std::ofstream out((build.directory() / "Makefile").string());
		out << "# Generated makefile" << std::endl;
		BuildGraph const& bg = build.build_graph();
		Graph const& g = bg.graph();

		// Fill the "all" rule
		{
			std::set<Node*> all;
			std::set<Node*> phony;

			// Search leaf nodes in the graph which are not virtual nodes
			for (auto vertex_range = boost::vertices(g);
			     vertex_range.first != vertex_range.second;
			     ++vertex_range.first)
			{
				auto vertex = *vertex_range.first;
				auto& node = bg.node(vertex);
				if (node->is_virtual())
				{
					if (!node->name().empty())
						phony.insert(node.get());
					continue;
				}
				// Check if a node is not a dependency of something else
				// then it should be in the "all" rule.
				auto out_edge_range = boost::out_edges(vertex, g);
				if (out_edge_range.first == out_edge_range.second)
					all.insert(node.get());
			}

			// Add outputs of the all virtual node
			for (auto in_edge_range = boost::in_edges(build.virtual_node("all")->index, g);
			     in_edge_range.first != in_edge_range.second;
			     ++in_edge_range.first)
			{
				all.insert(
					bg.node(boost::source(*in_edge_range.first, g)).get()
				);
			}

			out << ".PHONY:" << std::endl;
			out << ".PHONY: all";
			for (auto node: phony)
				out << ' ' << node->name();
			out << "\n\n";

			if (!all.empty())
			{
				out << "all:";
				for (auto& node: all)
				{
					if (node->is_file())
						out << ' ' << node->relative_path(build.directory()).string();
					else if (node->is_virtual() && !node->name().empty())
						out << ' ' << node->name();
				}
				out << "\n\n";
			}
		}

		for (auto vertex_range = boost::vertices(g);
		     vertex_range.first != vertex_range.second;
		     ++vertex_range.first)
		{
			auto vertex = *vertex_range.first;
			auto node = bg.node(vertex).get();

			auto in_edge_range = boost::in_edges(vertex, g);

			if (in_edge_range.first == in_edge_range.second)
				continue;

			if (node->is_virtual())
				out << node->name() << ':';
			else
				out << node->relative_path(build.directory()).string() << ':';

			for (GraphTraits::in_edge_iterator i = in_edge_range.first;
			     i != in_edge_range.second; ++i)
			{
				auto node = bg.node(boost::source(*i, g)).get();
				if (node->is_file())
					out << ' ' << node->relative_path(build.directory()).string();
				else if (node->is_virtual())
					out << ' ' << node->name();
			}
			out << std::endl;

			std::unordered_set<Command const*> seen_commands;
			for (; in_edge_range.first != in_edge_range.second;
			     ++in_edge_range.first)
			{
				Command const* cmd_ptr = &bg.link(*in_edge_range.first).command();
				if (seen_commands.count(cmd_ptr) != 0)
					continue;
				seen_commands.insert(cmd_ptr);
				for (auto const& shell_command: cmd_ptr->shell_commands())
				{
					out << '\t';
					this->dump_command(out, shell_command.dump());
					out << std::endl;
				}
			}
			out << std::endl;
		}

	}
	void Makefile::dump_command(
		    std::ostream& out,
		    std::vector<std::string> const& cmd)
	{
		out << quote<CommandParser::make>(cmd);
	}

	bool Makefile::is_available(Build& build) const
	{
		return build.fs().which("make") != boost::none;
	}
}}

