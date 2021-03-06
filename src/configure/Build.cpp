#include "Build.hpp"

#include "BuildGraph.hpp"
#include "Command.hpp"
#include "DependencyLink.hpp"
#include "Environ.hpp"
#include "error.hpp"
#include "Filesystem.hpp"
#include "log.hpp"
#include "lua/State.hpp"
#include "Platform.hpp"
#include "Rule.hpp"
#include "quote.hpp"
#include "utils/path.hpp"
#include "PropertyMap.hpp"
#include "utils/path.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/scope_exit.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>

namespace fs = boost::filesystem;

namespace configure {

	struct Build::Impl
	{
		fs::path                                 configure_program;
		fs::path                                 root_directory;
		lua::State&                              lua;
		std::vector<fs::path>                    project_stack;
		std::vector<fs::path>                    configured_projects;
		std::vector<fs::path>                    build_stack;
		std::unordered_map<std::string, NodePtr> virtual_nodes;
		std::unordered_map<fs::path, NodePtr>    file_nodes;
		std::unordered_map<fs::path, NodePtr>    directory_nodes;
		BuildGraph::FileProperties               properties;
		BuildGraph                               build_graph;
		NodePtr                                  root_node;
		Filesystem                               fs;
		Environ                                  env;
		fs::path                                 env_path;
		fs::path                                 properties_path;
		std::map<std::string, std::string>       options;
		std::map<std::string, std::string>       build_args;
		std::unique_ptr<Platform>                host_platform;
		std::unique_ptr<Platform>                target_platform;

		Impl(Build& build, fs::path configure_program, lua::State& lua,
		     fs::path directory)
		    : configure_program(std::move(configure_program))
		    , root_directory(std::move(directory))
		    , lua(lua)
		    , project_stack()
		    , configured_projects()
		    , build_stack()
		    , virtual_nodes()
		    , file_nodes()
		    , directory_nodes()
		    , properties()
		    , build_graph(properties)
		    , root_node(this->build_graph.add_node<VirtualNode>(""))
		    , fs(build)
		    , env()
		    , env_path(root_directory / ".build" / "env")
		    , properties_path(root_directory / ".build" / "properties")
		    , options()
		    , build_args()
		    , host_platform(nullptr)
		    , target_platform(nullptr)
		{}
	};

	Build::Build(path_t configure_program, lua::State& lua, fs::path directory)
	    : _this(new Impl(
	        *this, std::move(configure_program), lua, std::move(directory)))
	{
		_this->build_stack.push_back(_this->root_directory);
		if (fs::is_regular_file(_this->env_path))
		{
			try { _this->env.load(_this->env_path); }
			catch (...) {
				CONFIGURE_THROW(
					error::InvalidEnviron("Couldn't load the configuration file")
						<< error::path(_this->env_path)
						<< error::nested(std::current_exception())
				);
			}
		}

		if (fs::is_regular_file(_this->properties_path))
		{
			try {
				std::ifstream in(_this->properties_path.string(), std::ios::binary);
				boost::archive::binary_iarchive ar(in);
				// ar & _this->properties; // XXX Unordered map not yet supported
				BuildGraph::FileProperties::size_type size;
				ar >> size;
				_this->properties.reserve(size);
				for (BuildGraph::FileProperties::size_type i = 0; i < size; ++i)
				{
					BuildGraph::FileProperties::value_type el;
					ar >> el;
					_this->properties.insert(std::move(el));
				}
			} catch (...) {
				CONFIGURE_THROW(
					error::InvalidEnviron("Couldn't load properties")
						<< error::path(_this->properties_path)
						<< error::nested(std::current_exception())
				);
			}
		}
	}

	Build::Build(path_t configure_program, lua::State& lua,
	             fs::path root_directory,
	             std::map<std::string, std::string> build_args)
	    : Build(std::move(configure_program), lua, std::move(root_directory))
	{
		for (auto& pair: build_args)
			_this->build_args[Environ::normalize(pair.first)] = pair.second;

	}

	Build::~Build()
	{
		auto cache = _this->root_directory / ".build";
		if (fs::is_directory(cache))
		{
			try {
				_this->env.save(_this->env_path);
			}
			catch (...) {
				log::error("Couldn't save environ in", _this->env_path, ":",
						   error_string());
			}
			try {
				std::ofstream out(_this->properties_path.string(), std::ios::binary);
				boost::archive::binary_oarchive ar(out);
				// ar & _this->properties; // XXX Unordered map not yet supported
				BuildGraph::FileProperties::size_type s = _this->properties.size();
				ar << s;
				for (auto& el: _this->properties)
					ar << el;
			} catch (...) {
				log::error("Couldn't save properties in", _this->properties_path, ":",
						   error_string());
			}
		}
		else
		{
			log::debug("Build directory", cache,
			           "not found, dropping the environ");
		}
	}

