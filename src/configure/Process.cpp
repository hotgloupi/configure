#include "Process.hpp"
#include "log.hpp"
#include "Filesystem.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/process.hpp>

#include <cassert>

namespace configure {

	namespace {

		template <typename Executor>
		struct InitializerBase
		{
			// POSIX API
			virtual void on_fork_setup(Executor&) const
			{ std::abort(); }

			virtual void on_fork_error(Executor&) const
			{ std::abort(); }

			virtual void on_fork_success(Executor&) const
			{ std::abort(); }

			virtual void on_exec_setup(Executor&) const
			{ std::abort(); }

			virtual void on_exec_error(Executor&) const
			{ std::abort(); }

			// Windows API
			virtual void on_CreateProcess_setup(Executor&) const
			{ std::abort(); }

			virtual void on_CreateProcess_error(Executor&) const
			{ std::abort(); }

			virtual void on_CreateProcess_success(Executor&) const
			{ std::abort(); }
		};

		template <typename Executor, typename ConcreteInitializer>
		struct InitializerWrapper : InitializerBase<Executor>
		{
			ConcreteInitializer _i;

			template <typename... Args>
			explicit InitializerWrapper(Args&&... args)
				: _i(std::forward<Args>(args)...)
			{}

#ifdef BOOST_POSIX_API
			void on_fork_setup(Executor& e) const override
			{ _i.on_fork_setup(e); }

			void on_fork_error(Executor& e) const override
			{ _i.on_fork_error(e); }

			void on_fork_success(Executor& e) const override
			{ _i.on_fork_success(e); }

			void on_exec_setup(Executor& e) const override
			{ _i.on_exec_setup(e); }

			void on_exec_error(Executor& e) const override
			{ _i.on_exec_error(e); }
#else
			void on_CreateProcess_setup(Executor& e) const override
			{ _i.on_CreateProcess_setup(e); }

			void on_CreateProcess_error(Executor& e) const override
			{ _i.on_CreateProcess_error(e); }

			void on_CreateProcess_success(Executor& e) const override
			{ _i.on_CreateProcess_success(e); }
#endif
		};

		template <typename Executor>
		struct BasicInitializer
		{
			typedef std::unique_ptr<InitializerBase<Executor>> InitializerPtr;
			typedef Executor executor_type;
			InitializerPtr _i;

			explicit BasicInitializer(InitializerPtr ptr)
				: _i(std::move(ptr))
			{}

			BasicInitializer(BasicInitializer&& other)
				: _i(std::move(other._i))
			{}

			void on_fork_setup(Executor& e) const { _i->on_fork_setup(e); }
			void on_fork_error(Executor& e) const { _i->on_fork_error(e); }
			void on_fork_success(Executor& e) const { _i->on_fork_success(e); }
			void on_exec_setup(Executor& e) const { _i->on_exec_setup(e); }
			void on_exec_error(Executor& e) const { _i->on_exec_error(e); }
			void on_CreateProcess_setup(Executor& e) const
			{ _i->on_CreateProcess_setup(e); }
			void on_CreateProcess_error(Executor& e) const
			{ _i->on_CreateProcess_error(e); }
			void on_CreateProcess_success(Executor& e) const
			{ _i->on_CreateProcess_success(e); }
		};

		typedef BasicInitializer<boost::process::executor> Initializer;

		template <typename ConcreteInitializer>
		Initializer make_initializer(ConcreteInitializer const& initializer)
		{
			typedef typename Initializer::executor_type Executor;
			typedef InitializerWrapper<Executor, ConcreteInitializer> Wrapper;
			return Initializer(
			  Initializer::InitializerPtr(new Wrapper(initializer)));
		}

	} // !anonymous

	struct Process::Impl
	{
		Command const command;
		Options const options;
		boost::process::pipe stdout_pipe;
		boost::process::pipe stderr_pipe;
		boost::iostreams::file_descriptor_sink stdout_sink;
		boost::iostreams::file_descriptor_sink stderr_sink;
		boost::iostreams::file_descriptor_source stdout_source;
		boost::iostreams::file_descriptor_source stderr_source;
		boost::iostreams::stream<boost::iostreams::file_descriptor_source> stdout_stream;
		boost::iostreams::stream<boost::iostreams::file_descriptor_source> stderr_stream;
		boost::optional<ExitCode> exit_code;
		boost::process::child child;

		Impl(Command cmd, Options options)
			: command(_prepare_command(std::move(cmd)))
			, options(std::move(options))
			, stdout_pipe(boost::process::create_pipe())
			, stderr_pipe(boost::process::create_pipe())
			, stdout_sink(stdout_pipe.sink, boost::iostreams::close_handle)
			, stderr_sink(stderr_pipe.sink, boost::iostreams::close_handle)
			, stdout_source(stdout_pipe.source, boost::iostreams::close_handle)
			, stderr_source(stderr_pipe.source, boost::iostreams::close_handle)
			, stdout_stream(stdout_source)
			, stderr_stream(stderr_source)
			, exit_code(boost::none)
			, child(_create_child())
		{}

		boost::process::child _create_child()
		{
			using namespace boost::process::initializers;
			std::vector<Initializer> all;

			// push args
			all.push_back(make_initializer(set_args(this->command)));

			// Working directory
			if (this->options.working_directory)
				all.push_back(make_initializer(start_in_dir(
				  this->options.working_directory.get().string())));

			if (this->options.inherit_env)
				all.push_back(make_initializer(inherit_env()));

			if (this->options.stdout == Stream::PIPE)
				all.push_back(make_initializer(bind_stdout(this->stdout_sink)));

			log::debug("Spawning process:", boost::join(this->command, " "), "with", all.size(), "initializers");
			return _execute<1>(all, std::move(all.at(0)));
		}

		static size_t const boost_process_max_args = 10;

		template <size_t idx, typename... Args>
		typename std::enable_if<
		  idx != boost_process_max_args, boost::process::child>::type
		_execute(std::vector<Initializer>& initializers, Args&&... args)
		{
			assert(idx <= initializers.size());
			if (idx == initializers.size())
				return boost::process::execute(std::forward<Args>(args)...);
			else
				return _execute<idx + 1>(
				  initializers, std::forward<Args>(args)..., std::move(initializers[idx]));
		}

		template <size_t idx, typename... Args>
		typename std::enable_if<
		  idx == boost_process_max_args, boost::process::child>::type
		_execute(std::vector<Initializer>&, Args&&...)
		{ std::abort(); }

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
	{ return _this->exit_code; }

	Process::ExitCode Process::wait()
	{
		if (!_this->exit_code)
		{
			_this->exit_code = boost::process::wait_for_exit(_this->child);
			_this->stdout_sink.close();
			_this->stderr_sink.close();
		}
		return _this->exit_code.get();
	}

	Process::ExitCode Process::call(Command cmd, Options options)
	{
		Process p(std::move(cmd), std::move(options));
		return p.wait();
	}

	std::string Process::check_output(Command cmd, Options options)
	{
		options.stdout = Stream::PIPE;
		Process p(std::move(cmd), std::move(options));
		if (p.wait() != 0)
			throw std::runtime_error("Program failed");
		return std::string(
		  std::istreambuf_iterator<char>(p._this->stdout_stream),
		  std::istreambuf_iterator<char>());
	}
}
