#include "fetch.hpp"

#include <configure/Filesystem.hpp>
#include <configure/ShellCommand.hpp>
#include <configure/error.hpp>
#include <configure/Process.hpp>

namespace configure { namespace commands {

	namespace fs = boost::filesystem;

	void fetch(std::string const& uri, boost::filesystem::path const& dest)
	{
		ShellCommand cmd;
		if (auto wget = Filesystem::which("wget"))
			cmd.append(*wget, "--no-check-certificate", uri, "-O", dest);
		else if (auto curl = Filesystem::which("curl"))
			cmd.append(*curl, "-L", uri, "-o", dest);
		else if (auto powershell = Filesystem::which("powershell.exe"))
			cmd.append(*powershell, "-command",
				       "(new-object System.Net.WebClient).DownloadFile('" +
				         uri + "', '" + dest.string() + "')");
		else
			CONFIGURE_THROW(
				error::FileNotFound("Couldn't find any download program")
				<< error::help("Please install curl or wget")
			);

		Process::Options options;
		Process::check_call(cmd.dump(), options);
	}

}}