	void Build::clear_properties()
	{
		_this->properties.clear();
	}

	void Build::configure(fs::path const& project_directory,
	                      fs::path const& sub_directory,
	                      bool has_args)
	{
		try {
			if (!project_directory.is_absolute())
				CONFIGURE_THROW(error::InvalidProject("Not an absolute path"));
			if (!fs::is_directory(project_directory))
				CONFIGURE_THROW(error::InvalidProject("Not a directory"));
			if (fs::equivalent(project_directory, this->directory()))
				CONFIGURE_THROW(error::InvalidProject("Same as build directory"));
		} catch (error::InvalidProject& err) {
			throw err << error::path(project_directory);
		}
		_this->project_stack.push_back(project_directory);
		_this->configured_projects.push_back(project_directory);
		_this->build_stack.push_back(_prepare_build_directory(sub_directory));
		log::status("Configuring project", this->project_directory(), "in", this->directory());

		BOOST_SCOPE_EXIT((&_this)){
			_this->project_stack.pop_back();
			_this->build_stack.pop_back();
		} BOOST_SCOPE_EXIT_END

		try {
			_this->lua.load(find_project_file(project_directory), 1);
			if (lua_isnil(_this->lua.ptr(), -1))
				CONFIGURE_THROW(error::InvalidProject(
					"No function returned from the configuration script")
						<< error::path(project_directory));
			_this->lua.construct<std::reference_wrapper<Build>>(*this);
			if (has_args)
				_this->lua.pushvalue(-3);
			_this->lua.call(has_args ? 2 : 1);
		} catch (error::Base&) { //XXX insert project stack here
			//e << error::message(
			//	"While configuring project " + project_directory.string()
			//);
			throw;
		}
		// Last project on the stack
		if (_this->project_stack.size() == 1)
			_finalize_build_directory();
	}

	fs::path const& Build::project_directory() const
	{
		if (_this->project_stack.empty())
			throw std::logic_error("No project on the stack");
		return _this->project_stack.back();
	}

	std::vector<fs::path> const& Build::configured_projects() const
	{ return _this->configured_projects; }

	fs::path const& Build::root_directory() const
	{ return _this->root_directory; }

	fs::path const& Build::directory() const
	{ return _this->build_stack.back(); }

	NodePtr const& Build::root_node() const
	{ return _this->root_node; }

	BuildGraph const& Build::build_graph() const
	{ return _this->build_graph; }

	Filesystem& Build::fs()
	{ return _this->fs; }

	Environ& Build::env()
	{ return _this->env; }

	fs::path const& Build::configure_program() const
	{ return _this->configure_program; }

	std::map<std::string, std::string> const& Build::options() const
	{ return _this->options; }

	template<typename T>
	boost::optional<typename Environ::const_ref<T>::type>
	Build::option(std::string name,
	              std::string description)
	{
		name = Environ::normalize(std::move(name));
		_this->options[name] = std::move(description);
		auto it = _this->build_args.find(name);
		if (it != _this->build_args.end())
		{
			log::debug(
				"Found option", name, "in command line arguments:", it->second);
			if (std::is_same<T, bool>::value)
			{
				std::string str = boost::to_lower_copy(it->second);
				static std::map<std::string, bool> conversions = {
					{"true", true}, {"false", false},
					{"yes", true}, {"no", false},
					{"y", true}, {"n", false},
					{"1", true}, {"0", false},
				};
				bool value;
				try { value = conversions.at(str); }
				catch (...) {
					CONFIGURE_THROW(
						error::InvalidOption("Cannot convert '" + it->second +
						                     "' to a boolean")
					);
				}
				_this->env.set(name, value);
			}
			else if (std::is_same<T, Path>::value)
			{
				_this->env.set(name, Path(it->second));
			}
			else
			{
				try {
					_this->env.set(name, boost::lexical_cast<T>(it->second));
				} catch (boost::bad_lexical_cast&) {
					CONFIGURE_THROW(
						error::InvalidOption(
							"Cannot create " + it->first +
							" option from value '" + it->second + "'"
						)
					);
				}
			}
			_this->build_args.erase(it);
		}
		if (_this->env.has(name))
		{
			return boost::optional<typename Environ::const_ref<T>::type>(_this->env.get<T>(name));
		}
		return boost::none;
	}

	template<typename T>
	typename Environ::const_ref<T>::type
	Build::option(std::string name,
	              std::string description,
	              typename Environ::const_ref<T>::type default_value)
	{
		if (auto res = this->option<T>(name, std::move(description)))
			return res.get();
		return _this->env.set<T>(std::move(name), default_value);
	}

