#pragma once

#include "fwd.hpp"
#include "lua/traceback.hpp"

#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <stdexcept>

namespace configure {

	// Transform the current exception to string.
	std::string error_string();

	// Transform the given exception to string.
	std::string error_string(std::exception_ptr const& e,
	                         unsigned int indent = 0);


	// Returns errno on Unix platforms and GetLastError() on Windows.
	int last_error();

	// Throw an exception
#define CONFIGURE_THROW BOOST_THROW_EXCEPTION

	// Returns a SystemError instance filled with the last error code.
#define CONFIGURE_SYSTEM_ERROR(msg)                                           \
	::configure::error::SystemError(msg)                                      \
		<< ::configure::error::error_code(                                    \
			boost::system::error_code(                                        \
				::configure::last_error(),                                    \
				boost::system::system_category()                              \
			)                                                                 \
		)                                                                     \
/**/

    // Throw a system error with the last error code.
#define CONFIGURE_THROW_SYSTEM_ERROR(msg) \
	CONFIGURE_THROW(CONFIGURE_SYSTEM_ERROR(msg))

	namespace error {

		struct Base
			: virtual std::exception
			, virtual boost::exception
		{};

#define MAKE_EXCEPTION(name)                                                  \
	struct name : virtual Base                                                \
	{                                                                         \
		std::string _what;                                                    \
		explicit name(std::string what = "")                                  \
		    : _what(std::string(#name) +                                      \
		            (!what.empty() ? ": " + std::move(what) : std::string())) \
		{}                                                                    \
		virtual char const* what() const throw() { return _what.c_str(); }    \
	} /**/

		MAKE_EXCEPTION(BuildError);
		MAKE_EXCEPTION(CommandAlreadySet);
		MAKE_EXCEPTION(InvalidArgument);
		MAKE_EXCEPTION(InvalidEnviron);
		MAKE_EXCEPTION(InvalidGenerator);
		MAKE_EXCEPTION(InvalidKey);
		MAKE_EXCEPTION(InvalidNode);
		MAKE_EXCEPTION(InvalidOption);
		MAKE_EXCEPTION(InvalidPath);
		MAKE_EXCEPTION(InvalidProject);
		MAKE_EXCEPTION(InvalidRule);
		MAKE_EXCEPTION(InvalidSourceNode);
		MAKE_EXCEPTION(InvalidVirtualNode);
		MAKE_EXCEPTION(LuaError);
		MAKE_EXCEPTION(OptionAlreadySet);
		MAKE_EXCEPTION(PlatformError);
		MAKE_EXCEPTION(FileNotFound);
		MAKE_EXCEPTION(SystemError);
		MAKE_EXCEPTION(RuntimeError);

#undef MAKE_EXCEPTION
	} // !error

}

#include <boost/exception/detail/type_info.hpp>

#define MAKE_ERROR_INFO(tag, type) \
		namespace configure { namespace error { \
			namespace info { struct tag; } \
			typedef boost::error_info<struct info::tag, type> tag; \
		}} \
		namespace boost { \
			template<> inline std::string tag_type_name<configure::error::info::tag>() { return #tag; } \
		} \
/**/

		MAKE_ERROR_INFO(path, boost::filesystem::path);
		MAKE_ERROR_INFO(command, std::vector<std::string>);
		MAKE_ERROR_INFO(nested, std::exception_ptr);
		MAKE_ERROR_INFO(lua_function, std::string);
		MAKE_ERROR_INFO(lua_traceback, std::vector<lua::Frame>);
		MAKE_ERROR_INFO(node, NodePtr);
		MAKE_ERROR_INFO(error_code, boost::system::error_code);
		MAKE_ERROR_INFO(help, std::string);

#undef MAKE_ERROR_INFO

namespace configure { namespace error {
		template <typename E>
		nested make_nested(E&& e)
		{
			return nested(std::make_exception_ptr(std::forward<E>(e)));
		}
	}
}
