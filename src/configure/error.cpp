#include "error.hpp"
#include "Node.hpp"

#include <boost/config.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/exception/diagnostic_information.hpp>

#ifdef BOOST_WINDOWS_API
# include <Windows.h>
#else
# include <errno.h>
#endif

namespace configure {

	int last_error()
	{
#ifdef BOOST_WINDOWS_API
		return ::GetLastError();
#else
		return errno;
#endif
	}

	std::string error_string()
	{ return error_string(std::current_exception()); }

	static std::string what(std::exception_ptr const& e)
	{
		try { std::rethrow_exception(e); }
		catch (std::exception const& err) { return err.what(); }
		catch (...) { return "Unknown error type"; }
		std::abort();
	}

	std::string error_string(std::exception_ptr const& e, unsigned int indent)
	{
		using boost::get_error_info;
		std::string padding(indent, ' ');
		std::string res = padding + what(e) + "\n";
		try { std::rethrow_exception(e); }
		catch (boost::exception const& e) {
			if (auto ptr = get_error_info<error::path>(e))
				res += padding + "  Path: " + ptr->string() + "\n";
			if (auto ptr = get_error_info<error::lua_function>(e))
				res += padding + "  Lua function: " + *ptr + "\n";
			if (auto ptr = get_error_info<error::node>(e))
				res += padding + "  Node: " + (*ptr)->string() + "\n";
			if (auto ptr = get_error_info<error::lua_traceback>(e))
			{
				res += padding + "  Lua traceback:\n";
				for (auto const& frame: *ptr)
				{
					if (frame.builtin &&frame.name.empty())
						continue;
					res += padding + "    ";
					if (frame.builtin)
						res += "{builtin}: ";
					else
						res += frame.source.string() + ":" +
						       std::to_string(frame.current_line) + ": ";

					res += "in ";
					if (!frame.kind.empty())
						res += frame.kind + " ";
					if (frame.kind != "method")
						res += "function ";
					if (!frame.name.empty())
						res += frame.name + "() ";
					if (!frame.builtin)
						res += "defined at " +
						       frame.source.filename().string() + ":" +
						       std::to_string(frame.first_function_line);
					res += "\n";
				}
			}
			if (auto ptr = get_error_info<error::command>(e))
				res += padding + "  Command: " + boost::join(*ptr, " ") + "\n";
			if (auto ptr = get_error_info<error::error_code>(e))
				res += padding + "  " + ptr->category().name() + " error: " +
				       std::to_string(ptr->value()) + " (" + ptr->message() +
				       ")\n";
			if (auto ptr = get_error_info<error::nested>(e))
				res += padding + "  Initial error: " +
				       boost::trim_left_copy(error_string(*ptr, indent + 2)) + "\n";
			if (auto ptr = get_error_info<error::help>(e))
				res += padding + "  Help: " + (*ptr) + "\n";
		}
		catch (...) {}
		return boost::trim_right_copy(res);
	}

}
