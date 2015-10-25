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
			auto tarball_resolved = boost::filesystem::canonical(tarball);
			tarball_resolved.make_preferred();
			auto dest_dir_resolved = boost::filesystem::canonical(dest_dir);
			dest_dir_resolved.make_preferred();
#ifdef _WIN32
			tarball_resolved = boost::replace_all_copy(tarball_resolved.string(), "\\", "/");
			dest_dir_resolved = boost::replace_all_copy(dest_dir_resolved.string(), "\\", "/");
#endif
			Process::Options options;
			Process::check_call({"tar", "-xf", tarball_resolved.string(), "-C",
			                     dest_dir_resolved.string(), "--strip-components=1",
#ifdef _WIN32
			                     // We force tar to ignore colon in file names (assuming gnu tar)
			                     "--force-local"
#endif
			                     },
			                    options);
		}
		else if (ext == ".zip")
		{
			Process::Options options;
			Process::check_call({"unzip", "-o", tarball.string(), "-d",
			                     dest_dir.string()},
			                    options);
		}
		else
			throw std::runtime_error("Unknown tarball extension '" + ext + "'");
	}

}}
