#include "Rule.hpp"

#include "Command.hpp"

namespace configure {

	Rule::Rule()
	{}

	Rule::~Rule()
	{}

	Rule& Rule::add_source(NodePtr source)
	{
		_sources.push_back(std::move(source));
		return *this;
	}

	Rule& Rule::add_sources(std::vector<NodePtr> const& sources)
	{
		_sources.reserve(_sources.size() + sources.size());
		_sources.insert(_sources.end(), sources.begin(), sources.end());
		return *this;
	}

	Rule& Rule::add_target(NodePtr target)
	{
		_targets.push_back(std::move(target));
		return *this;
	}

	Rule& Rule::add_targets(std::vector<NodePtr> const& targets)
	{
		_targets.reserve(_targets.size() + targets.size());
		_targets.insert(_targets.end(), targets.begin(), targets.end());
		return *this;
	}

	Rule& Rule::add_shell_command(ShellCommand command)
	{
		_shell_commands.push_back(std::move(command));
		return *this;
	}

	Rule& Rule::add_shell_commands(std::vector<ShellCommand> const& commands)
	{
		_shell_commands.reserve(_shell_commands.size() + commands.size());
		_shell_commands.insert(_shell_commands.end(), commands.begin(), commands.end());
		return *this;
	}

	CommandPtr Rule::command() const
	{
		return CommandPtr(new Command(_shell_commands));
	}

}
