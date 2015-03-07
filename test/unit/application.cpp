#include "tools/TemporaryDirectory.hpp"
#include <configure/Application.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

namespace fs = boost::filesystem;

typedef configure::Application app_t;

BOOST_AUTO_TEST_CASE(invalid_args)
{
	BOOST_CHECK_THROW(configure::Application(0, nullptr), std::exception);
	BOOST_CHECK_THROW(configure::Application(-1, nullptr), std::exception);
	BOOST_CHECK_THROW(
		(configure::Application(std::vector<std::string>())),
		std::exception
	);
}

BOOST_AUTO_TEST_CASE(missing_project)
{
	TemporaryDirectory env;
	BOOST_CHECK_THROW(app_t({"pif"}), std::exception);
}

BOOST_AUTO_TEST_CASE(no_build_dir)
{
	TemporaryDirectory env;
	env.create_file("configure.lua", "-- nothing\n");
	app_t app({"pif"});
	BOOST_CHECK_EQUAL(app.build_directories().size(), 0u);
	BOOST_CHECK_EQUAL(app.project_directory(), env.dir());
}
