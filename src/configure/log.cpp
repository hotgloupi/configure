#include "log.hpp"

namespace configure { namespace log {

	Level& level()
	{
		static Level value = Level::status;
		return value;
	}

	bool is_enabled(Level lvl)
	{
		return static_cast<int>(lvl) >= static_cast<int>(level());
	}

}}