	template <typename T>
	typename Environ::const_ref<T>::type
	Build::lazy_option(std::string name,
	                   std::string description,
	                   std::function<T()> const& fn)
	{
		name = Environ::normalize(name);
		if (auto res = this->option<T>(name, std::move(description)))
			return res.get();
		try {
			return _this->env.set<T>(name, fn());
		} catch (...) {
			CONFIGURE_THROW(
				error::RuntimeError("Couldn't compute lazy option '" + name + "'")
					<< error::help("Try to define '" + name + "' directly")
					<< error::nested(std::current_exception())
			);
		}

	}

#define INSTANCIATE(T) \
	template \
	boost::optional<Environ::const_ref<T>::type> \
	Build::option<T>(std::string name, \
	                 std::string description); \
	template \
	Environ::const_ref<T>::type \
	Build::option<T>(std::string name, \
	                 std::string description, \
	                 Environ::const_ref<T>::type default_value); \
	template \
	Environ::const_ref<T>::type \
	Build::lazy_option(std::string name, \
	                   std::string description, \
	                   std::function<T()> const& fn); \
/**/

	INSTANCIATE(std::string);
	INSTANCIATE(int64_t);
	INSTANCIATE(bool);
	INSTANCIATE(fs::path);

	fs::path Build::_prepare_build_directory(fs::path const& sub_directory)
	{
		fs::create_directories(this->directory() / sub_directory / ".build");
		return fs::canonical(this->directory() / sub_directory);
	}


	void Build::_finalize_build_directory()
	{
		std::unordered_set<fs::path> directories;
		for (auto const& pair: _this->file_nodes)
		{
			if (utils::starts_with(pair.first, this->root_directory()))
				directories.insert(pair.first.parent_path());
		}
		for (auto const& pair: _this->directory_nodes)
		{
			if (utils::starts_with(pair.first, this->root_directory()))
				directories.insert(pair.first);
		}

		for (auto&& d: directories)
		{
			log::debug("Creating directory", d);
			fs::create_directories(d);
		}
	}

	std::vector<fs::path> const& Build::possible_configure_files()
	{
		static std::vector<fs::path> ret {
			"configure.lua",
			".configure.lua",
			".config/project.lua",
		};
		return ret;
	}

	fs::path Build::find_project_file(fs::path project_directory)
	{
		for (auto&& p: possible_configure_files())
			if (fs::is_regular_file(project_directory / p))
				return project_directory / p;
		throw std::runtime_error("No configuration file found in " + project_directory.string());
	}

