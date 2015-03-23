#include <configure/Process.hpp>

#include <iostream>

using configure::Process;

#ifdef _WIN32
# define LS_COMMAND "dir"
#else
# define LS_COMMAND "ls"
#endif

BOOST_AUTO_TEST_CASE(ls)
{
	for (int i = 0; i < 1000; i++)
	{
		auto out = Process::check_output({"ls"});
		BOOST_CHECK(!out.empty());
	}
}

BOOST_AUTO_TEST_CASE(empty_output)
{
#ifdef BOOST_POSIX_API
		auto out = Process::check_output({"true"});
		BOOST_CHECK(out.empty());
#endif
}
