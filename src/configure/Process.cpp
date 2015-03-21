#include "Process.hpp"
#include "log.hpp"
#include "Filesystem.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <cassert>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__DYNAMIC__)
#include <crt_externs.h>
#endif

namespace io = boost::iostreams;

namespace configure {

	namespace {

		char** get_environ()
		{
#if defined(__APPLE__) && defined(__DYNAMIC__)
			return *_NSGetEnviron();
#else
			return environ;
#endif
		}

		struct Pipe
		{
		private:
			io::file_descriptor_source _source;
			io::file_descriptor_sink _sink;

		public:
			Pipe()
			{
				int fds[2];
				if (::pipe(fds) == -1)
					throw std::runtime_error("pipe error");
				_source = io::file_descriptor_source(fds[0], io::file_descriptor_flags::close_handle);
				_sink = io::file_descriptor_sink(fds[1], io::file_descriptor_flags::close_handle);
			}

		public:
			io::file_descriptor_sink& sink() { return _sink; }
			io::file_descriptor_source& source() { return _source; }

		public:
			void close()
			{
				_sink.close();
				_source.close();
			}
		};

	} // !anonymous

	struct Process::Impl
	{
		Command const command;
		Options const options;
		boost::optional<ExitCode> exit_code;
		io::file_descriptor_source stdout_source;
		pid_t child;

		Impl(Command cmd, Options options)
			: command(_prepare_command(std::move(cmd)))
			, options(std::move(options))
			, exit_code(boost::none)
			, child(_create_child())
		{}

		pid_t _create_child()
		{
			char** env = nullptr;
			// Working directory
			if (this->options.working_directory)
			{
				throw "not there";
			}

			if (this->options.inherit_env)
			{
				env = get_environ();
			}

			std::unique_ptr<Pipe> stdout_pipe;
			if (this->options.stdout_ == Stream::PIPE)
			{
				stdout_pipe = std::unique_ptr<Pipe>(new Pipe);
			}

			log::debug("Spawning process:", boost::join(this->command, " "), "initializers");
			pid_t child = ::fork();
			if (child < 0)
			{
				throw std::runtime_error("fork()");
			}
			else if (child == 0) // Child
			{
				if (this->options.stdout_ == Stream::PIPE)
				{
				retry_dup2:
					int ret = ::dup2(stdout_pipe->sink().handle(), STDOUT_FILENO);
					if (ret == -1) {
						if (errno == EINTR) goto retry_dup2;
						::exit(EXIT_FAILURE);
					}
				}
				stdout_pipe.reset();
				std::vector<char const*> args;
				for (auto& arg: this->command)
					args.push_back(arg.c_str());
				args.push_back(nullptr);
				::execve(args[0], (char**) &args[0], env);
				::exit(EXIT_FAILURE);
			}
			else // Parent
			{
				if (this->options.stdout_ == Stream::PIPE)
					this->stdout_source = stdout_pipe->source();
			}
			return child;
		}

		Command _prepare_command(Command cmd)
		{
			cmd[0] = Filesystem::which(cmd[0]).get().string();
			return std::move(cmd);
		}

	};

	Process::Process(Command cmd, Options options)
		: _this(new Impl(std::move(cmd), std::move(options)))
	{}

	Process::~Process()
	{ this->wait(); }

	Process::Options const& Process::options() const
	{ return _this->options; }

	boost::optional<Process::ExitCode> Process::exit_code()
	{
		if (!_this->exit_code)
		{
			pid_t ret;
			int status;
			do
			{
				log::debug("Checking exit status of child", _this->child);
				ret = ::waitpid(_this->child, &status, WNOHANG);
			} while (ret == -1 && errno == EINTR);
			if (ret == -1)
				throw std::runtime_error("Waitpid failed");
			if (ret != 0)
			{
				log::debug("The child", _this->child,
				           "exited with status code", WEXITSTATUS(status));
				_this->exit_code = WEXITSTATUS(status);
			}
			else
				log::debug("The child", _this->child, "is still alive");
		}
		return _this->exit_code;
	}

	Process::ExitCode Process::wait()
	{
		while (!this->exit_code())
			log::debug("Waiting for child", _this->child, "to terminate");
		return _this->exit_code.get();
	}

	Process::ExitCode Process::call(Command cmd, Options options)
	{
		Process p(std::move(cmd), std::move(options));
		return p.wait();
	}

	std::string Process::check_output(Command cmd, Options options)
	{
		options.stdout_ = Stream::PIPE;
		Process p(std::move(cmd), std::move(options));

		char buf[4096];
		std::string res;
		std::streamsize size;
		auto& src = p._this->stdout_source;
		do {
			size = src.read(buf, sizeof(buf));
			if (size > 0)
			{
				log::debug("read", size, "bytes from child", p._this->child, "stdout");
				res.append(buf, size);
			}
		} while (size > 0);
		if (p.wait() != 0)
			throw std::runtime_error("Program failed");
		return res;
	}
}
