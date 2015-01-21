#pragma once

#include "lua/traceback.hpp"

#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem/path.hpp>

#include <string>
#include <stdexcept>

namespace configure {

	std::string error_string();
	std::string error_string(std::exception_ptr const& e);


# define CONFIGURE_THROW BOOST_THROW_EXCEPTION

	namespace error {

		struct Base
			: virtual std::exception
			, virtual boost::exception
		{};

#define MAKE_EXCEPTION(name) \
		struct name : virtual Base { \
			std::string _what; \
			explicit name(std::string what = "") \
				: _what(std::string(#name) + (!what.empty() ? ": " + std::move(what) : std::string())) \
			{} \
			virtual char const* what() const throw() { return _what.c_str(); } \
		} \
/**/

		MAKE_EXCEPTION(CommandAlreadySet);
		MAKE_EXCEPTION(InvalidEnviron);
		MAKE_EXCEPTION(InvalidGenerator);
		MAKE_EXCEPTION(InvalidKey);
		MAKE_EXCEPTION(InvalidOption);
		MAKE_EXCEPTION(InvalidPath);
		MAKE_EXCEPTION(InvalidProject);
		MAKE_EXCEPTION(InvalidRule);
		MAKE_EXCEPTION(InvalidSourceNode);
		MAKE_EXCEPTION(InvalidVirtualNode);
		MAKE_EXCEPTION(LuaError);
		MAKE_EXCEPTION(OptionAlreadySet);
		MAKE_EXCEPTION(PlatformError);

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
		MAKE_ERROR_INFO(nested, std::exception_ptr);
		MAKE_ERROR_INFO(lua_function, std::string);
		MAKE_ERROR_INFO(lua_traceback, std::vector<lua::Frame>);

#undef MAKE_ERROR_INFO

