#pragma once

#include <boost/filesystem.hpp>

#include <fstream>

namespace fs = boost::filesystem;

struct TemporaryDirectory
{
	fs::path _dir;
	fs::path _old_cwd;

	TemporaryDirectory()
		: _dir{fs::temp_directory_path() / fs::unique_path()}
		, _old_cwd{fs::current_path()}
	{
		fs::create_directories(_dir);
		_dir = fs::canonical(_dir);
		fs::current_path(_dir);
	}

	~TemporaryDirectory()
	{
		fs::current_path(_old_cwd);
		fs::remove_all(_dir);
	}

	void create_file(fs::path const& p)
	{
		std::ofstream out{(_dir / p).string()};
		out.close();
	}

	void create_file(fs::path const& p, std::string const& content)
	{
		std::ofstream out{(_dir / p).string()};
		out << content;
		out.close();
	}

	fs::path const& dir() { return _dir; }
};
