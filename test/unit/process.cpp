#include <configure/Process.hpp>

using configure::Process;

#ifdef _WIN32
# define LS_COMMAND "dir"
#else
# define LS_COMMAND "ls"
#endif

BOOST_AUTO_TEST_CASE(ls)
{
	auto out = Process::check_output({LS_COMMAND});
	BOOST_CHECK(!out.empty());
}
