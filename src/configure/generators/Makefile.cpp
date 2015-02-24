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
	{ return "Makefile"; }

	static std::string relative_node_path(Build& b, Node& n)
	{ return n.relative_path(b.directory()).string(); }

	static std::string absolute_node_path(Build&, Node& n)
	{ return n.path().string(); }

	void Makefile::generate(Build& build) const
	{
		bool use_relpath = build.option<bool>(
		    "GENERATOR_" + this->name() + "_USE_RELATIVE_PATH",
		    this->name() + " generator uses relative path",
		    this->use_relative_path()
		);

		std::string (*node_path)(Build&, Node&);
		if (use_relpath)
			node_path = &relative_node_path;
		else
			node_path = &absolute_node_path;

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

			// Add outputs of the root virtual node
			for (auto out_edge_range = boost::out_edges(build.root_node()->index, g);
			     out_edge_range.first != out_edge_range.second;
			     ++out_edge_range.first)
			{
				all.insert(
					bg.node(boost::target(*out_edge_range.first, g)).get()
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
						out << ' ' << node_path(build, *node);
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
				out << node_path(build, *node) << ':';

			for (GraphTraits::in_edge_iterator i = in_edge_range.first;
			     i != in_edge_range.second; ++i)
			{
				auto node = bg.node(boost::source(*i, g)).get();
				if (node->is_file())
					out << ' ' << node_path(build, *node);
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
		    std::vector<std::string> const& cmd) const
	{
#ifdef _WIN32
		if (cmd.size() == 1)
		{
			// Workaroung a bug in GNU Make, when commands contain a double quote
			// they are spawned through CreateProcess() as 'sh -c \"COMMAND HERE\"'
			// when the argument does have any special character other than slash
			// and there is only one argument, quotes are left ...
			auto res = quote<CommandParser::make>(cmd);
			if (res[0] == '"' && res.back() == '"' )
			{
				res = res.substr(1, res.size() - 2);
			}
			out << res;
			return;
		}
#endif
		out << quote<CommandParser::make>(cmd);
	}

	bool Makefile::is_available(Build& build) const
	{
		return build.fs().which("make") != boost::none;
	}
	std::vector<std::string>
	Makefile::build_command(Build& build, std::string const& target) const
	{
		return {
			"make", "-C", build.directory().string(),
			target.empty() ? "all" : target
		};
	}

	bool Makefile::use_relative_path() const { return true; }
}}

