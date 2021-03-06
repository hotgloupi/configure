#include "Process.hpp"
#include "log.hpp"
#include "Filesystem.hpp"
#include "quote.hpp"
#include "error.hpp"

#include <boost/config.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <cassert>
#include <stdexcept>
#include <inttypes.h>
#include <string.h>

#if defined(BOOST_POSIX_API)
# include <sys/wait.h>
# include <unistd.h>
# if defined(__APPLE__) && defined(__DYNAMIC__)
#  include <crt_externs.h> // For _NSGetEnviron()
# endif
#elif defined(BOOST_WINDOWS_API)
# include <Windows.h>
# include <io.h>
#else
# error "Unsupported platform"
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

#ifdef BOOST_WINDOWS_API
		typedef HANDLE file_descriptor_t;
#else
		typedef int file_descriptor_t;
#endif

		struct Pipe
		{
		private:
			io::file_descriptor_source _source;
			io::file_descriptor_sink _sink;

		public:
			Pipe()
			{
				file_descriptor_t fds[2];
#ifdef BOOST_WINDOWS_API
				if (!::CreatePipe(&fds[0], &fds[1], NULL, 0))
					CONFIGURE_THROW_SYSTEM_ERROR("CreatePipe()");
#else
				if (::pipe(fds) == -1)
					CONFIGURE_THROW_SYSTEM_ERROR("pipe()");
#endif
				_source = io::file_descriptor_source(
				  fds[0], io::file_descriptor_flags::close_handle);
				_sink = io::file_descriptor_sink(
				  fds[1], io::file_descriptor_flags::close_handle);
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

#ifdef BOOST_WINDOWS_API
		struct Child
		{
		public:
			typedef HANDLE process_handle_type;
		private:
			PROCESS_INFORMATION _proc_info;

		public:
			explicit Child(PROCESS_INFORMATION const& proc_info)
			    : _proc_info(proc_info)
			{}

			Child(Child&& other)
				: _proc_info(other._proc_info)
			{
				other._proc_info.hProcess = INVALID_HANDLE_VALUE;
				other._proc_info.hThread = INVALID_HANDLE_VALUE;
			}

			~Child()
			{
				::CloseHandle(_proc_info.hProcess);
				::CloseHandle(_proc_info.hThread);
			}

			process_handle_type process_handle() const { return _proc_info.hProcess; }
			bool wait(int ms, int& exit_code)
			{
				int ret = ::WaitForSingleObject(
					this->process_handle(),
					(ms < 0 ? INFINITE : ms)
				);
				switch (ret)
				{
				case WAIT_FAILED:
					CONFIGURE_THROW_SYSTEM_ERROR("WaitForSingleObject()");
				case WAIT_ABANDONED:
					log::warning(
					  "Wait on child", *this, "has been abandoned");
					break;
				case WAIT_TIMEOUT:
					log::debug("The child", *this, "is still alive");
					break;
				case WAIT_OBJECT_0:
				{
					DWORD exit_code_out;
					if (!::GetExitCodeProcess(this->process_handle(), &exit_code_out))
						CONFIGURE_THROW_SYSTEM_ERROR("GetExitCodeProcess()");
					log::debug("The child", *this,
							   "exited with status code", exit_code_out);
					exit_code = exit_code_out;
					return true;
				}
				}
				return false;
			}
		};
#else
		struct Child
		{
		public:
			typedef pid_t process_handle_type;
		private:
			process_handle_type _pid;

		public:
			explicit Child(process_handle_type pid) : _pid(pid) {}
			process_handle_type process_handle() const { return _pid; }
			bool wait(int ms, int& exit_code)
			{
				pid_t ret;
				int status;
				int options = 0;
				if (ms == 0)
					options = WNOHANG;
				else if (options > 0)
					assert(false && "not implemented");
				do
				{
					log::debug("Checking exit status of child", *this);
					ret = ::waitpid(this->process_handle(), &status, options);
				} while (ret == -1 && errno == EINTR);
				if (ret == -1)
					CONFIGURE_THROW_SYSTEM_ERROR("waitpid()");
				if (ret != 0)
				{
					log::debug("The child", *this,
							   "exited with status code", WEXITSTATUS(status));
					exit_code = WEXITSTATUS(status);
					return true;
				}
				else
					log::debug("The child", *this, "is still alive");
				return false;
			}
		};
#endif

		std::ostream& operator <<(std::ostream& out, Child const& child)
		{
			return out << "<Process " << child.process_handle() << ">";
		}

	} // !anonymous

	struct Process::Impl
	{
		Command const command;
		Options const options;
		boost::optional<ExitCode> exit_code;
		io::file_descriptor_sink stdin_sink;
		io::file_descriptor_source stdout_source;
		io::file_descriptor_source stderr_source;
		Child child;

		Impl(Command cmd, Options options)
			: command(_prepare_command(std::move(cmd)))
			, options(std::move(options))
			, exit_code(boost::none)
			, child(_create_child())
		{
			log::debug("Spawn process for command:", boost::join(this->command, " "));
		}

#ifdef BOOST_POSIX_API
		pid_t _create_child()
		{
			char** env = {NULL};

			if (this->options.inherit_env)
			{
				env = get_environ();
			}

			std::unique_ptr<Pipe> stdin_pipe;
			std::unique_ptr<Pipe> stdout_pipe;
			std::unique_ptr<Pipe> stderr_pipe;
			std::tuple<bool, Stream, std::unique_ptr<Pipe>&, int> channels[] = {
				std::make_tuple(false,
				                this->options.stdin_,
				                std::ref(stdin_pipe),
				                STDIN_FILENO),
				std::make_tuple(true,
				                this->options.stdout_,
				                std::ref(stdout_pipe),
				                STDOUT_FILENO),
				std::make_tuple(true,
				                this->options.stderr_,
				                std::ref(stderr_pipe),
				                STDERR_FILENO),
			};

			for (auto& channel: channels)
				if (std::get<1>(channel) == Stream::PIPE)
				{
					log::debug("Creating pipe for", std::get<3>(channel));
					std::get<2>(channel).reset(new Pipe);
				}

			log::debug("Spawning process:", boost::join(this->command, " "));
			pid_t child = ::fork();
			if (child < 0)
			{
				CONFIGURE_THROW_SYSTEM_ERROR("fork()");
			}
			else if (child == 0) // Child
			{
				if (this->options.working_directory)
				{
					auto const& dir = this->options.working_directory.get();
					if (::chdir(dir.c_str()) < 0)
					{
						log::error("Cannot set working directory to '" +
						             dir.string() + "':",
						           ::strerror(errno));
						::exit(EXIT_FAILURE);
					}
				}
				for (auto& channel: channels)
				{
					Stream kind = std::get<1>(channel);
					bool is_sink = std::get<0>(channel);
					int old_fd = std::get<3>(channel);
					if (kind == Stream::PIPE)
					{
						int new_fd = (is_sink ?
						              std::get<2>(channel)->sink().handle() :
						              std::get<2>(channel)->source().handle());
					retry_dup2:
						int ret = ::dup2(new_fd, old_fd);
						if (ret == -1) {
							if (errno == EINTR) goto retry_dup2;
							::exit(EXIT_FAILURE);
						}
					}
					else if (kind == Stream::DEVNULL)
					{
						log::debug("Closing", old_fd);
						::close(old_fd);
					}
				}
				stdin_pipe.reset();
				stdout_pipe.reset();
				stderr_pipe.reset();
				std::vector<char const*> args;
				for (auto& arg: this->command)
					args.push_back(arg.c_str());
				args.push_back(nullptr);
				::execve(args[0], (char**) &args[0], env);
				::exit(EXIT_FAILURE);
			}
			else // Parent
			{
				if (this->options.stdin_ == Stream::PIPE)
					this->stdin_sink = stdin_pipe->sink();
				if (this->options.stdout_ == Stream::PIPE)
					this->stdout_source = stdout_pipe->source();
				if (this->options.stderr_ == Stream::PIPE)
					this->stderr_source = stderr_pipe->source();
			}
			return child;
		}
#elif defined(BOOST_WINDOWS_API)
		Child _create_child()
		{
			LPSECURITY_ATTRIBUTES proc_attrs = 0;
			LPSECURITY_ATTRIBUTES thread_attrs = 0;
			BOOL inherit_handles = false;
			LPVOID env = nullptr;
			LPCTSTR work_dir = nullptr;
			if (this->options.working_directory)
				work_dir = this->options.working_directory.get().string().c_str();
#if (_WIN32_WINNT >= 0x0600)
			DWORD creation_flags = EXTENDED_STARTUPINFO_PRESENT;
			STARTUPINFOEX startup_info_ex;
			ZeroMemory(&startup_info_ex, sizeof(STARTUPINFOEX));
			STARTUPINFO& startup_info = startup_info_ex.StartupInfo;
			startup_info.cb = sizeof(STARTUPINFOEX);
#else
			DWORD creation_flags = 0;
			STARTUPINFO startup_info;
			ZeroMemory(&startup_info, sizeof(STARTUPINFO));
			startup_info.cb = sizeof(STARTUPINFO);
#endif

			std::unique_ptr<Pipe> stdin_pipe;
			if (this->options.stdin_ == Stream::PIPE)
			{
				stdin_pipe.reset(new Pipe);
				::SetHandleInformation(stdin_pipe->source().handle(),
				                       HANDLE_FLAG_INHERIT,
				                       HANDLE_FLAG_INHERIT);
				startup_info.hStdInput = stdin_pipe->source().handle();
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
				inherit_handles = true;
				this->stdin_sink = stdin_pipe->sink();
			}
			else if (this->options.stdin_ == Stream::DEVNULL)
			{
				startup_info.hStdInput = INVALID_HANDLE_VALUE;
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
			}

			std::unique_ptr<Pipe> stdout_pipe;
			if (this->options.stdout_ == Stream::PIPE)
			{
				stdout_pipe.reset(new Pipe);
				::SetHandleInformation(stdout_pipe->sink().handle(),
				                       HANDLE_FLAG_INHERIT,
				                       HANDLE_FLAG_INHERIT);
				startup_info.hStdOutput = stdout_pipe->sink().handle();
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
				inherit_handles = true;
				this->stdout_source = stdout_pipe->source();
			}
			else if (this->options.stdout_ == Stream::DEVNULL)
			{
				startup_info.hStdOutput = INVALID_HANDLE_VALUE;
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
			}

			std::unique_ptr<Pipe> stderr_pipe;
			if (this->options.stderr_ == Stream::PIPE)
			{
				stderr_pipe.reset(new Pipe);
				::SetHandleInformation(stderr_pipe->sink().handle(),
				                       HANDLE_FLAG_INHERIT,
				                       HANDLE_FLAG_INHERIT);
				startup_info.hStdError = stderr_pipe->sink().handle();
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
				inherit_handles = true;
				this->stderr_source = stderr_pipe->source();
			}
			else if (this->options.stderr_ == Stream::DEVNULL)
			{
				startup_info.hStdError = INVALID_HANDLE_VALUE;
				startup_info.dwFlags |= STARTF_USESTDHANDLES;
			}

			PROCESS_INFORMATION proc_info;
			std::unique_ptr<char, void (*)(void*)> cmd_line(
			  ::_strdup(
			    quote<CommandParser::windows_shell>(this->command).c_str()),
			  &::free);
			if (cmd_line == nullptr)
				throw std::bad_alloc();
			auto ret = ::CreateProcess(this->command.at(0).c_str(),
			                           cmd_line.get(),
			                           proc_attrs,
			                           thread_attrs,
			                           inherit_handles,
			                           creation_flags,
			                           env,
			                           work_dir,
			                           &startup_info,
			                           &proc_info);
			if (!ret)
				CONFIGURE_THROW_SYSTEM_ERROR("CreateProcess()");
			return Child(proc_info);
		}
#endif // !BOOST_WINDOWS_API

		Command _prepare_command(Command cmd)
		{
			if (auto exe = Filesystem::which(cmd[0]))
				cmd[0] = exe.get().string();
			else
				CONFIGURE_THROW(
					error::FileNotFound(
						"Cannot find the program '" + cmd[0] + "'"
					)
						<< error::command(cmd)
				);
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
			int exit_code;
			if (_this->child.wait(0, exit_code))
				_this->exit_code = exit_code;
		}
		return _this->exit_code;
	}

	Process::ExitCode Process::wait()
	{
		if (!this->exit_code())
		{
			log::debug("Waiting for child", _this->child, "to terminate");
			int exit_code;
			bool ended = _this->child.wait(-1, exit_code);
			if (!ended)
				throw std::logic_error("Should be terminated");
			_this->exit_code = exit_code;
		}
		return _this->exit_code.get();
	}

	Process::ExitCode Process::call(Command cmd, Options options)
	{
		Process p(std::move(cmd), std::move(options));
		return p.wait();
	}

	void Process::check_call(Command cmd, Options options)
	{
		auto ret = Process::call(std::move(cmd), std::move(options));
		if (ret != 0)
			CONFIGURE_THROW(error::RuntimeError(
			  "Program failed with exit code " + std::to_string(ret)));
	}

	std::string Process::check_output(Command cmd)
	{
		Options options;
		options.stdout_ = Stream::PIPE;
		options.stderr_ = Stream::DEVNULL;
		return Process::check_output(std::move(cmd), std::move(options), false);
	}

	std::string Process::check_output(Command cmd, Options options)
	{
		return Process::check_output(std::move(cmd), std::move(options), false);
	}

	std::string Process::check_output(Command cmd, Options options, bool ignore_errors)
	{
		Process p(std::move(cmd), std::move(options));

		char buf[4096];
		std::string res;
#ifdef BOOST_WINDOWS_API
		DWORD size;
#else
		ssize_t size;
#endif
		std::vector<io::file_descriptor_source*> srcs;
		if (p._this->options.stdout_ == Stream::PIPE)
			srcs.push_back(&p._this->stdout_source);
		if (p._this->options.stderr_ == Stream::PIPE)
			srcs.push_back(&p._this->stderr_source);
		size_t closed = 0;
		while (closed < srcs.size())
		{
			for (auto& src: srcs)
			{
				if (src == nullptr)
					continue;
#ifdef BOOST_WINDOWS_API
				bool success =
				  ::ReadFile(src->handle(), buf, sizeof(buf), &size, NULL);
				if (!success)
				{
					switch (::GetLastError())
					{
					case ERROR_MORE_DATA:
						break;
					case ERROR_BROKEN_PIPE:
						log::debug("output pipe of", p._this->child, "ended");
						break;
					default:
						CONFIGURE_THROW_SYSTEM_ERROR("ReadFile()");
					}
				}
#else
				size = ::read(src->handle(), buf, sizeof(buf));
				if (size < 0)
				{
					if (errno == EINTR)
					{
						log::debug("Read interrupted by a signal, let's retry");
						continue;
					}
					CONFIGURE_THROW_SYSTEM_ERROR("read()");
				}
				log::debug("Read from", p._this->child, "returned", size);
#endif
				if (size > 0)
				{
					log::debug(
					  "read", size, "bytes from child", p._this->child, "stdout");
					res.append(buf, size);
				}
				if (size == 0)
				{
					src = nullptr;
					closed += 1;
				}
			}
		}
		if (p.wait() != 0 && !ignore_errors)
			throw std::runtime_error("Program failed");
		return res;
	}
}
