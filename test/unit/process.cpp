#include <configure/Process.hpp>
#include <configure/error.hpp>

#include <iostream>

using configure::Process;

#ifdef _WIN32
# define LS_COMMAND {"cmd", "/k", "dir"}
#else
# define LS_COMMAND {"ls"}
#endif

BOOST_AUTO_TEST_CASE(ls)
{
	auto out = Process::check_output(LS_COMMAND);
	BOOST_CHECK(!out.empty());
}

BOOST_AUTO_TEST_CASE(empty_output)
{
#ifdef BOOST_POSIX_API
		auto out = Process::check_output({"true"});
		BOOST_CHECK(out.empty());
#endif
}