	NodePtr& Build::virtual_node(std::string const& name)
	{
		if (name.empty())
			CONFIGURE_THROW(error::InvalidVirtualNode("Name cannot be empty"));
		auto it = _this->virtual_nodes.find(name);
		if (it != _this->virtual_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<VirtualNode>(name);
		log::debug("Created node", node);
		return (_this->virtual_nodes[name] = std::move(node));
	}

	NodePtr& Build::file_node(fs::path path)
	{
		path.make_preferred();
		if (!path.is_absolute())
			throw std::runtime_error("Not an absolute path: " + path.string());
		auto it = _this->file_nodes.find(path);
		if (it != _this->file_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<FileNode>(path);
		log::debug("Created node", node);
		return (_this->file_nodes[path] = std::move(node));
	}

	NodePtr& Build::directory_node(fs::path path)
	{
		path.make_preferred();
		if (!path.is_absolute())
			throw std::runtime_error("Not an absolute path: " + path.string());
		auto it = _this->directory_nodes.find(path);
		if (it != _this->directory_nodes.end())
			return it->second;
		auto node = _this->build_graph.add_node<DirectoryNode>(path);
		log::debug("Created node", node);
		return (_this->directory_nodes[path] = std::move(node));
	}

	NodePtr& Build::source_node(fs::path const& path)
	{
		fs::path src;
		if (!path.is_absolute())
			src = this->project_directory() / path;
		else
		{
			src = path;
			if (utils::starts_with(src, this->root_directory()))
				CONFIGURE_THROW(
					error::InvalidSourceNode("Generated file cannot be a source node")
						<< error::path(src)
				);
		}
		if (!fs::is_regular_file(src))
			CONFIGURE_THROW(error::InvalidSourceNode("File not found")
				<< error::path(src)
			);
		return this->file_node(src);
	}

	NodePtr& Build::target_node(fs::path const& path)
	{
		if (path.is_absolute())
			return this->file_node(path);
		return this->file_node(this->directory() / path);
	}

	void Build::add_rule(Rule const& rule)
	{
		CommandPtr cmd = rule.command();
		if (rule.targets().empty())
			CONFIGURE_THROW(error::InvalidRule("No target specified"));

		if (rule.sources().empty())
			for (auto& target: rule.targets())
			{
				try {
					_this->build_graph.link(*_this->root_node, *target).command(cmd);
				} catch (error::CommandAlreadySet&) {
					CONFIGURE_THROW(
						error::InvalidRule(
							"Trying to generate " + target->string() + " twice"
						) << error::nested(std::current_exception())
					);
				}
			}
		else
			for (auto& target: rule.targets())
				for (auto& source: rule.sources())
				{
					log::debug("Add commands to generate", target, "from", source);
					try {
						_this->build_graph.link(*source, *target)
							.command(cmd);
					} catch (error::CommandAlreadySet&) {
						CONFIGURE_THROW(
						  error::InvalidRule("Trying to generate " +
						                     target->string() + " from " +
						                     source->string() + " twice")
						  << error::nested(std::current_exception())
						  //<< error::help("Has '" +
						  //_this->build_graph.link(*source, *target).command()
						  //+ "'")
						  );
					}
				}
	}

	Platform& Build::host()
	{
		if (_this->host_platform == nullptr)
		{
			_this->host_platform.reset(new Platform(Platform::current()));
			if (auto vendor = this->option<std::string>("host-vendor", "Host vendor"))
				_this->host_platform->vendor(*vendor);
			if (auto os = this->option<std::string>("host-os", "Host OS"))
				_this->host_platform->os(*os);
			if (auto arch = this->option<std::string>("host-arch", "Host architecture"))
				_this->host_platform->arch(*arch);
			if (auto sub_arch = this->option<std::string>("host-sub-arch", "Host sub-architecture"))
				_this->host_platform->sub_arch(*sub_arch);
		}
		return *_this->host_platform;
	}

	Platform& Build::target()
	{
		if (_this->target_platform == nullptr)
		{
			_this->target_platform.reset(new Platform(Platform::current()));
			if (auto vendor = this->option<std::string>("target-vendor", "Target vendor"))
				_this->target_platform->vendor(*vendor);
			if (auto os = this->option<std::string>("target-os", "Target OS"))
				_this->target_platform->os(*os);
			if (auto os_version = this->option<std::string>("target-os-version", "Target OS version (Major.minor)"))
				_this->target_platform->os_version(*os_version);
			if (auto arch = this->option<std::string>("target-arch", "Target architecture"))
				_this->target_platform->arch(*arch);
			if (auto sub_arch = this->option<std::string>("target-sub-arch", "Target sub-architecture"))
				_this->target_platform->sub_arch(*sub_arch);
		}
		return *_this->target_platform;
	}

	void Build::dump_graphviz(std::ostream& out) const
	{
		struct Writer {
			Build const& _b;
			BuildGraph const& _g;

			Writer(Build const& b, BuildGraph const& g) : _b(b), _g(g) {}

			void operator ()(std::ostream& out, Node::index_type idx) const
			{
				auto& node = _g.node(idx);
				switch (node->kind())
				{
				case Node::file_node:
				case Node::directory_node:
					out << "[label=\"" << node->relative_path(_b.directory()).string() << "\"]";
					break;
				case Node::virtual_node:
					out << "[label=\"" << node->name() << "\"]";
					break;
				default:
					out << "[label=\"" << "UNKNOWN" << "\"]";
				}
			}

			void operator ()(std::ostream& out, DependencyLink::index_type idx) const
			{
				auto const& cmd = _g.link(idx).command();
				out << "[label=\"";
				bool first = true;
				for (auto&& shell_command: cmd.shell_commands())
				{
					if (first) first = false;
					else out << "\\n";
					out << quote<CommandParser::unix_shell>(shell_command.dump());
				}
				out << "\"]";

			}
		} writer(*this, _this->build_graph);
		boost::write_graphviz(
			out,
			_this->build_graph.graph(),
			writer,
			writer
		);
	}

	void Build::dump_options(std::ostream& out) const
	{
		for (auto& p: this->options())
		{
			out << "  - " << p.first;
			if (_this->env.has(p.first))
				out << " = " << _this->env.as_string(p.first);
			else
				out << " not set";
			out << " (" << p.second << ')' << std::endl;
		}
	}

	void Build::dump_env(std::ostream& out) const
	{
		for (auto& key: _this->env.keys())
		{
			out << "  - " << key << " = "
			    << _this->env.as_string(key) << std::endl;
		}
	}

	void Build::dump_targets(std::ostream& out) const
	{
		for (auto& p: _this->file_nodes)
		{
			if (utils::starts_with(p.first, this->directory()))
				out << "  - " << p.second->relative_path(this->directory())
				    << std::endl;
		}
	}

	lua::State& Build::lua_state() const
	{ return _this->lua; }

	void Build::visit_targets(std::function<void(NodePtr&)> const& fn)
	{
		// XXX lock file nodes
		for (auto& p: _this->directory_nodes)
		{
			if (utils::starts_with(p.first, this->directory()))
				fn(p.second);
		}
		for (auto& p: _this->file_nodes)
		{
			if (utils::starts_with(p.first, this->directory()))
				fn(p.second);
		}
	}

}
