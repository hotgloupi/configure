#pragma once

#include "fwd.hpp"

#include "ShellCommand.hpp"

#include <vector>

namespace configure {

	// Store one or more commands used to generate a file
	class Command
	{
	private:
		std::vector<ShellCommand> _shell_commands;

	public:
		Command(std::vector<ShellCommand> shell_commands);
		virtual ~Command();

	public:
		std::vector<ShellCommand> const& shell_commands() const
		{ return _shell_commands; }
	};

}
