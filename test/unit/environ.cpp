#include "tools/TemporaryDirectory.hpp"

#include <configure/Environ.hpp>
#include <configure/error.hpp>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace configure;

BOOST_AUTO_TEST_CASE(normalize)
{
#define N Environ::normalize

	BOOST_CHECK_EQUAL(N("A"), "A");
	BOOST_CHECK_EQUAL(N("a"), "A");
	BOOST_CHECK_EQUAL(N("aA"), "AA");
	BOOST_CHECK_EQUAL(N("a-A"), "A_A");
	BOOST_CHECK_EQUAL(N("a_A"), "A_A");
	BOOST_CHECK_EQUAL(N("zZ"), "ZZ");
	BOOST_CHECK_EQUAL(N("a-0"), "A_0");
	BOOST_CHECK_EQUAL(N("a-9"), "A_9");

	BOOST_CHECK_THROW(N("0A"), error::InvalidKey);
	BOOST_CHECK_THROW(N("_A"), error::InvalidKey);
	BOOST_CHECK_THROW(N("-A"), error::InvalidKey);
	BOOST_CHECK_THROW(N(""), error::InvalidKey);
	BOOST_CHECK_THROW(N("--"), error::InvalidKey);
	BOOST_CHECK_THROW(N("_"), error::InvalidKey);
	BOOST_CHECK_THROW(N("a_"), error::InvalidKey);
	BOOST_CHECK_THROW(N("a-"), error::InvalidKey);
	BOOST_CHECK_THROW(N("abé"), error::InvalidKey);
	BOOST_CHECK_THROW(N("test~"), error::InvalidKey)
	BOOST_CHECK_THROW(N("test#"), error::InvalidKey)
	BOOST_CHECK_THROW(N("test$"), error::InvalidKey)
	BOOST_CHECK_THROW(N("test%"), error::InvalidKey)
	BOOST_CHECK_THROW(N("test!"), error::InvalidKey)
	BOOST_CHECK_THROW(N("t e s t"), error::InvalidKey)
	BOOST_CHECK_THROW(N(" test"), error::InvalidKey)
	BOOST_CHECK_THROW(N("test "), error::InvalidKey)

#undef N
}


BOOST_AUTO_TEST_CASE(ignore_case)
{
	Environ e;
	e.set_default<std::string>("test-lol", "lol");
	BOOST_CHECK_EQUAL(e.get<std::string>("TEST_LOL"), "lol");
	BOOST_CHECK_EQUAL(e.get<std::string>("test-lol"), "lol");
	BOOST_CHECK_EQUAL(e.get<std::string>("TEST-lol"), "lol");

	e.set<std::string>("test-lol", "lal");
	BOOST_CHECK_EQUAL(e.get<std::string>("TEST_LOL"), "lal");
	BOOST_CHECK_EQUAL(e.get<std::string>("test-lol"), "lal");
	BOOST_CHECK_EQUAL(e.get<std::string>("TEST-lol"), "lal");
}


BOOST_AUTO_TEST_CASE(set_default)
{
	Environ e;
	e.set_default<std::string>("test", "lol");
	BOOST_CHECK_EQUAL(e.get<std::string>("test"), "lol");
	e.set_default<std::string>("test", "not funny");
	BOOST_CHECK_EQUAL(e.get<std::string>("test"), "lol");
}

BOOST_AUTO_TEST_CASE(all_types)
{
#define CHECK(kind_, type, value) \
	{ \
		Environ e; \
		BOOST_CHECK(!e.has("test")); \
		BOOST_CHECK(!e.has("test_1")); \
		BOOST_CHECK(!e.has("test-2")); \
		BOOST_CHECK(!e.has("test3")); \
		BOOST_CHECK_EQUAL(e.get<type>("test", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test_1", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test-2", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test3", value), value); \
		e.set<type>("test", value); \
		e.set<type>("test_1", value); \
		e.set<type>("test-2", value); \
		e.set<type>("test3", value); \
		BOOST_CHECK_EQUAL(e.get<type>("test", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test_1", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test-2", value), value); \
		BOOST_CHECK_EQUAL(e.get<type>("test3", value), value); \
		BOOST_CHECK(e.has("test")); \
		BOOST_CHECK(e.has("Test")); \
		BOOST_CHECK(e.has("Test-1")); \
		BOOST_CHECK(e.has("Test_1")); \
		BOOST_CHECK(e.has("Test_2")); \
		BOOST_CHECK(e.has("TEST_2")); \
		BOOST_CHECK(e.has("TEST3")); \
		BOOST_CHECK_EQUAL(e.kind("test"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("Test"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("Test-1"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("Test_1"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("Test_2"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("TEST_2"), Environ::Kind::kind_); \
		BOOST_CHECK_EQUAL(e.kind("TEST3"), Environ::Kind::kind_); \
	} \
/**/
	CHECK(boolean, bool, true);
	CHECK(boolean, bool, false);
	CHECK(integer, int, 42);
	CHECK(integer, int, 0);
	CHECK(path, fs::path, "test");
	CHECK(string, std::string, "test");
#undef CHECK_HAS
}

BOOST_AUTO_TEST_CASE(serialize)
{
	TemporaryDirectory temp;
	{
		Environ e;
		e.set<bool>("b1", true);
		e.set<bool>("b2", false);
		e.set<std::string>("s1", "custom string éàô");
		e.set<int>("i1", 42);
		e.set<fs::path>("p1", "pif/paf/pouf");
		e.save(temp.dir() / "env");
	}
	{
		Environ e;
		e.load(temp.dir() / "env");
		BOOST_CHECK_EQUAL(e.get<bool>("b1"), true);
		BOOST_CHECK_EQUAL(e.get<bool>("b2"), false);
		BOOST_CHECK_EQUAL(e.get<std::string>("s1"), "custom string éàô");
		BOOST_CHECK_EQUAL(e.get<int>("i1"), 42);
		BOOST_CHECK_EQUAL(e.get<fs::path>("p1"), "pif/paf/pouf");
	}
}
