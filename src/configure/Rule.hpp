#pragma once

#include "fwd.hpp"
#include "Node.hpp"
#include "ShellCommand.hpp"

#include <vector>

namespace configure {

	// Temporary object to ease rule construction.
	class Rule
	{
	private:
		std::vector<NodePtr>      _sources;
		std::vector<NodePtr>      _targets;
		std::vector<ShellCommand> _shell_commands;

	public:
		Rule();
		~Rule();

	public:
		Rule& add_source(NodePtr source);
		Rule& add_sources(std::vector<NodePtr> const& source);
		Rule& add_target(NodePtr target);
		Rule& add_targets(std::vector<NodePtr> const& targets);
		Rule& add_shell_command(ShellCommand command);
		Rule& add_shell_commands(std::vector<ShellCommand> const& commands);

	public:
		std::vector<NodePtr> const& sources() const { return _sources; }
		std::vector<NodePtr> const& targets() const { return _targets; }
		std::vector<ShellCommand> const& shell_commands() const
		{ return _shell_commands; }

	public:
		CommandPtr command() const;
	};

}
