#include "Makefile.hpp"

#include <configure/BuildGraph.hpp>
#include <configure/Build.hpp>
#include <configure/Command.hpp>
#include <configure/Filesystem.hpp>
#include <configure/Graph.hpp>
#include <configure/quote.hpp>
#include <configure/utils/path.hpp>
#include <configure/PropertyMap.hpp>
#include <configure/log.hpp>
#include <configure/Rule.hpp>
#include <configure/ShellCommand.hpp>
#include <configure/error.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <unordered_set>

namespace configure { namespace generators {

	Makefile::Makefile(Build& build,
	                   path_t project_directory,
	                   path_t configure_exe,
	                   char const* name)
		: Generator(
			build,
			std::move(project_directory),
			std::move(configure_exe),
			(name == nullptr ? Makefile::name() : name)
		)
	{}

	static std::string relative_node_path(Build& b, Node& n)
	{ return n.relative_path(b.directory()).string(); }

	static std::string absolute_node_path(Build&, Node& n)
	{ return n.path().string(); }

	void Makefile::prepare()
	{
		BuildGraph const& bg = _build.build_graph();
		Graph const& g = bg.graph();

		{
			Rule regen;
			regen.add_target(_build.target_node("Makefile"));
			for (auto& project_dir: _build.project_stack())
			{
				auto p = Build::find_project_file(project_dir);
				auto& n = _build.source_node(p);
				regen.add_source(n);
			}
			ShellCommand regen_cmd;
			regen_cmd.append(
				_build.target_node(_configure_exe),
				"--project", _build.project_stack().at(0),
				_build.directory()
			);
			regen.add_shell_command(std::move(regen_cmd));
			//ShellCommand make_cmd;
			//make_cmd.extend(this->build_command("all"));
			//regen.add_shell_command(std::move(make_cmd));
			_build.add_rule(std::move(regen));
		}


		for (auto vertex_range = boost::vertices(g);
		     vertex_range.first != vertex_range.second;
		     ++vertex_range.first)
		{
			auto vertex = *vertex_range.first;
			auto& node = bg.node(vertex);

			if (node->is_virtual())
			{
				if (node->name().empty())
					continue;
				_virtual_nodes.push_back(node);
			}
			else
			{
				// Check for nodes that are not generated by us (no inputs).
				// (this does not apply to virtual nodes, that always
				// considered as targets).
				auto in_edge_range = boost::in_edges(vertex, g);
				if (in_edge_range.first == in_edge_range.second)
				{
					_sources.push_back(node);
					continue;
				}
			}

			// At this point we know that the node is a target.
			_targets.push_back(node);

			auto out_edge_range = boost::out_edges(vertex, g);
			if (out_edge_range.first == out_edge_range.second)
			{
				// Nothing depends on this node, this target is final.
				if (!node->is_virtual())
					_final_targets.push_back(node);
			}
			else
			{
				// Something depends on this Node, it is a also a source.
				if (!node->is_virtual())
					_sources.push_back(node);

				// The node is still considered final if the root node depends
				// on it.
				if (bg.has_link(*_build.root_node(), *node))
					_final_targets.push_back(node);
			}
		}

		for (auto& node: _sources)
		{
			if (!node->is_file() || !node->has_property("language"))
				continue;

			auto& lang = node->property<std::string>("language");
			if (lang == "c" || lang == "c++")
			{
				auto include_directories = node->property<std::vector<boost::filesystem::path>>("include_directories");
				NodePtr first_target;
				for (auto out_edge_range = boost::out_edges(node->index, g);
					 out_edge_range.first != out_edge_range.second;
					 ++out_edge_range.first)
				{
					first_target = bg.node(boost::target(*out_edge_range.first, g));
					break;
				}
				if (first_target == nullptr)
				{
					log::warning("Ignore implicit dependencies for",
					             node->string(), ", no target node found...");
					continue;
				}

				auto& target = _build.target_node(
					first_target->relative_path(_build.directory()).string() + ".mk"
				);
				_includes.push_back(target);
				ShellCommand cmd;
				cmd.append(
					_configure_exe, "-E" , "c-header-dependencies",
					target, node
				);
				for (auto out_edge_range = boost::out_edges(node->index, g);
					 out_edge_range.first != out_edge_range.second;
					 ++out_edge_range.first)
					cmd.append(bg.node(boost::target(*out_edge_range.first, g)));
				cmd.append("--");
				for (auto& dir: include_directories)
					cmd.append(_build.directory_node(dir));
				_build.add_rule(
					Rule()
						.add_source(node)
						.add_target(target)
						.add_shell_command(std::move(cmd))
				);
			}
		}

		// We still need to add dependencies to the final targets
		for (auto& node: _includes)
			_targets.push_back(node);
	}

	namespace {

		struct RelativePathShellFormatter
			: public ShellFormatter
		{
			boost::filesystem::path const& _build_dir;
			boost::filesystem::path const& _project_dir;

			RelativePathShellFormatter(Build& build,
			                           boost::filesystem::path const& project_dir)
				: _build_dir(build.directory())
				, _project_dir(project_dir)
			{}

			std::string
			operator ()(boost::filesystem::path const& value) const override
			{
				if (utils::starts_with(value, _project_dir))
					return utils::relative_path(value, _build_dir).string();
				return value.string();
			}

		};

	}

