#include "extract.hpp"

#include <configure/log.hpp>
#include <configure/Process.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace configure { namespace commands {

	void extract(boost::filesystem::path const& tarball,
	             boost::filesystem::path const& dest_dir)
	{
		auto ext = tarball.extension().string();
		if (ext == ".tgz" || boost::ends_with(tarball.string(), ".tar.gz"))
		{
			Process::Options options;
			Process::check_call({"tar", "-xf", tarball.string(), "-C",
			                     dest_dir.string(), "--strip-components=1"},
			                    options);
		}
		else if (ext == ".zip")
		{
			Process::Options options;
			Process::check_call({"unzip", tarball.string(), "-d", "-o",
			                     dest_dir.string()},
			                    options);
		}
		else
			throw std::runtime_error("Unknown tarball extension '" + ext + "'");
	}

}}
