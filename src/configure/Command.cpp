#include "Command.hpp"

namespace configure {

	Command::Command(std::vector<ShellCommand> shell_commands)
		: _shell_commands(shell_commands)
	{}

	Command::~Command() {}

}
