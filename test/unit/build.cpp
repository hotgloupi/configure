#include "tools/TemporaryProject.hpp"

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

using namespace configure;

namespace {

	fs::path const& test_dir()
	{
		static fs::path res = fs::absolute(fs::path(__FILE__).parent_path());
		return res;
	}

	fs::path lua_test(std::string const& name)
	{
		return test_dir() / "lua" / (name + ".lua");
	}
}

BOOST_AUTO_TEST_CASE(empty)
{
	TemporaryDirectory temp;
	lua::State state;
	Build build(state, temp.dir());
	BOOST_CHECK_THROW(build.project_directory(), std::exception);
	BOOST_CHECK_EQUAL(build.directory(), temp.dir());
}

static char const* empty_configure =
  "function configure(build)\nend"
;

BOOST_AUTO_TEST_CASE(simple)
{
	TemporaryDirectory temp;
	temp.create_file("configure.lua", empty_configure);
	lua::State state;
	Build build(state, temp.dir() / "build");
	build.configure(temp.dir());
	BOOST_CHECK_EQUAL(build.directory(), temp.dir() / "build");
	BOOST_CHECK_THROW(build.project_directory(), std::exception);
}

BOOST_AUTO_TEST_CASE(configure_invalid_project_dir)
{
	TemporaryDirectory temp;
	lua::State state;
	Build build(state, temp.dir());

	// Try to pass a file instead of a directory
	temp.create_file("FILE");
	BOOST_CHECK_THROW(build.configure(temp.dir() / "FILE"), error::InvalidProject);

	// Inexistent directory
	BOOST_CHECK_THROW(build.configure(temp.dir() / "NOT_HERE"), error::InvalidProject);

	// Relative path for first project
	BOOST_CHECK_THROW(build.configure("RELATIVE"), error::InvalidProject);

	// Same as build dir
	BOOST_CHECK_THROW(build.configure(temp.dir()), error::InvalidProject);
}

BOOST_AUTO_TEST_CASE(inexistent_source_node)
{
	TemporaryProject project(
	    "function configure(build)\n"
	    "  build:source_node(Path:new('NOT_THERE'))\n"
	    "end"
	);
	BOOST_CHECK_THROW(project.configure(), error::LuaError);
}

BOOST_AUTO_TEST_CASE(empty_option)
{
	TemporaryDirectory temp;
	lua::State state;
	Build build(state, temp.dir() / "build");
	BOOST_CHECK_EQUAL(
		build.option<std::string>("key", "description"),
		boost::none
	);
}

BOOST_AUTO_TEST_CASE(default_option)
{
	TemporaryDirectory temp;
	lua::State state;
	Build build(state, temp.dir() / "build");
	BOOST_CHECK_EQUAL(
		build.option<std::string>("key", "description", "default value"),
		"default value"
	);
	BOOST_CHECK_EQUAL(
		build.option<std::string>("key", "description").get(),
		std::string("default value")
	);
}

BOOST_AUTO_TEST_CASE(option_read_environ)
{
	TemporaryDirectory temp;
	lua::State state;
	Build build(state, temp.dir() / "build");
	build.env().set<std::string>("key", "some value");
	BOOST_CHECK_EQUAL(
		build.option<std::string>("key", "description", "default value"),
		"some value"
	);

}