	void Makefile::generate() const
	{
		bool use_relpath = _build.option<bool>(
		    "GENERATOR_" + std::string(this->name()) + "_USE_RELATIVE_PATH",
		    std::string(this->name()) + " generator uses relative path",
		    this->use_relative_path()
		);
		std::unique_ptr<ShellFormatter> formatter;
		if (use_relpath)
			formatter = std::unique_ptr<ShellFormatter>(
				new RelativePathShellFormatter(_build, _project_directory)
			);
		else
			formatter = std::unique_ptr<ShellFormatter>(new ShellFormatter);

		std::string (*node_path)(Build&, Node&);
		if (use_relpath)
			node_path = &relative_node_path;
		else
			node_path = &absolute_node_path;

		auto& makefile_node = _build.target_node("Makefile");
		std::ofstream out(makefile_node->path().string());
		out << "# Generated makefile" << std::endl;
		if (_name == "Makefile")
		{
			// This speed up the makefile
			out << "MAKEFLAGS += --no-builtin-rules" << std::endl;
			out << ".SUFFIXES:" << std::endl;
		}
		BuildGraph const& bg = _build.build_graph();
		Graph const& g = bg.graph();

		// Fill the "all" and ".PHONY" rules
		out << ".PHONY:" << std::endl;
		out << ".PHONY: all";
		for (auto node: _virtual_nodes)
			out << ' ' << node->name();
		out << "\n\n";

		if (!_final_targets.empty())
		{
			out << "all:";
			for (auto& node: _final_targets)
			{
				if (node->is_file())
					out << ' ' << node_path(_build, *node);
				else if (node->is_virtual() && !node->name().empty())
					out << ' ' << node->name();
			}
			out << "\n\n";

			out << "clean:\n";
			for (auto& node: _targets)
				if (node->is_file())
					out << '\t' << "rm -f " << node_path(_build, *node) << '\n';
			out << "\n";
		}

		auto generated_commands_property_name =
			std::string(this->name()) + "_GENERATED_COMMANDS";
		std::unordered_set<Node const*> to_delete;
		for (auto& node: _targets)
		{
			if (node->is_virtual())
				out << node->name() << ':';
			else
				out << node_path(_build, *node) << ':';

			auto in_edge_range = boost::in_edges(node->index, g);
			for (GraphTraits::in_edge_iterator i = in_edge_range.first;
			     i != in_edge_range.second; ++i)
			{
				auto node = bg.node(boost::source(*i, g)).get();
				if (node->is_file())
					out << ' ' << node_path(_build, *node);
				else if (node->is_virtual())
					out << ' ' << node->name();
			}
			out << std::endl;

			std::vector<std::string> command_strings;
			std::unordered_set<Command const*> seen_commands;
			for (; in_edge_range.first != in_edge_range.second;
			     ++in_edge_range.first)
			{
				auto& link = bg.link(*in_edge_range.first);
				Command const* cmd_ptr = &link.command();
				if (seen_commands.insert(cmd_ptr).second == false)
					continue;
				for (auto const& shell_command: cmd_ptr->shell_commands())
				{
					command_strings.push_back(
						this->dump_command(
							shell_command.string(_build, link, *formatter)
						)
					);
					out << '\t' << command_strings.back() << std::endl;
				}
			}
			out << std::endl;
			if (node->is_file())
			{
				bool had_commands_property = node->has_property(
					generated_commands_property_name
				);
				node->set_property<std::string>(
					generated_commands_property_name,
					boost::join(command_strings, "\n")
				);
				if (had_commands_property &&
				    node != makefile_node &&
				    node->properties().dirty())
					to_delete.insert(node.get());
			}
		}

		for (auto node: to_delete)
		{
			log::verbose("Deleting node", node->string(), "(command line changed)");
			try {
				boost::filesystem::remove(node->path());
			} catch (...) {
				log::warning("Couldn't remove", node->string(), ":", error_string());
			}
		}
		this->include_dependencies(out, use_relpath);
	}

	void Makefile::include_dependencies(std::ostream& out, bool relative) const
	{
		if (relative)
			for (auto& node: _includes)
				out << "-include " << relative_node_path(_build, *node) << std::endl;
		else
			for (auto& node: _includes)
				out << "-include " << absolute_node_path(_build, *node) << std::endl;
	}

	std::string Makefile::dump_command(std::vector<std::string> const& cmd) const
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
			return res;
		}
#endif
		return quote<CommandParser::make>(cmd);
	}

	bool Makefile::is_available(Build& build)
	{ return build.fs().which("make") != boost::none; }

	std::vector<std::string>
	Makefile::build_command(std::string const& target) const
	{
		return {
			"make", "-C", _build.directory().string(),
			target.empty() ? "all" : target
		};
	}

	bool Makefile::use_relative_path() const { return true; }
}}

