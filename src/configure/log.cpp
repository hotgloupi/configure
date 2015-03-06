#include "log.hpp"

namespace configure { namespace log {

	static Level get_default_log_level()
	{
		if (getenv("CONFIGURE_DEBUG") != nullptr)
			return Level::debug;
		else if(getenv("CONFIGURE_VERBOSE") != nullptr)
			return Level::verbose;
		else return Level::status;
	}

	Level& level()
	{
		static Level value = get_default_log_level();
		return value;
	}

	bool is_enabled(Level lvl)
	{
		return static_cast<int>(lvl) >= static_cast<int>(level());
	}

}}
