#include "Application.hpp"
#include "log.hpp"
#include "lua/State.hpp"
#include "Build.hpp"
#include "bind.hpp"
#include "quote.hpp"
#include "commands.hpp"
#include "Filesystem.hpp"

#include "generators.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <iostream>
#include <map>

namespace fs = boost::filesystem;

namespace configure {

	namespace {

		std::vector<std::string> args_to_vector(int ac, char ** av)
		{
			std::vector<std::string> res;
			for (int i = 0; i < ac; ++i)
				res.push_back(av[i]);
			return res;
		}

		bool is_project_directory(fs::path const& dir)
		{
			for (auto&& p: Build::possible_configure_files())
				if (fs::is_regular_file(dir / p))
					return true;
			return false;
		}

		bool is_build_directory(fs::path const& dir)
		{
			return fs::is_regular_file(dir / ".build" / "env");
		}

	}

	struct Application::Impl
	{
		fs::path                           program_name;
		std::vector<std::string>           args;
		path_t                             current_directory;
		std::vector<path_t>                build_directories;
		path_t                             project_directory;
		std::map<std::string, std::string> build_variables;
		bool                               dump_graph_mode;
		bool                               dump_options;
		bool                               dump_env;
		bool                               dump_targets;
		bool                               build_mode;
		std::string                        build_target;
		std::vector<std::string>           builtin_command_args;
		Impl(std::vector<std::string> args)
			: program_name(args.at(0))
			, args(std::move(args))
			, current_directory(fs::current_path())
			, build_directories()
			, project_directory()
			, build_variables()
			, dump_graph_mode(false)
			, dump_options(false)
			, dump_env(false)
			, dump_targets(false)
			, build_mode(false)
			, build_target()
			, builtin_command_args()
		{ this->args.erase(this->args.begin()); }
	};

	Application::Application(int ac, char** av)
		: Application(args_to_vector(ac, av))
	{}

	Application::Application(std::vector<std::string> args)
		: _this(new Impl(std::move(args)))
	{ _parse_args(); }

	Application::~Application() {}

	void Application::run()
	{
		if (!_this->builtin_command_args.empty())
			return commands::execute(_this->builtin_command_args);
		log::debug("Current directory:", _this->current_directory);
		log::debug("Program name:", _this->program_name);
		if (_this->build_directories.empty())
			throw std::runtime_error("No build directory specified");
		fs::path project_file = Build::find_project_file(_this->project_directory);
		fs::path package;
		char const* lib = ::getenv("CONFIGURE_LIBRARY_DIR");
		if (lib != nullptr)
			package = lib;
		else
			package = fs::canonical(
				_this->program_name.parent_path().parent_path()
				/ "share" / "configure" / "lib"
			);
		package /= "?.lua";
		lua::State lua;
		bind(lua);
		lua.global("configure_library_dir", package.string());
		lua.load(
			"require 'package'\n"
			"package.path = configure_library_dir\n"
		);
		for (auto const& directory: _this->build_directories)
		{
			Build build(lua, directory, _this->build_variables);
			build.configure(_this->project_directory);
			if (_this->dump_graph_mode)
				build.dump_graphviz(std::cout);
			log::debug("Generating the build files in", build.directory());
			auto generator = this->_generator(build);
			assert(generator != nullptr);
			generator->prepare();
			generator->generate();
			log::status("Build files generated successfully in",
						build.directory(), "(", generator->name(), ")");
			if (_this->dump_options)
			{
				std::cout << "Available options:\n";
				build.dump_options(std::cout);
			}
			if (_this->dump_env)
			{
				std::cout << "Environment variables:\n";
				build.dump_env(std::cout);
			}
			if (_this->dump_targets)
			{
				std::cout << "Build targets:\n";
				build.dump_targets(std::cout);
			}
			if (_this->build_mode)
			{
				log::status("Starting build in", build.directory());
				auto cmd = generator->build_command(_this->build_target);
				int res = ::system(
#ifdef _WIN32
				    quote<CommandParser::windows_shell>(cmd).c_str()
#else
				    quote<CommandParser::unix_shell>(cmd).c_str()
#endif
				);
				if (res != 0)
					CONFIGURE_THROW(
						error::BuildError("Build failed with exit code " + std::to_string(res))
						<< error::path(build.directory())
						<< error::command(cmd)
					);
			}
		}

	}

	fs::path const& Application::program_name() const
	{ return _this->program_name; }

	fs::path const& Application::project_directory() const
	{ return _this->project_directory; }

	std::vector<fs::path> const& Application::build_directories() const
	{ return _this->build_directories; }

	std::unique_ptr<Generator> Application::_generator(Build& build) const
	{
		std::string name = build.option<std::string>(
			"GENERATOR",
			"Generator to use",
			generators::first_available(build)
		);
		return generators::from_name(
			name,
			build,
			_this->project_directory,
			*build.fs().which(_this->program_name.string())
		);
	}

