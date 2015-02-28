
#include <configure/PropertyMap.hpp>

BOOST_AUTO_TEST_CASE(dirtyness)
{
	configure::PropertyMap m;

	BOOST_CHECK_EQUAL(m.dirty(), false);

	m.set<std::string>("test", "lol");
	BOOST_CHECK_EQUAL(m.dirty(), true);

	m.mark_clean();
	BOOST_CHECK_EQUAL(m.dirty(), false);

	m.set<std::string>("test", "lol");
	BOOST_CHECK_EQUAL(m.dirty(), false);

	m.set<std::string>("test", "lol2");
	BOOST_CHECK_EQUAL(m.dirty(), true);
}
