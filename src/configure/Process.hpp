#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include <memory>
#include <vector>
#include <string>

namespace configure {

	class Process
	{
	public:
		typedef int ExitCode;
		typedef std::vector<std::string> Command;
		enum class Stream
		{
			STDIN,
			STDOUT,
			STDERR,
			PIPE,
			DEVNULL,
		};
		struct Options
		{
			boost::optional<boost::filesystem::path> working_directory;
			Stream stdin_;
			Stream stdout_;
			Stream stderr_;
			bool inherit_env;

			Options()
				: stdin_(Stream::STDIN)
				, stdout_(Stream::STDOUT)
				, stderr_(Stream::STDERR)
				, inherit_env(true)
			{}
		};

	private:
		struct Impl;
		std::unique_ptr<Impl> _this;

	public:
		Process(Command cmd, Options options);
		~Process();

		Options const& options() const;

		boost::optional<ExitCode> exit_code();

		ExitCode wait();

	public:

		static ExitCode call(Command cmd, Options options = Options());
		static std::string
		check_output(Command cmd, Options options = Options());
	};

}
