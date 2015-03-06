#include <configure/error.hpp>

#include <boost/filesystem/path.hpp>

namespace configure { namespace utils {

	boost::filesystem::path
	relative_path(boost::filesystem::path const& full,
	              boost::filesystem::path const& start)
	{
		// Stolen from http://stackoverflow.com/questions/10167382/boostfilesystem-get-relative-path
		if (!full.is_absolute())
			CONFIGURE_THROW(
				error::InvalidPath("path must be absolute when computing relative path")
					<< error::path(full)
			);

		if (!start.is_absolute())
			CONFIGURE_THROW(
				error::InvalidPath("start path must be absolute when computing relative path")
					<< error::path(start)
			);
		boost::filesystem::path ret;
		auto it_start = start.begin(), end_start = start.end();
		auto it_full = full.begin(), end_full = full.end();
		// Find common base
		while (it_start != end_start && it_full != end_full && *it_start == *it_full)
		{
			++it_start;
			++it_full;
		}

		// Navigate backwards in directory to reach previously found base
		for (; it_start != end_start; ++it_start )
		{
			if( (*it_start) != "." )
				ret /= "..";
		}

		// Now navigate down the directory branch
		for (; it_full != end_full; ++it_full)
			ret /= *it_full;
		if (ret.empty())
			return ".";
		return ret;
	}

	bool starts_with(boost::filesystem::path const& path,
	                 boost::filesystem::path const& prefix)
	{
		auto it = path.begin(), end = path.end();
		for (auto&& p: prefix)
		{
			if (it == end) return false;
			if (p == ".") continue;
			if (*it != p) return false;
			++it;
		}
		return true;
	}

}}

