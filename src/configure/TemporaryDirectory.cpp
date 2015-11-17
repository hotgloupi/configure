#include <configure/TemporaryDirectory.hpp>
#include <configure/log.hpp>
#include <configure/error.hpp>

namespace configure
{
	namespace fs = boost::filesystem;

	TemporaryDirectory::TemporaryDirectory()
	    : _dir{boost::filesystem::temp_directory_path() /
	           boost::filesystem::unique_path()}
	{
		boost::filesystem::create_directories(_dir);
		_dir = boost::filesystem::canonical(_dir);
	}

	TemporaryDirectory::~TemporaryDirectory()
	{
		try
		{
			boost::filesystem::remove_all(_dir);
		}
		catch (...)
		{
			log::warning("Couldn't cleanup '" + _dir.string() + "': " +
			             error_string());
		}
	}

	fs::path const& TemporaryDirectory::path() const { return _dir; }
}
