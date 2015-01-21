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

}
