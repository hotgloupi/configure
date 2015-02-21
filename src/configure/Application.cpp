#include "Application.hpp"
#include "log.hpp"
#include "lua/State.hpp"
#include "Build.hpp"
#include "bind.hpp"

#include "generators/Shell.hpp"
#include "generators/Makefile.hpp"
#include "generators/NMakefile.hpp"

#include <boost/algorithm/string.hpp>
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
			return fs::is_regular_file(dir / ".configure.env");
		}

	}

	struct Application::Impl
	{
		std::string                        program_name;
		std::vector<std::string>           args;
		path_t                             current_directory;
		std::vector<path_t>                build_directories;
		path_t                             project_directory;
		std::map<std::string, std::string> build_variables;
		bool                               dump_mode;
		std::string                        generator;
	};

	Application::Application(int ac, char** av)
		: Application(args_to_vector(ac, av))
	{}

	Application::Application(std::vector<std::string> args)
		: _this(new Impl)
	{
		_this->program_name = args.at(0);
		_this->args = std::move(args);
		_this->current_directory = fs::current_path();
		_this->dump_mode = false;
		_this->args.erase(_this->args.begin());
		_parse_args();
	}

	Application::~Application() {}

	void Application::run()
	{
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
				fs::absolute(_this->program_name).parent_path().parent_path()
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
			if (_this->dump_mode)
				build.dump_graphviz(std::cout);
			log::debug("Generating the build files in", build.directory());
			this->_generate(build);
		}

	}

	std::string const& Application::program_name() const
	{ return _this->program_name; }

	fs::path const& Application::project_directory() const
	{ return _this->project_directory; }

	std::vector<fs::path> const& Application::build_directories() const
	{ return _this->build_directories; }

	void Application::_generate(Build& build)
	{
		static std::vector<std::unique_ptr<Generator>> generators;
		if (generators.empty())
		{
#define ADD_GENERATOR(T) \
			{ \
				std::unique_ptr<Generator> g(new T()); \
				log::debug( \
					"Generator", g->name(), "is", \
					(g->is_available(build) ? "available" : "not available")); \
				generators.push_back(std::move(g));\
			} \
/**/
			ADD_GENERATOR(generators::Makefile);
			ADD_GENERATOR(generators::NMakefile);
			ADD_GENERATOR(generators::Shell);
#undef ADD_GENERATOR
		}

		std::string generator_name = _this->generator;
		if (generator_name.empty())
		{
			for (auto& gen: generators)
				if (gen->is_available(build))
				{
					generator_name = gen->name();
					break;
				}
			if (generator_name.empty())
				CONFIGURE_THROW(
					error::InvalidGenerator(
						"No generator available for your platform"
					)
				);
		}

		generator_name = build.option<std::string>(
			"GENERATOR",
			"Generator to use",
			generator_name
		);

		boost::algorithm::to_lower(generator_name);
		for (auto& gen: generators)
			if (boost::algorithm::to_lower_copy(gen->name()) == generator_name)
			{
				if (!gen->is_available(build))
					CONFIGURE_THROW(
						error::InvalidGenerator(
							"Generator '" + generator_name + "' is not available"
						)
					);
				gen->generate(build);
				return;
			}
		CONFIGURE_THROW(
			error::InvalidGenerator("Unknown generator '" + generator_name + "'")
		);
	}
	void Application::print_help()
	{
		std::cout
			<< "Usage: " << _this->program_name
			<< " [OPTIONS] [VARIABLES...] [BUILD_DIRS...]\n"
			<< "\n"
			<< "    Configure your project's builds in one or more directories.\n"
			<< "\n"
			<< "Positional arguments:\n"
			<< "  -h, --help" << "             " << "Show this help and exit\n"
			<< "\n"
			<< "Optional arguments:\n"

			<< "  -G, --generator NAME" << " "
			<< "Specify the generator to use (alternative to variable GENERATOR)"

			<< "  -p, --project PATH" << " "
			<< "Specify the project to configure instead of detecting it\n"

			<< "  --dump " << " " << "Dump the build graph\n"

			<< "  --version" << "             "
			<< "Print version\n"

			<< "\n"
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
		enum class NextArg { project, generator, other };
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
				continue;
			}

			if (next_arg == NextArg::generator)
			{
				_this->generator = arg;
				next_arg = NextArg::other;
				continue;
			}

			if (arg == "-p" || arg == "--project")
			{
				next_arg = NextArg::project;
				continue;
			}
			if (arg == "--dump")
			{
				_this->dump_mode = true;
				continue;
			}
			if (arg == "-G" || arg == "--generator")
			{
				next_arg = NextArg::generator;
				continue;
			}

			auto it = arg.find('=');
			if (it != std::string::npos)
			{
				_this->build_variables[arg.substr(0, it)] = arg.substr(it + 1, std::string::npos);
				continue;
			}
			if (is_build_directory(arg) || !fs::exists(arg) || (fs::is_directory(arg) && fs::is_empty(arg)))
				_this->build_directories.push_back(fs::absolute(arg));
			else
				std::cout << "UNKNOWN ARG: " << arg << std::endl;
		}
		if (next_arg != NextArg::other)
			throw std::runtime_error{"Expecting argument for the flag " + _this->args.back()};
		if (!has_project)
		{
			if (is_project_directory(_this->current_directory))
				_this->project_directory = _this->current_directory;
			else
				throw std::runtime_error{"No project to configure"};
		}
	}

}
