#include <configure/quote.hpp>

using namespace configure;

typedef CommandParser P;

BOOST_AUTO_TEST_CASE(no_quote)
{
	BOOST_CHECK_EQUAL("abczLOL", quote<P::unix_shell>({"abczLOL"}));
	BOOST_CHECK_EQUAL("abczLOL", quote<P::windows_shell>({"abczLOL"}));
	BOOST_CHECK_EQUAL("abczLOL", quote<P::make>({"abczLOL"}));
	BOOST_CHECK_EQUAL("abczLOL", quote<P::nmake>({"abczLOL"}));
}

BOOST_AUTO_TEST_CASE(space)
{
	BOOST_CHECK_EQUAL("\" \"", quote<P::unix_shell>({" "}));
	BOOST_CHECK_EQUAL("\" \"", quote<P::windows_shell>({" "}));
	BOOST_CHECK_EQUAL("\" \"", quote<P::make>({" "}));
	BOOST_CHECK_EQUAL("\" \"", quote<P::nmake>({" "}));
}

BOOST_AUTO_TEST_CASE(dollar)
{
	std::string ref = "$";
	BOOST_CHECK_EQUAL("\"\\$\"", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("$", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("\"\\$$\"", quote<P::make>({ref}));
	BOOST_CHECK_EQUAL("$", quote<P::nmake>({ref}));

	ref = " $";
	BOOST_CHECK_EQUAL("\" \\$\"", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("\" $\"", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("\" $\"", quote<P::nmake>({ref}));
	BOOST_CHECK_EQUAL("\" \\$$\"", quote<P::make>({ref}));
}

BOOST_AUTO_TEST_CASE(backslash)
{
	std::string ref = "\\ ";
	BOOST_CHECK_EQUAL("\"\\\\ \"", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("\"\\\\ \"", quote<P::make>({ref}));
	BOOST_CHECK_EQUAL("\"\\ \"", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("\"\\ \"", quote<P::nmake>({ref}));

	ref = "\\";
	BOOST_CHECK_EQUAL("\"\\\\\"", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("\"\\\\\"", quote<P::make>({ref}));
	BOOST_CHECK_EQUAL("\\", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("\\", quote<P::nmake>({ref}));

	ref = "\\\"";
	BOOST_CHECK_EQUAL("\"\\\\\"\"", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("\"\\\\\"\"", quote<P::make>({ref}));
	BOOST_CHECK_EQUAL("\\\\\\\"", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("\\\\\\\"", quote<P::nmake>({ref}));
}

BOOST_AUTO_TEST_CASE(percent)
{
	std::string ref = "%";
	BOOST_CHECK_EQUAL("%", quote<P::unix_shell>({ref}));
	BOOST_CHECK_EQUAL("%", quote<P::make>({ref}));
	BOOST_CHECK_EQUAL("%%", quote<P::windows_shell>({ref}));
	BOOST_CHECK_EQUAL("%%", quote<P::nmake>({ref}));
}
