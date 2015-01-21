#pragma once

#include <iostream>

namespace configure { namespace log {

	template<bool>
	void _print() { std::cout << std::endl; }

	template<bool is_first, typename T, typename... Args>
	void _print(T&& first, Args&&... tail)
	{
		if (!is_first) std::cout << ' ';
		std::cout << first;
		_print<false>(std::forward<Args>(tail)...);
	}


	template<typename... Args>
	void print(Args&&... args) { _print<true>(std::forward<Args>(args)...); }

	enum class Level
	{ debug, verbose, status, warning, error, fatal };

	template<Level lvl, typename... Args>
	void log(Args&&... args) { print(std::forward<Args>(args)...); }

	template<typename... Args>
	void debug(Args&&... args)
	{ log<Level::debug>("[DEBUG]", std::forward<Args>(args)...); }

	template<typename... Args>
	void verbose(Args&&... args)
	{ log<Level::verbose>("~~", std::forward<Args>(args)...); }

	template<typename... Args>
	void status(Args&&... args)
	{ log<Level::status>("--", std::forward<Args>(args)...); }

	template<typename... Args>
	void warning(Args&&... args)
	{ log<Level::warning>("[WARNING]", std::forward<Args>(args)...); }

	template<typename... Args>
	void error(Args&&... args)
	{ log<Level::error>("[ERROR]", std::forward<Args>(args)...); }

	template<typename... Args>
	void fatal(Args&&... args)
	{ log<Level::fatal>("[FATAL]", std::forward<Args>(args)...); }

}}
