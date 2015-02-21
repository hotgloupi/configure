#pragma once

#include <iostream>

namespace configure { namespace log {

	template<bool>
	void _print() { std::cout << std::endl; }

	template<bool is_first, typename T, typename... Args>
	void _print(T&& first, Args&&... tail)
	{
		if (!is_first) std::cout << ' ';
		std::cerr << first;
		_print<false>(std::forward<Args>(tail)...);
	}


	template<typename... Args>
	void print(Args&&... args) { _print<true>(std::forward<Args>(args)...); }

	enum class Level : int
	{ debug = 0, verbose, status, warning, error };

	Level& level();
	bool is_enabled(Level lvl);

	template<Level lvl>
	char const* _prefix()
	{
		switch (lvl)
		{
		case Level::debug: return "[DEBUG]";
		case Level::verbose: return "~~";
		case Level::status: return "--";
		case Level::warning: return "[WARNING]";
		case Level::error: return "[ERROR]";
		default: return "??";
		}
	}


	template<Level lvl, typename... Args>
	void log(Args&&... args)
	{ if (is_enabled(lvl)) print(_prefix<lvl>(), std::forward<Args>(args)...); }

	template<typename... Args>
	void debug(Args&&... args)
	{ log<Level::debug>(std::forward<Args>(args)...); }

	template<typename... Args>
	void verbose(Args&&... args)
	{ log<Level::verbose>(std::forward<Args>(args)...); }

	template<typename... Args>
	void status(Args&&... args)
	{ log<Level::status>(std::forward<Args>(args)...); }

	template<typename... Args>
	void warning(Args&&... args)
	{ log<Level::warning>(std::forward<Args>(args)...); }

	template<typename... Args>
	void error(Args&&... args)
	{ log<Level::error>(std::forward<Args>(args)...); }

}}
