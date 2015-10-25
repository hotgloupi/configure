#pragma once

#include <vector>
#include <string>

namespace configure {

	enum class CommandParser {
		unix_shell,
		windows_shell,
		nmake,
		make,
	};

	template<CommandParser target>
	std::string quote(std::vector<std::string> const& cmd);

	template<CommandParser target>
	std::string quote_arg(std::string const& arg);

	std::string quote_arg(CommandParser target, std::string const& arg);
}
