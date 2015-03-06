#include <configure/utils/path.hpp>
#include <configure/error.hpp>


BOOST_AUTO_TEST_CASE(path)
{
	using namespace configure::utils;
	BOOST_CHECK_THROW(
	    relative_path("pif", "/paf"),
	    configure::error::InvalidPath
	);
	BOOST_CHECK_THROW(
	    relative_path("/pif", "paf"),
	    configure::error::InvalidPath
	);

	BOOST_CHECK_EQUAL(
	    relative_path("/pif", "/paf"),
	    "../pif"
	);

	BOOST_CHECK_EQUAL(
	    relative_path("/pif", "/pif"),
	    "."
	);
}