	void Application::print_help()
	{
		std::cout
			<< "Usage: " << _this->program_name
			<< "  [OPTION]... [BUILD_DIR]... [KEY=VALUE]...\n"
			<< "\n"
			<< "  Configure your project's builds in one or more directories.\n"
			<< "\n"
			<< "Positional arguments:\n"

			<< "  BUILD_DIR" << "             "
			<< "Build directory to configure\n"

			<< "  KEY=VALUE" << "             "
			<< "Set a variable for selected build directories\n"

			<< "\n"

			<< "Optional arguments:\n"

			<< "  -G, --generator NAME" << "  "
			<< "Specify the generator to use (alternative to variable GENERATOR)\n"

			<< "  -p, --project PATH" << "    "
			<< "Specify the project to configure instead of detecting it\n"

			<< "  --dump-graph" << "          "
			<< "Dump the build graph\n"

			<< "  --dump-options" << "        "
			<< "Dump all options\n"

			<< "  --dump-env" << "            "
			<< "Dump all environment variables\n"

			<< "  --dump-targets" << "        "
			<< "Dump all targets\n"

			<< "  -d, --debug" << "           "
			<< "Enable debug output\n"

			<< "  -v, --verbose" << "         "
			<< "Enable verbose output\n"

			<< "  --version" << "             "
			<< "Print version\n"

			<< "  -h, --help" << "            "
			<< "Show this help and exit\n"

			<< "  -b,--build" << "            "
			<< "Start a build in specified directories\n"

			<< "  -t,--target" << "           "
			<< "Specify the target to build\n"

			<< "  -E,--execute" << "          "
			<< "Execute a builtin command\n"
		;
	}

	void Application::exit()
	{
		::exit(0);
	}

	void Application::_parse_args()
	{
		// Searching help and version flags first (ignoring command line errors
		// if any).
		for (auto const& arg: _this->args)
		{
			if (arg == "-h" || arg == "--help")
			{
				this->print_help();
				this->exit();
			}
			if (arg == "--version")
			{
#ifndef CONFIGURE_VERSION_STRING
# define CONFIGURE_VERSION_STRING "unknown"
#endif
				log::print("configure version", CONFIGURE_VERSION_STRING);
				this->exit();
			}
		}

		bool has_project = false;
		enum class NextArg { project, generator, target, builtin_command, other };
		NextArg next_arg = NextArg::other;
		for (auto const& arg: _this->args)
		{
			if (next_arg == NextArg::project)
			{
				if (!is_project_directory(arg))
					throw std::runtime_error{"Invalid project directory"};
				if (!has_project)
				{
					_this->project_directory = fs::canonical(arg);
					has_project = true;
				}
				else if (_this->project_directory == fs::canonical(arg))
				{
					std::cerr << "Warning: Project directory specified more than once.\n";
				}
				else
				{
					throw std::runtime_error{"Cannot operate on multiple projects"};
				}
				next_arg = NextArg::other;
			}
			else if (next_arg == NextArg::generator)
			{
				_this->build_variables["GENERATOR"] = arg;
				next_arg = NextArg::other;
			}
			else if (next_arg == NextArg::target)
			{
				_this->build_target = arg;
				next_arg = NextArg::other;
			}
			else if (next_arg == NextArg::builtin_command)
				_this->builtin_command_args.push_back(arg);
			else if (arg == "-p" || arg == "--project")
				next_arg = NextArg::project;
			else if (arg == "--dump-graph")
				_this->dump_graph_mode = true;
			else if (arg == "--dump-options")
				_this->dump_options = true;
			else if (arg == "--dump-env")
				_this->dump_env = true;
			else if (arg == "--dump-targets")
				_this->dump_targets = true;
			else if (arg == "-G" || arg == "--generator")
				next_arg = NextArg::generator;
			else if (arg == "-d" || arg == "--debug")
				log::level() = log::Level::debug;
			else if (arg == "-v" || arg == "--verbose")
				log::level() = log::Level::verbose;
			else if (arg == "-t" || arg == "--target")
				next_arg = NextArg::target;
			else if (arg == "-b" || arg == "--build")
				_this->build_mode = true;
			else if (arg == "-E" || arg == "--execute")
				next_arg = NextArg::builtin_command;
			else if (arg.find('=') != std::string::npos)
			{
				auto it = arg.find('=');
				if (it != std::string::npos)
				{
					_this->build_variables[arg.substr(0, it)] =
						arg.substr(it + 1, std::string::npos);
					continue;
				}
			}
			else if (is_build_directory(arg) ||
			         !fs::exists(arg) ||
			         (fs::is_directory(arg) && fs::is_empty(arg)))
			{
				_this->build_directories.push_back(fs::absolute(arg));
			}
			else
			{
				CONFIGURE_THROW(
					error::InvalidArgument("Unknown argument '" + arg + "'")
				);
			}
		}
		if (next_arg != NextArg::other && next_arg != NextArg::builtin_command)
		{
			CONFIGURE_THROW(
				error::InvalidArgument(
					"Missing argument for flag '" + _this->args.back() + "'"
				)
			);
		}
		if (!has_project && _this->builtin_command_args.empty())
		{
			if (is_project_directory(_this->current_directory))
				_this->project_directory = _this->current_directory;
			else
				throw std::runtime_error{"No project to configure"};
		}
	}

}
