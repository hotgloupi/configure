#include <configure/utils/path.hpp>
#include <configure/error.hpp>

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_CASE(path)
{
# define P1 "pif"
# define P2 "paf"
#ifdef _WIN32
# define ABS_P1 "c:\\pif"
# define ABS_P2 "c:/paf"
#else
# define ABS_P1 "/pif"
# define ABS_P2 "/paf"
#endif
	using namespace configure::utils;
	BOOST_CHECK_THROW(
	    relative_path(P1, ABS_P2),
	    configure::error::InvalidPath
	);
	BOOST_CHECK_THROW(
	    relative_path(ABS_P1, P2),
	    configure::error::InvalidPath
	);

	BOOST_CHECK_EQUAL(
	    relative_path(ABS_P1, ABS_P2),
	    fs::path("..") / P1
	);

	BOOST_CHECK_EQUAL(
	    relative_path(ABS_P2, ABS_P1),
	    fs::path("..") / P2
	);

	BOOST_CHECK_EQUAL(
	    relative_path(ABS_P1, ABS_P1),
	    "."
	);

	BOOST_CHECK_EQUAL(
	    relative_path(ABS_P2, ABS_P2),
	    "."
	);
}
