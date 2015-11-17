#pragma once

#include <boost/filesystem.hpp>

namespace configure
{
	class TemporaryDirectory
	{
	private:
		boost::filesystem::path _dir;

	public:
		TemporaryDirectory();
		~TemporaryDirectory();

	public:
		boost::filesystem::path const& path() const;
	};
}
