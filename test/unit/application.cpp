
#include <configure/Application.hpp>

#include <boost/filesystem.hpp>

#include <fstream>
#include <vector>

namespace fs = boost::filesystem;

typedef configure::Application app_t;

BOOST_AUTO_TEST_CASE(invalid_args)
{
	std::vector<std::string> empty_args;
	BOOST_CHECK_THROW(configure::Application(0, nullptr), std::exception);
	BOOST_CHECK_THROW(configure::Application(-1, nullptr), std::exception);
	BOOST_CHECK_THROW(configure::Application(empty_args), std::exception);
}

struct Env
{
	fs::path _dir;
	fs::path _old_cwd;

	Env()
		: _dir{fs::temp_directory_path() / fs::unique_path()}
		, _old_cwd{fs::current_path()}
	{
		fs::create_directories(_dir);
		fs::current_path(_dir);
	}

	~Env()
	{
		fs::current_path(_old_cwd);
		fs::remove_all(_dir);
	}

	void add_project()
	{
		std::ofstream out{(_dir / "configure.lua").string()};
		out << "something";
		out.close();
	}

	fs::path const& dir() { return _dir; }
};

BOOST_AUTO_TEST_CASE(missing_project)
{
	Env env;
	BOOST_CHECK_THROW(app_t({"pif"}), std::exception);
}

BOOST_AUTO_TEST_CASE(no_build_dir)
{
	Env env;
	env.add_project();
	app_t app({"pif"});
	BOOST_CHECK_EQUAL(app.build_directories().size(), 0);
	BOOST_CHECK_EQUAL(app.project_directory(), env.dir());
}
