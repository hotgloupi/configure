#include "touch.hpp"

#include <configure/Filesystem.hpp>
#include <configure/Process.hpp>
#include <configure/Platform.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

namespace configure { namespace commands {

	void touch(boost::filesystem::path const& dest)
	{
		if (auto touch = Filesystem::which("touch"))
			Process::check_call({touch->string(), dest.string()});
		else if (Platform::current().os() == Platform::OS::windows)
		{
			if (!boost::filesystem::exists(dest))
				std::ofstream(dest.string()).close();
			else
				Process::check_call(
				  {"copy", "/b", dest.string() + "+,,", dest.string()});
		}
	}

}}
