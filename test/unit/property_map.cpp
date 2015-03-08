
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

BOOST_AUTO_TEST_CASE(dirty_key)
{
	configure::PropertyMap m;

	BOOST_CHECK_EQUAL(m.dirty("test"), false);
	m.set<std::string>("test", "lol");
	BOOST_CHECK_EQUAL(m.dirty("test"), true);
	BOOST_CHECK_EQUAL(m.dirty("TEST"), true);
	m.mark_clean();
	BOOST_CHECK_EQUAL(m.dirty("TEST"), false);
	BOOST_CHECK_EQUAL(m.dirty("test"), false);
	m.set<std::string>("tesT", "lol");
	BOOST_CHECK_EQUAL(m.dirty("TEST"), false);
	BOOST_CHECK_EQUAL(m.dirty("test"), false);

	m.set<std::string>("TesT", "lol2");
	BOOST_CHECK_EQUAL(m.dirty("test"), true);
	BOOST_CHECK_EQUAL(m.dirty("Test"), true);
	BOOST_CHECK_EQUAL(m.dirty("tesT"), true);
}
